#pragma once

#include "console/common/Types.hpp"

#include <drogon/HttpController.h>

namespace console::api {

/**
 * @brief Authentication controller
 *
 * Handles user authentication, token management, and authorization
 */
class AuthController : public drogon::HttpController<AuthController> {
  public:
    METHOD_LIST_BEGIN
    // Login endpoint
    ADD_METHOD_TO(AuthController::login, "/api/v1/auth/login", drogon::Post);
    // Logout endpoint
    ADD_METHOD_TO(AuthController::logout, "/api/v1/auth/logout", drogon::Post);
    // Refresh token
    ADD_METHOD_TO(AuthController::refresh, "/api/v1/auth/refresh", drogon::Post);
    // Get current user
    ADD_METHOD_TO(AuthController::me, "/api/v1/auth/me", drogon::Get, "AuthFilter");
    // Get session info (handles auth internally, no middleware needed)
    ADD_METHOD_TO(AuthController::session, "/api/v1/auth/session", drogon::Get);
    // Change password
    ADD_METHOD_TO(AuthController::change_password, "/api/v1/auth/password", drogon::Put, "AuthFilter");
    METHOD_LIST_END

    /**
     * @brief Login with credentials
     * @param req Request containing username and password
     * @param callback Response callback with JWT token
     */
    void login(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /**
     * @brief Logout current user
     * @param req Request with authentication token
     * @param callback Response callback
     */
    void logout(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /**
     * @brief Refresh authentication token
     * @param req Request with refresh token
     * @param callback Response callback with new tokens
     */
    void refresh(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /**
     * @brief Get current user information
     * @param req Authenticated request
     * @param callback Response with user info
     */
    void me(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /**
     * @brief Get session information (alias for me)
     * @param req Authenticated request
     * @param callback Response with session info
     */
    void session(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /**
     * @brief Change user password
     * @param req Request with old and new password
     * @param callback Response callback
     */
    void change_password(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback);

  private:
    static String extract_bearer_token(const drogon::HttpRequestPtr& req);
    static UserInfo get_user_from_request(const drogon::HttpRequestPtr& req);
};

} // namespace console::api
