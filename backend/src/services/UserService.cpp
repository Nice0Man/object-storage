#include "console/services/UserService.hpp"

#include "console/common/Logger.hpp"

#include <algorithm>
#include <regex>

namespace console::services {

using namespace console::models;

UserService::UserService(std::shared_ptr<clients::IMinioAdminClient> admin_client) : admin_client_(admin_client) {
    CONSOLE_LOG_INFO("UserService initialized");
}

Result<Vector<User>, ApiError>
UserService::list_users(const UserInfo& admin_info) {
    CONSOLE_LOG_DEBUG("Listing users");

    if (auto error = require_admin(admin_info)) {
        return Err<Vector<User>>(*error);
    }

    auto result = admin_client_->list_users();
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to list users: {}", result.error());
        return Err<Vector<User>>(ApiError(HttpStatus::InternalServerError, "Failed to list users: " + result.error()));
    }

    CONSOLE_LOG_INFO("Successfully listed {} users", result.value().size());
    return Result<Vector<User>, ApiError>(ok_tag, result.value());
}

Result<User, ApiError>
UserService::get_user(const UserInfo& admin_info, const String& access_key) {
    CONSOLE_LOG_DEBUG("Getting user: {}", access_key);

    if (auto error = require_admin(admin_info)) {
        return Err<User>(*error);
    }

    if (auto error = validate_access_key(access_key)) {
        return Err<User>(*error);
    }

    auto result = admin_client_->get_user(access_key);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to get user {}: {}", access_key, result.error());
        return Err<User>(ApiError(HttpStatus::NotFound, "User not found: " + result.error()));
    }

    return Result<User, ApiError>(ok_tag, result.value());
}

Result<User, ApiError>
UserService::create_user(const UserInfo& admin_info,
                         const String& access_key,
                         const String& secret_key,
                         const Vector<String>& policies,
                         bool is_admin) {
    CONSOLE_LOG_INFO("Creating user: {} (is_admin: {})", access_key, is_admin);

    if (auto error = require_admin(admin_info)) {
        return Err<User>(*error);
    }

    if (auto error = validate_access_key(access_key)) {
        return Err<User>(*error);
    }

    if (auto error = validate_secret_key(secret_key)) {
        return Err<User>(*error);
    }

    // Create user with admin flag
    auto result = admin_client_->create_user(access_key, secret_key, is_admin);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to create user {}: {}", access_key, result.error());
        return Err<User>(ApiError(HttpStatus::InternalServerError, "Failed to create user: " + result.error()));
    }

    // Attach policies if provided
    for (const auto& policy_name : policies) {
        auto policy_result = admin_client_->set_user_policy(access_key, policy_name);
        if (!policy_result) {
            CONSOLE_LOG_WARN(
                "Failed to attach policy {} to user {}: {}", policy_name, access_key, policy_result.error());
        }
    }

    CONSOLE_LOG_INFO("Successfully created user: {}", access_key);
    return Result<User, ApiError>(ok_tag, result.value());
}

Result<void, ApiError>
UserService::update_user(const UserInfo& admin_info,
                         const String& access_key,
                         const Optional<String>& new_secret_key,
                         const Optional<Vector<String>>& policies) {
    CONSOLE_LOG_INFO("Updating user: {}", access_key);

    if (auto error = require_admin(admin_info)) {
        return Result<void, ApiError>(err_tag, *error);
    }

    if (auto error = validate_access_key(access_key)) {
        return Result<void, ApiError>(err_tag, *error);
    }

    // Update secret key if provided
    if (new_secret_key) {
        if (auto error = validate_secret_key(*new_secret_key)) {
            return Result<void, ApiError>(err_tag, *error);
        }

        // Delete and recreate user with new credentials
        auto delete_result = admin_client_->delete_user(access_key);
        if (!delete_result) {
            CONSOLE_LOG_ERROR("Failed to delete user for update: {}", delete_result.error());
            return Result<void, ApiError>(
                err_tag, ApiError(HttpStatus::InternalServerError, "Failed to update user credentials"));
        }

        auto create_result = admin_client_->create_user(access_key, *new_secret_key);
        if (!create_result) {
            CONSOLE_LOG_ERROR("Failed to recreate user: {}", create_result.error());
            return Result<void, ApiError>(
                err_tag, ApiError(HttpStatus::InternalServerError, "Failed to update user credentials"));
        }
    }

    // Update policies if provided
    if (policies) {
        for (const auto& policy_name : *policies) {
            auto result = admin_client_->set_user_policy(access_key, policy_name);
            if (!result) {
                CONSOLE_LOG_WARN("Failed to set policy {} for user {}: {}", policy_name, access_key, result.error());
            }
        }
    }

    CONSOLE_LOG_INFO("Successfully updated user: {}", access_key);
    return Ok<ApiError>();
}

Result<void, ApiError>
UserService::delete_user(const UserInfo& admin_info, const String& access_key) {
    CONSOLE_LOG_INFO("Deleting user: {}", access_key);

    if (auto error = require_admin(admin_info)) {
        return Result<void, ApiError>(err_tag, *error);
    }

    if (auto error = validate_access_key(access_key)) {
        return Result<void, ApiError>(err_tag, *error);
    }

    // Prevent deleting self
    if (access_key == admin_info.access_key) {
        return Result<void, ApiError>(err_tag, ApiError(HttpStatus::BadRequest, "Cannot delete your own user account"));
    }

    auto result = admin_client_->delete_user(access_key);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to delete user {}: {}", access_key, result.error());
        return Result<void, ApiError>(
            err_tag, ApiError(HttpStatus::InternalServerError, "Failed to delete user: " + result.error()));
    }

    CONSOLE_LOG_INFO("Successfully deleted user: {}", access_key);
    return Ok<ApiError>();
}

Result<void, ApiError>
UserService::set_user_status(const UserInfo& admin_info, const String& access_key, bool enabled) {
    CONSOLE_LOG_INFO("Setting user {} status to: {}", access_key, enabled);

    if (auto error = require_admin(admin_info)) {
        return Result<void, ApiError>(err_tag, *error);
    }

    return Result<void, ApiError>(err_tag,
                                  ApiError(HttpStatus::NotImplemented, "User status change not yet implemented"));
}

Result<void, ApiError>
UserService::attach_user_policy(const UserInfo& admin_info, const String& access_key, const String& policy_name) {
    CONSOLE_LOG_INFO("Attaching policy {} to user {}", policy_name, access_key);

    if (auto error = require_admin(admin_info)) {
        return Result<void, ApiError>(err_tag, *error);
    }

    auto result = admin_client_->set_user_policy(access_key, policy_name);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to attach policy to user: {}", result.error());
        return Result<void, ApiError>(
            err_tag, ApiError(HttpStatus::InternalServerError, "Failed to attach policy: " + result.error()));
    }

    CONSOLE_LOG_INFO("Successfully attached policy to user");
    return Ok<ApiError>();
}

Result<void, ApiError>
UserService::detach_user_policy(const UserInfo& admin_info, const String& access_key, const String& policy_name) {
    CONSOLE_LOG_INFO("Detaching policy {} from user {}", policy_name, access_key);

    if (auto error = require_admin(admin_info)) {
        return Result<void, ApiError>(err_tag, *error);
    }

    return Result<void, ApiError>(err_tag,
                                  ApiError(HttpStatus::NotImplemented, "Policy detachment not yet implemented"));
}

Result<Vector<String>, ApiError>
UserService::list_user_policies(const UserInfo& admin_info, const String& access_key) {
    CONSOLE_LOG_DEBUG("Listing policies for user: {}", access_key);

    if (auto error = require_admin(admin_info)) {
        return Err<Vector<String>>(*error);
    }

    // Get user info (includes policies)
    auto user_result = get_user(admin_info, access_key);
    if (!user_result) {
        return Err<Vector<String>>(user_result.error());
    }

    return Result<Vector<String>, ApiError>(ok_tag, user_result.value().policies());
}

Result<void, ApiError>
UserService::add_user_to_group(const UserInfo& admin_info, const String& access_key, const String& group_name) {
    CONSOLE_LOG_INFO("Adding user {} to group {}", access_key, group_name);

    if (auto error = require_admin(admin_info)) {
        return Result<void, ApiError>(err_tag, *error);
    }

    // Get current groups
    Vector<String> groups = {group_name};

    auto result = admin_client_->update_user_groups(access_key, groups);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to add user to group: {}", result.error());
        return Result<void, ApiError>(
            err_tag, ApiError(HttpStatus::InternalServerError, "Failed to add user to group: " + result.error()));
    }

    CONSOLE_LOG_INFO("Successfully added user to group");
    return Ok<ApiError>();
}

Result<void, ApiError>
UserService::remove_user_from_group(const UserInfo& admin_info, const String& access_key, const String& group_name) {
    CONSOLE_LOG_INFO("Removing user {} from group {}", access_key, group_name);

    if (auto error = require_admin(admin_info)) {
        return Result<void, ApiError>(err_tag, *error);
    }

    return Result<void, ApiError>(err_tag, ApiError(HttpStatus::NotImplemented, "Group removal not yet implemented"));
}

// Private methods

Optional<ApiError>
UserService::require_admin(const UserInfo& user_info) {
    CONSOLE_LOG_INFO("require_admin: checking user '{}', access_key='{}', is_admin={}",
                     user_info.account_name,
                     user_info.access_key,
                     user_info.is_admin);

    if (!user_info.is_admin) {
        CONSOLE_LOG_WARN("require_admin: ACCESS DENIED - user '{}' is not admin", user_info.access_key);
        return ApiError(HttpStatus::Forbidden, "Admin privileges required for this operation");
    }

    CONSOLE_LOG_INFO("require_admin: ACCESS GRANTED - user '{}' is admin", user_info.access_key);
    return std::nullopt;
}

Optional<ApiError>
UserService::validate_access_key(const String& access_key) {
    if (access_key.empty()) {
        return ApiError(HttpStatus::BadRequest, "Access key is required");
    }

    if (access_key.length() < 3) {
        return ApiError(HttpStatus::BadRequest, "Access key must be at least 3 characters long");
    }

    if (access_key.length() > 128) {
        return ApiError(HttpStatus::BadRequest, "Access key exceeds maximum length of 128 characters");
    }

    // Validate format (alphanumeric, underscore, hyphen)
    std::regex key_regex("^[a-zA-Z0-9_-]+$");
    if (!std::regex_match(access_key, key_regex)) {
        return ApiError(HttpStatus::BadRequest, "Access key contains invalid characters");
    }

    return std::nullopt;
}

Optional<ApiError>
UserService::validate_secret_key(const String& secret_key) {
    if (secret_key.empty()) {
        return ApiError(HttpStatus::BadRequest, "Secret key is required");
    }

    // SECURITY: Increased minimum from 8 to 16 for stronger secrets
    if (secret_key.length() < 16) {
        return ApiError(HttpStatus::BadRequest, "Secret key must be at least 16 characters long");
    }

    if (secret_key.length() > 128) {
        return ApiError(HttpStatus::BadRequest, "Secret key exceeds maximum length of 128 characters");
    }

    // SECURITY: Check for common weak patterns
    const std::vector<String> weak_patterns = {"password", "12345678", "qwerty", "admin123", "secret"};

    String lower_key = secret_key;
    std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(), ::tolower);

    for (const auto& weak : weak_patterns) {
        if (lower_key.find(weak) != String::npos) {
            return ApiError(HttpStatus::BadRequest, "Secret key contains weak patterns - use a stronger key");
        }
    }

    return std::nullopt;
}

} // namespace console::services
