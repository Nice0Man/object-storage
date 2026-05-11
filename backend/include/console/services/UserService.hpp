#pragma once

#include "console/clients/StorageClient.hpp"
#include "console/services/IUserService.hpp"

#include <memory>

namespace console::services {

/**
 * @brief User management service implementation
 *
 * Provides business logic for user operations with validation,
 * permission checks, and error handling.
 */
class UserService : public IUserService {
  public:
    /**
     * @brief Construct UserService with MinIO Admin client
     *
     * @param admin_client MinIO Admin client for user operations
     */
    explicit UserService(std::shared_ptr<clients::IMinioAdminClient> admin_client);

    ~UserService() override = default;

    // IUserService implementation
    Result<Vector<models::User>, models::ApiError> list_users(const UserInfo& admin_info) override;

    Result<models::User, models::ApiError> get_user(const UserInfo& admin_info, const String& access_key) override;

    Result<models::User, models::ApiError> create_user(const UserInfo& admin_info,
                                                       const String& access_key,
                                                       const String& secret_key,
                                                       const Vector<String>& policies = {},
                                                       bool is_admin = false) override;

    Result<void, models::ApiError> update_user(const UserInfo& admin_info,
                                               const String& access_key,
                                               const Optional<String>& new_secret_key,
                                               const Optional<Vector<String>>& policies) override;

    Result<void, models::ApiError> delete_user(const UserInfo& admin_info, const String& access_key) override;

    Result<void, models::ApiError> set_user_status(const UserInfo& admin_info,
                                                   const String& access_key,
                                                   bool enabled) override;

    Result<void, models::ApiError> attach_user_policy(const UserInfo& admin_info,
                                                      const String& access_key,
                                                      const String& policy_name) override;

    Result<void, models::ApiError> detach_user_policy(const UserInfo& admin_info,
                                                      const String& access_key,
                                                      const String& policy_name) override;

    Result<Vector<String>, models::ApiError> list_user_policies(const UserInfo& admin_info,
                                                                const String& access_key) override;

    Result<void, models::ApiError> add_user_to_group(const UserInfo& admin_info,
                                                     const String& access_key,
                                                     const String& group_name) override;

    Result<void, models::ApiError> remove_user_from_group(const UserInfo& admin_info,
                                                          const String& access_key,
                                                          const String& group_name) override;

  private:
    /**
     * @brief Check if user has admin privileges
     *
     * @param user_info User to check
     * @return Optional error if not admin
     */
    Optional<models::ApiError> require_admin(const UserInfo& user_info);

    /**
     * @brief Validate access key format
     *
     * @param access_key Access key to validate
     * @return Optional error if validation fails
     */
    Optional<models::ApiError> validate_access_key(const String& access_key);

    /**
     * @brief Validate secret key format
     *
     * @param secret_key Secret key to validate
     * @return Optional error if validation fails
     */
    Optional<models::ApiError> validate_secret_key(const String& secret_key);

    std::shared_ptr<clients::IMinioAdminClient> admin_client_;
};

} // namespace console::services
