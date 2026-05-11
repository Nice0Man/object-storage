

#include "console/services/AuthService.hpp"

#include "console/common/Logger.hpp"
#include "console/utils/JWT.hpp"
#include "console/utils/PasswordHash.hpp"
#include "console/utils/TokenBlacklist.hpp"

#include <chrono>
#include <ctime>
#include <jwt-cpp/traits/nlohmann-json/traits.h>
#include <regex>

namespace console::services {

using namespace console::models;
using namespace console::utils;

AuthService::AuthService(std::shared_ptr<clients::IMinioAdminClient> admin_client,
                         std::shared_ptr<Config> config,
                         std::shared_ptr<storage::DatabaseManager> db_manager)
    : admin_client_(admin_client), config_(config), db_manager_(db_manager) {
    CONSOLE_LOG_INFO("AuthService initialized");

    // Initialize JWT with config values
    auto jwt_secret = config_->get<String>("auth.jwt_secret").value_or("default-secret-change-me");
    auto encryption_passphrase = config_->get<String>("auth.encryption_passphrase").value_or("default-passphrase");
    auto encryption_salt = config_->get<String>("auth.encryption_salt").value_or("default-salt");

    JWT::initialize(jwt_secret, encryption_passphrase, encryption_salt);
}

Result<Json::Value, ApiError>
AuthService::login(const String& username, const String& password) {
    CONSOLE_LOG_INFO("Login attempt for user: {}", username);

    // Validate credentials format
    if (auto error = validate_credentials(username, password)) {
        CONSOLE_LOG_WARN("Invalid credentials format for user: {}", username);
        return Err<Json::Value, models::ApiError>(*error);
    }

    // Authenticate with Object Storage and get STS credentials
    auto auth_result = authenticate_with_minio(username, password);
    if (!auth_result) {
        CONSOLE_LOG_ERROR("Authentication failed for user: {}", username);
        return Err<Json::Value, models::ApiError>(auth_result.error());
    }

    // Generate JWT tokens
    auto tokens = generate_tokens(auth_result.value());

    CONSOLE_LOG_INFO("Login successful for user: {}", username);
    return Ok<Json::Value, models::ApiError>(tokens);
}

Result<void, ApiError>
AuthService::logout(const String& token) {
    if (!token.empty()) {
        try {
            auto decoded = jwt::decode<jwt::traits::nlohmann_json>(token);
            auto exp_claim = decoded.get_expires_at();
            utils::TokenBlacklist::instance().add_token(token, exp_claim);
        } catch (...) {
            auto fallback_exp = std::chrono::system_clock::now() + std::chrono::hours(24);
            utils::TokenBlacklist::instance().add_token(token, fallback_exp);
        }
    }

    CONSOLE_LOG_INFO("User logged out successfully");
    return Ok<models::ApiError>();
}

Result<Json::Value, ApiError>
AuthService::refresh_token(const String& refresh_token) {
    // Validate refresh token
    auto claims_result = JWT::validate_token(refresh_token);

    if (claims_result.is_err()) {
        return Err<Json::Value, models::ApiError>(
            ApiError(HttpStatus::Unauthorized, "Invalid or expired refresh token: " + claims_result.error()));
    }

    if (utils::TokenBlacklist::instance().is_blacklisted(refresh_token)) {
        return Err<Json::Value, models::ApiError>(ApiError(HttpStatus::Unauthorized, "Token has been revoked"));
    }

    // Convert claims to UserInfo
    UserInfo user_info = JWT::claims_to_userinfo(claims_result.value());

    // Generate new tokens
    auto tokens = generate_tokens(user_info);

    CONSOLE_LOG_INFO("Token refreshed successfully for user: {}", user_info.access_key);
    return Ok<Json::Value, models::ApiError>(tokens);
}

Result<models::User, ApiError>
AuthService::get_current_user(const String& token) {
    // Validate token
    auto validate_result = validate_token(token);
    if (!validate_result) {
        return Err<models::User, models::ApiError>(validate_result.error());
    }

    auto& user_info = validate_result.value();

    // Create User model from validated UserInfo
    // User details are already populated from JWT token validation
    models::User user(user_info);

    return Ok<models::User, models::ApiError>(user);
}

Result<void, ApiError>
AuthService::change_password(const String& token, const String& old_password, const String& new_password) {
    // Validate token
    auto validate_result = validate_token(token);
    if (!validate_result) {
        return Err<void, models::ApiError>(validate_result.error());
    }

    auto& user_info = validate_result.value();

    // Validate new password
    if (new_password.length() < 8) {
        return Err<void, models::ApiError>(
            ApiError(HttpStatus::BadRequest, "Password must be at least 8 characters long"));
    }

    // Re-authenticate with old password to verify
    auto auth_result = authenticate_with_minio(user_info.access_key, old_password);
    if (!auth_result) {
        return Err<void, models::ApiError>(ApiError(HttpStatus::Unauthorized, "Current password is incorrect"));
    }

    // Get current user from database
    if (!db_manager_) {
        return Err<void, models::ApiError>(
            ApiError(HttpStatus::InternalServerError, "Database manager not initialized"));
    }

    auto user_result = db_manager_->get_user(user_info.access_key);
    if (!user_result) {
        return Err<void, models::ApiError>(ApiError(HttpStatus::NotFound, "User not found"));
    }

    // Update password with new hash
    auto db_user = user_result.value();
    db_user.secret_key = utils::PasswordHash::hash(new_password);
    db_user.updated_at = std::time(nullptr);

    auto update_result = db_manager_->update_user(db_user);
    if (!update_result) {
        CONSOLE_LOG_ERROR("Failed to update password for user: {}", user_info.access_key);
        return Err<void, models::ApiError>(ApiError(HttpStatus::InternalServerError, "Failed to update password"));
    }

    CONSOLE_LOG_INFO("Password changed successfully for user: {}", user_info.access_key);
    return Ok<models::ApiError>();
}

Result<UserInfo, ApiError>
AuthService::validate_token(const String& token) {
    if (utils::TokenBlacklist::instance().is_blacklisted(token)) {
        return Err<UserInfo, models::ApiError>(ApiError(HttpStatus::Unauthorized, "Token has been revoked"));
    }

    // Validate JWT
    auto claims_result = JWT::validate_token(token);

    if (claims_result.is_err()) {
        return Err<UserInfo, models::ApiError>(
            ApiError(HttpStatus::Unauthorized, "Invalid or expired token: " + claims_result.error()));
    }

    // Convert claims to UserInfo
    UserInfo user_info = JWT::claims_to_userinfo(claims_result.value());

    return Ok<UserInfo, models::ApiError>(user_info);
}

// Private methods

Result<UserInfo, ApiError>
AuthService::authenticate_with_minio(const String& username, const String& password) {
    // Authenticate user against local database
    if (!db_manager_) {
        CONSOLE_LOG_ERROR("Database manager not initialized");
        return Err<UserInfo, models::ApiError>(
            ApiError(HttpStatus::InternalServerError, "Authentication service not properly configured"));
    }

    // Look up user by access_key (username)
    auto user_result = db_manager_->get_user(username);
    if (!user_result) {
        CONSOLE_LOG_WARN("User not found: {}", username);
        return Err<UserInfo, models::ApiError>(ApiError(HttpStatus::Unauthorized, "Invalid username or password"));
    }

    auto db_user = user_result.value();

    // Check if user is active
    if (db_user.status != "active") {
        CONSOLE_LOG_WARN("User account is not active: {}", username);
        return Err<UserInfo, models::ApiError>(ApiError(HttpStatus::Forbidden, "Account is not active"));
    }

    bool password_ok = false;
    if (utils::PasswordHash::is_hashed(db_user.secret_key)) {
        password_ok = utils::PasswordHash::verify(password, db_user.secret_key);
    } else if (utils::PasswordHash::secure_equals(password, db_user.secret_key)) {
        // Legacy rows stored the login secret as plaintext (e.g. via LocalAdminClient). Upgrade to PBKDF2.
        password_ok = true;
        db_user.secret_key = utils::PasswordHash::hash(password);
        db_user.updated_at = std::time(nullptr);
        auto upgraded = db_manager_->update_user(db_user);
        if (!upgraded) {
            CONSOLE_LOG_ERROR("Failed to persist password hash upgrade for user {}: {}", username, upgraded.error());
        } else {
            CONSOLE_LOG_WARN("Migrated legacy plaintext password to PBKDF2 for user: {}", username);
        }
    }

    if (!password_ok) {
        CONSOLE_LOG_WARN("Invalid password for user: {}", username);
        return Err<UserInfo, models::ApiError>(ApiError(HttpStatus::Unauthorized, "Invalid username or password"));
    }

    // Create UserInfo from database user
    UserInfo user_info;
    user_info.access_key = db_user.access_key;
    user_info.secret_key = db_user.secret_key; // Keep hashed for session
    user_info.session_token = "";
    user_info.account_name = db_user.account_name;
    user_info.is_admin = db_user.is_admin;
    user_info.created_at = std::chrono::system_clock::from_time_t(db_user.created_at);

    CONSOLE_LOG_INFO("User authenticated successfully: {}", username);
    return Ok<UserInfo, models::ApiError>(user_info);
}

Json::Value
AuthService::generate_tokens(const UserInfo& user_info) {
    auto token_expiry_hours = config_->get<int>("auth.token_expiry_hours").value_or(24);
    auto refresh_expiry_hours = config_->get<int>("auth.refresh_token_expiry_hours").value_or(168);

    // Convert UserInfo to JWTClaims
    JWTClaims access_claims = userinfo_to_claims(user_info, std::chrono::hours(token_expiry_hours));
    JWTClaims refresh_claims = userinfo_to_claims(user_info, std::chrono::hours(refresh_expiry_hours));

    // Generate access token (short-lived)
    auto access_token_result = JWT::generate_token(access_claims);
    auto refresh_token_result = JWT::generate_token(refresh_claims);

    // Build response
    Json::Value response;
    response["access_token"] = access_token_result.value_or("error");
    response["refresh_token"] = refresh_token_result.value_or("error");
    response["token_type"] = "Bearer";
    response["expires_in"] = token_expiry_hours * 3600; // seconds

    Json::Value user_json;
    user_json["access_key"] = user_info.access_key;
    user_json["account_name"] = user_info.account_name;
    user_json["is_admin"] = user_info.is_admin;
    response["user"] = user_json;

    return response;
}

Optional<ApiError>
AuthService::validate_credentials(const String& username, const String& password) {
    if (username.empty()) {
        return ApiError(HttpStatus::BadRequest, "Username is required");
    }

    if (password.empty()) {
        return ApiError(HttpStatus::BadRequest, "Password is required");
    }

    if (username.length() < 3) {
        return ApiError(HttpStatus::BadRequest, "Username must be at least 3 characters long");
    }

    if (password.length() < 8) {
        return ApiError(HttpStatus::BadRequest, "Password must be at least 8 characters long");
    }

    // Validate username format (alphanumeric, underscore, hyphen)
    std::regex username_regex("^[a-zA-Z0-9_-]+$");
    if (!std::regex_match(username, username_regex)) {
        return ApiError(HttpStatus::BadRequest, "Username contains invalid characters");
    }

    return {}; // No error
}

// Helper functions to convert between UserInfo and JWTClaims
JWTClaims
AuthService::userinfo_to_claims(const UserInfo& user_info, Duration expiry) {
    JWTClaims claims;

    // Standard claims
    claims.issuer = "object-storage-console"; // Must match JWT::verify_token expectation
    claims.subject = user_info.access_key;
    claims.issued_at = std::chrono::system_clock::now();
    claims.expires_at = claims.issued_at + expiry;

    // STS credentials (will be encrypted)
    claims.sts_access_key_id = user_info.access_key;
    claims.sts_secret_access_key = user_info.secret_key;
    claims.sts_session_token = user_info.session_token;

    // Account info
    claims.account_access_key = user_info.access_key;
    claims.account_name = user_info.account_name;

    // Policies
    claims.policies = user_info.policies;

    // Additional metadata
    claims.custom_fields["is_admin"] = user_info.is_admin ? "true" : "false";

    return claims;
}

} // namespace console::services
