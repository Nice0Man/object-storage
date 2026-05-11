// Copyright 2025 OpenMaxIO Contributors
// Licensed under AGPL-3.0

#pragma once

#include "console/common/Types.hpp"
#include "console/models/Bucket.hpp"
#include "console/models/Error.hpp"

#include <json/json.h>

namespace console::services {

/**
 * @brief Bucket management service interface
 *
 * Provides business logic for bucket operations including CRUD,
 * policy management, versioning, encryption, and more.
 */
class IBucketService {
  public:
    virtual ~IBucketService() = default;

    /**
     * @brief List all buckets accessible to the user
     *
     * @param user_info User context with credentials
     * @return Result containing list of buckets or error
     */
    virtual Result<Vector<models::Bucket>, models::ApiError> list_buckets(const UserInfo& user_info) = 0;

    /**
     * @brief Create a new bucket
     *
     * @param user_info User context with credentials
     * @param name Bucket name (must be DNS-compatible)
     * @param region AWS region
     * @param object_locking Enable object locking
     * @return Result containing created bucket info or error
     */
    virtual Result<models::Bucket, models::ApiError> create_bucket(const UserInfo& user_info,
                                                                   const String& name,
                                                                   const String& region,
                                                                   bool object_locking) = 0;

    /**
     * @brief Delete a bucket
     *
     * @param user_info User context with credentials
     * @param name Bucket name
     * @return Result indicating success or error
     */
    virtual Result<void, models::ApiError> delete_bucket(const UserInfo& user_info, const String& name) = 0;

    /**
     * @brief Get detailed bucket information
     *
     * @param user_info User context with credentials
     * @param name Bucket name
     * @return Result containing bucket info or error
     */
    virtual Result<models::Bucket, models::ApiError> get_bucket_info(const UserInfo& user_info, const String& name) = 0;

    /**
     * @brief Set bucket policy (IAM)
     *
     * @param user_info User context with credentials
     * @param name Bucket name
     * @param policy_json Policy document in JSON format
     * @return Result indicating success or error
     */
    virtual Result<void, models::ApiError> set_bucket_policy(const UserInfo& user_info,
                                                             const String& name,
                                                             const String& policy_json) = 0;

    /**
     * @brief Get bucket policy
     *
     * @param user_info User context with credentials
     * @param name Bucket name
     * @return Result containing policy JSON or error
     */
    virtual Result<String, models::ApiError> get_bucket_policy(const UserInfo& user_info, const String& name) = 0;

    /**
     * @brief Enable/disable bucket versioning
     *
     * @param user_info User context with credentials
     * @param name Bucket name
     * @param enabled Enable or disable versioning
     * @return Result indicating success or error
     */
    virtual Result<void, models::ApiError> set_bucket_versioning(const UserInfo& user_info,
                                                                 const String& name,
                                                                 bool enabled) = 0;

    /**
     * @brief Get bucket versioning status
     *
     * @param user_info User context with credentials
     * @param name Bucket name
     * @return Result containing versioning status or error
     */
    virtual Result<bool, models::ApiError> get_bucket_versioning(const UserInfo& user_info, const String& name) = 0;

    /**
     * @brief Set bucket tags
     *
     * @param user_info User context with credentials
     * @param name Bucket name
     * @param tags Key-value pairs of tags
     * @return Result indicating success or error
     */
    virtual Result<void, models::ApiError> set_bucket_tags(const UserInfo& user_info,
                                                           const String& name,
                                                           const StringMap& tags) = 0;

    /**
     * @brief Get bucket tags
     *
     * @param user_info User context with credentials
     * @param name Bucket name
     * @return Result containing tags or error
     */
    virtual Result<StringMap, models::ApiError> get_bucket_tags(const UserInfo& user_info, const String& name) = 0;

    /**
     * @brief Delete bucket tags
     *
     * @param user_info User context with credentials
     * @param name Bucket name
     * @return Result indicating success or error
     */
    virtual Result<void, models::ApiError> delete_bucket_tags(const UserInfo& user_info, const String& name) = 0;

    /**
     * @brief Check if bucket exists
     */
    virtual Result<bool, models::ApiError> bucket_exists(const UserInfo& user_info, const String& name) = 0;

    /**
     * @brief Get bucket encryption configuration
     */
    virtual Result<Json::Value, models::ApiError> get_bucket_encryption(const UserInfo& user_info,
                                                                        const String& name) = 0;

    /**
     * @brief Set bucket encryption configuration
     */
    virtual Result<void, models::ApiError> set_bucket_encryption(const UserInfo& user_info,
                                                                 const String& name,
                                                                 const Json::Value& config) = 0;

    /**
     * @brief Get bucket lifecycle rules
     */
    virtual Result<Json::Value, models::ApiError> get_bucket_lifecycle(const UserInfo& user_info,
                                                                       const String& name) = 0;

    /**
     * @brief Set bucket lifecycle rules
     */
    virtual Result<void, models::ApiError> set_bucket_lifecycle(const UserInfo& user_info,
                                                                const String& name,
                                                                const Json::Value& rules) = 0;

    /**
     * @brief Get bucket object lock configuration
     */
    virtual Result<Json::Value, models::ApiError> get_bucket_object_lock(const UserInfo& user_info,
                                                                         const String& name) = 0;

    /**
     * @brief Set bucket object lock configuration
     */
    virtual Result<void, models::ApiError> set_bucket_object_lock(const UserInfo& user_info,
                                                                  const String& name,
                                                                  const Json::Value& config) = 0;
};

} // namespace console::services
