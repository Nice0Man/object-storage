
//
#include "console/api/AuthController.hpp"

#include "console/common/Config.hpp"
#include "console/common/Logger.hpp"

#include <chrono>
#include <json/json.h>
#include <jwt-cpp/jwt.h>

namespace console::api {

void
AuthController::login(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    try {
        auto json = req->getJsonObject();
        if (!json) {
            Json::Value error;
            error["error"] = "Invalid JSON";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k400BadRequest);
            callback(resp);
            return;
        }

        String username = (*json).get("username", "").asString();
        String password = (*json).get("password", "").asString();

        if (username.empty() || password.empty()) {
            Json::Value error;
            error["error"] = "Username and password are required";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k400BadRequest);
            callback(resp);
            return;
        }

        CONSOLE_LOG_INFO("Login attempt for user: {}", username);

        // Validate credentials
        auto user = validate_credentials(username, password);
        if (!user.has_value()) {
            Json::Value error;
            error["error"] = "Invalid credentials";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k401Unauthorized);
            callback(resp);
            return;
        }

        // Generate JWT token
        String token = generate_jwt_token(*user);

        Json::Value response;
        response["token"] = token;
        response["user"]["username"] = user->account_name;
        response["user"]["access_key"] = user->access_key;
        response["user"]["is_admin"] = user->is_admin;

        CONSOLE_LOG_INFO("User {} logged in successfully", username);

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k200OK);
        callback(resp);

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Login error: {}", e.what());
        Json::Value error;
        error["error"] = "Internal server error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

void
AuthController::logout(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    // TODO(Nice0Man): Implement token blacklisting if needed

    Json::Value response;
    response["message"] = "Logged out successfully";

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

void
AuthController::refresh(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    // TODO(Nice0Man): Implement refresh token logic

    Json::Value error;
    error["error"] = "Not implemented";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(drogon::k501NotImplemented);
    callback(resp);
}

void
AuthController::me(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    // TODO(Nice0Man): Extract user from JWT token in middleware

    Json::Value response;
    response["username"] = "admin";
    response["access_key"] = "minioadmin";
    response["is_admin"] = true;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
AuthController::change_password(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    // TODO(Nice0Man): Implement password change logic

    Json::Value error;
    error["error"] = "Not implemented";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(drogon::k501NotImplemented);
    callback(resp);
}

String
AuthController::generate_jwt_token(const UserInfo& user) const {
    auto& config = Config::instance();
    auto& auth_config = config.auth();

    auto token = jwt::create()
                     .set_issuer("object-storage-console")
                     .set_type("JWT")
                     .set_issued_at(std::chrono::system_clock::now())
                     .set_expires_at(std::chrono::system_clock::now() + auth_config.token_expiry)
                     .set_payload_claim("username", jwt::claim(user.account_name))
                     .set_payload_claim("access_key", jwt::claim(user.access_key))
                     .set_payload_claim("is_admin", jwt::claim(std::to_string(user.is_admin)))
                     .sign(jwt::algorithm::hs256{auth_config.jwt_secret});

    return token;
}

Optional<UserInfo>
AuthController::validate_credentials(const String& username, const String& password) const {
    // TODO(Nice0Man): Implement real credential validation with S3 backend

    // For now, accept minioadmin/minioadmin  // pragma: allowlist secret
    if (username == "minioadmin" && password == "minioadmin") {
        UserInfo user;
        user.account_name = username;
        user.access_key = username;
        user.secret_key = password;
        user.is_admin = true;
        user.created_at = std::chrono::system_clock::now();

        return user;
    }

    return std::nullopt;
}

} // namespace console::api
