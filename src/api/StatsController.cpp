#include "console/api/StatsController.hpp"

#include "console/common/Logger.hpp"
#include "console/common/ServiceLocator.hpp"
#include "console/services/BucketService.hpp"
#include "console/services/UserService.hpp"

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

} // namespace console::api
