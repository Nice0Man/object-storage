#include "console/services/BucketService.hpp"

#include "console/common/Logger.hpp"
#include "console/services/PolicyEvaluator.hpp"

#include <json/json.h>
#include <sstream>
#include <unordered_map>

namespace console::services {

using namespace console::models;

BucketService::BucketService(std::shared_ptr<clients::IStorageClient> storage_client)
    : storage_client_(storage_client) {
    CONSOLE_LOG_INFO("BucketService initialized");
}

Result<Vector<Bucket>, ApiError>
BucketService::list_buckets(const UserInfo& user_info) {
    CONSOLE_LOG_DEBUG("Listing buckets for user: {}", user_info.access_key);

    auto result = storage_client_->list_buckets();
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to list buckets: {}", result.error());
        return Err<Vector<Bucket>>(
            ApiError(HttpStatus::InternalServerError, "Failed to list buckets: " + result.error()));
    }

    CONSOLE_LOG_INFO("Successfully listed {} buckets", result.value().size());
    return Result<Vector<Bucket>, ApiError>(ok_tag, result.value());
}

Result<Bucket, ApiError>
BucketService::create_bucket(const UserInfo& user_info, const String& name, const String& region, bool object_locking) {
    CONSOLE_LOG_INFO("Creating bucket: {} in region: {}", name, region);

    if (auto error = validate_bucket_name(name)) {
        CONSOLE_LOG_WARN("Invalid bucket name: {}", name);
        return Err<Bucket>(*error);
    }

    auto result = storage_client_->create_bucket(name, region);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to create bucket {}: {}", name, result.error());
        return Err<Bucket>(ApiError(HttpStatus::InternalServerError, "Failed to create bucket: " + result.error()));
    }

    CONSOLE_LOG_INFO("Successfully created bucket: {}", name);

    // Get bucket info to return
    auto bucket_result = storage_client_->get_bucket(name);
    if (!bucket_result) {
        return Err<Bucket>(ApiError(HttpStatus::InternalServerError, "Bucket created but failed to retrieve info"));
    }

    return Result<Bucket, ApiError>(ok_tag, bucket_result.value());
}

Result<void, ApiError>
BucketService::delete_bucket(const UserInfo& user_info, const String& name) {
    CONSOLE_LOG_INFO("Deleting bucket: {}", name);

    auto result = storage_client_->delete_bucket(name);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to delete bucket {}: {}", name, result.error());
        return Result<void, ApiError>(
            err_tag, ApiError(HttpStatus::InternalServerError, "Failed to delete bucket: " + result.error()));
    }

    CONSOLE_LOG_INFO("Bucket deleted: {}", name);
    return Ok<ApiError>();
}

Result<Bucket, ApiError>
BucketService::get_bucket_info(const UserInfo& user_info, const String& name) {
    CONSOLE_LOG_DEBUG("Getting bucket info: {}", name);

    auto result = storage_client_->get_bucket(name);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to get bucket info for {}: {}", name, result.error());
        return Err<Bucket>(ApiError(HttpStatus::NotFound, "Bucket not found: " + result.error()));
    }

    return Result<Bucket, ApiError>(ok_tag, result.value());
}

Result<void, ApiError>
BucketService::set_bucket_policy(const UserInfo& user_info, const String& name, const String& policy_json) {
    CONSOLE_LOG_INFO("Setting policy for bucket: {}", name);

    if (auto error = validate_bucket_name(name)) {
        return Result<void, ApiError>(err_tag, *error);
    }

    // Validate policy JSON if provided
    if (!policy_json.empty() && policy_json != "{}") {
        if (auto error = validate_policy_json(policy_json)) {
            return Result<void, ApiError>(err_tag, *error);
        }
    }

    // Check bucket exists
    auto exists_result = storage_client_->bucket_exists(name);
    if (!exists_result || !exists_result.value()) {
        return Result<void, ApiError>(err_tag, ApiError(HttpStatus::NotFound, "Bucket not found: " + name));
    }

    // Set policy using LocalStorageClient
    auto result = storage_client_->set_bucket_policy(name, policy_json);
    if (!result) {
        return Result<void, ApiError>(
            err_tag, ApiError(HttpStatus::InternalServerError, "Failed to set policy: " + result.error()));
    }

    CONSOLE_LOG_INFO("Bucket {} policy set successfully", name);
    return Ok<ApiError>();
}

Result<String, ApiError>
BucketService::get_bucket_policy(const UserInfo& user_info, const String& name) {
    CONSOLE_LOG_DEBUG("Getting bucket policy for: {}", name);

    // Check bucket exists
    auto exists_result = storage_client_->bucket_exists(name);
    if (!exists_result || !exists_result.value()) {
        return Err<String>(ApiError(HttpStatus::NotFound, "Bucket not found: " + name));
    }

    // Get policy using LocalStorageClient
    auto result = storage_client_->get_bucket_policy(name);
    if (!result) {
        return Err<String>(ApiError(HttpStatus::InternalServerError, "Failed to get policy: " + result.error()));
    }

    return Result<String, ApiError>(ok_tag, result.value());
}

Result<void, ApiError>
BucketService::set_bucket_versioning(const UserInfo& user_info, const String& name, bool enabled) {
    CONSOLE_LOG_INFO("Setting bucket versioning for {}: {}", name, enabled);

    // Check bucket exists
    auto exists_result = storage_client_->bucket_exists(name);
    if (!exists_result || !exists_result.value()) {
        return Result<void, ApiError>(err_tag, ApiError(HttpStatus::NotFound, "Bucket not found: " + name));
    }

    // Set versioning using LocalStorageClient
    auto result = storage_client_->set_bucket_versioning(name, enabled);
    if (!result) {
        return Result<void, ApiError>(
            err_tag, ApiError(HttpStatus::InternalServerError, "Failed to set versioning: " + result.error()));
    }

    CONSOLE_LOG_INFO("Bucket {} versioning set to {}", name, enabled);
    return Ok<ApiError>();
}

Result<bool, ApiError>
BucketService::get_bucket_versioning(const UserInfo& user_info, const String& name) {
    CONSOLE_LOG_DEBUG("Getting bucket versioning for: {}", name);

    // Check bucket exists
    auto exists_result = storage_client_->bucket_exists(name);
    if (!exists_result || !exists_result.value()) {
        return Err<bool>(ApiError(HttpStatus::NotFound, "Bucket not found: " + name));
    }

    // Get versioning using LocalStorageClient
    auto result = storage_client_->get_bucket_versioning(name);
    if (!result) {
        return Err<bool>(ApiError(HttpStatus::InternalServerError, "Failed to get versioning: " + result.error()));
    }

    return Result<bool, ApiError>(ok_tag, result.value());
}

Result<void, ApiError>
BucketService::set_bucket_tags(const UserInfo& user_info, const String& name, const StringMap& tags) {
    CONSOLE_LOG_INFO("Setting tags for bucket: {}", name);

    // Validate access
    auto access_error = validate_access(user_info, name, "PutBucketTagging");
    if (access_error) {
        return Result<void, ApiError>(err_tag, *access_error);
    }

    // Check bucket exists
    auto exists_result = storage_client_->bucket_exists(name);
    if (!exists_result || !exists_result.value()) {
        return Result<void, ApiError>(err_tag, ApiError(HttpStatus::NotFound, "Bucket not found: " + name));
    }

    // Validate tag count (AWS limit is 50)
    if (tags.size() > 50) {
        return Result<void, ApiError>(err_tag, ApiError(HttpStatus::BadRequest, "Maximum 50 tags allowed per bucket"));
    }

    // Set tags using LocalStorageClient
    auto result = storage_client_->set_bucket_tags(name, tags);
    if (!result) {
        return Result<void, ApiError>(
            err_tag, ApiError(HttpStatus::InternalServerError, "Failed to set bucket tags: " + result.error()));
    }

    CONSOLE_LOG_INFO("Set {} tags for bucket {}", tags.size(), name);
    return Ok<ApiError>();
}

Result<StringMap, ApiError>
BucketService::get_bucket_tags(const UserInfo& user_info, const String& name) {
    CONSOLE_LOG_DEBUG("Getting tags for bucket: {}", name);

    // Validate access
    auto access_error = validate_access(user_info, name, "GetBucketTagging");
    if (access_error) {
        return Err<StringMap>(*access_error);
    }

    // Check bucket exists
    auto exists_result = storage_client_->bucket_exists(name);
    if (!exists_result || !exists_result.value()) {
        return Err<StringMap>(ApiError(HttpStatus::NotFound, "Bucket not found: " + name));
    }

    // Get tags using LocalStorageClient
    auto result = storage_client_->get_bucket_tags(name);
    if (!result) {
        return Err<StringMap>(
            ApiError(HttpStatus::InternalServerError, "Failed to get bucket tags: " + result.error()));
    }

    return Result<StringMap, ApiError>(ok_tag, result.value());
}

Result<void, ApiError>
BucketService::delete_bucket_tags(const UserInfo& user_info, const String& name) {
    CONSOLE_LOG_INFO("Deleting tags for bucket: {}", name);

    // Validate access
    auto access_error = validate_access(user_info, name, "PutBucketTagging");
    if (access_error) {
        return Result<void, ApiError>(err_tag, *access_error);
    }

    // Check bucket exists
    auto exists_result = storage_client_->bucket_exists(name);
    if (!exists_result || !exists_result.value()) {
        return Result<void, ApiError>(err_tag, ApiError(HttpStatus::NotFound, "Bucket not found: " + name));
    }

    // Delete tags using LocalStorageClient
    auto result = storage_client_->delete_bucket_tags(name);
    if (!result) {
        return Result<void, ApiError>(
            err_tag, ApiError(HttpStatus::InternalServerError, "Failed to delete bucket tags: " + result.error()));
    }

    return Ok<ApiError>();
}

Result<bool, ApiError>
BucketService::bucket_exists(const UserInfo& user_info, const String& name) {
    CONSOLE_LOG_DEBUG("Checking if bucket exists: {}", name);

    auto result = storage_client_->bucket_exists(name);
    if (!result) {
        return Err<bool>(
            ApiError(HttpStatus::InternalServerError, "Failed to check bucket existence: " + result.error()));
    }

    return Result<bool, ApiError>(ok_tag, result.value());
}

Optional<ApiError>
BucketService::validate_bucket_name(const String& name) {
    if (name.empty()) {
        return ApiError(HttpStatus::BadRequest, "Bucket name cannot be empty");
    }

    if (name.length() < 3 || name.length() > 63) {
        return ApiError(HttpStatus::BadRequest, "Bucket name must be between 3 and 63 characters");
    }

    // Check for valid characters (lowercase letters, numbers, dots, hyphens only)
    for (char c : name) {
        if (!std::islower(c) && !std::isdigit(c) && c != '.' && c != '-') {
            return ApiError(
                HttpStatus::BadRequest,
                "Bucket name contains invalid characters (only lowercase letters, numbers, dots, and hyphens allowed)");
        }
    }

    // Cannot start or end with dot or hyphen
    if (name.front() == '.' || name.front() == '-' || name.back() == '.' || name.back() == '-') {
        return ApiError(HttpStatus::BadRequest, "Bucket name cannot start or end with dot or hyphen");
    }

    // Check for consecutive dots
    if (name.find("..") != String::npos) {
        return ApiError(HttpStatus::BadRequest, "Bucket name cannot contain consecutive dots");
    }

    // Must contain at least one lowercase letter (prevents IP address-like names)
    bool has_letter = false;
    for (char c : name) {
        if (std::islower(c)) {
            has_letter = true;
            break;
        }
    }
    if (!has_letter) {
        return ApiError(HttpStatus::BadRequest, "Bucket name must contain at least one lowercase letter");
    }

    return std::nullopt;
}

Optional<ApiError>
BucketService::validate_access(const UserInfo& user, const String& bucket_name, const String& action) {
    // Use PolicyEvaluator for proper IAM-style policy evaluation
    PolicyEvaluator evaluator;

    // Map internal action names to S3 actions
    String s3_action = action;
    static const std::unordered_map<String, String> action_map = {
        {"ListBucket", S3Actions::ListBucket},
        {"CreateBucket", S3Actions::CreateBucket},
        {"DeleteBucket", S3Actions::DeleteBucket},
        {"GetBucketPolicy", S3Actions::GetBucketPolicy},
        {"PutBucketPolicy", S3Actions::PutBucketPolicy},
        {"GetBucketVersioning", S3Actions::GetBucketVersioning},
        {"PutBucketVersioning", S3Actions::PutBucketVersioning},
        {"GetBucketTagging", S3Actions::GetBucketTagging},
        {"PutBucketTagging", S3Actions::PutBucketTagging},
        {"GetBucketLocation", S3Actions::GetBucketLocation},
    };

    auto it = action_map.find(action);
    if (it != action_map.end()) {
        s3_action = it->second;
    }

    auto result = evaluator.evaluate_user_access(user, s3_action, bucket_name);

    if (!result.is_allowed()) {
        CONSOLE_LOG_WARN("Access denied for user {} on bucket {}: {}", user.access_key, bucket_name, result.reason);
        return ApiError(HttpStatus::Forbidden, result.reason);
    }

    return std::nullopt;
}

Optional<ApiError>
BucketService::validate_policy_json(const String& policy_json) {
    if (policy_json.empty()) {
        return std::nullopt;
    }

    // Parse and validate JSON structure
    Json::CharReaderBuilder builder;
    Json::Value policy;
    std::istringstream stream(policy_json);
    std::string errors;

    if (!Json::parseFromStream(builder, stream, &policy, &errors)) {
        return ApiError(HttpStatus::BadRequest, "Invalid JSON in policy: " + errors);
    }

    // Validate required fields for IAM policy format
    if (!policy.isMember("Version")) {
        CONSOLE_LOG_WARN("Policy missing Version field, defaulting to 2012-10-17");
    }

    if (!policy.isMember("Statement")) {
        return ApiError(HttpStatus::BadRequest, "Policy must contain 'Statement' field");
    }

    if (!policy["Statement"].isArray()) {
        return ApiError(HttpStatus::BadRequest, "Policy 'Statement' must be an array");
    }

    // Validate each statement
    for (const auto& stmt : policy["Statement"]) {
        if (!stmt.isMember("Effect")) {
            return ApiError(HttpStatus::BadRequest, "Each statement must have an 'Effect' field");
        }

        String effect = stmt["Effect"].asString();
        if (effect != "Allow" && effect != "Deny") {
            return ApiError(HttpStatus::BadRequest, "Effect must be 'Allow' or 'Deny'");
        }

        if (!stmt.isMember("Action") && !stmt.isMember("NotAction")) {
            return ApiError(HttpStatus::BadRequest, "Each statement must have 'Action' or 'NotAction' field");
        }

        if (!stmt.isMember("Resource") && !stmt.isMember("NotResource")) {
            return ApiError(HttpStatus::BadRequest, "Each statement must have 'Resource' or 'NotResource' field");
        }
    }

    return std::nullopt;
}

Result<Json::Value, models::ApiError>
BucketService::get_bucket_encryption(const UserInfo& user_info, const String& name) {
    CONSOLE_LOG_DEBUG("Getting encryption for bucket: {}", name);

    auto result = storage_client_->get_bucket_encryption(name);
    if (!result) {
        return Err<Json::Value>(ApiError(HttpStatus::InternalServerError, result.error()));
    }
    return Ok<Json::Value, ApiError>(result.value());
}

Result<void, models::ApiError>
BucketService::set_bucket_encryption(const UserInfo& user_info, const String& name, const Json::Value& config) {
    CONSOLE_LOG_INFO("Setting encryption for bucket: {}", name);

    auto result = storage_client_->set_bucket_encryption(name, config);
    if (!result) {
        return Err<void>(ApiError(HttpStatus::InternalServerError, result.error()));
    }
    return Ok<ApiError>();
}

Result<Json::Value, models::ApiError>
BucketService::get_bucket_lifecycle(const UserInfo& user_info, const String& name) {
    CONSOLE_LOG_DEBUG("Getting lifecycle for bucket: {}", name);

    auto result = storage_client_->get_bucket_lifecycle(name);
    if (!result) {
        return Err<Json::Value>(ApiError(HttpStatus::InternalServerError, result.error()));
    }
    return Ok<Json::Value, ApiError>(result.value());
}

Result<void, models::ApiError>
BucketService::set_bucket_lifecycle(const UserInfo& user_info, const String& name, const Json::Value& rules) {
    CONSOLE_LOG_INFO("Setting lifecycle for bucket: {}", name);

    auto result = storage_client_->set_bucket_lifecycle(name, rules);
    if (!result) {
        return Err<void>(ApiError(HttpStatus::InternalServerError, result.error()));
    }
    return Ok<ApiError>();
}

Result<Json::Value, models::ApiError>
BucketService::get_bucket_object_lock(const UserInfo& user_info, const String& name) {
    CONSOLE_LOG_DEBUG("Getting object lock config for bucket: {}", name);

    auto result = storage_client_->get_bucket_object_lock(name);
    if (!result) {
        return Err<Json::Value>(ApiError(HttpStatus::InternalServerError, result.error()));
    }
    return Ok<Json::Value, ApiError>(result.value());
}

Result<void, models::ApiError>
BucketService::set_bucket_object_lock(const UserInfo& user_info, const String& name, const Json::Value& config) {
    CONSOLE_LOG_INFO("Setting object lock config for bucket: {}", name);

    auto result = storage_client_->set_bucket_object_lock(name, config);
    if (!result) {
        return Err<void>(ApiError(HttpStatus::InternalServerError, result.error()));
    }
    return Ok<ApiError>();
}

} // namespace console::services
