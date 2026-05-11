#pragma once

#include <drogon/HttpController.h>

namespace console::api {

/**
 * @brief Health check controller
 *
 * Provides endpoints for health checks and service status
 */
class HealthController : public drogon::HttpController<HealthController> {
  public:
    METHOD_LIST_BEGIN
    // Health check endpoint
    ADD_METHOD_TO(HealthController::health, "/api/v1/health", drogon::Get);
    // Readiness check (swagger: /api/v1/ready)
    ADD_METHOD_TO(HealthController::ready, "/api/v1/ready", drogon::Get);
    // Liveness check (swagger: /api/v1/live)
    ADD_METHOD_TO(HealthController::live, "/api/v1/live", drogon::Get);
    // Version info
    ADD_METHOD_TO(HealthController::version, "/api/v1/version", drogon::Get);
    METHOD_LIST_END

    /**
     * @brief Health check endpoint
     * Returns overall service health
     */
    void health(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback) const;

    /**
     * @brief Readiness check
     * Returns whether service is ready to accept requests
     */
    void ready(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) const;

    /**
     * @brief Liveness check
     * Returns whether service is alive
     */
    void live(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) const;

    /**
     * @brief Version information
     * Returns service version and build info
     */
    void version(const drogon::HttpRequestPtr& req,
                 std::function<void(const drogon::HttpResponsePtr&)>&& callback) const;
};

} // namespace console::api
