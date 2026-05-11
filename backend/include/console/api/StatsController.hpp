#pragma once

#include "console/services/IBucketService.hpp"
#include "console/services/IUserService.hpp"

#include <drogon/HttpController.h>

#include <memory>

namespace console::api {

/**
 * @brief Statistics API Controller
 *
 * Provides system statistics and metrics for dashboard
 */
class StatsController : public drogon::HttpController<StatsController> {
  public:
    METHOD_LIST_BEGIN

    // System statistics
    ADD_METHOD_TO(StatsController::get_system_stats, "/api/v1/stats/system", drogon::Get, "AuthFilter");

    // Storage capacity statistics
    ADD_METHOD_TO(StatsController::get_capacity, "/api/v1/stats/capacity", drogon::Get, "AuthFilter");

    // Recent activity
    ADD_METHOD_TO(StatsController::get_activity, "/api/v1/stats/activity", drogon::Get, "AuthFilter");

    // Server statistics
    ADD_METHOD_TO(StatsController::get_servers, "/api/v1/stats/servers", drogon::Get, "AuthFilter");

    // Drive statistics
    ADD_METHOD_TO(StatsController::get_drives, "/api/v1/stats/drives", drogon::Get, "AuthFilter");

    // Pool statistics
    ADD_METHOD_TO(StatsController::get_pools, "/api/v1/stats/pools", drogon::Get, "AuthFilter");

    // API error statistics
    ADD_METHOD_TO(StatsController::get_api_errors, "/api/v1/stats/api-errors", drogon::Get, "AuthFilter");

    // Data throughput statistics
    ADD_METHOD_TO(StatsController::get_data_throughput, "/api/v1/stats/data-throughput", drogon::Get, "AuthFilter");

    // Encryption statistics
    ADD_METHOD_TO(StatsController::get_encryption_stats, "/api/v1/stats/encryption", drogon::Get, "AuthFilter");

    METHOD_LIST_END

    // Handlers
    void get_system_stats(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void get_capacity(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void get_activity(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void get_servers(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void get_drives(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void get_pools(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void get_api_errors(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void get_data_throughput(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void get_encryption_stats(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback);

  private:
    UserInfo get_user_from_request(const drogon::HttpRequestPtr& req);
};

} // namespace console::api
