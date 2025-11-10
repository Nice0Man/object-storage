#include "console/services/UserService.hpp"
#include "console/utils/Logger.hpp"
#include <regex>

namespace console::services {

using namespace console::models;
using namespace console::utils;

UserService::UserService(
    std::shared_ptr<clients::IMinioAdminClient> admin_client
) : admin_client_(admin_client) {
    LOG_INFO("UserService initialized");
}

Result<Vector<User>, ApiError> UserService::list_users(
    const UserInfo& admin_info
) {
    LOG_DEBUG("Listing users");

    if (auto error = require_admin(admin_info)) {
        return Err(*error);
    }

    auto result = admin_client_->list_users();
    if (!result) {
        LOG_ERROR("Failed to list users: {}", result.error());
        return Err(ApiError(
            HttpStatus::InternalServerError,
            "Failed to list users: " + result.error()
        ));
    }

    LOG_INFO("Successfully listed {} users", result.value().size());
    return Ok(result.value());
}

Result<User, ApiError> UserService::get_user(
    const UserInfo& admin_info,
    const String& access_key
) {
    LOG_DEBUG("Getting user: {}", access_key);

    if (auto error = require_admin(admin_info)) {
        return Err(*error);
    }

    if (auto error = validate_access_key(access_key)) {
        return Err(*error);
    }

    auto result = admin_client_->get_user_info(access_key);
    if (!result) {
        LOG_ERROR("Failed to get user {}: {}", access_key, result.error());
        return Err(ApiError(
            HttpStatus::NotFound,
            "User not found: " + result.error()
        ));
    }

    return Ok(result.value());
}

Result<User, ApiError> UserService::create_user(
    const UserInfo& admin_info,
    const String& access_key,
    const String& secret_key,
    const Vector<String>& policies
) {
    LOG_INFO("Creating user: {}", access_key);

    if (auto error = require_admin(admin_info)) {
        return Err(*error);
    }

    if (auto error = validate_access_key(access_key)) {
        return Err(*error);
    }

    if (auto error = validate_secret_key(secret_key)) {
        return Err(*error);
    }

    // Create user
    auto result = admin_client_->create_user(access_key, secret_key);
    if (!result) {
        LOG_ERROR("Failed to create user {}: {}", access_key, result.error());
        return Err(ApiError(
            HttpStatus::InternalServerError,
            "Failed to create user: " + result.error()
        ));
    }

    // Attach policies if provided
    for (const auto& policy_name : policies) {
        auto policy_result = admin_client_->set_user_policy(
            access_key,
            policy_name
        );
        if (!policy_result) {
            LOG_WARN("Failed to attach policy {} to user {}: {}",
                     policy_name, access_key, policy_result.error());
        }
    }

    LOG_INFO("Successfully created user: {}", access_key);
    return Ok(result.value());
}

Result<void, ApiError> UserService::update_user(
    const UserInfo& admin_info,
    const String& access_key,
    const Optional<String>& new_secret_key,
    const Optional<Vector<String>>& policies
) {
    LOG_INFO("Updating user: {}", access_key);

    if (auto error = require_admin(admin_info)) {
        return Err(*error);
    }

    if (auto error = validate_access_key(access_key)) {
        return Err(*error);
    }

    // Update secret key if provided
    if (new_secret_key) {
        if (auto error = validate_secret_key(*new_secret_key)) {
            return Err(*error);
        }

        // Delete and recreate user with new credentials
        auto delete_result = admin_client_->delete_user(access_key);
        if (!delete_result) {
            LOG_ERROR("Failed to delete user for update: {}", 
                      delete_result.error());
            return Err(ApiError(
                HttpStatus::InternalServerError,
                "Failed to update user credentials"
            ));
        }

        auto create_result = admin_client_->create_user(
            access_key,
            *new_secret_key
        );
        if (!create_result) {
            LOG_ERROR("Failed to recreate user: {}", create_result.error());
            return Err(ApiError(
                HttpStatus::InternalServerError,
                "Failed to update user credentials"
            ));
        }
    }

    // Update policies if provided
    if (policies) {
        for (const auto& policy_name : *policies) {
            auto result = admin_client_->set_user_policy(
                access_key,
                policy_name
            );
            if (!result) {
                LOG_WARN("Failed to set policy {} for user {}: {}",
                         policy_name, access_key, result.error());
            }
        }
    }

    LOG_INFO("Successfully updated user: {}", access_key);
    return Ok();
}

Result<void, ApiError> UserService::delete_user(
    const UserInfo& admin_info,
    const String& access_key
) {
    LOG_INFO("Deleting user: {}", access_key);

    if (auto error = require_admin(admin_info)) {
        return Err(*error);
    }

    if (auto error = validate_access_key(access_key)) {
        return Err(*error);
    }

    // Prevent deleting self
    if (access_key == admin_info.access_key) {
        return Err(ApiError(
            HttpStatus::BadRequest,
            "Cannot delete your own user account"
        ));
    }

    auto result = admin_client_->delete_user(access_key);
    if (!result) {
        LOG_ERROR("Failed to delete user {}: {}", access_key, result.error());
        return Err(ApiError(
            HttpStatus::InternalServerError,
            "Failed to delete user: " + result.error()
        ));
    }

    LOG_INFO("Successfully deleted user: {}", access_key);
    return Ok();
}

Result<void, ApiError> UserService::set_user_status(
    const UserInfo& admin_info,
    const String& access_key,
    bool enabled
) {
    LOG_INFO("Setting user {} status to: {}", access_key, enabled);

    if (auto error = require_admin(admin_info)) {
        return Err(*error);
    }

    // TODO(Nice0Man): Implement user enable/disable via MinIO Admin API
    return Err(ApiError(
        HttpStatus::NotImplemented,
        "User status change not yet implemented"
    ));
}

Result<void, ApiError> UserService::attach_user_policy(
    const UserInfo& admin_info,
    const String& access_key,
    const String& policy_name
) {
    LOG_INFO("Attaching policy {} to user {}", policy_name, access_key);

    if (auto error = require_admin(admin_info)) {
        return Err(*error);
    }

    auto result = admin_client_->set_user_policy(access_key, policy_name);
    if (!result) {
        LOG_ERROR("Failed to attach policy to user: {}", result.error());
        return Err(ApiError(
            HttpStatus::InternalServerError,
            "Failed to attach policy: " + result.error()
        ));
    }

    LOG_INFO("Successfully attached policy to user");
    return Ok();
}

Result<void, ApiError> UserService::detach_user_policy(
    const UserInfo& admin_info,
    const String& access_key,
    const String& policy_name
) {
    LOG_INFO("Detaching policy {} from user {}", policy_name, access_key);

    if (auto error = require_admin(admin_info)) {
        return Err(*error);
    }

    // TODO(Nice0Man): Implement policy detachment
    return Err(ApiError(
        HttpStatus::NotImplemented,
        "Policy detachment not yet implemented"
    ));
}

Result<Vector<String>, ApiError> UserService::list_user_policies(
    const UserInfo& admin_info,
    const String& access_key
) {
    LOG_DEBUG("Listing policies for user: {}", access_key);

    if (auto error = require_admin(admin_info)) {
        return Err(*error);
    }

    // Get user info (includes policies)
    auto user_result = get_user(admin_info, access_key);
    if (!user_result) {
        return Err(user_result.error());
    }

    return Ok(user_result.value().policies());
}

Result<void, ApiError> UserService::add_user_to_group(
    const UserInfo& admin_info,
    const String& access_key,
    const String& group_name
) {
    LOG_INFO("Adding user {} to group {}", access_key, group_name);

    if (auto error = require_admin(admin_info)) {
        return Err(*error);
    }

    // Get current groups
    Vector<String> groups = {group_name};
    
    auto result = admin_client_->update_user_groups(access_key, groups);
    if (!result) {
        LOG_ERROR("Failed to add user to group: {}", result.error());
        return Err(ApiError(
            HttpStatus::InternalServerError,
            "Failed to add user to group: " + result.error()
        ));
    }

    LOG_INFO("Successfully added user to group");
    return Ok();
}

Result<void, ApiError> UserService::remove_user_from_group(
    const UserInfo& admin_info,
    const String& access_key,
    const String& group_name
) {
    LOG_INFO("Removing user {} from group {}", access_key, group_name);

    if (auto error = require_admin(admin_info)) {
        return Err(*error);
    }

    // TODO(Nice0Man): Implement group removal
    return Err(ApiError(
        HttpStatus::NotImplemented,
        "Group removal not yet implemented"
    ));
}

// Private methods

Optional<ApiError> UserService::require_admin(const UserInfo& user_info) {
    if (!user_info.is_admin) {
        return ApiError(
            HttpStatus::Forbidden,
            "Admin privileges required for this operation"
        );
    }
    return {};  // No error
}

Optional<ApiError> UserService::validate_access_key(const String& access_key) {
    if (access_key.empty()) {
        return ApiError(HttpStatus::BadRequest, "Access key is required");
    }

    if (access_key.length() < 3) {
        return ApiError(
            HttpStatus::BadRequest,
            "Access key must be at least 3 characters long"
        );
    }

    if (access_key.length() > 128) {
        return ApiError(
            HttpStatus::BadRequest,
            "Access key exceeds maximum length of 128 characters"
        );
    }

    // Validate format (alphanumeric, underscore, hyphen)
    std::regex key_regex("^[a-zA-Z0-9_-]+$");
    if (!std::regex_match(access_key, key_regex)) {
        return ApiError(
            HttpStatus::BadRequest,
            "Access key contains invalid characters"
        );
    }

    return {};  // No error
}

Optional<ApiError> UserService::validate_secret_key(const String& secret_key) {
    if (secret_key.empty()) {
        return ApiError(HttpStatus::BadRequest, "Secret key is required");
    }

    if (secret_key.length() < 8) {
        return ApiError(
            HttpStatus::BadRequest,
            "Secret key must be at least 8 characters long"
        );
    }

    if (secret_key.length() > 128) {
        return ApiError(
            HttpStatus::BadRequest,
            "Secret key exceeds maximum length of 128 characters"
        );
    }

    return {};  // No error
}

} // namespace console::services

