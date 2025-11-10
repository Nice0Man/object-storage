

#include "console/services/AuthService.hpp"
#include "console/utils/Logger.hpp"
#include <chrono>
#include <regex>

namespace console::services {

using namespace console::models;
using namespace console::utils;

AuthService::AuthService(
    std::shared_ptr<clients::IMinioAdminClient> admin_client,
    std::shared_ptr<Config> config
) : admin_client_(admin_client), config_(config) {
    LOG_INFO("AuthService initialized");
}

Result<Json::Value, ApiError> AuthService::login(
    const String& username,
    const String& password
) {
    LOG_INFO("Login attempt for user: {}", username);

    // Validate credentials format
    if (auto error = validate_credentials(username, password)) {
        LOG_WARN("Invalid credentials format for user: {}", username);
        return Err(*error);
    }

    // Authenticate with MinIO and get STS credentials
    auto auth_result = authenticate_with_minio(username, password);
    if (!auth_result) {
        LOG_ERROR("Authentication failed for user: {}", username);
        return Err(auth_result.error());
    }

    // Generate JWT tokens
    auto tokens = generate_tokens(auth_result.value());
    
    LOG_INFO("Login successful for user: {}", username);
    return Ok(tokens);
}

Result<void, ApiError> AuthService::logout(const String& token) {
    // Validate token first
    auto validate_result = validate_token(token);
    if (!validate_result) {
        return Err(validate_result.error());
    }

    // Add token to blacklist
    {
        std::lock_guard<std::mutex> lock(blacklist_mutex_);
        token_blacklist_.insert(token);
    }

    LOG_INFO("User logged out successfully");
    return Ok();
}

Result<Json::Value, ApiError> AuthService::refresh_token(
    const String& refresh_token
) {
    // Validate refresh token
    auto jwt_secret = config_->get_string("auth.jwt_secret", "");
    auto user_info_opt = JWT::validate_token(refresh_token, jwt_secret);
    
    if (!user_info_opt) {
        return Err(ApiError(
            HttpStatus::Unauthorized,
            "Invalid or expired refresh token"
        ));
    }

    // Check if token is blacklisted
    {
        std::lock_guard<std::mutex> lock(blacklist_mutex_);
        if (token_blacklist_.count(refresh_token) > 0) {
            return Err(ApiError(
                HttpStatus::Unauthorized,
                "Token has been revoked"
            ));
        }
    }

    // Generate new tokens
    auto tokens = generate_tokens(*user_info_opt);
    
    LOG_INFO("Token refreshed successfully for user: {}", 
             user_info_opt->access_key);
    return Ok(tokens);
}

Result<models::User, ApiError> AuthService::get_current_user(
    const String& token
) {
    // Validate token
    auto validate_result = validate_token(token);
    if (!validate_result) {
        return Err(validate_result.error());
    }

    auto& user_info = validate_result.value();

    // Get user details from MinIO Admin API
    auto user_result = admin_client_->get_user_info(user_info.access_key);
    if (!user_result) {
        return Err(ApiError(
            HttpStatus::InternalServerError,
            "Failed to fetch user information: " + user_result.error()
        ));
    }

    return Ok(user_result.value());
}

Result<void, ApiError> AuthService::change_password(
    const String& token,
    const String& old_password,
    const String& new_password
) {
    // Validate token
    auto validate_result = validate_token(token);
    if (!validate_result) {
        return Err(validate_result.error());
    }

    auto& user_info = validate_result.value();

    // Validate new password
    if (new_password.length() < 8) {
        return Err(ApiError(
            HttpStatus::BadRequest,
            "Password must be at least 8 characters long"
        ));
    }

    // Re-authenticate with old password to verify
    auto auth_result = authenticate_with_minio(
        user_info.access_key,
        old_password
    );
    if (!auth_result) {
        return Err(ApiError(
            HttpStatus::Unauthorized,
            "Current password is incorrect"
        ));
    }

    // TODO(Nice0Man): Implement password change via MinIO Admin API
    // Currently MinIO doesn't have direct password change API
    // We would need to delete and recreate user, or use LDAP/IDP
    
    return Err(ApiError(
        HttpStatus::NotImplemented,
        "Password change not yet implemented"
    ));
}

Result<UserInfo, ApiError> AuthService::validate_token(const String& token) {
    // Check if token is blacklisted
    {
        std::lock_guard<std::mutex> lock(blacklist_mutex_);
        if (token_blacklist_.count(token) > 0) {
            return Err(ApiError(
                HttpStatus::Unauthorized,
                "Token has been revoked"
            ));
        }
    }

    // Validate JWT
    auto jwt_secret = config_->get_string("auth.jwt_secret", "");
    auto user_info_opt = JWT::validate_token(token, jwt_secret);
    
    if (!user_info_opt) {
        return Err(ApiError(
            HttpStatus::Unauthorized,
            "Invalid or expired token"
        ));
    }

    return Ok(*user_info_opt);
}

// Private methods

Result<UserInfo, ApiError> AuthService::authenticate_with_minio(
    const String& username,
    const String& password
) {
    // TODO(Nice0Man): Implement actual MinIO STS authentication
    // For now, using hardcoded credentials for development
    
    // In production, this should:
    // 1. Call MinIO STS AssumeRole or AssumeRoleWithWebIdentity
    // 2. Get temporary credentials (AccessKeyId, SecretAccessKey, SessionToken)
    // 3. Return UserInfo with these credentials
    
    if (username == "minioadmin" && password == "minioadmin") {
        UserInfo user_info;
        user_info.access_key = username;
        user_info.secret_key = password;
        user_info.session_token = "";
        user_info.account_name = "Administrator";
        user_info.is_admin = true;
        user_info.created_at = std::chrono::system_clock::now();
        
        return Ok(user_info);
    }
    
    return Err(ApiError(
        HttpStatus::Unauthorized,
        "Invalid username or password"
    ));
}

Json::Value AuthService::generate_tokens(const UserInfo& user_info) {
    auto jwt_secret = config_->get_string("auth.jwt_secret", 
                                          "default-secret-change-me");
    auto token_expiry_hours = config_->get_int("auth.token_expiry_hours", 24);
    auto refresh_expiry_hours = config_->get_int(
        "auth.refresh_token_expiry_hours", 168
    );

    // Generate access token (short-lived)
    auto access_token = JWT::generate_token(
        user_info,
        jwt_secret,
        std::chrono::hours(token_expiry_hours)
    );

    // Generate refresh token (long-lived)
    auto refresh_token = JWT::generate_token(
        user_info,
        jwt_secret,
        std::chrono::hours(refresh_expiry_hours)
    );

    // Build response
    Json::Value response;
    response["access_token"] = access_token;
    response["refresh_token"] = refresh_token;
    response["token_type"] = "Bearer";
    response["expires_in"] = token_expiry_hours * 3600;  // seconds
    
    Json::Value user_json;
    user_json["access_key"] = user_info.access_key;
    user_json["account_name"] = user_info.account_name;
    user_json["is_admin"] = user_info.is_admin;
    response["user"] = user_json;

    return response;
}

Optional<ApiError> AuthService::validate_credentials(
    const String& username,
    const String& password
) {
    if (username.empty()) {
        return ApiError(HttpStatus::BadRequest, "Username is required");
    }

    if (password.empty()) {
        return ApiError(HttpStatus::BadRequest, "Password is required");
    }

    if (username.length() < 3) {
        return ApiError(
            HttpStatus::BadRequest,
            "Username must be at least 3 characters long"
        );
    }

    if (password.length() < 8) {
        return ApiError(
            HttpStatus::BadRequest,
            "Password must be at least 8 characters long"
        );
    }

    // Validate username format (alphanumeric, underscore, hyphen)
    std::regex username_regex("^[a-zA-Z0-9_-]+$");
    if (!std::regex_match(username, username_regex)) {
        return ApiError(
            HttpStatus::BadRequest,
            "Username contains invalid characters"
        );
    }

    return {};  // No error
}

} // namespace console::services

