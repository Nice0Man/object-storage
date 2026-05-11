#include "console/services/ObjectService.hpp"

#include "console/clients/LocalStorageClient.hpp"
#include "console/common/Logger.hpp"
#include "console/common/ServiceLocator.hpp"
#include "console/services/PolicyEvaluator.hpp"

#include <unordered_map>

namespace console::services {

using namespace console::models;

ObjectService::ObjectService(std::shared_ptr<clients::IStorageClient> storage_client)
    : storage_client_(storage_client), max_single_upload_size_(5ULL * 1024 * 1024 * 1024), // 5GB
      multipart_threshold_(100ULL * 1024 * 1024) {                                         // 100MB
    CONSOLE_LOG_INFO("ObjectService initialized");
}

Result<Vector<Object>, ApiError>
ObjectService::list_objects(const UserInfo& user_info,
                            const String& bucket_name,
                            const String& prefix,
                            bool recursive,
                            int max_keys) {
    CONSOLE_LOG_DEBUG("Listing objects in bucket: {} with prefix: {}", bucket_name, prefix);

    clients::ListObjectsOptions options;
    options.prefix = prefix;
    options.recursive = recursive;
    options.max_keys = max_keys;

    auto result = storage_client_->list_objects(bucket_name, options);

    if (!result) {
        CONSOLE_LOG_ERROR("Failed to list objects in bucket {}: {}", bucket_name, result.error());
        return Err<Vector<Object>>(
            ApiError(HttpStatus::InternalServerError, "Failed to list objects: " + result.error()));
    }

    // Get objects from response
    auto objects = result.value().objects;

    CONSOLE_LOG_INFO("Listed {} objects from bucket: {}", objects.size(), bucket_name);
    return Result<Vector<Object>, ApiError>(ok_tag, objects);
}

Result<Object, ApiError>
ObjectService::get_object_info(const UserInfo& user_info, const String& bucket_name, const String& object_key) {
    CONSOLE_LOG_DEBUG("Getting object info: {} from bucket: {}", object_key, bucket_name);

    if (auto error = validate_object_key(object_key)) {
        return Err<Object>(*error);
    }

    auto result = storage_client_->stat_object(bucket_name, object_key);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to get object info for {}/{}: {}", bucket_name, object_key, result.error());
        return Err<Object>(ApiError(HttpStatus::NotFound, "Object not found: " + result.error()));
    }

    return Result<Object, ApiError>(ok_tag, result.value());
}

Result<ByteArray, ApiError>
ObjectService::download_object(const UserInfo& user_info, const String& bucket_name, const String& object_key) {
    CONSOLE_LOG_INFO("Downloading object: {} from bucket: {}", object_key, bucket_name);

    if (auto error = validate_object_key(object_key)) {
        return Err<ByteArray>(*error);
    }

    auto result = storage_client_->get_object(bucket_name, object_key);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to download object {}/{}: {}", bucket_name, object_key, result.error());
        return Err<ByteArray>(
            ApiError(HttpStatus::InternalServerError, "Failed to download object: " + result.error()));
    }

    CONSOLE_LOG_INFO("Successfully downloaded object: {} ({} bytes)", object_key, result.value().size());
    return Result<ByteArray, ApiError>(ok_tag, result.value());
}

Result<Object, ApiError>
ObjectService::upload_object(const UserInfo& user_info,
                             const String& bucket_name,
                             const String& object_key,
                             const ByteArray& data,
                             const String& content_type,
                             const StringMap& metadata) {
    CONSOLE_LOG_INFO("Uploading object: {} to bucket: {} ({} bytes)", object_key, bucket_name, data.size());

    if (auto error = validate_object_key(object_key)) {
        return Err<Object>(*error);
    }

    if (data.size() > max_single_upload_size_) {
        return Err<Object>(ApiError(HttpStatus::BadRequest, "Object size exceeds maximum single upload size"));
    }

    auto result = storage_client_->put_object(bucket_name, object_key, data, content_type, metadata);

    if (!result) {
        CONSOLE_LOG_ERROR("Failed to upload object {}/{}: {}", bucket_name, object_key, result.error());
        return Err<Object>(ApiError(HttpStatus::InternalServerError, "Failed to upload object: " + result.error()));
    }

    CONSOLE_LOG_INFO("Successfully uploaded object: {}", object_key);

    // Get object info to return
    auto info_result = storage_client_->stat_object(bucket_name, object_key);
    if (!info_result) {
        return Err<Object>(ApiError(HttpStatus::InternalServerError, "Object uploaded but failed to retrieve info"));
    }

    return Result<Object, ApiError>(ok_tag, info_result.value());
}

// ============================================================================
// Server-Side Encryption (SSE) Operations
// ============================================================================

Result<Object, ApiError>
ObjectService::upload_object_sse(const UserInfo& user_info,
                                 const String& bucket_name,
                                 const String& object_key,
                                 const ByteArray& data,
                                 const String& content_type,
                                 const StringMap& metadata,
                                 const String& sse_customer_key) {
    CONSOLE_LOG_INFO("Uploading object with SSE: {} to bucket: {} ({} bytes)", object_key, bucket_name, data.size());

    if (auto error = validate_object_key(object_key)) {
        return Err<Object>(*error);
    }

    if (data.size() > max_single_upload_size_) {
        return Err<Object>(ApiError(HttpStatus::BadRequest, "Object size exceeds maximum single upload size"));
    }

    // Get LocalStorageClient for SSE support
    auto local_storage = ServiceLocator::storage_client();
    if (!local_storage) {
        return Err<Object>(ApiError(HttpStatus::InternalServerError, "Storage client not available"));
    }

    auto result = local_storage->put_object(bucket_name, object_key, data, content_type, metadata, sse_customer_key);

    if (!result) {
        CONSOLE_LOG_ERROR("Failed to upload SSE object {}/{}: {}", bucket_name, object_key, result.error());
        return Err<Object>(ApiError(HttpStatus::InternalServerError, "Failed to upload object: " + result.error()));
    }

    CONSOLE_LOG_INFO("Successfully uploaded SSE object: {}", object_key);

    // Get object info to return
    auto info_result = storage_client_->stat_object(bucket_name, object_key);
    if (!info_result) {
        return Err<Object>(ApiError(HttpStatus::InternalServerError, "Object uploaded but failed to retrieve info"));
    }

    return Result<Object, ApiError>(ok_tag, info_result.value());
}

Result<ByteArray, ApiError>
ObjectService::download_object_sse(const UserInfo& user_info,
                                   const String& bucket_name,
                                   const String& object_key,
                                   const String& sse_customer_key) {
    CONSOLE_LOG_INFO("Downloading SSE object: {} from bucket: {}", object_key, bucket_name);

    if (auto error = validate_object_key(object_key)) {
        return Err<ByteArray>(*error);
    }

    // Get LocalStorageClient for SSE support
    auto local_storage = ServiceLocator::storage_client();
    if (!local_storage) {
        return Err<ByteArray>(ApiError(HttpStatus::InternalServerError, "Storage client not available"));
    }

    auto result = local_storage->get_object(bucket_name, object_key, sse_customer_key);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to download SSE object {}/{}: {}", bucket_name, object_key, result.error());
        return Err<ByteArray>(
            ApiError(HttpStatus::InternalServerError, "Failed to download object: " + result.error()));
    }

    CONSOLE_LOG_INFO("Successfully downloaded SSE object: {} ({} bytes)", object_key, result.value().size());
    return Result<ByteArray, ApiError>(ok_tag, result.value());
}

Result<void, ApiError>
ObjectService::delete_object(const UserInfo& user_info, const String& bucket_name, const String& object_key) {
    CONSOLE_LOG_INFO("Deleting object: {} from bucket: {}", object_key, bucket_name);

    if (auto error = validate_object_key(object_key)) {
        return Result<void, ApiError>(err_tag, *error);
    }

    auto result = storage_client_->delete_object(bucket_name, object_key);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to delete object {}/{}: {}", bucket_name, object_key, result.error());
        return Result<void, ApiError>(
            err_tag, ApiError(HttpStatus::InternalServerError, "Failed to delete object: " + result.error()));
    }

    CONSOLE_LOG_INFO("Successfully deleted object: {}", object_key);
    return Ok<ApiError>();
}

Result<Object, ApiError>
ObjectService::copy_object(const UserInfo& user_info,
                           const String& source_bucket,
                           const String& source_key,
                           const String& dest_bucket,
                           const String& dest_key) {
    CONSOLE_LOG_INFO("Copying object from {}/{} to {}/{}", source_bucket, source_key, dest_bucket, dest_key);

    if (auto error = validate_object_key(source_key)) {
        return Err<Object>(*error);
    }

    if (auto error = validate_object_key(dest_key)) {
        return Err<Object>(*error);
    }

    auto result = storage_client_->copy_object(source_bucket, source_key, dest_bucket, dest_key);

    if (!result) {
        CONSOLE_LOG_ERROR("Failed to copy object: {}", result.error());
        return Err<Object>(ApiError(HttpStatus::InternalServerError, "Failed to copy object: " + result.error()));
    }

    CONSOLE_LOG_INFO("Successfully copied object");

    // Get dest object info
    auto info_result = storage_client_->stat_object(dest_bucket, dest_key);
    if (!info_result) {
        return Err<Object>(ApiError(HttpStatus::InternalServerError, "Object copied but failed to retrieve info"));
    }

    return Result<Object, ApiError>(ok_tag, info_result.value());
}

Result<StringMap, ApiError>
ObjectService::get_object_tags(const UserInfo& user_info, const String& bucket_name, const String& object_key) {
    CONSOLE_LOG_DEBUG("Getting tags for object: {}/{}", bucket_name, object_key);

    if (auto error = validate_object_key(object_key)) {
        return Err<StringMap>(*error);
    }

    auto result = storage_client_->get_object_tags(bucket_name, object_key);
    if (!result) {
        return Err<StringMap>(
            ApiError(HttpStatus::InternalServerError, "Failed to get object tags: " + result.error()));
    }

    return Result<StringMap, ApiError>(ok_tag, result.value());
}

Result<void, ApiError>
ObjectService::set_object_tags(const UserInfo& user_info,
                               const String& bucket_name,
                               const String& object_key,
                               const StringMap& tags) {
    CONSOLE_LOG_INFO("Setting {} tags for object: {}/{}", tags.size(), bucket_name, object_key);

    if (auto error = validate_object_key(object_key)) {
        return Result<void, ApiError>(err_tag, *error);
    }

    auto result = storage_client_->set_object_tags(bucket_name, object_key, tags);
    if (!result) {
        return Result<void, ApiError>(
            err_tag, ApiError(HttpStatus::InternalServerError, "Failed to set object tags: " + result.error()));
    }

    return Ok<ApiError>();
}

Result<void, ApiError>
ObjectService::delete_object_tags(const UserInfo& user_info, const String& bucket_name, const String& object_key) {
    CONSOLE_LOG_INFO("Deleting tags for object: {}/{}", bucket_name, object_key);

    if (auto error = validate_object_key(object_key)) {
        return Result<void, ApiError>(err_tag, *error);
    }

    // Delete tags by setting empty tag map
    StringMap empty_tags;
    auto result = storage_client_->set_object_tags(bucket_name, object_key, empty_tags);
    if (!result) {
        return Err<void, ApiError>(
            ApiError(HttpStatus::InternalServerError, "Failed to delete object tags: " + result.error()));
    }

    return Ok<ApiError>();
}

Result<String, ApiError>
ObjectService::generate_presigned_url(const UserInfo& user_info,
                                      const String& bucket_name,
                                      const String& object_key,
                                      const String& method,
                                      int expiry_seconds) {
    CONSOLE_LOG_INFO("Generating presigned URL for: {}/{} (expires in {}s)", bucket_name, object_key, expiry_seconds);

    if (auto error = validate_object_key(object_key)) {
        return Err<String>(*error);
    }

    if (expiry_seconds <= 0 || expiry_seconds > 7 * 24 * 3600) {
        return Err<String>(ApiError(HttpStatus::BadRequest, "Expiry must be between 1 second and 7 days"));
    }

    auto result = storage_client_->generate_presigned_url(bucket_name, object_key, expiry_seconds, method);

    if (!result) {
        return Err<String>(
            ApiError(HttpStatus::InternalServerError, "Failed to generate presigned URL: " + result.error()));
    }

    return Result<String, ApiError>(ok_tag, result.value());
}

Optional<ApiError>
ObjectService::validate_object_key(const String& key) {
    if (key.empty()) {
        return ApiError(HttpStatus::BadRequest, "Object key cannot be empty");
    }

    if (key.length() > 1024) {
        return ApiError(HttpStatus::BadRequest, "Object key cannot exceed 1024 characters");
    }

    // Check for invalid characters
    if (key.find('\0') != String::npos) {
        return ApiError(HttpStatus::BadRequest, "Object key contains invalid characters");
    }

    return std::nullopt;
}

Optional<ApiError>
ObjectService::validate_access(const UserInfo& user,
                               const String& bucket_name,
                               const String& object_key,
                               const String& action) {
    // Use PolicyEvaluator for proper IAM-style policy evaluation
    PolicyEvaluator evaluator;

    // Map internal action names to S3 actions
    String s3_action = action;
    static const std::unordered_map<String, String> action_map = {
        {"GetObject", S3Actions::GetObject},
        {"PutObject", S3Actions::PutObject},
        {"DeleteObject", S3Actions::DeleteObject},
        {"ListObjects", S3Actions::ListBucket},
        {"GetObjectMetadata", S3Actions::GetObject},
        {"PutObjectMetadata", S3Actions::PutObject},
        {"GetObjectTagging", S3Actions::GetObjectTagging},
        {"PutObjectTagging", S3Actions::PutObjectTagging},
        {"DeleteObjectTagging", S3Actions::DeleteObjectTagging},
        {"CopyObject", S3Actions::PutObject},
        {"AbortMultipartUpload", S3Actions::AbortMultipartUpload},
        {"ListMultipartUploadParts", S3Actions::ListMultipartUploadParts},
        {"ListBucketMultipartUploads", S3Actions::ListBucketMultipartUploads},
    };

    auto it = action_map.find(action);
    if (it != action_map.end()) {
        s3_action = it->second;
    }

    auto result = evaluator.evaluate_user_access(user, s3_action, bucket_name, object_key);

    if (!result.is_allowed()) {
        CONSOLE_LOG_WARN(
            "Access denied for user {} on {}/{}: {}", user.access_key, bucket_name, object_key, result.reason);
        return ApiError(HttpStatus::Forbidden, result.reason);
    }

    return std::nullopt;
}

Result<Json::Value, ApiError>
ObjectService::delete_objects(const UserInfo& user_info, const String& bucket_name, const Vector<String>& object_keys) {
    CONSOLE_LOG_DEBUG("Batch deleting {} objects from bucket: {}", object_keys.size(), bucket_name);

    Json::Value response;
    Json::Value deleted(Json::arrayValue);
    Json::Value errors(Json::arrayValue);

    for (const auto& key : object_keys) {
        // Validate access for each object
        auto access_error = validate_access(user_info, bucket_name, key, "DeleteObject");
        if (access_error) {
            Json::Value error_item;
            error_item["key"] = key;
            error_item["error"] = access_error->message();
            errors.append(error_item);
            continue;
        }

        // Delete object
        auto result = storage_client_->delete_object(bucket_name, key);
        if (!result) {
            Json::Value error_item;
            error_item["key"] = key;
            error_item["error"] = result.error();
            errors.append(error_item);
            CONSOLE_LOG_ERROR("Failed to delete object {}: {}", key, result.error());
        } else {
            Json::Value deleted_item;
            deleted_item["key"] = key;
            deleted.append(deleted_item);
            CONSOLE_LOG_INFO("Deleted object: {}/{}", bucket_name, key);
        }
    }

    response["deleted"] = deleted;
    response["errors"] = errors;

    return Result<Json::Value, ApiError>(ok_tag, response);
}

Result<void, ApiError>
ObjectService::set_object_metadata(const UserInfo& user_info,
                                   const String& bucket_name,
                                   const String& object_key,
                                   const StringMap& metadata) {
    CONSOLE_LOG_DEBUG("Setting metadata for object: {}/{}", bucket_name, object_key);

    // Validate access
    auto access_error = validate_access(user_info, bucket_name, object_key, "PutObjectMetadata");
    if (access_error) {
        return Err<void>(*access_error);
    }

    // Set metadata via client
    auto result = storage_client_->set_object_metadata(bucket_name, object_key, metadata);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to set metadata for object {}/{}: {}", bucket_name, object_key, result.error());
        return Err<void>(ApiError(HttpStatus::InternalServerError, "Failed to set object metadata: " + result.error()));
    }

    CONSOLE_LOG_INFO("Set metadata for object: {}/{}", bucket_name, object_key);
    return Result<void, ApiError>(ok_tag);
}

// ============================================================================
// Multipart Upload Operations
// ============================================================================

Result<clients::MultipartUploadInfo, ApiError>
ObjectService::initiate_multipart_upload(const UserInfo& user_info,
                                         const String& bucket_name,
                                         const String& object_key,
                                         const String& content_type,
                                         const StringMap& metadata) {
    CONSOLE_LOG_INFO("Initiating multipart upload: {}/{}", bucket_name, object_key);

    if (auto error = validate_object_key(object_key)) {
        return Err<clients::MultipartUploadInfo>(*error);
    }

    auto access_error = validate_access(user_info, bucket_name, object_key, "PutObject");
    if (access_error) {
        return Err<clients::MultipartUploadInfo>(*access_error);
    }

    auto result = storage_client_->initiate_multipart_upload(bucket_name, object_key, content_type, metadata);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to initiate multipart upload for {}/{}: {}", bucket_name, object_key, result.error());
        return Err<clients::MultipartUploadInfo>(
            ApiError(HttpStatus::InternalServerError, "Failed to initiate multipart upload: " + result.error()));
    }

    CONSOLE_LOG_INFO("Multipart upload initiated: {} for {}/{}", result.value().upload_id, bucket_name, object_key);
    return Result<clients::MultipartUploadInfo, ApiError>(ok_tag, result.value());
}

Result<String, ApiError>
ObjectService::upload_part(const UserInfo& user_info,
                           const String& bucket_name,
                           const String& object_key,
                           const String& upload_id,
                           int part_number,
                           const ByteArray& data) {
    CONSOLE_LOG_DEBUG("Uploading part {} for {}/{} ({} bytes)", part_number, bucket_name, object_key, data.size());

    // Validate part number
    if (part_number < 1 || part_number > 10000) {
        return Err<String>(ApiError(HttpStatus::BadRequest, "Part number must be between 1 and 10000"));
    }

    // Validate part size (minimum 5MB except for last part)
    // Note: We don't enforce minimum here as we don't know if it's the last part
    // The client is responsible for proper part sizing

    auto access_error = validate_access(user_info, bucket_name, object_key, "PutObject");
    if (access_error) {
        return Err<String>(*access_error);
    }

    auto result = storage_client_->upload_part(bucket_name, object_key, upload_id, part_number, data);
    if (!result) {
        CONSOLE_LOG_ERROR(
            "Failed to upload part {} for {}/{}: {}", part_number, bucket_name, object_key, result.error());
        return Err<String>(ApiError(HttpStatus::InternalServerError, "Failed to upload part: " + result.error()));
    }

    CONSOLE_LOG_DEBUG("Part {} uploaded for {}/{}, ETag: {}", part_number, bucket_name, object_key, result.value());
    return Result<String, ApiError>(ok_tag, result.value());
}

Result<models::Object, ApiError>
ObjectService::complete_multipart_upload(const UserInfo& user_info,
                                         const String& bucket_name,
                                         const String& object_key,
                                         const String& upload_id,
                                         const Vector<clients::CompletedPart>& parts) {
    CONSOLE_LOG_INFO(
        "Completing multipart upload {} for {}/{} ({} parts)", upload_id, bucket_name, object_key, parts.size());

    if (parts.empty()) {
        return Err<models::Object>(ApiError(HttpStatus::BadRequest, "At least one part is required"));
    }

    auto access_error = validate_access(user_info, bucket_name, object_key, "PutObject");
    if (access_error) {
        return Err<models::Object>(*access_error);
    }

    auto result = storage_client_->complete_multipart_upload(bucket_name, object_key, upload_id, parts);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to complete multipart upload for {}/{}: {}", bucket_name, object_key, result.error());
        return Err<models::Object>(
            ApiError(HttpStatus::InternalServerError, "Failed to complete multipart upload: " + result.error()));
    }

    CONSOLE_LOG_INFO("Multipart upload completed: {}/{}", bucket_name, object_key);
    return Result<models::Object, ApiError>(ok_tag, result.value());
}

Result<void, ApiError>
ObjectService::abort_multipart_upload(const UserInfo& user_info,
                                      const String& bucket_name,
                                      const String& object_key,
                                      const String& upload_id) {
    CONSOLE_LOG_INFO("Aborting multipart upload {} for {}/{}", upload_id, bucket_name, object_key);

    auto access_error = validate_access(user_info, bucket_name, object_key, "AbortMultipartUpload");
    if (access_error) {
        return Result<void, ApiError>(err_tag, *access_error);
    }

    auto result = storage_client_->abort_multipart_upload(bucket_name, object_key, upload_id);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to abort multipart upload for {}/{}: {}", bucket_name, object_key, result.error());
        return Result<void, ApiError>(
            err_tag, ApiError(HttpStatus::InternalServerError, "Failed to abort multipart upload: " + result.error()));
    }

    CONSOLE_LOG_INFO("Multipart upload aborted: {} for {}/{}", upload_id, bucket_name, object_key);
    return Ok<ApiError>();
}

Result<clients::MultipartUploadInfo, ApiError>
ObjectService::list_parts(const UserInfo& user_info,
                          const String& bucket_name,
                          const String& object_key,
                          const String& upload_id) {
    CONSOLE_LOG_DEBUG("Listing parts for upload {} ({}/{})", upload_id, bucket_name, object_key);

    auto access_error = validate_access(user_info, bucket_name, object_key, "ListMultipartUploadParts");
    if (access_error) {
        return Err<clients::MultipartUploadInfo>(*access_error);
    }

    auto result = storage_client_->list_parts(bucket_name, object_key, upload_id);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to list parts for {}/{}: {}", bucket_name, object_key, result.error());
        return Err<clients::MultipartUploadInfo>(
            ApiError(HttpStatus::InternalServerError, "Failed to list parts: " + result.error()));
    }

    return Result<clients::MultipartUploadInfo, ApiError>(ok_tag, result.value());
}

Result<Vector<clients::MultipartUploadInfo>, ApiError>
ObjectService::list_multipart_uploads(const UserInfo& user_info, const String& bucket_name, const String& prefix) {
    CONSOLE_LOG_DEBUG("Listing multipart uploads for bucket {} with prefix '{}'", bucket_name, prefix);

    auto access_error = validate_access(user_info, bucket_name, "", "ListBucketMultipartUploads");
    if (access_error) {
        return Err<Vector<clients::MultipartUploadInfo>>(*access_error);
    }

    auto result = storage_client_->list_multipart_uploads(bucket_name, prefix);
    if (!result) {
        CONSOLE_LOG_ERROR("Failed to list multipart uploads for {}: {}", bucket_name, result.error());
        return Err<Vector<clients::MultipartUploadInfo>>(
            ApiError(HttpStatus::InternalServerError, "Failed to list multipart uploads: " + result.error()));
    }

    CONSOLE_LOG_DEBUG("Found {} multipart uploads in bucket {}", result.value().size(), bucket_name);
    return Result<Vector<clients::MultipartUploadInfo>, ApiError>(ok_tag, result.value());
}

} // namespace console::services
