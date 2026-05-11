// Copyright 2025 OpenMaxIO Contributors
// Licensed under AGPL-3.0

#pragma once

#include "console/common/Types.hpp"
#include "console/models/Error.hpp"
#include "console/models/User.hpp"

namespace console::services {

/**
 * @brief User management service interface
 *
 * Provides business logic for user operations including CRUD,
 * policy management, service accounts, and more.
 */
class IUserService {
  public:
    virtual ~IUserService() = default;

    /**
     * @brief List all users (admin only)
     *
     * @param admin_info Admin user context
     * @return Result containing list of users or error
     */
    virtual Result<Vector<models::User>, models::ApiError> list_users(const UserInfo& admin_info) = 0;

    /**
     * @brief Get user information
     *
     * @param admin_info Admin user context
     * @param access_key User access key
     * @return Result containing user info or error
     */
    virtual Result<models::User, models::ApiError> get_user(const UserInfo& admin_info, const String& access_key) = 0;

    /**
     * @brief Create a new user
     *
     * @param admin_info Admin user context
     * @param access_key User access key
     * @param secret_key User secret key
     * @param policies List of policy names to attach
     * @param is_admin Whether user has admin privileges
     * @return Result containing created user or error
     */
    virtual Result<models::User, models::ApiError> create_user(const UserInfo& admin_info,
                                                               const String& access_key,
                                                               const String& secret_key,
                                                               const Vector<String>& policies = {},
                                                               bool is_admin = false) = 0;

    /**
     * @brief Update user (change credentials or policies)
     *
     * @param admin_info Admin user context
     * @param access_key User access key
     * @param new_secret_key New secret key (optional)
     * @param policies List of policy names to attach (optional)
     * @return Result indicating success or error
     */
    virtual Result<void, models::ApiError> update_user(const UserInfo& admin_info,
                                                       const String& access_key,
                                                       const Optional<String>& new_secret_key = {},
                                                       const Optional<Vector<String>>& policies = {}) = 0;

    /**
     * @brief Delete a user
     *
     * @param admin_info Admin user context
     * @param access_key User access key
     * @return Result indicating success or error
     */
    virtual Result<void, models::ApiError> delete_user(const UserInfo& admin_info, const String& access_key) = 0;

    /**
     * @brief Enable/disable a user
     *
     * @param admin_info Admin user context
     * @param access_key User access key
     * @param enabled Enable or disable user
     * @return Result indicating success or error
     */
    virtual Result<void, models::ApiError> set_user_status(const UserInfo& admin_info,
                                                           const String& access_key,
                                                           bool enabled) = 0;

    /**
     * @brief Attach policy to user
     *
     * @param admin_info Admin user context
     * @param access_key User access key
     * @param policy_name Policy name
     * @return Result indicating success or error
     */
    virtual Result<void, models::ApiError> attach_user_policy(const UserInfo& admin_info,
                                                              const String& access_key,
                                                              const String& policy_name) = 0;

    /**
     * @brief Detach policy from user
     *
     * @param admin_info Admin user context
     * @param access_key User access key
     * @param policy_name Policy name
     * @return Result indicating success or error
     */
    virtual Result<void, models::ApiError> detach_user_policy(const UserInfo& admin_info,
                                                              const String& access_key,
                                                              const String& policy_name) = 0;

    /**
     * @brief List user's policies
     *
     * @param admin_info Admin user context
     * @param access_key User access key
     * @return Result containing list of policy names or error
     */
    virtual Result<Vector<String>, models::ApiError> list_user_policies(const UserInfo& admin_info,
                                                                        const String& access_key) = 0;

    /**
     * @brief Add user to group
     *
     * @param admin_info Admin user context
     * @param access_key User access key
     * @param group_name Group name
     * @return Result indicating success or error
     */
    virtual Result<void, models::ApiError> add_user_to_group(const UserInfo& admin_info,
                                                             const String& access_key,
                                                             const String& group_name) = 0;

    /**
     * @brief Remove user from group
     *
     * @param admin_info Admin user context
     * @param access_key User access key
     * @param group_name Group name
     * @return Result indicating success or error
     */
    virtual Result<void, models::ApiError> remove_user_from_group(const UserInfo& admin_info,
                                                                  const String& access_key,
                                                                  const String& group_name) = 0;
};

} // namespace console::services
