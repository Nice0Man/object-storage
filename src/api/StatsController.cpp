#include "console/api/StatsController.hpp"

#include "console/clients/LocalAdminClient.hpp"
#include "console/common/Logger.hpp"
#include "console/common/ServiceLocator.hpp"
#include "console/services/BucketService.hpp"
#include "console/services/ObjectService.hpp"
#include "console/services/UserService.hpp"
#include "console/storage/DatabaseManager.hpp"

#include <ctime>
#include <filesystem>
#include <json/json.h>

namespace console::api {

UserInfo
StatsController::get_user_from_request(const drogon::HttpRequestPtr& req) {
    try {
        return req->attributes()->get<UserInfo>("user_info");
    } catch (...) {
        CONSOLE_LOG_ERROR("No user_info found in request attributes");
        return UserInfo{};
    }
}

void
StatsController::get_system_stats(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto user_info = get_user_from_request(req);

    Json::Value stats;

    try {
        // Get bucket service
        auto bucket_service = ServiceLocator::bucket_service();
        if (!bucket_service) {
            Json::Value error;
            error["error"] = "Service not available";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        // Get buckets
        auto buckets_result = bucket_service->list_buckets(user_info);

        int64_t total_size = 0;
        int64_t total_objects = 0;
        int bucket_count = 0;

        if (buckets_result.is_ok()) {
            bucket_count = buckets_result.value().size();
            for (const auto& bucket : buckets_result.value()) {
                total_size += bucket.size_bytes();
                total_objects += bucket.object_count();
            }
        }

        // Get user count (only if user is admin to avoid warning logs)
        int user_count = 0;
        if (user_info.is_admin) {
            auto user_service = ServiceLocator::user_service();
            if (user_service) {
                auto users_result = user_service->list_users(user_info);
                if (users_result.is_ok()) {
                    user_count = users_result.value().size();
                }
            }
        } else {
            // For non-admin users, try to get user count from admin client directly
            auto admin_client = ServiceLocator::admin_client();
            if (admin_client) {
                auto users_result = admin_client->list_users();
                if (users_result.is_ok()) {
                    user_count = users_result.value().size();
                }
            }
        }

        // Build response
        stats["buckets"] = bucket_count;
        stats["objects"] = static_cast<Json::Int64>(total_objects);
        stats["users"] = user_count;
        stats["storage_used"] = static_cast<Json::Int64>(total_size);

        // Get storage capacity from pools/drives
        int64_t storage_total = 0;
        auto db_manager = ServiceLocator::database();
        if (db_manager) {
            auto pools_result = db_manager->list_storage_pools();
            if (pools_result) {
                for (const auto& pool : pools_result.value()) {
                    storage_total += pool.capacity;
                }
            }
            // If no pools, try drives directly
            if (storage_total == 0) {
                auto drives_result = db_manager->list_drives();
                if (drives_result) {
                    for (const auto& drive : drives_result.value()) {
                        storage_total += drive.capacity;
                    }
                }
            }
        }
        // Fallback to reasonable default if no infrastructure data
        if (storage_total == 0) {
            storage_total = 1LL * 1024 * 1024 * 1024 * 1024; // 1 TiB default
        }
        stats["storage_total"] = static_cast<Json::Int64>(storage_total);
        stats["storage_available"] = static_cast<Json::Int64>(storage_total - total_size);

        // System status
        stats["status"] = "online";
        stats["timestamp"] = static_cast<Json::Int64>(std::time(nullptr));

        auto resp = drogon::HttpResponse::newHttpJsonResponse(stats);
        callback(resp);

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Error getting system stats: {}", e.what());
        Json::Value error;
        error["error"] = "Internal server error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

void
StatsController::get_capacity(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto user_info = get_user_from_request(req);

    Json::Value capacity;

    try {
        auto bucket_service = ServiceLocator::bucket_service();
        if (!bucket_service) {
            Json::Value error;
            error["error"] = "Service not available";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        auto buckets_result = bucket_service->list_buckets(user_info);

        int64_t total_size = 0;
        if (buckets_result.is_ok()) {
            for (const auto& bucket : buckets_result.value()) {
                total_size += bucket.size_bytes();
            }
        }

        // Get storage capacity from pools/drives
        int64_t storage_total = 0;
        auto db_manager = ServiceLocator::database();
        if (db_manager) {
            auto pools_result = db_manager->list_storage_pools();
            if (pools_result) {
                for (const auto& pool : pools_result.value()) {
                    storage_total += pool.capacity;
                }
            }
            if (storage_total == 0) {
                auto drives_result = db_manager->list_drives();
                if (drives_result) {
                    for (const auto& drive : drives_result.value()) {
                        storage_total += drive.capacity;
                    }
                }
            }
        }
        if (storage_total == 0) {
            storage_total = 1LL * 1024 * 1024 * 1024 * 1024; // 1 TiB default
        }

        // Capacity information
        capacity["total"] = static_cast<Json::Int64>(storage_total);
        capacity["used"] = static_cast<Json::Int64>(total_size);
        capacity["available"] = static_cast<Json::Int64>(storage_total - total_size);
        capacity["usage_percent"] = storage_total > 0 ? static_cast<double>(total_size) / storage_total * 100.0 : 0.0;

        auto resp = drogon::HttpResponse::newHttpJsonResponse(capacity);
        callback(resp);

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Error getting capacity: {}", e.what());
        Json::Value error;
        error["error"] = "Internal server error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

void
StatsController::get_activity(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto user_info = get_user_from_request(req);

    Json::Value activity;

    try {
        auto bucket_service = ServiceLocator::bucket_service();
        if (!bucket_service) {
            Json::Value error;
            error["error"] = "Service not available";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        auto buckets_result = bucket_service->list_buckets(user_info);

        activity["recent_buckets"] = Json::Value(Json::arrayValue);

        if (buckets_result.is_ok()) {
            // Get up to 10 most recent buckets by creation date
            auto buckets = buckets_result.value();

            // Sort by creation date (most recent first)
            std::sort(buckets.begin(), buckets.end(), [](const auto& a, const auto& b) {
                return a.creation_date() > b.creation_date();
            });

            int count = 0;
            for (const auto& bucket : buckets) {
                if (count >= 10)
                    break;

                Json::Value bucket_activity;
                bucket_activity["name"] = bucket.name();
                bucket_activity["objects"] = static_cast<Json::Int64>(bucket.object_count());
                bucket_activity["size"] = static_cast<Json::Int64>(bucket.size_bytes());

                // Convert time_point to timestamp
                auto time_t_val = std::chrono::system_clock::to_time_t(bucket.creation_date());
                bucket_activity["timestamp"] = static_cast<Json::Int64>(time_t_val);

                activity["recent_buckets"].append(bucket_activity);
                count++;
            }
        }

        activity["total_activity"] = activity["recent_buckets"].size();

        auto resp = drogon::HttpResponse::newHttpJsonResponse(activity);
        callback(resp);

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Error getting activity: {}", e.what());
        Json::Value error;
        error["error"] = "Internal server error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

void
StatsController::get_servers(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto user_info = get_user_from_request(req);

    Json::Value response;

    try {
        auto db_manager = ServiceLocator::database();
        if (!db_manager) {
            Json::Value error;
            error["error"] = "Database not available";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        auto servers_result = db_manager->list_servers();
        if (!servers_result) {
            CONSOLE_LOG_ERROR("Failed to get servers: {}", servers_result.error());
            Json::Value error;
            error["error"] = "Failed to retrieve server data";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        Json::Value servers(Json::arrayValue);
        int online_count = 0;
        int offline_count = 0;
        auto now = std::time(nullptr);

        for (const auto& server : servers_result.value()) {
            Json::Value server_json;
            server_json["id"] = server.id;
            server_json["name"] = server.name;
            server_json["status"] = server.status;
            server_json["endpoint"] = server.endpoint;

            // Calculate uptime in seconds from start timestamp
            // server.uptime stores the start timestamp when server came online
            int64_t uptime_seconds = 0;
            if (server.status == "online" && server.uptime > 0) {
                uptime_seconds = now - server.uptime;
                if (uptime_seconds < 0)
                    uptime_seconds = 0;
            }
            server_json["uptime"] = static_cast<Json::Int64>(uptime_seconds);

            servers.append(server_json);

            if (server.status == "online") {
                online_count++;
            } else {
                offline_count++;
            }
        }

        response["servers"] = servers;
        response["online_count"] = online_count;
        response["offline_count"] = offline_count;
        response["total_count"] = servers_result.value().size();

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        callback(resp);

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Error getting server stats: {}", e.what());
        Json::Value error;
        error["error"] = "Internal server error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

void
StatsController::get_drives(const drogon::HttpRequestPtr& req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto user_info = get_user_from_request(req);

    Json::Value response;

    try {
        auto db_manager = ServiceLocator::database();
        if (!db_manager) {
            Json::Value error;
            error["error"] = "Database not available";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        auto drives_result = db_manager->list_drives();
        if (!drives_result) {
            CONSOLE_LOG_ERROR("Failed to get drives: {}", drives_result.error());
            Json::Value error;
            error["error"] = "Failed to retrieve drive data";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        Json::Value drives(Json::arrayValue);
        int online_count = 0;
        int offline_count = 0;

        // Don't send all drives for performance, just summary
        for (const auto& drive : drives_result.value()) {
            if (drive.status == "online") {
                online_count++;
            } else {
                offline_count++;
            }
        }

        response["drives"] = drives; // Empty for performance
        response["online_count"] = online_count;
        response["offline_count"] = offline_count;
        response["total_count"] = drives_result.value().size();

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        callback(resp);

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Error getting drive stats: {}", e.what());
        Json::Value error;
        error["error"] = "Internal server error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

void
StatsController::get_pools(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto user_info = get_user_from_request(req);

    Json::Value response(Json::arrayValue);

    try {
        auto db_manager = ServiceLocator::database();
        if (!db_manager) {
            Json::Value error;
            error["error"] = "Database not available";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        auto pools_result = db_manager->list_storage_pools();
        if (!pools_result) {
            CONSOLE_LOG_ERROR("Failed to get storage pools: {}", pools_result.error());
            Json::Value error;
            error["error"] = "Failed to retrieve pool data";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        for (const auto& pool : pools_result.value()) {
            Json::Value pool_json;
            pool_json["id"] = pool.id;
            pool_json["name"] = pool.name;
            pool_json["capacity"] = static_cast<Json::Int64>(pool.capacity);
            pool_json["used"] = static_cast<Json::Int64>(pool.used);
            pool_json["available"] = static_cast<Json::Int64>(pool.available);
            pool_json["drives_count"] = pool.drives_count;
            pool_json["online_drives"] = pool.online_drives;
            pool_json["offline_drives"] = pool.offline_drives;

            response.append(pool_json);
        }

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        callback(resp);

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Error getting pool stats: {}", e.what());
        Json::Value error;
        error["error"] = "Internal server error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

void
StatsController::get_api_errors(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto user_info = get_user_from_request(req);

    Json::Value response;

    try {
        auto db_manager = ServiceLocator::database();
        if (!db_manager) {
            Json::Value error;
            error["error"] = "Database not available";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        // Get API stats for last 24 hours
        auto now = std::time(nullptr);
        auto from_timestamp = now - (24 * 3600);

        auto stats_result = db_manager->get_api_request_stats(from_timestamp, now);
        if (!stats_result) {
            CONSOLE_LOG_ERROR("Failed to get API stats: {}", stats_result.error());
            Json::Value error;
            error["error"] = "Failed to retrieve API error data";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        // Group by hour
        std::map<int64_t, std::tuple<int, int, int>> hourly_stats; // timestamp -> (total, 4xx, 5xx)

        for (const auto& stat : stats_result.value()) {
            // Round to hour
            int64_t hour_ts = (stat.timestamp / 3600) * 3600;

            auto& [total, err_4xx, err_5xx] = hourly_stats[hour_ts];
            total++;

            if (stat.status_code >= 400 && stat.status_code < 500) {
                err_4xx++;
            } else if (stat.status_code >= 500) {
                err_5xx++;
            }
        }

        Json::Value data(Json::arrayValue);
        int total_requests = 0;
        int total_errors = 0;

        // Fill in missing hours with zeros
        for (int i = 23; i >= 0; i--) {
            int64_t hour_ts = now - (i * 3600);
            hour_ts = (hour_ts / 3600) * 3600;

            auto it = hourly_stats.find(hour_ts);
            int requests = 0, error_4xx = 0, error_5xx = 0;

            if (it != hourly_stats.end()) {
                requests = std::get<0>(it->second);
                error_4xx = std::get<1>(it->second);
                error_5xx = std::get<2>(it->second);
            }

            Json::Value point;
            char time_buf[6];
            std::strftime(time_buf, sizeof(time_buf), "%H:%M", std::localtime(&hour_ts));

            point["timestamp"] = static_cast<Json::Int64>(hour_ts);
            point["time"] = time_buf;
            point["requests"] = requests;
            point["error_4xx"] = error_4xx;
            point["error_5xx"] = error_5xx;

            total_requests += requests;
            total_errors += (error_4xx + error_5xx);
            data.append(point);
        }

        response["data"] = data;
        response["total_requests"] = total_requests;
        response["total_errors"] = total_errors;
        // error_rate as percentage of failed requests
        response["error_rate"] = total_requests > 0 ? (static_cast<double>(total_errors) / total_requests * 100.0)
                                                    : 0.0;

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        callback(resp);

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Error getting API error stats: {}", e.what());
        Json::Value error;
        error["error"] = "Internal server error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

void
StatsController::get_data_throughput(const drogon::HttpRequestPtr& req,
                                     std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto user_info = get_user_from_request(req);

    Json::Value response;

    try {
        auto db_manager = ServiceLocator::database();
        if (!db_manager) {
            Json::Value error;
            error["error"] = "Database not available";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        // Get throughput stats for last 24 hours
        auto now = std::time(nullptr);
        auto from_timestamp = now - (24 * 3600);

        auto stats_result = db_manager->get_throughput_stats(from_timestamp, now);
        if (!stats_result) {
            CONSOLE_LOG_ERROR("Failed to get throughput stats: {}", stats_result.error());
            Json::Value error;
            error["error"] = "Failed to retrieve throughput data";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        // Group by hour and aggregate
        std::map<int64_t, std::tuple<int64_t, int64_t, int64_t>> hourly_stats; // timestamp -> (read, write, total)

        for (const auto& stat : stats_result.value()) {
            // Round to hour
            int64_t hour_ts = (stat.timestamp / 3600) * 3600;

            auto& [read_bytes, write_bytes, total_bytes] = hourly_stats[hour_ts];
            read_bytes += stat.read_bytes;
            write_bytes += stat.write_bytes;
            total_bytes += stat.total_bytes;
        }

        Json::Value data(Json::arrayValue);
        int64_t total_read = 0;
        int64_t total_write = 0;
        int64_t peak_throughput = 0;

        // Fill in all 24 hours
        for (int i = 23; i >= 0; i--) {
            int64_t hour_ts = now - (i * 3600);
            hour_ts = (hour_ts / 3600) * 3600;

            auto it = hourly_stats.find(hour_ts);
            int64_t read_bytes = 0, write_bytes = 0, total_bytes = 0;

            if (it != hourly_stats.end()) {
                read_bytes = std::get<0>(it->second);
                write_bytes = std::get<1>(it->second);
                total_bytes = std::get<2>(it->second);
            }

            Json::Value point;
            char time_buf[6];
            std::strftime(time_buf, sizeof(time_buf), "%H:%M", std::localtime(&hour_ts));

            point["timestamp"] = static_cast<Json::Int64>(hour_ts);
            point["time"] = time_buf;
            point["read_bytes"] = static_cast<Json::Int64>(read_bytes);
            point["write_bytes"] = static_cast<Json::Int64>(write_bytes);
            point["total_bytes"] = static_cast<Json::Int64>(total_bytes);

            total_read += read_bytes;
            total_write += write_bytes;
            if (total_bytes > peak_throughput) {
                peak_throughput = total_bytes;
            }

            data.append(point);
        }

        response["data"] = data;
        response["total_read"] = static_cast<Json::Int64>(total_read);
        response["total_write"] = static_cast<Json::Int64>(total_write);
        response["average_throughput"] = static_cast<Json::Int64>((total_read + total_write) / 24);
        response["peak_throughput"] = static_cast<Json::Int64>(peak_throughput);

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        callback(resp);

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Error getting data throughput stats: {}", e.what());
        Json::Value error;
        error["error"] = "Internal server error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

void
StatsController::get_encryption_stats(const drogon::HttpRequestPtr& req,
                                      std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto user_info = get_user_from_request(req);

    Json::Value response;

    try {
        auto bucket_service = ServiceLocator::bucket_service();
        auto object_service = ServiceLocator::object_service();

        if (!bucket_service || !object_service) {
            Json::Value error;
            error["error"] = "Service not available";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        int64_t total_objects = 0;
        int64_t encrypted_objects = 0;
        int64_t sse_s3_count = 0;
        int64_t sse_c_count = 0;
        int64_t unencrypted_objects = 0;
        int64_t encrypted_size = 0;
        int64_t unencrypted_size = 0;

        // Get all buckets
        auto buckets_result = bucket_service->list_buckets(user_info);
        if (buckets_result.is_ok()) {
            for (const auto& bucket : buckets_result.value()) {
                // List objects in each bucket
                auto objects_result = object_service->list_objects(user_info, bucket.name(), "", true, 10000);
                if (objects_result.is_ok()) {
                    for (const auto& obj : objects_result.value()) {
                        total_objects++;

                        // Get object info for encryption details
                        auto info_result = object_service->get_object_info(user_info, bucket.name(), obj.key());
                        if (info_result.is_ok()) {
                            const auto& info = info_result.value();
                            if (info.encrypted()) {
                                encrypted_objects++;
                                encrypted_size += info.size();

                                if (info.sse_type() == "SSE-S3") {
                                    sse_s3_count++;
                                } else if (info.sse_type() == "SSE-C") {
                                    sse_c_count++;
                                }
                            } else {
                                unencrypted_objects++;
                                unencrypted_size += info.size();
                            }
                        }
                    }
                }
            }
        }

        // Build response
        response["total_objects"] = static_cast<Json::Int64>(total_objects);
        response["encrypted_objects"] = static_cast<Json::Int64>(encrypted_objects);
        response["unencrypted_objects"] = static_cast<Json::Int64>(unencrypted_objects);
        response["sse_s3_count"] = static_cast<Json::Int64>(sse_s3_count);
        response["sse_c_count"] = static_cast<Json::Int64>(sse_c_count);
        response["encrypted_size"] = static_cast<Json::Int64>(encrypted_size);
        response["unencrypted_size"] = static_cast<Json::Int64>(unencrypted_size);
        response["encryption_percentage"] = total_objects > 0
                                                ? (static_cast<double>(encrypted_objects) / total_objects * 100.0)
                                                : 0.0;

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        callback(resp);

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Error getting encryption stats: {}", e.what());
        Json::Value error;
        error["error"] = "Internal server error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

} // namespace console::api
