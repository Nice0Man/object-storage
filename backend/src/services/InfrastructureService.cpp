#include "console/services/InfrastructureService.hpp"

#include "console/common/Logger.hpp"
#include "console/common/ServiceLocator.hpp"
#include "console/services/BucketService.hpp"

#include <iomanip>
#include <random>
#include <sstream>

namespace console::services {

using namespace console::models;

InfrastructureService::InfrastructureService(std::shared_ptr<storage::DatabaseManager> db_manager)
    : db_manager_(std::move(db_manager)) {
    CONSOLE_LOG_INFO("InfrastructureService initialized");
}

// ============================================================================
// Summary
// ============================================================================

Result<InfrastructureSummary, ApiError>
InfrastructureService::get_summary(const UserInfo& user) {
    CONSOLE_LOG_DEBUG("Getting infrastructure summary");

    InfrastructureSummary summary;

    // Get servers
    auto servers_result = db_manager_->list_servers();
    if (servers_result) {
        summary.total_servers = static_cast<int>(servers_result.value().size());
        for (const auto& server : servers_result.value()) {
            if (server.status == "online") {
                summary.online_servers++;
            } else {
                summary.offline_servers++;
            }
        }
    }

    // Get drives
    auto drives_result = db_manager_->list_drives();
    if (drives_result) {
        summary.total_drives = static_cast<int>(drives_result.value().size());
        for (const auto& drive : drives_result.value()) {
            if (drive.status == "online") {
                summary.online_drives++;
            } else {
                summary.offline_drives++;
            }
            summary.total_capacity += drive.capacity;
            summary.used_capacity += drive.used;
            summary.available_capacity += drive.available;
        }
    }

    // Get pools
    auto pools_result = db_manager_->list_storage_pools();
    if (pools_result) {
        summary.total_pools = static_cast<int>(pools_result.value().size());
        for (const auto& pool : pools_result.value()) {
            summary.pools.push_back(db_pool_to_details(pool));
            summary.object_data += pool.used; // Approximate object data
        }
    }

    return Result<InfrastructureSummary, ApiError>(ok_tag, summary);
}

// ============================================================================
// Server Operations
// ============================================================================

Result<Vector<ServerDetails>, ApiError>
InfrastructureService::list_servers(const UserInfo& user) {
    CONSOLE_LOG_DEBUG("Listing servers");

    auto servers_result = db_manager_->list_servers();
    if (!servers_result) {
        return Err<Vector<ServerDetails>>(
            ApiError(HttpStatus::InternalServerError, "Failed to list servers: " + servers_result.error()));
    }

    Vector<ServerDetails> details;
    for (const auto& server : servers_result.value()) {
        details.push_back(db_server_to_details(server));
    }

    return Result<Vector<ServerDetails>, ApiError>(ok_tag, details);
}

Result<ServerDetails, ApiError>
InfrastructureService::get_server(const UserInfo& user, const String& id) {
    CONSOLE_LOG_DEBUG("Getting server: {}", id);

    auto server_result = db_manager_->get_server(id);
    if (!server_result) {
        return Err<ServerDetails>(ApiError(HttpStatus::NotFound, "Server not found: " + id));
    }

    return Result<ServerDetails, ApiError>(ok_tag, db_server_to_details(server_result.value()));
}

Result<ServerDetails, ApiError>
InfrastructureService::add_server(const UserInfo& user, const AddServerRequest& request) {
    CONSOLE_LOG_INFO("Adding server: {} at {}", request.name, request.endpoint);

    // Validate request
    if (request.name.empty()) {
        return Err<ServerDetails>(ApiError(HttpStatus::BadRequest, "Server name is required"));
    }
    if (request.endpoint.empty()) {
        return Err<ServerDetails>(ApiError(HttpStatus::BadRequest, "Server endpoint is required"));
    }

    // Create server
    storage::DbServer server;
    server.id = generate_server_id();
    server.name = request.name;
    server.endpoint = request.endpoint;
    server.status = "online"; // Assume online initially
    server.uptime = 0;
    server.last_heartbeat = std::time(nullptr);

    auto upsert_result = db_manager_->upsert_server(server);
    if (!upsert_result) {
        return Err<ServerDetails>(
            ApiError(HttpStatus::InternalServerError, "Failed to add server: " + upsert_result.error()));
    }

    // Add drives if specified
    for (const auto& path : request.drive_paths) {
        storage::DbDrive drive;
        drive.id = generate_drive_id(server.id);
        drive.server_id = server.id;
        drive.path = path;
        drive.status = "online";
        drive.capacity = 1024LL * 1024 * 1024 * 1024; // Default 1 TiB
        drive.used = 0;
        drive.available = drive.capacity;
        drive.last_check = std::time(nullptr);

        db_manager_->upsert_drive(drive);
    }

    CONSOLE_LOG_INFO("Server added: {} ({})", server.name, server.id);
    return get_server(user, server.id);
}

Result<void, ApiError>
InfrastructureService::remove_server(const UserInfo& user, const String& id) {
    CONSOLE_LOG_INFO("Removing server: {}", id);

    // Check if server exists
    auto server_result = db_manager_->get_server(id);
    if (!server_result) {
        return Result<void, ApiError>(err_tag, ApiError(HttpStatus::NotFound, "Server not found: " + id));
    }

    // Check for drives on this server
    auto drives_result = db_manager_->list_drives(id);
    if (drives_result && !drives_result.value().empty()) {
        return Result<void, ApiError>(
            err_tag, ApiError(HttpStatus::Conflict, "Cannot remove server with active drives. Remove drives first."));
    }

    auto delete_result = db_manager_->delete_server(id);
    if (!delete_result) {
        return Result<void, ApiError>(
            err_tag, ApiError(HttpStatus::InternalServerError, "Failed to remove server: " + delete_result.error()));
    }

    CONSOLE_LOG_INFO("Server removed: {}", id);
    return Ok<ApiError>();
}

Result<ServerDetails, ApiError>
InfrastructureService::check_server_health(const UserInfo& user, const String& id) {
    CONSOLE_LOG_DEBUG("Checking health for server: {}", id);

    auto server_result = db_manager_->get_server(id);
    if (!server_result) {
        return Err<ServerDetails>(ApiError(HttpStatus::NotFound, "Server not found: " + id));
    }

    auto server = server_result.value();
    auto now = std::time(nullptr);

    // Perform actual health check by trying to list buckets
    bool is_healthy = false;
    auto bucket_service = ServiceLocator::bucket_service();
    if (bucket_service) {
        UserInfo system_user;
        system_user.account_name = "system";
        system_user.is_admin = true;

        auto buckets_result = bucket_service->list_buckets(system_user);
        is_healthy = buckets_result.is_ok();
    }

    // Update server status based on health check result
    String old_status = server.status;
    server.status = is_healthy ? "online" : "offline";
    server.last_heartbeat = now;

    // Update uptime if server just came online
    if (is_healthy && server.uptime == 0) {
        server.uptime = now; // Store start timestamp
    } else if (!is_healthy) {
        server.uptime = 0;
    }

    if (old_status != server.status) {
        CONSOLE_LOG_INFO("Server '{}' health check: {} -> {}", server.name, old_status, server.status);
    }

    db_manager_->upsert_server(server);

    return Result<ServerDetails, ApiError>(ok_tag, db_server_to_details(server));
}

// ============================================================================
// Drive Operations
// ============================================================================

Result<Vector<DriveDetails>, ApiError>
InfrastructureService::list_drives(const UserInfo& user, const String& server_id) {
    CONSOLE_LOG_DEBUG("Listing drives for server: {}", server_id.empty() ? "all" : server_id);

    auto drives_result = db_manager_->list_drives(server_id);
    if (!drives_result) {
        return Err<Vector<DriveDetails>>(
            ApiError(HttpStatus::InternalServerError, "Failed to list drives: " + drives_result.error()));
    }

    Vector<DriveDetails> details;
    for (const auto& drive : drives_result.value()) {
        details.push_back(db_drive_to_details(drive));
    }

    return Result<Vector<DriveDetails>, ApiError>(ok_tag, details);
}

Result<DriveDetails, ApiError>
InfrastructureService::get_drive(const UserInfo& user, const String& id) {
    CONSOLE_LOG_DEBUG("Getting drive: {}", id);

    auto drive_result = db_manager_->get_drive(id);
    if (!drive_result) {
        return Err<DriveDetails>(ApiError(HttpStatus::NotFound, "Drive not found: " + id));
    }

    return Result<DriveDetails, ApiError>(ok_tag, db_drive_to_details(drive_result.value()));
}

Result<DriveDetails, ApiError>
InfrastructureService::add_drive(const UserInfo& user, const AddDriveRequest& request) {
    CONSOLE_LOG_INFO("Adding drive to server {}: {}", request.server_id, request.path);

    // Validate server exists
    auto server_result = db_manager_->get_server(request.server_id);
    if (!server_result) {
        return Err<DriveDetails>(ApiError(HttpStatus::NotFound, "Server not found: " + request.server_id));
    }

    // Create drive
    storage::DbDrive drive;
    drive.id = generate_drive_id(request.server_id);
    drive.server_id = request.server_id;
    drive.path = request.path;
    drive.status = "online";
    drive.capacity = request.capacity > 0 ? request.capacity : (1024LL * 1024 * 1024 * 1024); // Default 1 TiB
    drive.used = 0;
    drive.available = drive.capacity;
    drive.last_check = std::time(nullptr);

    auto upsert_result = db_manager_->upsert_drive(drive);
    if (!upsert_result) {
        return Err<DriveDetails>(
            ApiError(HttpStatus::InternalServerError, "Failed to add drive: " + upsert_result.error()));
    }

    CONSOLE_LOG_INFO("Drive added: {} on server {}", drive.id, request.server_id);
    return get_drive(user, drive.id);
}

Result<void, ApiError>
InfrastructureService::remove_drive(const UserInfo& user, const String& id) {
    CONSOLE_LOG_INFO("Removing drive: {}", id);

    auto drive_result = db_manager_->get_drive(id);
    if (!drive_result) {
        return Result<void, ApiError>(err_tag, ApiError(HttpStatus::NotFound, "Drive not found: " + id));
    }

    // Check if drive has data
    auto drive = drive_result.value();
    if (drive.used > 0) {
        return Result<void, ApiError>(
            err_tag, ApiError(HttpStatus::Conflict, "Cannot remove drive with data. Decommission first."));
    }

    auto delete_result = db_manager_->delete_drive(id);
    if (!delete_result) {
        return Result<void, ApiError>(
            err_tag, ApiError(HttpStatus::InternalServerError, "Failed to remove drive: " + delete_result.error()));
    }

    CONSOLE_LOG_INFO("Drive removed: {}", id);
    return Ok<ApiError>();
}

Result<void, ApiError>
InfrastructureService::set_drive_status(const UserInfo& user, const String& id, const String& status) {
    CONSOLE_LOG_INFO("Setting drive {} status to: {}", id, status);

    // Validate status
    if (status != "online" && status != "offline" && status != "healing" && status != "faulty") {
        return Result<void, ApiError>(
            err_tag, ApiError(HttpStatus::BadRequest, "Invalid status. Must be: online, offline, healing, faulty"));
    }

    auto drive_result = db_manager_->get_drive(id);
    if (!drive_result) {
        return Result<void, ApiError>(err_tag, ApiError(HttpStatus::NotFound, "Drive not found: " + id));
    }

    auto drive = drive_result.value();
    drive.status = status;
    drive.last_check = std::time(nullptr);

    auto upsert_result = db_manager_->upsert_drive(drive);
    if (!upsert_result) {
        return Result<void, ApiError>(
            err_tag, ApiError(HttpStatus::InternalServerError, "Failed to update drive: " + upsert_result.error()));
    }

    return Ok<ApiError>();
}

// ============================================================================
// Pool Operations
// ============================================================================

Result<Vector<PoolDetails>, ApiError>
InfrastructureService::list_pools(const UserInfo& user) {
    CONSOLE_LOG_DEBUG("Listing storage pools");

    auto pools_result = db_manager_->list_storage_pools();
    if (!pools_result) {
        return Err<Vector<PoolDetails>>(
            ApiError(HttpStatus::InternalServerError, "Failed to list pools: " + pools_result.error()));
    }

    Vector<PoolDetails> details;
    for (const auto& pool : pools_result.value()) {
        details.push_back(db_pool_to_details(pool));
    }

    return Result<Vector<PoolDetails>, ApiError>(ok_tag, details);
}

Result<PoolDetails, ApiError>
InfrastructureService::get_pool(const UserInfo& user, const String& id) {
    CONSOLE_LOG_DEBUG("Getting pool: {}", id);

    auto pool_result = db_manager_->get_storage_pool(id);
    if (!pool_result) {
        return Err<PoolDetails>(ApiError(HttpStatus::NotFound, "Pool not found: " + id));
    }

    return Result<PoolDetails, ApiError>(ok_tag, db_pool_to_details(pool_result.value()));
}

Result<PoolDetails, ApiError>
InfrastructureService::configure_pool(const UserInfo& user, const ConfigurePoolRequest& request) {
    CONSOLE_LOG_INFO("Configuring pool: {} with {} servers", request.name, request.server_ids.size());

    // Validate erasure coding configuration
    int set_size = request.erasure_data_shards + request.erasure_parity_shards;
    if (set_size < 4 || set_size > 16) {
        return Err<PoolDetails>(ApiError(HttpStatus::BadRequest, "Erasure set size must be between 4 and 16 drives"));
    }

    // Count total drives across servers
    int total_drives = 0;
    int64_t total_capacity = 0;

    for (const auto& server_id : request.server_ids) {
        auto drives_result = db_manager_->list_drives(server_id);
        if (drives_result) {
            for (const auto& drive : drives_result.value()) {
                if (drive.status == "online") {
                    total_drives++;
                    total_capacity += drive.capacity;
                }
            }
        }
    }

    if (total_drives < set_size) {
        return Err<PoolDetails>(
            ApiError(HttpStatus::BadRequest,
                     "Not enough drives. Need at least " + std::to_string(set_size) + " drives for erasure coding"));
    }

    // Create/update pool
    storage::DbStoragePool pool;
    pool.id = generate_pool_id();
    pool.name = request.name;
    pool.capacity = calculate_usable_capacity(total_capacity,
                                              request.erasure_data_shards,
                                              request.erasure_parity_shards);
    pool.used = 0;
    pool.available = pool.capacity;
    pool.drives_count = total_drives;
    pool.online_drives = total_drives;
    pool.offline_drives = 0;
    pool.last_update = std::time(nullptr);

    // Store erasure config in metadata as JSON
    Json::Value metadata;
    metadata["erasure_data_shards"] = request.erasure_data_shards;
    metadata["erasure_parity_shards"] = request.erasure_parity_shards;
    metadata["erasure_sets_count"] = calculate_erasure_sets(total_drives, set_size);

    Json::Value server_ids(Json::arrayValue);
    for (const auto& sid : request.server_ids) {
        server_ids.append(sid);
    }
    metadata["server_ids"] = server_ids;

    Json::StreamWriterBuilder writer;
    pool.metadata = Json::writeString(writer, metadata);

    auto upsert_result = db_manager_->upsert_storage_pool(pool);
    if (!upsert_result) {
        return Err<PoolDetails>(
            ApiError(HttpStatus::InternalServerError, "Failed to configure pool: " + upsert_result.error()));
    }

    CONSOLE_LOG_INFO("Pool configured: {} with {} drives in {} erasure sets",
                     pool.name,
                     total_drives,
                     calculate_erasure_sets(total_drives, set_size));

    return get_pool(user, pool.id);
}

Result<void, ApiError>
InfrastructureService::decommission_pool(const UserInfo& user, const String& id) {
    CONSOLE_LOG_INFO("Decommissioning pool: {}", id);

    auto pool_result = db_manager_->get_storage_pool(id);
    if (!pool_result) {
        return Result<void, ApiError>(err_tag, ApiError(HttpStatus::NotFound, "Pool not found: " + id));
    }

    auto pool = pool_result.value();
    if (pool.used > 0) {
        return Result<void, ApiError>(
            err_tag, ApiError(HttpStatus::Conflict, "Cannot decommission pool with data. Migrate data first."));
    }

    auto delete_result = db_manager_->delete_storage_pool(id);
    if (!delete_result) {
        return Result<void, ApiError>(err_tag,
                                      ApiError(HttpStatus::InternalServerError,
                                               "Failed to decommission pool: " + delete_result.error()));
    }

    CONSOLE_LOG_INFO("Pool decommissioned: {}", id);
    return Ok<ApiError>();
}

// ============================================================================
// Healing Operations
// ============================================================================

Result<Json::Value, ApiError>
InfrastructureService::get_heal_status(const UserInfo& user) {
    CONSOLE_LOG_DEBUG("Getting heal status");

    Json::Value status;
    status["healing"] = false;
    status["progress"] = 0;
    status["items_healed"] = 0;
    status["items_failed"] = 0;
    status["bytes_healed"] = 0;

    // Get drives needing healing
    auto drives_result = db_manager_->list_drives();
    if (drives_result) {
        Json::Value drives_status(Json::arrayValue);
        for (const auto& drive : drives_result.value()) {
            if (drive.status == "healing" || drive.status == "faulty") {
                Json::Value drive_json;
                drive_json["id"] = drive.id;
                drive_json["status"] = drive.status;
                drive_json["path"] = drive.path;
                drives_status.append(drive_json);
            }
        }
        status["drives_needing_heal"] = drives_status;
    }

    return Result<Json::Value, ApiError>(ok_tag, status);
}

Result<void, ApiError>
InfrastructureService::start_heal(const UserInfo& user, const String& pool_id) {
    CONSOLE_LOG_INFO("Starting heal for pool: {}", pool_id.empty() ? "all" : pool_id);

    // Get drives that need healing (offline status)
    auto drives_result = db_manager_->list_drives();
    if (!drives_result) {
        return Err<void>(ApiError(HttpStatus::InternalServerError, "Failed to list drives"));
    }

    int healed_count = 0;
    auto now = std::time(nullptr);

    for (auto& drive : drives_result.value()) {
        // Skip drives not in the target pool (if specified)
        if (!pool_id.empty()) {
            String drive_pool_id = "pool-" + drive.server_id;
            if (drive_pool_id != pool_id) {
                continue;
            }
        }

        // For local storage, healing means verifying the drive is accessible
        // and updating its status if the storage path exists
        if (drive.status == "offline") {
            // Check if we can access the storage
            auto bucket_service = ServiceLocator::bucket_service();
            if (bucket_service) {
                UserInfo system_user;
                system_user.account_name = "system";
                system_user.is_admin = true;

                auto check_result = bucket_service->list_buckets(system_user);
                if (check_result.is_ok()) {
                    // Storage is accessible, mark drive as online
                    drive.status = "online";
                    drive.last_check = now;
                    db_manager_->upsert_drive(drive);
                    healed_count++;
                    CONSOLE_LOG_INFO("Healed drive: {} (now online)", drive.id);
                }
            }
        }
    }

    CONSOLE_LOG_INFO("Heal process completed: {} drives restored", healed_count);

    return Ok<ApiError>();
}

// ============================================================================
// Helper Methods
// ============================================================================

ServerDetails
InfrastructureService::db_server_to_details(const storage::DbServer& server) {
    ServerDetails details;
    details.id = server.id;
    details.name = server.name;
    details.endpoint = server.endpoint;
    details.status = server.status;
    details.uptime = server.uptime;
    details.last_heartbeat = server.last_heartbeat;

    // Get drive stats for this server
    auto drives_result = db_manager_->list_drives(server.id);
    if (drives_result) {
        details.drives_count = static_cast<int>(drives_result.value().size());
        for (const auto& drive : drives_result.value()) {
            if (drive.status == "online") {
                details.online_drives++;
            } else {
                details.offline_drives++;
            }
            details.total_capacity += drive.capacity;
            details.used_capacity += drive.used;
            details.available_capacity += drive.available;
        }
    }

    return details;
}

DriveDetails
InfrastructureService::db_drive_to_details(const storage::DbDrive& drive) {
    DriveDetails details;
    details.id = drive.id;
    details.server_id = drive.server_id;
    details.path = drive.path;
    details.status = drive.status;
    details.capacity = drive.capacity;
    details.used = drive.used;
    details.available = drive.available;
    details.last_check = drive.last_check;

    // Calculate usage percent
    if (drive.capacity > 0) {
        details.usage_percent = static_cast<double>(drive.used) / static_cast<double>(drive.capacity) * 100.0;
    }

    // Determine health based on usage and status
    if (drive.status != "online") {
        details.health = "critical";
    } else if (details.usage_percent > 90.0) {
        details.health = "warning";
    } else {
        details.health = "healthy";
    }

    // Get server name
    auto server_result = db_manager_->get_server(drive.server_id);
    if (server_result) {
        details.server_name = server_result.value().name;
    }

    return details;
}

PoolDetails
InfrastructureService::db_pool_to_details(const storage::DbStoragePool& pool) {
    PoolDetails details;
    details.id = pool.id;
    details.name = pool.name;
    details.capacity = pool.capacity;
    details.used = pool.used;
    details.available = pool.available;
    details.drives_count = pool.drives_count;
    details.online_drives = pool.online_drives;
    details.offline_drives = pool.offline_drives;
    details.last_update = pool.last_update;

    // Calculate usage percent
    if (pool.capacity > 0) {
        details.usage_percent = static_cast<double>(pool.used) / static_cast<double>(pool.capacity) * 100.0;
    }

    // Determine status
    if (pool.offline_drives == 0 && pool.online_drives > 0) {
        details.status = "online";
    } else if (pool.online_drives == 0) {
        details.status = "offline";
    } else {
        details.status = "degraded";
    }

    details.object_data = pool.used;

    // Parse metadata for erasure config
    if (!pool.metadata.empty()) {
        Json::CharReaderBuilder reader;
        Json::Value metadata;
        std::istringstream s(pool.metadata);
        std::string errors;

        if (Json::parseFromStream(reader, s, &metadata, &errors)) {
            details.erasure_data_shards = metadata.get("erasure_data_shards", 8).asInt();
            details.erasure_parity_shards = metadata.get("erasure_parity_shards", 4).asInt();
            details.erasure_sets_count = metadata.get("erasure_sets_count", 1).asInt();

            int set_size = details.erasure_data_shards + details.erasure_parity_shards;
            details.erasure_set_size = std::to_string(set_size) + " drives per set";

            // Count servers from metadata
            if (metadata.isMember("server_ids")) {
                details.servers_count = static_cast<int>(metadata["server_ids"].size());
            }
        }
    }

    // Default erasure config if not in metadata
    if (details.erasure_data_shards == 0) {
        details.erasure_data_shards = 8;
        details.erasure_parity_shards = 4;
        details.erasure_set_size = "12 drives per set";
        details.erasure_sets_count = pool.drives_count / 12;
        if (details.erasure_sets_count == 0)
            details.erasure_sets_count = 1;
    }

    return details;
}

String
InfrastructureService::generate_server_id() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    ss << "server-";
    for (int i = 0; i < 8; i++) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

String
InfrastructureService::generate_drive_id(const String& server_id) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    ss << server_id << "-drive-";
    for (int i = 0; i < 4; i++) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

String
InfrastructureService::generate_pool_id() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    ss << "pool-";
    for (int i = 0; i < 8; i++) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

int
InfrastructureService::calculate_erasure_sets(int drives_count, int set_size) {
    if (set_size <= 0)
        return 0;
    return drives_count / set_size;
}

int64_t
InfrastructureService::calculate_usable_capacity(int64_t raw_capacity, int data_shards, int parity_shards) {
    // Usable capacity = raw_capacity * (data_shards / total_shards)
    int total_shards = data_shards + parity_shards;
    if (total_shards <= 0)
        return raw_capacity;

    return raw_capacity * data_shards / total_shards;
}

} // namespace console::services
