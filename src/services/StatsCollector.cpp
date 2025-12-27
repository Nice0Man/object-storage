#include "console/services/StatsCollector.hpp"

#include "console/common/Config.hpp"
#include "console/common/Logger.hpp"
#include "console/common/ServiceLocator.hpp"
#include "console/services/BucketService.hpp"

#include <chrono>
#include <filesystem>

namespace console::services {

StatsCollector::StatsCollector(std::shared_ptr<storage::DatabaseManager> database) : database_(std::move(database)) {
    CONSOLE_LOG_DEBUG("StatsCollector created");
}

StatsCollector::~StatsCollector() {
    stop();
    CONSOLE_LOG_DEBUG("StatsCollector destroyed");
}

void
StatsCollector::start() {
    if (running_.exchange(true)) {
        CONSOLE_LOG_WARN("StatsCollector already running");
        return;
    }

    CONSOLE_LOG_INFO("Starting StatsCollector background tasks");

    // Initialize timestamps
    auto now = std::time(nullptr);
    last_server_update_ = now;
    last_drive_update_ = now;
    last_pool_update_ = now;
    last_cleanup_ = now;

    // Start worker thread
    worker_thread_ = std::thread(&StatsCollector::worker_loop, this);

    CONSOLE_LOG_INFO("StatsCollector started successfully");
}

void
StatsCollector::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    CONSOLE_LOG_INFO("Stopping StatsCollector...");

    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }

    CONSOLE_LOG_INFO("StatsCollector stopped");
}

bool
StatsCollector::is_running() const {
    return running_;
}

void
StatsCollector::worker_loop() {
    CONSOLE_LOG_DEBUG("StatsCollector worker thread started");

    while (running_) {
        auto now = std::time(nullptr);

        // Check if it's time for server update
        if (now - last_server_update_ >= SERVER_UPDATE_INTERVAL) {
            collect_server_stats();
            last_server_update_ = now;
        }

        // Check if it's time for drive update
        if (now - last_drive_update_ >= DRIVE_UPDATE_INTERVAL) {
            collect_drive_stats();
            last_drive_update_ = now;
        }

        // Check if it's time for pool aggregation
        if (now - last_pool_update_ >= POOL_UPDATE_INTERVAL) {
            aggregate_pool_stats();
            last_pool_update_ = now;
        }

        // Check if it's time for cleanup
        if (now - last_cleanup_ >= CLEANUP_INTERVAL) {
            cleanup_old_data();
            last_cleanup_ = now;
        }

        // Sleep for 1 second between checks
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    CONSOLE_LOG_DEBUG("StatsCollector worker thread stopped");
}

void
StatsCollector::collect_server_stats() {
    if (!database_)
        return;

    CONSOLE_LOG_DEBUG("Collecting server statistics...");

    // Get existing servers and update heartbeat
    auto servers_result = database_->list_servers();
    if (!servers_result) {
        CONSOLE_LOG_WARN("Failed to get servers for heartbeat update");
        return;
    }

    auto now = std::time(nullptr);

    // Check actual S3 server availability via BucketService
    bool s3_available = false;
    auto bucket_service = ServiceLocator::bucket_service();
    if (bucket_service) {
        // Try to list buckets - if it works, server is online
        UserInfo system_user;
        system_user.account_name = "system";
        system_user.is_admin = true;

        auto buckets_result = bucket_service->list_buckets(system_user);
        s3_available = buckets_result.is_ok();

        if (!s3_available) {
            CONSOLE_LOG_WARN("S3 server health check failed: unable to list buckets");
        }
    }

    for (auto& server : servers_result.value()) {
        // Update last heartbeat
        server.last_heartbeat = now;

        // Update status based on actual S3 availability
        String old_status = server.status;
        server.status = s3_available ? "online" : "offline";

        // Update uptime: if online, calculate from start time; if offline, set to 0
        if (s3_available) {
            // uptime is stored as start timestamp, calculate seconds from then
            if (server.uptime > 0) {
                // Keep existing start time
            } else {
                // Server just came online, record start time
                server.uptime = now;
            }
        } else {
            server.uptime = 0;
        }

        if (old_status != server.status) {
            CONSOLE_LOG_INFO("Server '{}' status changed: {} -> {}", server.name, old_status, server.status);
        }

        database_->upsert_server(server);
    }

    CONSOLE_LOG_DEBUG("Server statistics updated for {} servers", servers_result.value().size());
}

void
StatsCollector::collect_drive_stats() {
    if (!database_)
        return;

    CONSOLE_LOG_DEBUG("Collecting drive statistics...");

    // Get existing drives and update check time
    auto drives_result = database_->list_drives();
    if (!drives_result) {
        CONSOLE_LOG_WARN("Failed to get drives for status update");
        return;
    }

    auto now = std::time(nullptr);

    // Get storage root path from config
    auto& config = Config::instance();
    auto storage_root = config.get<String>("storage.root_path").value_or("./storage");

    // Check S3 server availability
    bool s3_available = false;
    auto bucket_service = ServiceLocator::bucket_service();
    if (bucket_service) {
        UserInfo system_user;
        system_user.account_name = "system";
        system_user.is_admin = true;

        auto buckets_result = bucket_service->list_buckets(system_user);
        s3_available = buckets_result.is_ok();
    }

    for (auto& drive : drives_result.value()) {
        // Update last check time
        drive.last_check = now;

        // Update drive status based on S3 availability
        drive.status = s3_available ? "online" : "offline";

        // Get actual filesystem space info
        try {
            auto space_info = std::filesystem::space(storage_root);
            drive.capacity = static_cast<int64_t>(space_info.capacity);
            drive.available = static_cast<int64_t>(space_info.available);
            drive.used = drive.capacity - drive.available;
        } catch (const std::exception& e) {
            CONSOLE_LOG_DEBUG("Could not update filesystem space info: {}", e.what());
        }

        database_->upsert_drive(drive);
    }

    CONSOLE_LOG_DEBUG("Drive statistics updated for {} drives", drives_result.value().size());
}

void
StatsCollector::aggregate_pool_stats() {
    if (!database_)
        return;

    CONSOLE_LOG_DEBUG("Aggregating storage pool statistics...");

    // Get all drives
    auto drives_result = database_->list_drives();
    if (!drives_result) {
        CONSOLE_LOG_WARN("Failed to get drives for pool aggregation");
        return;
    }

    // Group drives by server (one pool per server)
    std::map<String, storage::DbStoragePool> pools;

    for (const auto& drive : drives_result.value()) {
        String pool_id = "pool-" + drive.server_id;

        auto& pool = pools[pool_id];
        if (pool.id.empty()) {
            pool.id = pool_id;
            // Use descriptive pool name
            if (drive.server_id == "primary-s3") {
                pool.name = "Primary Storage Pool";
            } else {
                pool.name = "Storage Pool (" + drive.server_id + ")";
            }
        }

        pool.capacity += drive.capacity;
        pool.used += drive.used;
        pool.available += drive.available;
        pool.drives_count++;

        if (drive.status == "online") {
            pool.online_drives++;
        } else {
            pool.offline_drives++;
        }
    }

    // Save aggregated pools
    auto now = std::time(nullptr);
    for (auto& [id, pool] : pools) {
        pool.last_update = now;
        database_->upsert_storage_pool(pool);
    }

    CONSOLE_LOG_DEBUG("Pool statistics aggregated for {} pools", pools.size());
}

void
StatsCollector::cleanup_old_data() {
    if (!database_)
        return;

    CONSOLE_LOG_INFO("Running cleanup of old statistics data...");

    auto now = std::time(nullptr);
    auto week_ago = now - (7 * 24 * 3600);

    auto api_cleanup = database_->cleanup_old_api_stats(week_ago);
    if (!api_cleanup) {
        CONSOLE_LOG_WARN("Failed to cleanup old API stats: {}", api_cleanup.error());
    }

    auto throughput_cleanup = database_->cleanup_old_throughput_stats(week_ago);
    if (!throughput_cleanup) {
        CONSOLE_LOG_WARN("Failed to cleanup old throughput stats: {}", throughput_cleanup.error());
    }

    CONSOLE_LOG_INFO("Cleanup of old statistics completed");
}

void
StatsCollector::update_server_stats_now() {
    collect_server_stats();
}

void
StatsCollector::update_drive_stats_now() {
    collect_drive_stats();
}

void
StatsCollector::update_pool_stats_now() {
    aggregate_pool_stats();
}

void
StatsCollector::cleanup_old_stats_now(int days_to_keep) {
    if (!database_)
        return;

    auto now = std::time(nullptr);
    auto cutoff = now - (days_to_keep * 24 * 3600);

    database_->cleanup_old_api_stats(cutoff);
    database_->cleanup_old_throughput_stats(cutoff);
}

void
StatsCollector::initialize_sample_data() {
    if (!database_)
        return;

    CONSOLE_LOG_INFO("Initializing storage infrastructure data from configuration...");

    auto now = std::time(nullptr);

    // Get S3 endpoint from configuration
    auto& config = Config::instance();
    const auto& s3_config = config.s3();

    String s3_endpoint = s3_config.endpoint;
    if (s3_endpoint.empty()) {
        s3_endpoint = "localhost:9000";
    }

    // Build proper URL with protocol
    String endpoint_url;
    if (s3_endpoint.find("://") == String::npos) {
        endpoint_url = (s3_config.use_ssl ? "https://" : "http://") + s3_endpoint;
    } else {
        endpoint_url = s3_endpoint;
    }

    // Check if server already exists and cleanup old mock data
    auto existing_servers = database_->list_servers();
    bool server_exists = false;
    if (existing_servers) {
        for (const auto& srv : existing_servers.value()) {
            // Remove old mock servers (server-1, server-2, etc.)
            if (srv.id.find("server-") == 0 && srv.id != "primary-s3") {
                database_->delete_server(srv.id);
                CONSOLE_LOG_INFO("Removed legacy mock server: {}", srv.id);
                continue;
            }

            if (srv.endpoint == endpoint_url || srv.id == "primary-s3") {
                server_exists = true;
            }
        }
    }

    if (!server_exists) {
        // Check actual S3 availability
        bool s3_available = false;
        auto bucket_service = ServiceLocator::bucket_service();
        if (bucket_service) {
            UserInfo system_user;
            system_user.account_name = "system";
            system_user.is_admin = true;

            auto buckets_result = bucket_service->list_buckets(system_user);
            s3_available = buckets_result.is_ok();
        }

        // Create server entry based on real configuration
        storage::DbServer server;
        server.id = "primary-s3";
        server.name = "Object Storage Server";
        server.endpoint = endpoint_url;
        server.status = s3_available ? "online" : "offline";
        server.uptime = s3_available ? now : 0; // Store start time when online
        server.last_heartbeat = now;

        database_->upsert_server(server);

        CONSOLE_LOG_INFO("Storage server registered: {} ({})", server.name, server.status);

        // Create a single logical drive for the storage
        storage::DbDrive drive;
        drive.id = "primary-s3-storage";
        drive.server_id = "primary-s3";
        drive.path = "/storage";
        drive.status = s3_available ? "online" : "offline";

        // Get actual storage info from filesystem if available
        auto storage_root = config.get<String>("storage.root_path").value_or("./storage");
        try {
            auto space_info = std::filesystem::space(storage_root);
            drive.capacity = static_cast<int64_t>(space_info.capacity);
            drive.available = static_cast<int64_t>(space_info.available);
            drive.used = drive.capacity - drive.available;
        } catch (const std::exception& e) {
            CONSOLE_LOG_WARN("Could not get filesystem space info: {}", e.what());
            // Fallback to reasonable defaults
            drive.capacity = 100LL * 1024 * 1024 * 1024; // 100 GiB default
            drive.used = 0;
            drive.available = drive.capacity;
        }
        drive.last_check = now;

        database_->upsert_drive(drive);
    }

    // Aggregate pools
    aggregate_pool_stats();

    CONSOLE_LOG_INFO("Storage infrastructure data initialized from configuration");
}

} // namespace console::services
