#pragma once

#include "console/services/IInfrastructureService.hpp"

#include <drogon/HttpController.h>

#include <memory>

namespace console::api {

/**
 * @brief Infrastructure API Controller
 *
 * Provides endpoints for managing servers, drives, and storage pools
 * according to object storage principles (erasure coding, distributed architecture)
 */
class InfrastructureController : public drogon::HttpController<InfrastructureController> {
  public:
    METHOD_LIST_BEGIN

    // Summary
    ADD_METHOD_TO(InfrastructureController::get_summary,
                  "/api/v1/infrastructure/summary",
                  drogon::Get,
                  "console::middleware::AuthMiddleware");

    // Server endpoints
    ADD_METHOD_TO(InfrastructureController::list_servers,
                  "/api/v1/infrastructure/servers",
                  drogon::Get,
                  "console::middleware::AuthMiddleware");
    ADD_METHOD_TO(InfrastructureController::get_server,
                  "/api/v1/infrastructure/servers/{id}",
                  drogon::Get,
                  "console::middleware::AuthMiddleware");
    ADD_METHOD_TO(InfrastructureController::add_server,
                  "/api/v1/infrastructure/servers",
                  drogon::Post,
                  "console::middleware::AuthMiddleware");
    ADD_METHOD_TO(InfrastructureController::remove_server,
                  "/api/v1/infrastructure/servers/{id}",
                  drogon::Delete,
                  "console::middleware::AuthMiddleware");
    ADD_METHOD_TO(InfrastructureController::check_server_health,
                  "/api/v1/infrastructure/servers/{id}/health",
                  drogon::Get,
                  "console::middleware::AuthMiddleware");

    // Drive endpoints
    ADD_METHOD_TO(InfrastructureController::list_drives,
                  "/api/v1/infrastructure/drives",
                  drogon::Get,
                  "console::middleware::AuthMiddleware");
    ADD_METHOD_TO(InfrastructureController::get_drive,
                  "/api/v1/infrastructure/drives/{id}",
                  drogon::Get,
                  "console::middleware::AuthMiddleware");
    ADD_METHOD_TO(InfrastructureController::add_drive,
                  "/api/v1/infrastructure/drives",
                  drogon::Post,
                  "console::middleware::AuthMiddleware");
    ADD_METHOD_TO(InfrastructureController::remove_drive,
                  "/api/v1/infrastructure/drives/{id}",
                  drogon::Delete,
                  "console::middleware::AuthMiddleware");
    ADD_METHOD_TO(InfrastructureController::set_drive_status,
                  "/api/v1/infrastructure/drives/{id}/status",
                  drogon::Put,
                  "console::middleware::AuthMiddleware");

    // Pool endpoints
    ADD_METHOD_TO(InfrastructureController::list_pools,
                  "/api/v1/infrastructure/pools",
                  drogon::Get,
                  "console::middleware::AuthMiddleware");
    ADD_METHOD_TO(InfrastructureController::get_pool,
                  "/api/v1/infrastructure/pools/{id}",
                  drogon::Get,
                  "console::middleware::AuthMiddleware");
    ADD_METHOD_TO(InfrastructureController::configure_pool,
                  "/api/v1/infrastructure/pools",
                  drogon::Post,
                  "console::middleware::AuthMiddleware");
    ADD_METHOD_TO(InfrastructureController::decommission_pool,
                  "/api/v1/infrastructure/pools/{id}",
                  drogon::Delete,
                  "console::middleware::AuthMiddleware");

    // Healing endpoints
    ADD_METHOD_TO(InfrastructureController::get_heal_status,
                  "/api/v1/infrastructure/heal",
                  drogon::Get,
                  "console::middleware::AuthMiddleware");
    ADD_METHOD_TO(InfrastructureController::start_heal,
                  "/api/v1/infrastructure/heal",
                  drogon::Post,
                  "console::middleware::AuthMiddleware");

    METHOD_LIST_END

    // Summary
    void get_summary(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // Server handlers
    void list_servers(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void get_server(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                    const String& id);
    void add_server(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void remove_server(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       const String& id);
    void check_server_health(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                             const String& id);

    // Drive handlers
    void list_drives(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void get_drive(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const String& id);
    void add_drive(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void remove_drive(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                      const String& id);
    void set_drive_status(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                          const String& id);

    // Pool handlers
    void list_pools(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void get_pool(const drogon::HttpRequestPtr& req,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                  const String& id);
    void configure_pool(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void decommission_pool(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                           const String& id);

    // Healing handlers
    void get_heal_status(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void start_heal(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

  private:
    UserInfo get_user_from_request(const drogon::HttpRequestPtr& req);
};

} // namespace console::api
