#include "console/clients/LocalAdminClient.hpp"

#include "console/common/Logger.hpp"

#include <iomanip>
#include <random>
#include <sstream>

namespace console::clients {

// ============================================================================
// Constructor
// ============================================================================

LocalAdminClient::LocalAdminClient(std::shared_ptr<storage::DatabaseManager> db_manager) : db_manager_(db_manager) {
    CONSOLE_LOG_INFO("LocalAdminClient initialized with DatabaseManager");
}

// ============================================================================
// Server Info
// ============================================================================

Result<models::ServerInfo, String>
LocalAdminClient::get_server_info() {
    models::ServerInfo info;
    info.set_version("1.0.0-local");
    info.set_region("local");

    // Get stats from database and add to backend map
    StringMap backend;
    backend["storage_type"] = "local-sqlite";

    auto user_count = db_manager_->get_user_count();
    auto group_count = db_manager_->get_group_count();
    auto policy_count = db_manager_->get_policy_count();

    if (user_count) {
        backend["users"] = std::to_string(user_count.value());
    }
    if (group_count) {
        backend["groups"] = std::to_string(group_count.value());
    }
    if (policy_count) {
        backend["policies"] = std::to_string(policy_count.value());
    }

    info.set_backend(backend);

    return Ok<models::ServerInfo, String>(info);
}

// ============================================================================
// User Management
// ============================================================================

Result<Vector<models::User>, String>
LocalAdminClient::list_users() {
    auto db_users_result = db_manager_->list_users("active");
    if (!db_users_result) {
        return Err<Vector<models::User>, String>(db_users_result.error());
    }

    Vector<models::User> users;
    for (const auto& db_user : db_users_result.value()) {
        users.push_back(db_user_to_model(db_user));
    }

    return Ok<Vector<models::User>, String>(users);
}

Result<models::User, String>
LocalAdminClient::get_user(const String& access_key) {
    auto db_user_result = db_manager_->get_user(access_key);
    if (!db_user_result) {
        return Err<models::User, String>(db_user_result.error());
    }

    return Ok<models::User, String>(db_user_to_model(db_user_result.value()));
}

Result<models::User, String>
LocalAdminClient::create_user(const String& access_key, const String& secret_key) {
    // Check if user already exists
    auto exists_result = db_manager_->user_exists(access_key);
    if (exists_result && exists_result.value()) {
        return Err<models::User, String>("User already exists: " + access_key);
    }

    // Create database user
    storage::DbUser db_user;
    db_user.access_key = access_key;
    db_user.secret_key = secret_key; // Should be hashed in production
    db_user.account_name = access_key;
    db_user.status = "active";
    db_user.is_admin = false;
    db_user.created_at = std::time(nullptr);
    db_user.updated_at = db_user.created_at;
    db_user.metadata = "{}";

    auto create_result = db_manager_->create_user(db_user);
    if (!create_result) {
        return Err<models::User, String>(create_result.error());
    }

    CONSOLE_LOG_INFO("User created: {}", access_key);
    return Ok<models::User, String>(db_user_to_model(db_user));
}

Result<void, String>
LocalAdminClient::delete_user(const String& access_key) {
    auto delete_result = db_manager_->delete_user(access_key);
    if (!delete_result) {
        return Err<void, String>(delete_result.error());
    }

    CONSOLE_LOG_INFO("User deleted: {}", access_key);
    return Ok<String>();
}

Result<void, String>
LocalAdminClient::set_user_policy(const String& access_key, const String& policy_name) {
    // Check if user exists
    auto user_result = db_manager_->get_user(access_key);
    if (!user_result) {
        return Err<void, String>("User not found: " + access_key);
    }

    // Check if policy exists
    auto policy_result = db_manager_->policy_exists(policy_name);
    if (!policy_result || !policy_result.value()) {
        return Err<void, String>("Policy not found: " + policy_name);
    }

    // Attach policy to user
    auto attach_result = db_manager_->attach_policy_to_user(access_key, policy_name);
    if (!attach_result) {
        return Err<void, String>(attach_result.error());
    }

    CONSOLE_LOG_INFO("Policy '{}' attached to user '{}'", policy_name, access_key);
    return Ok<String>();
}

Result<void, String>
LocalAdminClient::update_user_groups(const String& access_key, const Vector<String>& groups) {
    // Check if user exists
    auto user_result = db_manager_->get_user(access_key);
    if (!user_result) {
        return Err<void, String>("User not found: " + access_key);
    }

    // Get current groups
    auto current_groups_result = db_manager_->get_user_groups(access_key);
    Vector<String> current_groups;
    if (current_groups_result) {
        current_groups = current_groups_result.value();
    }

    // Remove user from groups not in new list
    for (const auto& current_group : current_groups) {
        if (std::find(groups.begin(), groups.end(), current_group) == groups.end()) {
            auto remove_result = db_manager_->remove_user_from_group(access_key, current_group);
            if (!remove_result) {
                CONSOLE_LOG_WARN(
                    "Failed to remove user '{}' from group '{}': {}", access_key, current_group, remove_result.error());
            }
        }
    }

    // Add user to new groups
    for (const auto& new_group : groups) {
        if (std::find(current_groups.begin(), current_groups.end(), new_group) == current_groups.end()) {
            // Check if group exists
            auto group_exists = db_manager_->group_exists(new_group);
            if (!group_exists || !group_exists.value()) {
                CONSOLE_LOG_WARN("Group '{}' does not exist, skipping", new_group);
                continue;
            }

            auto add_result = db_manager_->add_user_to_group(access_key, new_group);
            if (!add_result) {
                CONSOLE_LOG_WARN(
                    "Failed to add user '{}' to group '{}': {}", access_key, new_group, add_result.error());
            }
        }
    }

    CONSOLE_LOG_INFO("Updated groups for user '{}'", access_key);
    return Ok<String>();
}

// ============================================================================
// Group Management
// ============================================================================

Result<Vector<models::Group>, String>
LocalAdminClient::list_groups() {
    auto db_groups_result = db_manager_->list_groups("active");
    if (!db_groups_result) {
        return Err<Vector<models::Group>, String>(db_groups_result.error());
    }

    Vector<models::Group> groups;
    for (const auto& db_group : db_groups_result.value()) {
        auto group = db_group_to_model(db_group);

        // Get group members
        auto members_result = db_manager_->get_group_users(db_group.name);
        if (members_result) {
            group.set_members(members_result.value());
        }

        // Get group policies
        auto policies_result = db_manager_->get_group_policies(db_group.name);
        if (policies_result) {
            group.set_policies(policies_result.value());
        }

        groups.push_back(group);
    }

    return Ok<Vector<models::Group>, String>(groups);
}

Result<models::Group, String>
LocalAdminClient::get_group(const String& name) {
    auto db_group_result = db_manager_->get_group(name);
    if (!db_group_result) {
        return Err<models::Group, String>(db_group_result.error());
    }

    auto group = db_group_to_model(db_group_result.value());

    // Get group members
    auto members_result = db_manager_->get_group_users(name);
    if (members_result) {
        group.set_members(members_result.value());
    }

    // Get group policies
    auto policies_result = db_manager_->get_group_policies(name);
    if (policies_result) {
        group.set_policies(policies_result.value());
    }

    return Ok<models::Group, String>(group);
}

Result<bool, String>
LocalAdminClient::create_group(const CreateGroupRequest& request) {
    // Check if group already exists
    auto exists_result = db_manager_->group_exists(request.name);
    if (exists_result && exists_result.value()) {
        return Err<bool, String>("Group already exists: " + request.name);
    }

    // Create database group
    storage::DbGroup db_group;
    db_group.name = request.name;
    db_group.description = "Group: " + request.name;
    db_group.status = "active";
    db_group.created_at = std::time(nullptr);
    db_group.updated_at = db_group.created_at;
    db_group.metadata = "{}";

    auto create_result = db_manager_->create_group(db_group);
    if (!create_result) {
        return Err<bool, String>(create_result.error());
    }

    // Add members
    for (const auto& member : request.members) {
        auto add_result = db_manager_->add_user_to_group(member, request.name);
        if (!add_result) {
            CONSOLE_LOG_WARN("Failed to add member '{}' to group '{}': {}", member, request.name, add_result.error());
        }
    }

    // Attach policies
    for (const auto& policy : request.policies) {
        auto attach_result = db_manager_->attach_policy_to_group(request.name, policy);
        if (!attach_result) {
            CONSOLE_LOG_WARN(
                "Failed to attach policy '{}' to group '{}': {}", policy, request.name, attach_result.error());
        }
    }

    CONSOLE_LOG_INFO("Group created: {}", request.name);
    return Ok<bool, String>(true);
}

Result<bool, String>
LocalAdminClient::delete_group(const String& name) {
    auto delete_result = db_manager_->delete_group(name);
    if (!delete_result) {
        return Err<bool, String>(delete_result.error());
    }

    CONSOLE_LOG_INFO("Group deleted: {}", name);
    return Ok<bool, String>(true);
}

Result<bool, String>
LocalAdminClient::add_user_to_group(const String& user, const String& group) {
    auto add_result = db_manager_->add_user_to_group(user, group);
    if (!add_result) {
        return Err<bool, String>(add_result.error());
    }

    CONSOLE_LOG_INFO("User '{}' added to group '{}'", user, group);
    return Ok<bool, String>(true);
}

Result<bool, String>
LocalAdminClient::remove_user_from_group(const String& user, const String& group) {
    auto remove_result = db_manager_->remove_user_from_group(user, group);
    if (!remove_result) {
        return Err<bool, String>(remove_result.error());
    }

    CONSOLE_LOG_INFO("User '{}' removed from group '{}'", user, group);
    return Ok<bool, String>(true);
}

// ============================================================================
// Policy Management
// ============================================================================

Result<Vector<models::Policy>, String>
LocalAdminClient::list_policies() {
    auto db_policies_result = db_manager_->list_policies();
    if (!db_policies_result) {
        return Err<Vector<models::Policy>, String>(db_policies_result.error());
    }

    Vector<models::Policy> policies;
    for (const auto& db_policy : db_policies_result.value()) {
        policies.push_back(db_policy_to_model(db_policy));
    }

    return Ok<Vector<models::Policy>, String>(policies);
}

Result<bool, String>
LocalAdminClient::create_policy(const CreatePolicyRequest& request) {
    // Check if policy already exists
    auto exists_result = db_manager_->policy_exists(request.name);
    if (exists_result && exists_result.value()) {
        return Err<bool, String>("Policy already exists: " + request.name);
    }

    // Create database policy
    storage::DbPolicy db_policy;
    db_policy.name = request.name;
    db_policy.version = "2012-10-17"; // AWS IAM policy version
    db_policy.document = request.policy_document;
    db_policy.description = "Policy: " + request.name;
    db_policy.created_at = std::time(nullptr);
    db_policy.updated_at = db_policy.created_at;
    db_policy.metadata = "{}";

    auto create_result = db_manager_->create_policy(db_policy);
    if (!create_result) {
        return Err<bool, String>(create_result.error());
    }

    CONSOLE_LOG_INFO("Policy created: {}", request.name);
    return Ok<bool, String>(true);
}

Result<bool, String>
LocalAdminClient::delete_policy(const String& name) {
    auto delete_result = db_manager_->delete_policy(name);
    if (!delete_result) {
        return Err<bool, String>(delete_result.error());
    }

    CONSOLE_LOG_INFO("Policy deleted: {}", name);
    return Ok<bool, String>(true);
}

Result<models::Policy, String>
LocalAdminClient::get_policy(const String& name) {
    auto db_policy_result = db_manager_->get_policy(name);
    if (!db_policy_result) {
        return Err<models::Policy, String>(db_policy_result.error());
    }

    return Ok<models::Policy, String>(db_policy_to_model(db_policy_result.value()));
}

Result<bool, String>
LocalAdminClient::attach_policy(const AttachPolicyRequest& request) {
    if (request.entity_type == "user") {
        auto attach_result = db_manager_->attach_policy_to_user(request.entity_name, request.policy_name);
        if (!attach_result) {
            return Err<bool, String>(attach_result.error());
        }
        CONSOLE_LOG_INFO("Policy '{}' attached to user '{}'", request.policy_name, request.entity_name);
    } else if (request.entity_type == "group") {
        auto attach_result = db_manager_->attach_policy_to_group(request.entity_name, request.policy_name);
        if (!attach_result) {
            return Err<bool, String>(attach_result.error());
        }
        CONSOLE_LOG_INFO("Policy '{}' attached to group '{}'", request.policy_name, request.entity_name);
    } else {
        return Err<bool, String>("Invalid entity type: " + request.entity_type);
    }

    return Ok<bool, String>(true);
}

Result<bool, String>
LocalAdminClient::detach_policy(const AttachPolicyRequest& request) {
    if (request.entity_type == "user") {
        auto detach_result = db_manager_->detach_policy_from_user(request.entity_name, request.policy_name);
        if (!detach_result) {
            return Err<bool, String>(detach_result.error());
        }
        CONSOLE_LOG_INFO("Policy '{}' detached from user '{}'", request.policy_name, request.entity_name);
    } else if (request.entity_type == "group") {
        auto detach_result = db_manager_->detach_policy_from_group(request.entity_name, request.policy_name);
        if (!detach_result) {
            return Err<bool, String>(detach_result.error());
        }
        CONSOLE_LOG_INFO("Policy '{}' detached from group '{}'", request.policy_name, request.entity_name);
    } else {
        return Err<bool, String>("Invalid entity type: " + request.entity_type);
    }

    return Ok<bool, String>(true);
}

// ============================================================================
// Service Accounts
// ============================================================================

Result<models::ServiceAccount, String>
LocalAdminClient::create_service_account(const String& parent_user, const Optional<String>& policy) {
    // Check if parent user exists
    auto user_result = db_manager_->get_user(parent_user);
    if (!user_result) {
        return Err<models::ServiceAccount, String>("Parent user not found: " + parent_user);
    }

    // Generate service account credentials
    String access_key = generate_service_account_key(parent_user);
    String secret_key = generate_secret_key();

    // Create database service account
    storage::DbServiceAccount db_account;
    db_account.access_key = access_key;
    db_account.secret_key = secret_key;
    db_account.parent_user = parent_user;
    db_account.description = "Service account for " + parent_user;
    db_account.expiration = std::nullopt; // No expiration by default
    db_account.status = "active";
    db_account.created_at = std::time(nullptr);
    db_account.metadata = "{}";

    auto create_result = db_manager_->create_service_account(db_account);
    if (!create_result) {
        return Err<models::ServiceAccount, String>(create_result.error());
    }

    // Attach policy if provided
    if (policy.has_value()) {
        auto attach_result = db_manager_->attach_policy_to_user(access_key, policy.value());
        if (!attach_result) {
            CONSOLE_LOG_WARN("Failed to attach policy to service account: {}", attach_result.error());
        }
    }

    // Create model service account
    models::ServiceAccount account;
    account.access_key = access_key;
    account.secret_key = secret_key;
    account.parent_user = parent_user;
    // Note: status is stored in DB but not in model

    CONSOLE_LOG_INFO("Service account created: {} (parent: {})", access_key, parent_user);
    return Ok<models::ServiceAccount, String>(account);
}

Result<bool, String>
LocalAdminClient::delete_service_account(const String& access_key) {
    auto delete_result = db_manager_->delete_service_account(access_key);
    if (!delete_result) {
        return Err<bool, String>(delete_result.error());
    }

    CONSOLE_LOG_INFO("Service account deleted: {}", access_key);
    return Ok<bool, String>(true);
}

// ============================================================================
// Helper Methods
// ============================================================================

models::User
LocalAdminClient::db_user_to_model(const storage::DbUser& db_user) {
    UserInfo info;
    info.access_key = db_user.access_key;
    info.account_name = db_user.account_name;
    info.is_admin = db_user.is_admin;
    // Convert Unix timestamp to TimePoint
    info.created_at = std::chrono::system_clock::from_time_t(db_user.created_at);

    // Get user policies
    auto policies_result = db_manager_->get_user_policies(db_user.access_key, true);
    if (policies_result) {
        info.policies = policies_result.value();
    }

    return models::User(info);
}

models::Group
LocalAdminClient::db_group_to_model(const storage::DbGroup& db_group) {
    models::Group group(db_group.name);
    group.set_description(db_group.description);
    // Convert Unix timestamp to TimePoint
    auto timepoint = std::chrono::system_clock::from_time_t(db_group.created_at);
    group.set_created_at(timepoint);
    // Note: status is not in the Group model
    return group;
}

models::Policy
LocalAdminClient::db_policy_to_model(const storage::DbPolicy& db_policy) {
    models::Policy policy(db_policy.name);
    policy.set_version(db_policy.version);
    policy.set_description(db_policy.description);
    // Convert Unix timestamp to TimePoint
    auto timepoint = std::chrono::system_clock::from_time_t(db_policy.created_at);
    policy.set_created_at(timepoint);
    // Note: policy_document (JSON) parsing to statements should be done separately if needed
    return policy;
}

String
LocalAdminClient::generate_service_account_key(const String& parent_user) {
    // Generate SA key: sa-<parent>-<random>
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    ss << "sa-" << parent_user << "-";

    // Add 8 random hex characters
    for (int i = 0; i < 8; ++i) {
        ss << std::hex << dis(gen);
    }

    return ss.str();
}

String
LocalAdminClient::generate_secret_key(size_t length) {
    static const char charset[] = "0123456789"
                                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                  "abcdefghijklmnopqrstuvwxyz";

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(charset) - 2);

    String secret;
    secret.reserve(length);

    for (size_t i = 0; i < length; ++i) {
        secret += charset[dis(gen)];
    }

    return secret;
}

} // namespace console::clients
