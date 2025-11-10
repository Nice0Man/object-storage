#include "console/services/ObjectService.hpp"
#include "console/common/Logger.hpp"

namespace console::services {

using namespace console::models;

ObjectService::ObjectService(
    std::shared_ptr<clients::IMinioClient> minio_client
) : minio_client_(minio_client),
    max_single_upload_size_(5ULL * 1024 * 1024 * 1024),  // 5GB
    multipart_threshold_(100ULL * 1024 * 1024) {          // 100MB
    CONSOLE_LOG_INFO("ObjectService initialized");
}

Result<Vector<Object>, ApiError> ObjectService::list_objects(
    const UserInfo& user_info,
    const String& bucket_name,
    const String& prefix,
    bool recursive,
    int max_keys
) {
    CONSOLE_LOG_DEBUG("Listing objects in bucket: {} with prefix: {}", 
              bucket_name, prefix);

    clients::ListObjectsOptions options;
    options.prefix = prefix;
    options.recursive = recursive;
    options.max_keys = max_keys;
    
    auto result = minio_client_->list_objects(bucket_name, options);

    if (!result) {
        CONSOLE_LOG_ERROR("Failed to list objects in bucket {}: {}", 
                  bucket_name, result.error());
        return Err<Vector<Object>>(ApiError(
            HttpStatus::InternalServerError,
            "Failed to list objects: " + result.error()
        ));
    }

    // Get objects from response
    auto objects = result.value().objects;
    
    CONSOLE_LOG_INFO("Listed {} objects from bucket: {}", objects.size(), bucket_name);
    return Result<Vector<Object>, ApiError>(ok_tag, objects);
}

Result<Object, ApiError> ObjectService::get_object_info(
    const UserInfo& user_info,
    const String& bucket_name,
    const String& object_key
) {
    CONSOLE_LOG_DEBUG("Getting object info: {} from bucket: {}", 
              object_key, bucket_name);

    if (auto error = validate_object_key(object_key)) {
        return Err<Object>(*error);
    }

    auto result = minio_client_->stat_object(bucket_name, object_key);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to get object info for {}/{}: {}", 
                  bucket_name, object_key, result.error());
        return Err<Object>(ApiError(
            HttpStatus::NotFound,
            "Object not found: " + result.error()
        ));
    }

    return Result<Object, ApiError>(ok_tag, result.value());
}

Result<ByteArray, ApiError> ObjectService::download_object(
    const UserInfo& user_info,
    const String& bucket_name,
    const String& object_key
) {
    CONSOLE_LOG_INFO("Downloading object: {} from bucket: {}", 
             object_key, bucket_name);

    if (auto error = validate_object_key(object_key)) {
        return Err<ByteArray>(*error);
    }

    auto result = minio_client_->get_object(bucket_name, object_key);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to download object {}/{}: {}", 
                  bucket_name, object_key, result.error());
        return Err<ByteArray>(ApiError(
            HttpStatus::InternalServerError,
            "Failed to download object: " + result.error()
        ));
    }

    CONSOLE_LOG_INFO("Successfully downloaded object: {} ({} bytes)", 
             object_key, result.value().size());
    return Result<ByteArray, ApiError>(ok_tag, result.value());
}

Result<Object, ApiError> ObjectService::upload_object(
    const UserInfo& user_info,
    const String& bucket_name,
    const String& object_key,
    const ByteArray& data,
    const String& content_type,
    const StringMap& metadata
) {
    CONSOLE_LOG_INFO("Uploading object: {} to bucket: {} ({} bytes)", 
             object_key, bucket_name, data.size());

    if (auto error = validate_object_key(object_key)) {
        return Err<Object>(*error);
    }

    if (data.size() > max_single_upload_size_) {
        return Err<Object>(ApiError(
            HttpStatus::BadRequest,
            "Object size exceeds maximum single upload size"
        ));
    }

    clients::PutObjectOptions options;
    options.content_type = content_type;

    auto result = minio_client_->put_object(
        bucket_name,
        object_key,
        data,
        options
    );

    if (!result) {
        CONSOLE_LOG_ERROR("Failed to upload object {}/{}: {}", 
                  bucket_name, object_key, result.error());
        return Err<Object>(ApiError(
            HttpStatus::InternalServerError,
            "Failed to upload object: " + result.error()
        ));
    }

    CONSOLE_LOG_INFO("Successfully uploaded object: {}", object_key);

    // Get object info to return
    auto info_result = minio_client_->stat_object(bucket_name, object_key);
    if (!info_result) {
        return Err<Object>(ApiError(
            HttpStatus::InternalServerError,
            "Object uploaded but failed to retrieve info"
        ));
    }

    return Ok<Object>(info_result.value());
}

Result<void, ApiError> ObjectService::delete_object(
    const UserInfo& user_info,
    const String& bucket_name,
    const String& object_key
) {
    CONSOLE_LOG_INFO("Deleting object: {} from bucket: {}", 
             object_key, bucket_name);

    if (auto error = validate_object_key(object_key)) {
        return Result<void, ApiError>(err_tag, *error);
    }

    auto result = minio_client_->delete_object(bucket_name, object_key);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to delete object {}/{}: {}", 
                  bucket_name, object_key, result.error());
        return Result<void, ApiError>(err_tag, ApiError(
            HttpStatus::InternalServerError,
            "Failed to delete object: " + result.error()
        ));
    }

    CONSOLE_LOG_INFO("Successfully deleted object: {}", object_key);
    return Ok<ApiError>();
}

Result<Object, ApiError> ObjectService::copy_object(
    const UserInfo& user_info,
    const String& source_bucket,
    const String& source_key,
    const String& dest_bucket,
    const String& dest_key
) {
    CONSOLE_LOG_INFO("Copying object from {}/{} to {}/{}", 
             source_bucket, source_key, dest_bucket, dest_key);

    if (auto error = validate_object_key(source_key)) {
        return Err<Object>(*error);
    }

    if (auto error = validate_object_key(dest_key)) {
        return Err<Object>(*error);
    }

    auto result = minio_client_->copy_object(
        source_bucket,
        source_key,
        dest_bucket,
        dest_key
    );

    if (!result) {
        CONSOLE_LOG_ERROR("Failed to copy object: {}", result.error());
        return Err<Object>(ApiError(
            HttpStatus::InternalServerError,
            "Failed to copy object: " + result.error()
        ));
    }

    CONSOLE_LOG_INFO("Successfully copied object");

    // Get dest object info
    auto info_result = minio_client_->stat_object(dest_bucket, dest_key);
    if (!info_result) {
        return Err<Object>(ApiError(
            HttpStatus::InternalServerError,
            "Object copied but failed to retrieve info"
        ));
    }

    return Ok<Object>(info_result.value());
}

Result<StringMap, ApiError> ObjectService::get_object_tags(
    const UserInfo& user_info,
    const String& bucket_name,
    const String& object_key
) {
    CONSOLE_LOG_DEBUG("Getting tags for object: {}/{}", bucket_name, object_key);

    if (auto error = validate_object_key(object_key)) {
        return Err<StringMap>(*error);
    }

    auto result = minio_client_->get_object_tags(bucket_name, object_key);
    if (!result) {
        return Err<StringMap>(ApiError(
            HttpStatus::InternalServerError,
            "Failed to get object tags: " + result.error()
        ));
    }

    return Result<StringMap, ApiError>(ok_tag, result.value());
}

Result<void, ApiError> ObjectService::set_object_tags(
    const UserInfo& user_info,
    const String& bucket_name,
    const String& object_key,
    const StringMap& tags
) {
    CONSOLE_LOG_INFO("Setting {} tags for object: {}/{}", 
             tags.size(), bucket_name, object_key);

    if (auto error = validate_object_key(object_key)) {
        return Result<void, ApiError>(err_tag, *error);
    }

    auto result = minio_client_->set_object_tags(bucket_name, object_key, tags);
    if (!result) {
        return Result<void, ApiError>(err_tag, ApiError(
            HttpStatus::InternalServerError,
            "Failed to set object tags: " + result.error()
        ));
    }

    return Ok<ApiError>();
}

Result<void, ApiError> ObjectService::delete_object_tags(
    const UserInfo& user_info,
    const String& bucket_name,
    const String& object_key
) {
    CONSOLE_LOG_INFO("Deleting tags for object: {}/{}", bucket_name, object_key);

    if (auto error = validate_object_key(object_key)) {
        return Result<void, ApiError>(err_tag, *error);
    }

    auto result = minio_client_->delete_object_tags(bucket_name, object_key);
    if (!result) {
        return Result<void, ApiError>(err_tag, ApiError(
            HttpStatus::InternalServerError,
            "Failed to delete object tags: " + result.error()
        ));
    }

    return Ok<ApiError>();
}

Result<String, ApiError> ObjectService::generate_presigned_url(
    const UserInfo& user_info,
    const String& bucket_name,
    const String& object_key,
    const String& method,
    int expiry_seconds
) {
    CONSOLE_LOG_INFO("Generating presigned URL for: {}/{} (expires in {}s)", 
             bucket_name, object_key, expiry_seconds);

    if (auto error = validate_object_key(object_key)) {
        return Err<String>(*error);
    }

    if (expiry_seconds <= 0 || expiry_seconds > 7 * 24 * 3600) {
        return Err<String>(ApiError(
            HttpStatus::BadRequest,
            "Expiry must be between 1 second and 7 days"
        ));
    }

    auto result = minio_client_->get_presigned_object_url(
        bucket_name,
        object_key,
        expiry_seconds
    );

    if (!result) {
        return Err<String>(ApiError(
            HttpStatus::InternalServerError,
            "Failed to generate presigned URL: " + result.error()
        ));
    }

    return Result<String, ApiError>(ok_tag, result.value());
}

Optional<ApiError> ObjectService::validate_object_key(const String& key) {
    if (key.empty()) {
        return ApiError(HttpStatus::BadRequest, "Object key cannot be empty");
    }

    if (key.length() > 1024) {
        return ApiError(
            HttpStatus::BadRequest,
            "Object key cannot exceed 1024 characters"
        );
    }

    // Check for invalid characters
    if (key.find('\0') != String::npos) {
        return ApiError(
            HttpStatus::BadRequest,
            "Object key contains invalid characters"
        );
    }

    return std::nullopt;
}

Optional<ApiError> ObjectService::validate_access(
    const UserInfo& user,
    const String& bucket_name,
    const String& object_key,
    const String& action
) {
    // For admin users, allow all actions
    if (user.is_admin) {
        return std::nullopt;
    }

    // TODO: Implement proper policy-based access control
    if (user.policies.empty()) {
        return ApiError(
            HttpStatus::Forbidden,
            "User has no policies assigned"
        );
    }

    return std::nullopt;
}

} // namespace console::services
