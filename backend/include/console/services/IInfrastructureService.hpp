#pragma once

#include "console/common/Types.hpp"
#include "console/models/Error.hpp"
#include "console/storage/DatabaseManager.hpp"

#include <json/json.h>

namespace console::services {

/**
 * @brief Server info with extended details
 */
struct ServerDetails {
    String id;
    String name;
    String endpoint;
    String status; // online, offline, degraded
    int64_t uptime{0};
    int64_t last_heartbeat{0};
    int drives_count{0};
    int online_drives{0};
    int offline_drives{0};
    int64_t total_capacity{0};
    int64_t used_capacity{0};
    int64_t available_capacity{0};
    String version;
    String region;

    Json::Value to_json() const {
        Json::Value json;
        json["id"] = id;
        json["name"] = name;
        json["endpoint"] = endpoint;
        json["status"] = status;
        json["uptime"] = static_cast<Json::Int64>(uptime);
        json["last_heartbeat"] = static_cast<Json::Int64>(last_heartbeat);
        json["drives_count"] = drives_count;
        json["online_drives"] = online_drives;
        json["offline_drives"] = offline_drives;
        json["total_capacity"] = static_cast<Json::Int64>(total_capacity);
        json["used_capacity"] = static_cast<Json::Int64>(used_capacity);
        json["available_capacity"] = static_cast<Json::Int64>(available_capacity);
        json["version"] = version;
        json["region"] = region;
        return json;
    }
};

/**
 * @brief Drive info with extended details
 */
struct DriveDetails {
    String id;
    String server_id;
    String server_name;
    String path;
    String status; // online, offline, healing, faulty
    int64_t capacity{0};
    int64_t used{0};
    int64_t available{0};
    double usage_percent{0.0};
    int64_t last_check{0};
    String health; // healthy, warning, critical
    int read_errors{0};
    int write_errors{0};

    Json::Value to_json() const {
        Json::Value json;
        json["id"] = id;
        json["server_id"] = server_id;
        json["server_name"] = server_name;
        json["path"] = path;
        json["status"] = status;
        json["capacity"] = static_cast<Json::Int64>(capacity);
        json["used"] = static_cast<Json::Int64>(used);
        json["available"] = static_cast<Json::Int64>(available);
        json["usage_percent"] = usage_percent;
        json["last_check"] = static_cast<Json::Int64>(last_check);
        json["health"] = health;
        json["read_errors"] = read_errors;
        json["write_errors"] = write_errors;
        return json;
    }
};

/**
 * @brief Storage pool with erasure coding info
 */
struct PoolDetails {
    String id;
    String name;
    String status; // online, offline, degraded
    int64_t capacity{0};
    int64_t used{0};
    int64_t available{0};
    double usage_percent{0.0};
    int drives_count{0};
    int online_drives{0};
    int offline_drives{0};
    int servers_count{0};

    // Erasure coding configuration
    int erasure_data_shards{0};   // Number of data shards (e.g., 8)
    int erasure_parity_shards{0}; // Number of parity shards (e.g., 4)
    String erasure_set_size;      // e.g., "12 drives per set"
    int erasure_sets_count{0};    // Number of erasure sets

    int64_t object_data{0}; // Actual object data stored
    int64_t last_update{0};

    Json::Value to_json() const {
        Json::Value json;
        json["id"] = id;
        json["name"] = name;
        json["status"] = status;
        json["capacity"] = static_cast<Json::Int64>(capacity);
        json["used"] = static_cast<Json::Int64>(used);
        json["available"] = static_cast<Json::Int64>(available);
        json["usage_percent"] = usage_percent;
        json["drives_count"] = drives_count;
        json["online_drives"] = online_drives;
        json["offline_drives"] = offline_drives;
        json["servers_count"] = servers_count;

        // Erasure coding info
        Json::Value erasure;
        erasure["data_shards"] = erasure_data_shards;
        erasure["parity_shards"] = erasure_parity_shards;
        erasure["set_size"] = erasure_set_size;
        erasure["sets_count"] = erasure_sets_count;
        json["erasure_coding"] = erasure;

        json["object_data"] = static_cast<Json::Int64>(object_data);
        json["last_update"] = static_cast<Json::Int64>(last_update);
        return json;
    }
};

/**
 * @brief Dashboard summary for servers/drives/pools
 */
struct InfrastructureSummary {
    // Capacity
    int64_t total_capacity{0};
    int64_t used_capacity{0};
    int64_t available_capacity{0};
    int64_t object_data{0};

    // Servers
    int total_servers{0};
    int online_servers{0};
    int offline_servers{0};

    // Drives
    int total_drives{0};
    int online_drives{0};
    int offline_drives{0};

    // Pools
    int total_pools{0};
    Vector<PoolDetails> pools;

    Json::Value to_json() const {
        Json::Value json;

        // Capacity
        Json::Value capacity;
        capacity["total"] = static_cast<Json::Int64>(total_capacity);
        capacity["used"] = static_cast<Json::Int64>(used_capacity);
        capacity["available"] = static_cast<Json::Int64>(available_capacity);
        capacity["object_data"] = static_cast<Json::Int64>(object_data);
        json["capacity"] = capacity;

        // Servers
        Json::Value servers;
        servers["total"] = total_servers;
        servers["online"] = online_servers;
        servers["offline"] = offline_servers;
        json["servers"] = servers;

        // Drives
        Json::Value drives;
        drives["total"] = total_drives;
        drives["online"] = online_drives;
        drives["offline"] = offline_drives;
        json["drives"] = drives;

        // Pools
        Json::Value pools_json(Json::arrayValue);
        for (const auto& pool : pools) {
            pools_json.append(pool.to_json());
        }
        json["pools"] = pools_json;
        json["pools_count"] = total_pools;

        return json;
    }
};

/**
 * @brief Request to add a new server
 */
struct AddServerRequest {
    String name;
    String endpoint;
    String region;
    Vector<String> drive_paths; // Paths to storage drives on this server
};

/**
 * @brief Request to add a new drive
 */
struct AddDriveRequest {
    String server_id;
    String path;
    int64_t capacity{0}; // 0 = auto-detect
};

/**
 * @brief Request to configure a pool
 */
struct ConfigurePoolRequest {
    String name;
    Vector<String> server_ids;
    int erasure_data_shards{8};
    int erasure_parity_shards{4};
};

/**
 * @brief Interface for infrastructure management
 */
class IInfrastructureService {
  public:
    virtual ~IInfrastructureService() = default;

    // Summary
    virtual Result<InfrastructureSummary, models::ApiError> get_summary(const UserInfo& user) = 0;

    // Server operations
    virtual Result<Vector<ServerDetails>, models::ApiError> list_servers(const UserInfo& user) = 0;
    virtual Result<ServerDetails, models::ApiError> get_server(const UserInfo& user, const String& id) = 0;
    virtual Result<ServerDetails, models::ApiError> add_server(const UserInfo& user,
                                                               const AddServerRequest& request) = 0;
    virtual Result<void, models::ApiError> remove_server(const UserInfo& user, const String& id) = 0;
    virtual Result<ServerDetails, models::ApiError> check_server_health(const UserInfo& user, const String& id) = 0;

    // Drive operations
    virtual Result<Vector<DriveDetails>, models::ApiError> list_drives(const UserInfo& user,
                                                                       const String& server_id = "") = 0;
    virtual Result<DriveDetails, models::ApiError> get_drive(const UserInfo& user, const String& id) = 0;
    virtual Result<DriveDetails, models::ApiError> add_drive(const UserInfo& user, const AddDriveRequest& request) = 0;
    virtual Result<void, models::ApiError> remove_drive(const UserInfo& user, const String& id) = 0;
    virtual Result<void, models::ApiError> set_drive_status(const UserInfo& user,
                                                            const String& id,
                                                            const String& status) = 0;

    // Pool operations
    virtual Result<Vector<PoolDetails>, models::ApiError> list_pools(const UserInfo& user) = 0;
    virtual Result<PoolDetails, models::ApiError> get_pool(const UserInfo& user, const String& id) = 0;
    virtual Result<PoolDetails, models::ApiError> configure_pool(const UserInfo& user,
                                                                 const ConfigurePoolRequest& request) = 0;
    virtual Result<void, models::ApiError> decommission_pool(const UserInfo& user, const String& id) = 0;

    // Healing operations
    virtual Result<Json::Value, models::ApiError> get_heal_status(const UserInfo& user) = 0;
    virtual Result<void, models::ApiError> start_heal(const UserInfo& user, const String& pool_id = "") = 0;
};

} // namespace console::services
