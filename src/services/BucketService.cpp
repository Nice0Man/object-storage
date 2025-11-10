#include "console/services/BucketService.hpp"
#include "console/common/Logger.hpp"

namespace console::services {

using namespace console::models;

BucketService::BucketService(
    std::shared_ptr<clients::IMinioClient> minio_client
) : minio_client_(minio_client) {
    CONSOLE_LOG_INFO("BucketService initialized");
}

Result<Vector<Bucket>, ApiError> BucketService::list_buckets(
    const UserInfo& user_info
) {
    CONSOLE_LOG_DEBUG("Listing buckets for user: {}", user_info.access_key);

    auto result = minio_client_->list_buckets();
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to list buckets: {}", result.error());
        return Err<Vector<Bucket>>(ApiError(
            HttpStatus::InternalServerError,
            "Failed to list buckets: " + result.error()
        ));
    }

    CONSOLE_LOG_INFO("Successfully listed {} buckets", result.value().size());
    return Result<Vector<Bucket>, ApiError>(ok_tag, result.value());
}

Result<Bucket, ApiError> BucketService::create_bucket(
    const UserInfo& user_info,
    const String& name,
    const String& region,
    bool object_locking
) {
    CONSOLE_LOG_INFO("Creating bucket: {} in region: {}", name, region);

    if (auto error = validate_bucket_name(name)) {
        CONSOLE_LOG_WARN("Invalid bucket name: {}", name);
        return Err<Bucket>(*error);
    }

    auto result = minio_client_->create_bucket(name, region);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to create bucket {}: {}", name, result.error());
        return Err<Bucket>(ApiError(
            HttpStatus::InternalServerError,
            "Failed to create bucket: " + result.error()
        ));
    }

    CONSOLE_LOG_INFO("Successfully created bucket: {}", name);
    
    // Get bucket info to return
    auto bucket_result = minio_client_->get_bucket(name);
    if (!bucket_result) {
        return Err<Bucket>(ApiError(
            HttpStatus::InternalServerError,
            "Bucket created but failed to retrieve info"
        ));
    }

    return Result<Bucket, ApiError>(ok_tag, bucket_result.value());
}

Result<void, ApiError> BucketService::delete_bucket(
    const UserInfo& user_info,
    const String& name
) {
    CONSOLE_LOG_INFO("Deleting bucket: {}", name);

    auto result = minio_client_->delete_bucket(name);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to delete bucket {}: {}", name, result.error());
        return Result<void, ApiError>(err_tag, ApiError(
            HttpStatus::InternalServerError,
            "Failed to delete bucket: " + result.error()
        ));
    }

    CONSOLE_LOG_INFO("Bucket deleted: {}", name);
    return Ok<ApiError>();
}

Result<Bucket, ApiError> BucketService::get_bucket_info(
    const UserInfo& user_info,
    const String& name
) {
    CONSOLE_LOG_DEBUG("Getting bucket info: {}", name);

    auto result = minio_client_->get_bucket(name);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to get bucket info for {}: {}", name, result.error());
        return Err<Bucket>(ApiError(
            HttpStatus::NotFound,
            "Bucket not found: " + result.error()
        ));
    }

    return Result<Bucket, ApiError>(ok_tag, result.value());
}

Result<void, ApiError> BucketService::set_bucket_policy(
    const UserInfo& user_info,
    const String& name,
    const String& policy_json
) {
    CONSOLE_LOG_INFO("Setting policy for bucket: {}", name);

    if (auto error = validate_bucket_name(name)) {
        return Result<void, ApiError>(err_tag, *error);
    }

    // TODO: Implement set_bucket_policy in MinioClient
    return Result<void, ApiError>(err_tag, ApiError(
        HttpStatus::NotImplemented,
        "set_bucket_policy not yet implemented"
    ));
}

Result<String, ApiError> BucketService::get_bucket_policy(
    const UserInfo& user_info,
    const String& name
) {
    CONSOLE_LOG_DEBUG("Getting bucket policy for: {}", name);

    // TODO: Implement get_bucket_policy in MinioClient
    return Err<String>(ApiError(
        HttpStatus::NotImplemented,
        "get_bucket_policy not yet implemented"
    ));
}

Result<void, ApiError> BucketService::set_bucket_versioning(
    const UserInfo& user_info,
    const String& name,
    bool enabled
) {
    CONSOLE_LOG_INFO("Setting bucket versioning for {}: {}", name, enabled);

    // TODO: Implement set_bucket_versioning in MinioClient
    return Result<void, ApiError>(err_tag, ApiError(
        HttpStatus::NotImplemented,
        "set_bucket_versioning not yet implemented"
    ));
}

Result<bool, ApiError> BucketService::get_bucket_versioning(
    const UserInfo& user_info,
    const String& name
) {
    CONSOLE_LOG_DEBUG("Getting bucket versioning for: {}", name);

    // TODO: Implement get_bucket_versioning in MinioClient
    return Err<bool>(ApiError(
        HttpStatus::NotImplemented,
        "get_bucket_versioning not yet implemented"
    ));
}

Result<void, ApiError> BucketService::set_bucket_tags(
    const UserInfo& user_info,
    const String& name,
    const StringMap& tags
) {
    CONSOLE_LOG_INFO("Setting tags for bucket: {}", name);

    return Result<void, ApiError>(err_tag, ApiError(
        HttpStatus::NotImplemented,
        "Bucket tags not yet implemented"
    ));
}

Result<StringMap, ApiError> BucketService::get_bucket_tags(
    const UserInfo& user_info,
    const String& name
) {
    CONSOLE_LOG_DEBUG("Getting tags for bucket: {}", name);

    return Err<StringMap>(ApiError(
        HttpStatus::NotImplemented,
        "Bucket tags not yet implemented"
    ));
}

Result<void, ApiError> BucketService::delete_bucket_tags(
    const UserInfo& user_info,
    const String& name
) {
    CONSOLE_LOG_INFO("Deleting tags for bucket: {}", name);

    return Result<void, ApiError>(err_tag, ApiError(
        HttpStatus::NotImplemented,
        "Bucket tags not yet implemented"
    ));
}

Result<bool, ApiError> BucketService::bucket_exists(
    const UserInfo& user_info,
    const String& name
) {
    CONSOLE_LOG_DEBUG("Checking if bucket exists: {}", name);

    auto result = minio_client_->bucket_exists(name);
    if (!result) {
        return Err<bool>(ApiError(
            HttpStatus::InternalServerError,
            "Failed to check bucket existence: " + result.error()
        ));
    }

    return Result<bool, ApiError>(ok_tag, result.value());
}

Optional<ApiError> BucketService::validate_bucket_name(const String& name) {
    if (name.empty()) {
        return ApiError(HttpStatus::BadRequest, "Bucket name cannot be empty");
    }

    if (name.length() < 3 || name.length() > 63) {
        return ApiError(
            HttpStatus::BadRequest,
            "Bucket name must be between 3 and 63 characters"
        );
    }

    // Check for valid characters (lowercase letters, numbers, dots, hyphens)
    for (char c : name) {
        if (!std::isalnum(c) && c != '.' && c != '-') {
            return ApiError(
                HttpStatus::BadRequest,
                "Bucket name contains invalid characters"
            );
        }
    }

    // Cannot start or end with dot or hyphen
    if (name.front() == '.' || name.front() == '-' ||
        name.back() == '.' || name.back() == '-') {
        return ApiError(
            HttpStatus::BadRequest,
            "Bucket name cannot start or end with dot or hyphen"
        );
    }

    return std::nullopt;
}

Optional<ApiError> BucketService::validate_access(
    const UserInfo& user,
    const String& bucket_name,
    const String& action
) {
    // For admin users, allow all actions
    if (user.is_admin) {
        return std::nullopt;
    }

    // TODO: Implement proper policy-based access control
    // For now, just check if user has any policies
    if (user.policies.empty()) {
        return ApiError(
            HttpStatus::Forbidden,
            "User has no policies assigned"
        );
    }

    return std::nullopt;
}

} // namespace console::services
