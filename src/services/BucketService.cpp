

#include "console/services/BucketService.hpp"
#include "console/common/Logger.hpp"
#include <regex>
#include <json/json.h>

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
        return Err(ApiError(
            HttpStatus::InternalServerError,
            "Failed to list buckets: " + result.error()
        ));
    }

    CONSOLE_LOG_INFO("Successfully listed {} buckets", result.value().size());
    return Ok(result.value());
}

Result<Bucket, ApiError> BucketService::create_bucket(
    const UserInfo& user_info,
    const String& name,
    const String& region,
    bool object_locking
) {
    CONSOLE_LOG_INFO("Creating bucket: {} in region: {}", name, region);

    // Validate bucket name
    if (auto error = validate_bucket_name(name)) {
        CONSOLE_LOG_WARN("Invalid bucket name: {}", name);
        return Err(*error);
    }

    auto result = minio_client_->create_bucket(name, region, object_locking);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to create bucket {}: {}", name, result.error());
        return Err(ApiError(
            HttpStatus::InternalServerError,
            "Failed to create bucket: " + result.error()
        ));
    }

    CONSOLE_LOG_INFO("Successfully created bucket: {}", name);
    return Ok(result.value());
}

Result<void, ApiError> BucketService::delete_bucket(
    const UserInfo& user_info,
    const String& name
) {
    CONSOLE_LOG_INFO("Deleting bucket: {}", name);

    auto result = minio_client_->delete_bucket(name);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to delete bucket {}: {}", name, result.error());
        return Err(ApiError(
            HttpStatus::InternalServerError,
            "Failed to delete bucket: " + result.error()
        ));
    }

    CONSOLE_LOG_INFO("Successfully deleted bucket: {}", name);
    return Ok();
}

Result<Bucket, ApiError> BucketService::get_bucket_info(
    const UserInfo& user_info,
    const String& name
) {
    CONSOLE_LOG_DEBUG("Getting bucket info: {}", name);

    auto result = minio_client_->get_bucket_info(name);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to get bucket info for {}: {}", name, result.error());
        return Err(ApiError(
            HttpStatus::NotFound,
            "Bucket not found: " + result.error()
        ));
    }

    return Ok(result.value());
}

Result<void, ApiError> BucketService::set_bucket_policy(
    const UserInfo& user_info,
    const String& name,
    const String& policy_json
) {
    CONSOLE_LOG_INFO("Setting bucket policy for: {}", name);

    // Validate policy JSON
    if (auto error = validate_policy_json(policy_json)) {
        CONSOLE_LOG_WARN("Invalid policy JSON for bucket: {}", name);
        return Err(*error);
    }

    auto result = minio_client_->set_bucket_policy(name, policy_json);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to set bucket policy for {}: {}", 
                  name, result.error());
        return Err(ApiError(
            HttpStatus::InternalServerError,
            "Failed to set bucket policy: " + result.error()
        ));
    }

    CONSOLE_LOG_INFO("Successfully set bucket policy for: {}", name);
    return Ok();
}

Result<String, ApiError> BucketService::get_bucket_policy(
    const UserInfo& user_info,
    const String& name
) {
    CONSOLE_LOG_DEBUG("Getting bucket policy for: {}", name);

    auto result = minio_client_->get_bucket_policy(name);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to get bucket policy for {}: {}", 
                  name, result.error());
        return Err(ApiError(
            HttpStatus::InternalServerError,
            "Failed to get bucket policy: " + result.error()
        ));
    }

    return Ok(result.value());
}

Result<void, ApiError> BucketService::set_bucket_versioning(
    const UserInfo& user_info,
    const String& name,
    bool enabled
) {
    CONSOLE_LOG_INFO("Setting bucket versioning for {}: {}", name, enabled);

    auto result = minio_client_->set_bucket_versioning(name, enabled);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to set bucket versioning for {}: {}", 
                  name, result.error());
        return Err(ApiError(
            HttpStatus::InternalServerError,
            "Failed to set bucket versioning: " + result.error()
        ));
    }

    CONSOLE_LOG_INFO("Successfully set bucket versioning for: {}", name);
    return Ok();
}

Result<bool, ApiError> BucketService::get_bucket_versioning(
    const UserInfo& user_info,
    const String& name
) {
    CONSOLE_LOG_DEBUG("Getting bucket versioning for: {}", name);

    auto result = minio_client_->get_bucket_versioning(name);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to get bucket versioning for {}: {}", 
                  name, result.error());
        return Err(ApiError(
            HttpStatus::InternalServerError,
            "Failed to get bucket versioning: " + result.error()
        ));
    }

    return Ok(result.value());
}

Result<void, ApiError> BucketService::set_bucket_tags(
    const UserInfo& user_info,
    const String& name,
    const StringMap& tags
) {
    CONSOLE_LOG_INFO("Setting {} tags for bucket: {}", tags.size(), name);

    // TODO(Nice0Man): Implement bucket tagging
    return Err(ApiError(
        HttpStatus::NotImplemented,
        "Bucket tagging not yet implemented"
    ));
}

Result<StringMap, ApiError> BucketService::get_bucket_tags(
    const UserInfo& user_info,
    const String& name
) {
    CONSOLE_LOG_DEBUG("Getting tags for bucket: {}", name);

    // TODO(Nice0Man): Implement bucket tagging
    return Err(ApiError(
        HttpStatus::NotImplemented,
        "Bucket tagging not yet implemented"
    ));
}

Result<void, ApiError> BucketService::delete_bucket_tags(
    const UserInfo& user_info,
    const String& name
) {
    CONSOLE_LOG_INFO("Deleting tags for bucket: {}", name);

    // TODO(Nice0Man): Implement bucket tagging
    return Err(ApiError(
        HttpStatus::NotImplemented,
        "Bucket tagging not yet implemented"
    ));
}

// Private methods

Optional<ApiError> BucketService::validate_bucket_name(const String& name) {
    // S3 bucket naming rules:
    // - 3-63 characters
    // - Lowercase letters, numbers, hyphens, dots
    // - Must start with letter or number
    // - Cannot be formatted as IP address

    if (name.empty()) {
        return ApiError(HttpStatus::BadRequest, "Bucket name is required");
    }

    if (name.length() < 3 || name.length() > 63) {
        return ApiError(
            HttpStatus::BadRequest,
            "Bucket name must be between 3 and 63 characters"
        );
    }

    // Check valid characters
    std::regex bucket_regex("^[a-z0-9][a-z0-9.-]*[a-z0-9]$");
    if (!std::regex_match(name, bucket_regex)) {
        return ApiError(
            HttpStatus::BadRequest,
            "Bucket name contains invalid characters"
        );
    }

    // Cannot have consecutive dots
    if (name.find("..") != String::npos) {
        return ApiError(
            HttpStatus::BadRequest,
            "Bucket name cannot contain consecutive dots"
        );
    }

    // Cannot be IP address format
    std::regex ip_regex("^\\d+\\.\\d+\\.\\d+\\.\\d+$");
    if (std::regex_match(name, ip_regex)) {
        return ApiError(
            HttpStatus::BadRequest,
            "Bucket name cannot be formatted as IP address"
        );
    }

    return {};  // No error
}

Optional<ApiError> BucketService::validate_policy_json(
    const String& policy_json
) {
    if (policy_json.empty()) {
        return ApiError(HttpStatus::BadRequest, "Policy JSON is required");
    }

    // Try to parse as JSON
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::istringstream stream(policy_json);
    String errors;

    if (!Json::parseFromStream(builder, stream, &root, &errors)) {
        return ApiError(
            HttpStatus::BadRequest,
            "Invalid JSON: " + errors
        );
    }

    // Basic policy structure validation
    if (!root.isMember("Version")) {
        return ApiError(
            HttpStatus::BadRequest,
            "Policy must contain 'Version' field"
        );
    }

    if (!root.isMember("Statement") || !root["Statement"].isArray()) {
        return ApiError(
            HttpStatus::BadRequest,
            "Policy must contain 'Statement' array"
        );
    }

    return {};  // No error
}

} // namespace console::services

