// Copyright 2025 OpenMaxIO Contributors
// Licensed under AGPL-3.0

#pragma once

#include "console/common/Types.hpp"
#include "console/models/Error.hpp"
#include "console/models/Object.hpp"

#include <json/json.h>

namespace console::services {

/**
 * @brief Object management service interface
 *
 * Provides business logic for object operations including CRUD,
 * upload/download, metadata, tags, retention, and more.
 */
class IObjectService {
  public:
    virtual ~IObjectService() = default;

    /**
     * @brief List objects in a bucket
     *
     * @param user_info User context with credentials
     * @param bucket_name Bucket name
     * @param prefix Filter by prefix (path)
     * @param recursive List recursively
     * @param max_keys Maximum number of objects to return
     * @return Result containing list of objects or error
     */
    virtual Result<Vector<models::Object>, models::ApiError> list_objects(const UserInfo& user_info,
                                                                          const String& bucket_name,
                                                                          const String& prefix = "",
                                                                          bool recursive = false,
                                                                          int max_keys = 1000) = 0;

    /**
     * @brief Get object information (metadata only)
     *
     * @param user_info User context with credentials
     * @param bucket_name Bucket name
     * @param object_key Object key (path)
     * @return Result containing object info or error
     */
    virtual Result<models::Object, models::ApiError> get_object_info(const UserInfo& user_info,
                                                                     const String& bucket_name,
                                                                     const String& object_key) = 0;

    /**
     * @brief Download object data
     *
     * @param user_info User context with credentials
     * @param bucket_name Bucket name
     * @param object_key Object key (path)
     * @return Result containing object data or error
     */
    virtual Result<ByteArray, models::ApiError> download_object(const UserInfo& user_info,
                                                                const String& bucket_name,
                                                                const String& object_key) = 0;

    /**
     * @brief Upload object data
     *
     * @param user_info User context with credentials
     * @param bucket_name Bucket name
     * @param object_key Object key (path)
     * @param data Object data
     * @param content_type MIME type
     * @param metadata Additional metadata
     * @return Result containing uploaded object info or error
     */
    virtual Result<models::Object, models::ApiError> upload_object(
        const UserInfo& user_info,
        const String& bucket_name,
        const String& object_key,
        const ByteArray& data,
        const String& content_type = "application/octet-stream",
        const StringMap& metadata = {}) = 0;

    /**
     * @brief Delete an object
     *
     * @param user_info User context with credentials
     * @param bucket_name Bucket name
     * @param object_key Object key (path)
     * @return Result indicating success or error
     */
    virtual Result<void, models::ApiError> delete_object(const UserInfo& user_info,
                                                         const String& bucket_name,
                                                         const String& object_key) = 0;

    /**
     * @brief Delete multiple objects (batch operation)
     *
     * @param user_info User context with credentials
     * @param bucket_name Bucket name
     * @param object_keys List of object keys to delete
     * @return Result containing deletion results or error
     */
    virtual Result<Json::Value, models::ApiError> delete_objects(const UserInfo& user_info,
                                                                 const String& bucket_name,
                                                                 const Vector<String>& object_keys) = 0;

    /**
     * @brief Copy an object
     *
     * @param user_info User context with credentials
     * @param source_bucket Source bucket name
     * @param source_key Source object key
     * @param dest_bucket Destination bucket name
     * @param dest_key Destination object key
     * @return Result containing copied object info or error
     */
    virtual Result<models::Object, models::ApiError> copy_object(const UserInfo& user_info,
                                                                 const String& source_bucket,
                                                                 const String& source_key,
                                                                 const String& dest_bucket,
                                                                 const String& dest_key) = 0;

    /**
     * @brief Set object metadata
     *
     * @param user_info User context with credentials
     * @param bucket_name Bucket name
     * @param object_key Object key
     * @param metadata Metadata to set
     * @return Result indicating success or error
     */
    virtual Result<void, models::ApiError> set_object_metadata(const UserInfo& user_info,
                                                               const String& bucket_name,
                                                               const String& object_key,
                                                               const StringMap& metadata) = 0;

    /**
     * @brief Set object tags
     *
     * @param user_info User context with credentials
     * @param bucket_name Bucket name
     * @param object_key Object key
     * @param tags Tags to set
     * @return Result indicating success or error
     */
    virtual Result<void, models::ApiError> set_object_tags(const UserInfo& user_info,
                                                           const String& bucket_name,
                                                           const String& object_key,
                                                           const StringMap& tags) = 0;

    /**
     * @brief Get object tags
     *
     * @param user_info User context with credentials
     * @param bucket_name Bucket name
     * @param object_key Object key
     * @return Result containing tags or error
     */
    virtual Result<StringMap, models::ApiError> get_object_tags(const UserInfo& user_info,
                                                                const String& bucket_name,
                                                                const String& object_key) = 0;

    /**
     * @brief Delete object tags
     *
     * @param user_info User context with credentials
     * @param bucket_name Bucket name
     * @param object_key Object key
     * @return Result indicating success or error
     */
    virtual Result<void, models::ApiError> delete_object_tags(const UserInfo& user_info,
                                                              const String& bucket_name,
                                                              const String& object_key) = 0;

    /**
     * @brief Generate presigned URL for object download/upload
     *
     * @param user_info User context with credentials
     * @param bucket_name Bucket name
     * @param object_key Object key
     * @param method HTTP method (GET, PUT, etc.)
     * @param expiry_seconds URL expiration time in seconds
     * @return Result containing presigned URL or error
     */
    virtual Result<String, models::ApiError> generate_presigned_url(const UserInfo& user_info,
                                                                    const String& bucket_name,
                                                                    const String& object_key,
                                                                    const String& method = "GET",
                                                                    int expiry_seconds = 3600) = 0;
};

} // namespace console::services
