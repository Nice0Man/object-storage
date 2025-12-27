// Copyright 2025 OpenMaxIO Contributors
// Licensed under AGPL-3.0

#pragma once

#include "console/clients/LocalStorageClient.hpp"
#include "console/clients/StorageClient.hpp"
#include "console/services/IBucketService.hpp"

#include <memory>

namespace console::services {

/**
 * @brief Bucket management service implementation
 *
 * Provides business logic for bucket operations with validation,
 * caching, and error handling.
 */
class BucketService : public IBucketService {
  public:
    /**
     * @brief Construct BucketService with storage client
     *
     * @param storage_client Storage client implementing IStorageClient interface
     */
    explicit BucketService(std::shared_ptr<clients::IStorageClient> storage_client);

    ~BucketService() override = default;

    // IBucketService implementation
    Result<Vector<models::Bucket>, models::ApiError> list_buckets(const UserInfo& user_info) override;

    Result<models::Bucket, models::ApiError> create_bucket(const UserInfo& user_info,
                                                           const String& name,
                                                           const String& region,
                                                           bool object_locking) override;

    Result<void, models::ApiError> delete_bucket(const UserInfo& user_info, const String& name) override;

    Result<models::Bucket, models::ApiError> get_bucket_info(const UserInfo& user_info, const String& name) override;

    Result<void, models::ApiError> set_bucket_policy(const UserInfo& user_info,
                                                     const String& name,
                                                     const String& policy_json) override;

    Result<String, models::ApiError> get_bucket_policy(const UserInfo& user_info, const String& name) override;

    Result<void, models::ApiError> set_bucket_versioning(const UserInfo& user_info,
                                                         const String& name,
                                                         bool enabled) override;

    Result<bool, models::ApiError> get_bucket_versioning(const UserInfo& user_info, const String& name) override;

    Result<void, models::ApiError> set_bucket_tags(const UserInfo& user_info,
                                                   const String& name,
                                                   const StringMap& tags) override;

    Result<StringMap, models::ApiError> get_bucket_tags(const UserInfo& user_info, const String& name) override;

    Result<void, models::ApiError> delete_bucket_tags(const UserInfo& user_info, const String& name) override;

    Result<bool, models::ApiError> bucket_exists(const UserInfo& user_info, const String& name) override;

    Result<Json::Value, models::ApiError> get_bucket_encryption(const UserInfo& user_info, const String& name) override;

    Result<void, models::ApiError> set_bucket_encryption(const UserInfo& user_info,
                                                         const String& name,
                                                         const Json::Value& config) override;

    Result<Json::Value, models::ApiError> get_bucket_lifecycle(const UserInfo& user_info, const String& name) override;

    Result<void, models::ApiError> set_bucket_lifecycle(const UserInfo& user_info,
                                                        const String& name,
                                                        const Json::Value& rules) override;

    Result<Json::Value, models::ApiError> get_bucket_object_lock(const UserInfo& user_info,
                                                                 const String& name) override;

    Result<void, models::ApiError> set_bucket_object_lock(const UserInfo& user_info,
                                                          const String& name,
                                                          const Json::Value& config) override;

  private:
    /**
     * @brief Validate bucket name according to S3 naming rules
     *
     * @param name Bucket name to validate
     * @return Optional error if validation fails
     */
    Optional<models::ApiError> validate_bucket_name(const String& name);

    /**
     * @brief Validate user access to bucket for given action
     *
     * @param user User to validate
     * @param bucket_name Bucket name
     * @param action Action to validate
     * @return Optional error if validation fails
     */
    Optional<models::ApiError> validate_access(const UserInfo& user, const String& bucket_name, const String& action);

    /**
     * @brief Validate policy JSON format
     *
     * @param policy_json Policy document
     * @return Optional error if validation fails
     */
    Optional<models::ApiError> validate_policy_json(const String& policy_json);

    std::shared_ptr<clients::IStorageClient> storage_client_;
};

} // namespace console::services
