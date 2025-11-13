#include "console/api/StatsController.hpp"

#include "console/common/Logger.hpp"
#include "console/common/ServiceLocator.hpp"
#include "console/services/BucketService.hpp"
#include "console/services/UserService.hpp"

#include <cstdlib>
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

        // Get user count
        auto user_service = ServiceLocator::user_service();
        int user_count = 0;
        if (user_service) {
            auto users_result = user_service->list_users(user_info);
            if (users_result.is_ok()) {
                user_count = users_result.value().size();
            }
        }

        // Build response
        stats["buckets"] = bucket_count;
        stats["objects"] = static_cast<Json::Int64>(total_objects);
        stats["users"] = user_count;
        stats["storage_used"] = static_cast<Json::Int64>(total_size);

        // Storage capacity (mock for now - could be from config)
        stats["storage_total"] = static_cast<Json::Int64>(10LL * 1024 * 1024 * 1024 * 1024); // 10 TiB
        stats["storage_available"] = static_cast<Json::Int64>(stats["storage_total"].asInt64() - total_size);

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

        // Capacity information
        capacity["total"] = static_cast<Json::Int64>(10LL * 1024 * 1024 * 1024 * 1024); // 10 TiB
        capacity["used"] = static_cast<Json::Int64>(total_size);
        capacity["available"] = static_cast<Json::Int64>(capacity["total"].asInt64() - total_size);
        capacity["usage_percent"] = total_size > 0
                                        ? static_cast<double>(total_size) / capacity["total"].asInt64() * 100.0
                                        : 0.0;

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
        // Mock server data
        Json::Value servers(Json::arrayValue);

        // Simulate some servers
        int online_count = 10;
        int offline_count = 7;
        int total_count = 20;

        for (int i = 0; i < total_count; i++) {
            Json::Value server;
            server["id"] = "server-" + std::to_string(i + 1);
            server["name"] = "Server " + std::to_string(i + 1);
            server["status"] = (i < online_count) ? "online" : "offline";
            server["endpoint"] = "http://server-" + std::to_string(i + 1) + ".local:9000";
            server["uptime"] = (i < online_count) ? 86400 + (i * 1000) : 0;
            servers.append(server);
        }

        response["servers"] = servers;
        response["online_count"] = online_count;
        response["offline_count"] = offline_count;
        response["total_count"] = total_count;

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
        // Mock drive data
        Json::Value drives(Json::arrayValue);

        int online_count = 1900;
        int offline_count = 100;
        int total_count = 2000;

        // We don't list all drives, just provide summary
        response["drives"] = drives; // Empty for performance
        response["online_count"] = online_count;
        response["offline_count"] = offline_count;
        response["total_count"] = total_count;

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
        // Mock pool data
        for (int i = 1; i <= 2; i++) {
            Json::Value pool;
            pool["id"] = "pool" + std::to_string(i);
            pool["name"] = "Pool " + std::to_string(i);

            // 5.25 EiB capacity
            int64_t capacity = 5LL * 1024 * 1024 * 1024 * 1024 * 1024 + 256LL * 1024 * 1024 * 1024 * 1024;
            int64_t used = (i == 1) ? (4LL * 1024 * 1024 * 1024 * 1024 * 1024 + 31LL * 1024 * 1024 * 1024 * 1024)
                                    : (3LL * 1024 * 1024 * 1024 * 1024 * 1024 + 800LL * 1024 * 1024 * 1024 * 1024);

            pool["capacity"] = static_cast<Json::Int64>(capacity);
            pool["used"] = static_cast<Json::Int64>(used);
            pool["available"] = static_cast<Json::Int64>(capacity - used);
            pool["drives_count"] = 90;
            pool["online_drives"] = 80;
            pool["offline_drives"] = 10;

            response.append(pool);
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
        // Mock API error data (last 24 hours)
        Json::Value data(Json::arrayValue);

        auto now = std::time(nullptr);
        int total_errors = 0;

        for (int i = 23; i >= 0; i--) {
            Json::Value point;
            auto timestamp = now - (i * 3600);

            // Format time as HH:MM
            char time_buf[6];
            std::strftime(time_buf, sizeof(time_buf), "%H:%M", std::localtime(&timestamp));

            int error_4xx = std::rand() % 15;
            int error_5xx = std::rand() % 10;
            int count = error_4xx + error_5xx;

            point["timestamp"] = static_cast<Json::Int64>(timestamp);
            point["time"] = time_buf;
            point["count"] = count;
            point["error_4xx"] = error_4xx;
            point["error_5xx"] = error_5xx;

            total_errors += count;
            data.append(point);
        }

        response["data"] = data;
        response["total_errors"] = total_errors;
        response["error_rate"] = total_errors > 0 ? (total_errors / 24.0) : 0.0;

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
        // Mock throughput data (last 24 hours)
        Json::Value data(Json::arrayValue);

        auto now = std::time(nullptr);
        int64_t total_read = 0;
        int64_t total_write = 0;
        int64_t peak_throughput = 0;

        for (int i = 23; i >= 0; i--) {
            Json::Value point;
            auto timestamp = now - (i * 3600);

            // Format time as HH:MM
            char time_buf[6];
            std::strftime(time_buf, sizeof(time_buf), "%H:%M", std::localtime(&timestamp));

            // Random throughput (in bytes) - varying between 400GB to 1TB per hour
            int64_t read_bytes = (400LL + (std::rand() % 600)) * 1024 * 1024 * 1024;
            int64_t write_bytes = (300LL + (std::rand() % 500)) * 1024 * 1024 * 1024;
            int64_t total_bytes = read_bytes + write_bytes;

            point["timestamp"] = static_cast<Json::Int64>(timestamp);
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

} // namespace console::api
