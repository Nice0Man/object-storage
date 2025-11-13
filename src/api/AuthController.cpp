
//
#include "console/api/AuthController.hpp"

#include "console/common/Config.hpp"
#include "console/common/Logger.hpp"
#include "console/storage/DatabaseManager.hpp"
#include "console/utils/PasswordHash.hpp"
#include "console/utils/TokenBlacklist.hpp"

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

        // Support both username/password and accessKey/secretKey formats
        String username = (*json).get("username", "").asString();
        String password = (*json).get("password", "").asString();

        // If username/password not provided, try accessKey/secretKey
        if (username.empty()) {
            username = (*json).get("accessKey", "").asString();
        }
        if (password.empty()) {
            password = (*json).get("secretKey", "").asString();
        }

        if (username.empty() || password.empty()) {
            Json::Value error;
            error["error"] = "Username and password (or accessKey and secretKey) are required";
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
    try {
        // Extract token from Authorization header
        auto auth_header = req->getHeader("Authorization");
        if (!auth_header.empty() && auth_header.find("Bearer ") == 0) {
            String token = auth_header.substr(7); // Remove "Bearer " prefix

            // Decode token to get expiration time
            try {
                auto decoded = jwt::decode(token);
                auto exp_claim = decoded.get_expires_at();

                // Add token to blacklist with its expiration time
                utils::TokenBlacklist::instance().add_token(token, exp_claim);

                CONSOLE_LOG_INFO("Token blacklisted successfully");
            } catch (const std::exception& e) {
                // Token is invalid, but we still return success for logout
                CONSOLE_LOG_WARN("Could not decode token for blacklisting: {}", e.what());
            }
        }

        // Clean up expired tokens periodically
        utils::TokenBlacklist::instance().cleanup_expired();

        Json::Value response;
        response["message"] = "Logged out successfully";

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k200OK);
        callback(resp);

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Logout error: {}", e.what());
        // Still return success even if blacklisting fails
        Json::Value response;
        response["message"] = "Logged out successfully";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k200OK);
        callback(resp);
    }
}

void
AuthController::refresh(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    try {
        // Extract token from Authorization header
        auto auth_header = req->getHeader("Authorization");
        if (auth_header.empty() || auth_header.find("Bearer ") != 0) {
            Json::Value error;
            error["error"] = "Missing or invalid Authorization header";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k401Unauthorized);
            callback(resp);
            return;
        }

        String token = auth_header.substr(7); // Remove "Bearer " prefix

        // Validate the current token
        auto user = validate_token(token);
        if (!user.has_value()) {
            Json::Value error;
            error["error"] = "Invalid or expired token";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k401Unauthorized);
            callback(resp);
            return;
        }

        // Generate new token with refreshed expiration
        String new_token = generate_jwt_token(user.value());

        Json::Value response;
        response["token"] = new_token;
        response["user"]["username"] = user->access_key;
        response["user"]["access_key"] = user->access_key;
        response["user"]["is_admin"] = user->is_admin;

        CONSOLE_LOG_INFO("Token refreshed for user: {}", user->access_key);

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k200OK);
        callback(resp);

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Token refresh error: {}", e.what());
        Json::Value error;
        error["error"] = "Internal server error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

void
AuthController::me(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    try {
        // Extract token from Authorization header
        auto auth_header = req->getHeader("Authorization");
        if (auth_header.empty() || auth_header.find("Bearer ") != 0) {
            Json::Value error;
            error["error"] = "Missing or invalid Authorization header";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k401Unauthorized);
            callback(resp);
            return;
        }

        String token = auth_header.substr(7); // Remove "Bearer " prefix

        // Validate token and extract user info
        auto user = validate_token(token);
        if (!user.has_value()) {
            Json::Value error;
            error["error"] = "Invalid or expired token";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k401Unauthorized);
            callback(resp);
            return;
        }

        Json::Value response;
        response["username"] = user->account_name;
        response["access_key"] = user->access_key;
        response["is_admin"] = user->is_admin;

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        callback(resp);

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Get current user error: {}", e.what());
        Json::Value error;
        error["error"] = "Internal server error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

void
AuthController::session(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    try {
        // Extract token from Authorization header
        auto auth_header = req->getHeader("Authorization");
        if (auth_header.empty() || auth_header.find("Bearer ") != 0) {
            Json::Value error;
            error["error"] = "Missing or invalid Authorization header";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k401Unauthorized);
            callback(resp);
            return;
        }

        String token = auth_header.substr(7); // Remove "Bearer " prefix

        // Validate token and extract user info
        auto user = validate_token(token);
        if (!user.has_value()) {
            Json::Value error;
            error["error"] = "Unauthenticated";
            error["authenticated"] = false;
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k401Unauthorized);
            callback(resp);
            return;
        }

        // Calculate token expiry (assuming standard JWT exp claim)
        auto now = std::chrono::system_clock::now();
        auto expires_at = now + std::chrono::hours(24); // Default 24h
        auto expires_timestamp = std::chrono::system_clock::to_time_t(expires_at);

        // Format as ISO 8601
        std::tm tm = *std::gmtime(&expires_timestamp);
        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);

        Json::Value response;
        response["authenticated"] = true;
        response["username"] = user->account_name;
        response["access_key"] = user->access_key;
        response["is_admin"] = user->is_admin;
        response["expires_at"] = buffer;

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k200OK);
        callback(resp);

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Get session error: {}", e.what());
        Json::Value error;
        error["error"] = "Internal server error";
        error["authenticated"] = false;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

void
AuthController::change_password(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    try {
        // Extract token from Authorization header
        auto auth_header = req->getHeader("Authorization");
        if (auth_header.empty() || auth_header.find("Bearer ") != 0) {
            Json::Value error;
            error["error"] = "Missing or invalid Authorization header";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k401Unauthorized);
            callback(resp);
            return;
        }

        String token = auth_header.substr(7); // Remove "Bearer " prefix

        // Validate token and extract user info
        auto user = validate_token(token);
        if (!user.has_value()) {
            Json::Value error;
            error["error"] = "Invalid or expired token";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k401Unauthorized);
            callback(resp);
            return;
        }

        // Parse request body
        auto json = req->getJsonObject();
        if (!json) {
            Json::Value error;
            error["error"] = "Invalid JSON";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k400BadRequest);
            callback(resp);
            return;
        }

        String old_password = (*json).get("old_password", "").asString();
        String new_password = (*json).get("new_password", "").asString();

        if (old_password.empty() || new_password.empty()) {
            Json::Value error;
            error["error"] = "Old password and new password are required";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k400BadRequest);
            callback(resp);
            return;
        }

        // Validate new password length
        if (new_password.length() < 8) {
            Json::Value error;
            error["error"] = "New password must be at least 8 characters long";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k400BadRequest);
            callback(resp);
            return;
        }

        // Verify old password
        if (!validate_credentials(user->access_key, old_password).has_value()) {
            Json::Value error;
            error["error"] = "Invalid old password";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k401Unauthorized);
            callback(resp);
            return;
        }

        // Update password in database
        auto db_path = Config::instance().get<String>("database.path").value_or("console.db");
        auto db_manager = std::make_shared<storage::DatabaseManager>(db_path);

        // Get current user from database
        auto db_user_result = db_manager->get_user(user->access_key);
        if (!db_user_result) {
            CONSOLE_LOG_ERROR("Failed to get user from database: {}", user->access_key);
            Json::Value error;
            error["error"] = "User not found in database";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        // Hash new password and update
        auto db_user = db_user_result.value();
        db_user.secret_key = utils::PasswordHash::hash(new_password);
        db_user.updated_at = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        // Update user in database
        auto update_result = db_manager->update_user(db_user);
        if (!update_result) {
            CONSOLE_LOG_ERROR("Failed to update password for user: {}", user->access_key);
            Json::Value error;
            error["error"] = "Failed to update password: " + update_result.error();
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        CONSOLE_LOG_INFO("Password changed successfully for user: {}", user->access_key);

        Json::Value response;
        response["message"] = "Password changed successfully";

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        callback(resp);

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Password change error: {}", e.what());
        Json::Value error;
        error["error"] = "Internal server error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
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
                     .set_payload_claim("is_admin", jwt::claim(std::string(user.is_admin ? "true" : "false")))
                     .sign(jwt::algorithm::hs256{auth_config.jwt_secret});

    return token;
}

Optional<UserInfo>
AuthController::validate_credentials(const String& username, const String& password) const {
    try {
        // Get database path from config
        auto db_path = Config::instance().get<String>("database.path").value_or("console.db");

        // Create DatabaseManager instance
        auto db_manager = std::make_shared<storage::DatabaseManager>(db_path);

        // Look up user by access_key (username)
        auto user_result = db_manager->get_user(username);
        if (!user_result) {
            CONSOLE_LOG_WARN("User not found: {}", username);
            return std::nullopt;
        }

        auto db_user = user_result.value();

        // Check if user is active
        if (db_user.status != "active") {
            CONSOLE_LOG_WARN("User account is not active: {}", username);
            return std::nullopt;
        }

        // Verify password using secure hash comparison
        if (!utils::PasswordHash::verify(password, db_user.secret_key)) {
            CONSOLE_LOG_WARN("Invalid password for user: {}", username);
            return std::nullopt;
        }

        // Create UserInfo from database user
        UserInfo user;
        user.access_key = db_user.access_key;
        user.secret_key = db_user.secret_key; // Keep hashed for session
        user.account_name = db_user.account_name;
        user.is_admin = db_user.is_admin;
        user.created_at = std::chrono::system_clock::from_time_t(db_user.created_at);

        CONSOLE_LOG_INFO("User authenticated successfully via AuthController: {}", username);
        return user;

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Authentication error: {}", e.what());
        return std::nullopt;
    }
}

Optional<UserInfo>
AuthController::validate_token(const String& token) const {
    try {
        // Check if token is blacklisted
        if (utils::TokenBlacklist::instance().is_blacklisted(token)) {
            CONSOLE_LOG_DEBUG("Token is blacklisted (logged out)");
            return std::nullopt;
        }

        auto& config = Config::instance();
        auto& auth_config = config.auth();

        // Decode and verify the token
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
                            .allow_algorithm(jwt::algorithm::hs256{auth_config.jwt_secret})
                            .with_issuer("object-storage-console");

        verifier.verify(decoded);

        // Extract user information from claims
        UserInfo user;
        user.account_name = decoded.get_payload_claim("username").as_string();
        user.access_key = decoded.get_payload_claim("access_key").as_string();

        // Extract is_admin claim
        if (decoded.has_payload_claim("is_admin")) {
            auto is_admin_str = decoded.get_payload_claim("is_admin").as_string();
            user.is_admin = (is_admin_str == "1" || is_admin_str == "true");
        } else {
            user.is_admin = false;
        }

        return user;

    } catch (const std::exception& e) {
        CONSOLE_LOG_DEBUG("Token validation failed: {}", e.what());
        return std::nullopt;
    }
}

} // namespace console::api
