#pragma once

#include "console/services/IUserService.hpp"

#include <drogon/HttpController.h>

#include <memory>

namespace console::api {

/**
 * @brief Users API Controller
 *
 * Handles user management operations (admin only)
 */
class UsersController : public drogon::HttpController<UsersController> {
  public:
    METHOD_LIST_BEGIN

    // Apply authentication middleware to all methods
    ADD_METHOD_TO(UsersController::list, "/api/v1/users", drogon::Get, "AuthFilter");

    // Get user by access key
    ADD_METHOD_TO(UsersController::get, "/api/v1/users/{access_key}", drogon::Get, "AuthFilter");

    // Create user
    ADD_METHOD_TO(UsersController::create, "/api/v1/users", drogon::Post, "AuthFilter");

    // Update user
    ADD_METHOD_TO(UsersController::update, "/api/v1/users/{access_key}", drogon::Put, "AuthFilter");

    // Delete user
    ADD_METHOD_TO(UsersController::remove, "/api/v1/users/{access_key}", drogon::Delete, "AuthFilter");

    // User policies
    ADD_METHOD_TO(UsersController::list_policies, "/api/v1/users/{access_key}/policies", drogon::Get, "AuthFilter");
    ADD_METHOD_TO(UsersController::attach_policy,
                  "/api/v1/users/{access_key}/policies/{policy_name}",
                  drogon::Put,
                  "AuthFilter");
    ADD_METHOD_TO(UsersController::detach_policy,
                  "/api/v1/users/{access_key}/policies/{policy_name}",
                  drogon::Delete,
                  "AuthFilter");

    // User groups
    ADD_METHOD_TO(UsersController::add_to_group,
                  "/api/v1/users/{access_key}/groups/{group_name}",
                  drogon::Put,
                  "AuthFilter");
    ADD_METHOD_TO(UsersController::remove_from_group,
                  "/api/v1/users/{access_key}/groups/{group_name}",
                  drogon::Delete,
                  "AuthFilter");

    METHOD_LIST_END

    // Constructor
    void set_user_service(std::shared_ptr<services::IUserService> service);

    // Handlers
    void list(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void get(const drogon::HttpRequestPtr& req,
             std::function<void(const drogon::HttpResponsePtr&)>&& callback,
             const String& access_key);

    void create(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void update(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                const String& access_key);

    void remove(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                const String& access_key);

    void list_policies(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       const String& access_key);

    void attach_policy(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       const String& access_key,
                       const String& policy_name);

    void detach_policy(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       const String& access_key,
                       const String& policy_name);

    void add_to_group(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                      const String& access_key,
                      const String& group_name);

    void remove_from_group(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                           const String& access_key,
                           const String& group_name);

  private:
    UserInfo get_user_from_request(const drogon::HttpRequestPtr& req);

    std::shared_ptr<services::IUserService> user_service_;
};

} // namespace console::api
