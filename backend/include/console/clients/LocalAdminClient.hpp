#pragma once

#include "console/clients/StorageClient.hpp"
#include "console/storage/DatabaseManager.hpp"

#include <memory>

namespace console::clients {

/**
 * @brief Local Admin client implementation using SQLite backend
 *
 * Implements IMinioAdminClient interface for user, group, and policy management
 * using DatabaseManager instead of external MinIO admin API.
 */
class LocalAdminClient : public IMinioAdminClient {
  public:
    /**
     * @brief Construct LocalAdminClient with DatabaseManager
     *
     * @param db_manager DatabaseManager instance for storage operations
     */
    explicit LocalAdminClient(std::shared_ptr<storage::DatabaseManager> db_manager);

    ~LocalAdminClient() override = default;

    // ========================================================================
    // IMinioAdminClient Implementation
    // ========================================================================

    // Server info
    Result<models::ServerInfo, String> get_server_info() override;

    // User management
    Result<Vector<models::User>, String> list_users() override;
    Result<models::User, String> get_user(const String& access_key) override;
    Result<models::User, String> create_user(const String& access_key,
                                             const String& secret_key,
                                             bool is_admin = false) override;
    Result<void, String> delete_user(const String& access_key) override;
    Result<void, String> set_user_policy(const String& access_key, const String& policy_name) override;
    Result<void, String> update_user_groups(const String& access_key, const Vector<String>& groups) override;

    // Group management
    Result<Vector<models::Group>, String> list_groups() override;
    Result<models::Group, String> get_group(const String& name) override;
    Result<bool, String> create_group(const CreateGroupRequest& request) override;
    Result<bool, String> delete_group(const String& name) override;
    Result<bool, String> add_user_to_group(const String& user, const String& group) override;
    Result<bool, String> remove_user_from_group(const String& user, const String& group) override;

    // Policy management
    Result<Vector<models::Policy>, String> list_policies() override;
    Result<bool, String> create_policy(const CreatePolicyRequest& request) override;
    Result<bool, String> delete_policy(const String& name) override;
    Result<models::Policy, String> get_policy(const String& name) override;
    Result<bool, String> attach_policy(const AttachPolicyRequest& request) override;
    Result<bool, String> detach_policy(const AttachPolicyRequest& request) override;

    // Service accounts
    Result<models::ServiceAccount, String> create_service_account(
        const String& parent_user,
        const Optional<String>& policy = std::nullopt) override;

    Result<bool, String> delete_service_account(const String& access_key) override;

  private:
    // ========================================================================
    // Helper Methods
    // ========================================================================

    /**
     * @brief Convert database user to model user
     */
    models::User db_user_to_model(const storage::DbUser& db_user);

    /**
     * @brief Convert database group to model group
     */
    models::Group db_group_to_model(const storage::DbGroup& db_group);

    /**
     * @brief Convert database policy to model policy
     */
    models::Policy db_policy_to_model(const storage::DbPolicy& db_policy);

    /**
     * @brief Generate unique service account access key
     */
    String generate_service_account_key(const String& parent_user);

    /**
     * @brief Generate random secret key
     */
    String generate_secret_key(size_t length = 40);

    std::shared_ptr<storage::DatabaseManager> db_manager_;
};

} // namespace console::clients
