#include "console/services/ObjectService.hpp"
#include "console/utils/Logger.hpp"

namespace console::services {

using namespace console::models;
using namespace console::utils;

ObjectService::ObjectService(
    std::shared_ptr<clients::IMinioClient> minio_client
) : minio_client_(minio_client) {
    LOG_INFO("ObjectService initialized");
}

Result<Vector<Object>, ApiError> ObjectService::list_objects(
    const UserInfo& user_info,
    const String& bucket_name,
    const String& prefix,
    bool recursive,
    int max_keys
) {
    LOG_DEBUG("Listing objects in bucket: {} with prefix: {}", 
              bucket_name, prefix);

    auto result = minio_client_->list_objects(
        bucket_name,
        prefix,
        recursive
    );

    if (!result) {
        LOG_ERROR("Failed to list objects in bucket {}: {}", 
                  bucket_name, result.error());
        return Err(ApiError(
            HttpStatus::InternalServerError,
            "Failed to list objects: " + result.error()
        ));
    }

    // Apply max_keys limit
    auto objects = result.value();
    if (max_keys > 0 && objects.size() > static_cast<size_t>(max_keys)) {
        objects.resize(max_keys);
    }

    LOG_INFO("Listed {} objects from bucket: {}", objects.size(), bucket_name);
    return Ok(objects);
}

Result<Object, ApiError> ObjectService::get_object_info(
    const UserInfo& user_info,
    const String& bucket_name,
    const String& object_key
) {
    LOG_DEBUG("Getting object info: {} from bucket: {}", 
              object_key, bucket_name);

    if (auto error = validate_object_key(object_key)) {
        return Err(*error);
    }

    auto result = minio_client_->get_object_info(bucket_name, object_key);
    if (!result) {
        LOG_ERROR("Failed to get object info for {}/{}: {}", 
                  bucket_name, object_key, result.error());
        return Err(ApiError(
            HttpStatus::NotFound,
            "Object not found: " + result.error()
        ));
    }

    return Ok(result.value());
}

Result<ByteArray, ApiError> ObjectService::download_object(
    const UserInfo& user_info,
    const String& bucket_name,
    const String& object_key
) {
    LOG_INFO("Downloading object: {} from bucket: {}", 
             object_key, bucket_name);

    if (auto error = validate_object_key(object_key)) {
        return Err(*error);
    }

    auto result = minio_client_->download_object(bucket_name, object_key);
    if (!result) {
        LOG_ERROR("Failed to download object {}/{}: {}", 
                  bucket_name, object_key, result.error());
        return Err(ApiError(
            HttpStatus::InternalServerError,
            "Failed to download object: " + result.error()
        ));
    }

    LOG_INFO("Successfully downloaded object: {} ({} bytes)", 
             object_key, result.value().size());
    return Ok(result.value());
}

Result<Object, ApiError> ObjectService::upload_object(
    const UserInfo& user_info,
    const String& bucket_name,
    const String& object_key,
    const ByteArray& data,
    const String& content_type,
    const StringMap& metadata
) {
    LOG_INFO("Uploading object: {} to bucket: {} ({} bytes)", 
             object_key, bucket_name, data.size());

    if (auto error = validate_object_key(object_key)) {
        return Err(*error);
    }

    // Validate data size (max 5GB for single upload)
    const size_t max_size = 5ULL * 1024 * 1024 * 1024;  // 5GB
    if (data.size() > max_size) {
        return Err(ApiError(
            HttpStatus::BadRequest,
            "Object size exceeds maximum of 5GB for single upload"
        ));
    }

    auto result = minio_client_->upload_object(
        bucket_name,
        object_key,
        data,
        content_type
    );

    if (!result) {
        LOG_ERROR("Failed to upload object {}/{}: {}", 
                  bucket_name, object_key, result.error());
        return Err(ApiError(
            HttpStatus::InternalServerError,
            "Failed to upload object: " + result.error()
        ));
    }

    LOG_INFO("Successfully uploaded object: {}", object_key);
    return Ok(result.value());
}

Result<void, ApiError> ObjectService::delete_object(
    const UserInfo& user_info,
    const String& bucket_name,
    const String& object_key
) {
    LOG_INFO("Deleting object: {} from bucket: {}", object_key, bucket_name);

    if (auto error = validate_object_key(object_key)) {
        return Err(*error);
    }

    auto result = minio_client_->delete_object(bucket_name, object_key);
    if (!result) {
        LOG_ERROR("Failed to delete object {}/{}: {}", 
                  bucket_name, object_key, result.error());
        return Err(ApiError(
            HttpStatus::InternalServerError,
            "Failed to delete object: " + result.error()
        ));
    }

    LOG_INFO("Successfully deleted object: {}", object_key);
    return Ok();
}

Result<Json::Value, ApiError> ObjectService::delete_objects(
    const UserInfo& user_info,
    const String& bucket_name,
    const Vector<String>& object_keys
) {
    LOG_INFO("Batch deleting {} objects from bucket: {}", 
             object_keys.size(), bucket_name);

    Json::Value result;
    result["deleted"] = Json::Value(Json::arrayValue);
    result["errors"] = Json::Value(Json::arrayValue);

    for (const auto& key : object_keys) {
        auto delete_result = delete_object(user_info, bucket_name, key);
        if (delete_result) {
            result["deleted"].append(key);
        } else {
            Json::Value error;
            error["key"] = key;
            error["message"] = delete_result.error().message();
            result["errors"].append(error);
        }
    }

    result["total"] = static_cast<int>(object_keys.size());
    result["deleted_count"] = result["deleted"].size();
    result["error_count"] = result["errors"].size();

    LOG_INFO("Batch delete completed: {} deleted, {} errors", 
             result["deleted_count"].asInt(), 
             result["error_count"].asInt());

    return Ok(result);
}

Result<Object, ApiError> ObjectService::copy_object(
    const UserInfo& user_info,
    const String& source_bucket,
    const String& source_key,
    const String& dest_bucket,
    const String& dest_key
) {
    LOG_INFO("Copying object: {}/{} to {}/{}", 
             source_bucket, source_key, dest_bucket, dest_key);

    if (auto error = validate_object_key(source_key)) {
        return Err(*error);
    }
    if (auto error = validate_object_key(dest_key)) {
        return Err(*error);
    }

    // TODO(Nice0Man): Implement object copy via MinIO API
    return Err(ApiError(
        HttpStatus::NotImplemented,
        "Object copy not yet implemented"
    ));
}

Result<void, ApiError> ObjectService::set_object_metadata(
    const UserInfo& user_info,
    const String& bucket_name,
    const String& object_key,
    const StringMap& metadata
) {
    LOG_INFO("Setting metadata for object: {}/{}", bucket_name, object_key);

    // TODO(Nice0Man): Implement metadata update via MinIO API
    return Err(ApiError(
        HttpStatus::NotImplemented,
        "Object metadata update not yet implemented"
    ));
}

Result<void, ApiError> ObjectService::set_object_tags(
    const UserInfo& user_info,
    const String& bucket_name,
    const String& object_key,
    const StringMap& tags
) {
    LOG_INFO("Setting tags for object: {}/{}", bucket_name, object_key);

    // TODO(Nice0Man): Implement object tagging
    return Err(ApiError(
        HttpStatus::NotImplemented,
        "Object tagging not yet implemented"
    ));
}

Result<StringMap, ApiError> ObjectService::get_object_tags(
    const UserInfo& user_info,
    const String& bucket_name,
    const String& object_key
) {
    LOG_DEBUG("Getting tags for object: {}/{}", bucket_name, object_key);

    // TODO(Nice0Man): Implement object tagging
    return Err(ApiError(
        HttpStatus::NotImplemented,
        "Object tagging not yet implemented"
    ));
}

Result<String, ApiError> ObjectService::generate_presigned_url(
    const UserInfo& user_info,
    const String& bucket_name,
    const String& object_key,
    int expiry_seconds
) {
    LOG_INFO("Generating presigned URL for: {}/{} (expiry: {}s)", 
             bucket_name, object_key, expiry_seconds);

    if (auto error = validate_object_key(object_key)) {
        return Err(*error);
    }

    if (expiry_seconds < 1 || expiry_seconds > 604800) {  // Max 7 days
        return Err(ApiError(
            HttpStatus::BadRequest,
            "Expiry must be between 1 second and 7 days"
        ));
    }

    // TODO(Nice0Man): Implement presigned URL generation
    return Err(ApiError(
        HttpStatus::NotImplemented,
        "Presigned URL generation not yet implemented"
    ));
}

// Private methods

Optional<ApiError> ObjectService::validate_object_key(
    const String& object_key
) {
    if (object_key.empty()) {
        return ApiError(HttpStatus::BadRequest, "Object key is required");
    }

    if (object_key.length() > 1024) {
        return ApiError(
            HttpStatus::BadRequest,
            "Object key exceeds maximum length of 1024 characters"
        );
    }

    // Check for dangerous patterns
    if (object_key.find("..") != String::npos) {
        return ApiError(
            HttpStatus::BadRequest,
            "Object key cannot contain '..' (directory traversal)"
        );
    }

    if (object_key[0] == '/') {
        return ApiError(
            HttpStatus::BadRequest,
            "Object key cannot start with '/'"
        );
    }

    return {};  // No error
}

String ObjectService::sanitize_object_key(const String& object_key) {
    String sanitized = object_key;
    
    // Remove leading/trailing whitespace
    size_t start = sanitized.find_first_not_of(" \t\n\r");
    size_t end = sanitized.find_last_not_of(" \t\n\r");
    
    if (start != String::npos && end != String::npos) {
        sanitized = sanitized.substr(start, end - start + 1);
    }
    
    // Normalize multiple slashes to single slash
    size_t pos = 0;
    while ((pos = sanitized.find("//", pos)) != String::npos) {
        sanitized.replace(pos, 2, "/");
    }
    
    return sanitized;
}

} // namespace console::services

