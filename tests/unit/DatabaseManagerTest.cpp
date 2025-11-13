#include "console/storage/DatabaseManager.hpp"

#include <chrono>
#include <filesystem>
#include <gtest/gtest.h>
#include <thread>

using namespace console;
using namespace console::storage;

class DatabaseManagerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        std::cerr << "DEBUG: SetUp started" << std::endl;
        // Use in-memory database for tests
        db_path_ = ":memory:";
        std::cerr << "DEBUG: Creating DatabaseManager" << std::endl;
        // Use 0 threads for tests to avoid threading issues
        db_manager_ = std::make_unique<DatabaseManager>(db_path_, 0);
        std::cerr << "DEBUG: DatabaseManager created" << std::endl;

        // Initialize schema
        std::cerr << "DEBUG: Initializing schema" << std::endl;
        auto result = db_manager_->initialize_schema();
        ASSERT_TRUE(result) << "Failed to initialize schema: " << result.error();
        std::cerr << "DEBUG: Setup complete" << std::endl;
    }

    void TearDown() override { db_manager_.reset(); }

    std::unique_ptr<DatabaseManager> db_manager_;
    String db_path_;
};

// ============================================================================
// User Tests
// ============================================================================

TEST_F(DatabaseManagerTest, CreateUser_Success) {
    DbUser user;
    user.access_key = "test-user";
    user.secret_key = "test-secret";
    user.account_name = "Test User";
    user.status = "active";
    user.is_admin = false;
    user.created_at = std::time(nullptr);
    user.updated_at = user.created_at;
    user.metadata = "{}";

    auto result = db_manager_->create_user(user);
    ASSERT_TRUE(result) << "Failed to create user: " << result.error();

    // Verify user was created
    auto get_result = db_manager_->get_user("test-user");
    ASSERT_TRUE(get_result) << "Failed to get user: " << get_result.error();

    const auto& retrieved_user = get_result.value();
    EXPECT_EQ(retrieved_user.access_key, "test-user");
    EXPECT_EQ(retrieved_user.account_name, "Test User");
    EXPECT_FALSE(retrieved_user.is_admin);
}

TEST_F(DatabaseManagerTest, CreateUser_Duplicate) {
    DbUser user;
    user.access_key = "duplicate-user";
    user.secret_key = "secret";
    user.account_name = "User";
    user.status = "active";
    user.is_admin = false;
    user.created_at = std::time(nullptr);
    user.updated_at = user.created_at;
    user.metadata = "{}";

    auto result1 = db_manager_->create_user(user);
    ASSERT_TRUE(result1);

    // Try to create again
    auto result2 = db_manager_->create_user(user);
    ASSERT_FALSE(result2) << "Should fail to create duplicate user";
}

TEST_F(DatabaseManagerTest, GetUser_NotFound) {
    auto result = db_manager_->get_user("non-existent");
    ASSERT_FALSE(result);
}

TEST_F(DatabaseManagerTest, ListUsers_Success) {
    // Create multiple users
    for (int i = 0; i < 5; ++i) {
        DbUser user;
        user.access_key = "user" + std::to_string(i);
        user.secret_key = "secret" + std::to_string(i);
        user.account_name = "User " + std::to_string(i);
        user.status = "active";
        user.is_admin = (i == 0);
        user.created_at = std::time(nullptr);
        user.updated_at = user.created_at;
        user.metadata = "{}";

        ASSERT_TRUE(db_manager_->create_user(user));
    }

    auto result = db_manager_->list_users();
    ASSERT_TRUE(result) << "Failed to list users: " << result.error();

    // Should have 5 users + 1 default admin = 6 users
    EXPECT_GE(result.value().size(), 5);
}

TEST_F(DatabaseManagerTest, UpdateUser_Success) {
    DbUser user;
    user.access_key = "update-test";
    user.secret_key = "old-secret";
    user.account_name = "Old Name";
    user.status = "active";
    user.is_admin = false;
    user.created_at = std::time(nullptr);
    user.updated_at = user.created_at;
    user.metadata = "{}";

    ASSERT_TRUE(db_manager_->create_user(user));

    // Update user
    user.secret_key = "new-secret";
    user.account_name = "New Name";
    user.updated_at = std::time(nullptr);

    auto update_result = db_manager_->update_user(user);
    ASSERT_TRUE(update_result) << "Failed to update user: " << update_result.error();

    // Verify update
    auto get_result = db_manager_->get_user("update-test");
    ASSERT_TRUE(get_result);
    EXPECT_EQ(get_result.value().secret_key, "new-secret");
    EXPECT_EQ(get_result.value().account_name, "New Name");
}

TEST_F(DatabaseManagerTest, DeleteUser_Success) {
    DbUser user;
    user.access_key = "delete-test";
    user.secret_key = "secret";
    user.account_name = "Delete Me";
    user.status = "active";
    user.is_admin = false;
    user.created_at = std::time(nullptr);
    user.updated_at = user.created_at;
    user.metadata = "{}";

    ASSERT_TRUE(db_manager_->create_user(user));
    ASSERT_TRUE(db_manager_->user_exists("delete-test").value());

    // Delete user
    auto delete_result = db_manager_->delete_user("delete-test");
    ASSERT_TRUE(delete_result) << "Failed to delete user: " << delete_result.error();

    // Verify deletion
    auto exists_result = db_manager_->user_exists("delete-test");
    ASSERT_TRUE(exists_result);
    EXPECT_FALSE(exists_result.value());
}

TEST_F(DatabaseManagerTest, UserExists_Success) {
    DbUser user;
    user.access_key = "exists-test";
    user.secret_key = "secret";
    user.account_name = "Test";
    user.status = "active";
    user.is_admin = false;
    user.created_at = std::time(nullptr);
    user.updated_at = user.created_at;
    user.metadata = "{}";

    auto exists_before = db_manager_->user_exists("exists-test");
    ASSERT_TRUE(exists_before);
    EXPECT_FALSE(exists_before.value());

    ASSERT_TRUE(db_manager_->create_user(user));

    auto exists_after = db_manager_->user_exists("exists-test");
    ASSERT_TRUE(exists_after);
    EXPECT_TRUE(exists_after.value());
}

// ============================================================================
// Group Tests
// ============================================================================

TEST_F(DatabaseManagerTest, CreateGroup_Success) {
    DbGroup group;
    group.name = "test-group";
    group.description = "Test Group";
    group.status = "active";
    group.created_at = std::time(nullptr);
    group.updated_at = group.created_at;
    group.metadata = "{}";

    auto result = db_manager_->create_group(group);
    ASSERT_TRUE(result) << "Failed to create group: " << result.error();

    // Verify group was created
    auto get_result = db_manager_->get_group("test-group");
    ASSERT_TRUE(get_result) << "Failed to get group: " << get_result.error();
    EXPECT_EQ(get_result.value().name, "test-group");
}

TEST_F(DatabaseManagerTest, AddUserToGroup_Success) {
    // Create user
    DbUser user;
    user.access_key = "group-user";
    user.secret_key = "secret";
    user.account_name = "Group User";
    user.status = "active";
    user.is_admin = false;
    user.created_at = std::time(nullptr);
    user.updated_at = user.created_at;
    user.metadata = "{}";
    ASSERT_TRUE(db_manager_->create_user(user));

    // Create group
    DbGroup group;
    group.name = "test-group";
    group.description = "Test";
    group.status = "active";
    group.created_at = std::time(nullptr);
    group.updated_at = group.created_at;
    group.metadata = "{}";
    ASSERT_TRUE(db_manager_->create_group(group));

    // Add user to group
    auto result = db_manager_->add_user_to_group("group-user", "test-group");
    ASSERT_TRUE(result) << "Failed to add user to group: " << result.error();

    // Verify relationship
    auto groups_result = db_manager_->get_user_groups("group-user");
    ASSERT_TRUE(groups_result);
    EXPECT_EQ(groups_result.value().size(), 1);
    EXPECT_EQ(groups_result.value()[0], "test-group");
}

TEST_F(DatabaseManagerTest, RemoveUserFromGroup_Success) {
    // Setup: Create user, group, and relationship
    DbUser user;
    user.access_key = "remove-user";
    user.secret_key = "secret";
    user.account_name = "User";
    user.status = "active";
    user.is_admin = false;
    user.created_at = std::time(nullptr);
    user.updated_at = user.created_at;
    user.metadata = "{}";
    ASSERT_TRUE(db_manager_->create_user(user));

    DbGroup group;
    group.name = "remove-group";
    group.description = "Group";
    group.status = "active";
    group.created_at = std::time(nullptr);
    group.updated_at = group.created_at;
    group.metadata = "{}";
    ASSERT_TRUE(db_manager_->create_group(group));

    ASSERT_TRUE(db_manager_->add_user_to_group("remove-user", "remove-group"));

    // Remove user from group
    auto result = db_manager_->remove_user_from_group("remove-user", "remove-group");
    ASSERT_TRUE(result) << "Failed to remove user from group: " << result.error();

    // Verify removal
    auto groups_result = db_manager_->get_user_groups("remove-user");
    ASSERT_TRUE(groups_result);
    EXPECT_EQ(groups_result.value().size(), 0);
}

// ============================================================================
// Policy Tests
// ============================================================================

TEST_F(DatabaseManagerTest, CreatePolicy_Success) {
    DbPolicy policy;
    policy.name = "test-policy";
    policy.version = "2012-10-17";
    policy.document = R"({"Version":"2012-10-17","Statement":[]})";
    policy.description = "Test Policy";
    policy.created_at = std::time(nullptr);
    policy.updated_at = policy.created_at;
    policy.metadata = "{}";

    auto result = db_manager_->create_policy(policy);
    ASSERT_TRUE(result) << "Failed to create policy: " << result.error();

    // Verify policy was created
    auto get_result = db_manager_->get_policy("test-policy");
    ASSERT_TRUE(get_result);
    EXPECT_EQ(get_result.value().name, "test-policy");
}

TEST_F(DatabaseManagerTest, AttachPolicyToUser_Success) {
    // Create user
    DbUser user;
    user.access_key = "policy-user";
    user.secret_key = "secret";
    user.account_name = "User";
    user.status = "active";
    user.is_admin = false;
    user.created_at = std::time(nullptr);
    user.updated_at = user.created_at;
    user.metadata = "{}";
    ASSERT_TRUE(db_manager_->create_user(user));

    // Create policy
    DbPolicy policy;
    policy.name = "user-policy";
    policy.version = "2012-10-17";
    policy.document = "{}";
    policy.description = "Policy";
    policy.created_at = std::time(nullptr);
    policy.updated_at = policy.created_at;
    policy.metadata = "{}";
    ASSERT_TRUE(db_manager_->create_policy(policy));

    // Attach policy to user
    auto result = db_manager_->attach_policy_to_user("policy-user", "user-policy");
    ASSERT_TRUE(result) << "Failed to attach policy: " << result.error();

    // Verify attachment
    auto policies_result = db_manager_->get_user_policies("policy-user", false);
    ASSERT_TRUE(policies_result);
    EXPECT_EQ(policies_result.value().size(), 1);
    EXPECT_EQ(policies_result.value()[0], "user-policy");
}

TEST_F(DatabaseManagerTest, GetUserPolicies_IncludeGroups) {
    // Create user
    DbUser user;
    user.access_key = "complex-user";
    user.secret_key = "secret";
    user.account_name = "User";
    user.status = "active";
    user.is_admin = false;
    user.created_at = std::time(nullptr);
    user.updated_at = user.created_at;
    user.metadata = "{}";
    ASSERT_TRUE(db_manager_->create_user(user));

    // Create group
    DbGroup group;
    group.name = "complex-group";
    group.description = "Group";
    group.status = "active";
    group.created_at = std::time(nullptr);
    group.updated_at = group.created_at;
    group.metadata = "{}";
    ASSERT_TRUE(db_manager_->create_group(group));

    // Create policies
    DbPolicy direct_policy;
    direct_policy.name = "direct-policy";
    direct_policy.version = "2012-10-17";
    direct_policy.document = "{}";
    direct_policy.description = "Direct";
    direct_policy.created_at = std::time(nullptr);
    direct_policy.updated_at = direct_policy.created_at;
    direct_policy.metadata = "{}";
    ASSERT_TRUE(db_manager_->create_policy(direct_policy));

    DbPolicy group_policy;
    group_policy.name = "group-policy";
    group_policy.version = "2012-10-17";
    group_policy.document = "{}";
    group_policy.description = "Group";
    group_policy.created_at = std::time(nullptr);
    group_policy.updated_at = group_policy.created_at;
    group_policy.metadata = "{}";
    ASSERT_TRUE(db_manager_->create_policy(group_policy));

    // Attach direct policy to user
    ASSERT_TRUE(db_manager_->attach_policy_to_user("complex-user", "direct-policy"));

    // Add user to group
    ASSERT_TRUE(db_manager_->add_user_to_group("complex-user", "complex-group"));

    // Attach policy to group
    ASSERT_TRUE(db_manager_->attach_policy_to_group("complex-group", "group-policy"));

    // Get user policies without groups
    auto direct_only = db_manager_->get_user_policies("complex-user", false);
    ASSERT_TRUE(direct_only);
    EXPECT_EQ(direct_only.value().size(), 1);

    // Get user policies including groups
    auto all_policies = db_manager_->get_user_policies("complex-user", true);
    ASSERT_TRUE(all_policies);
    EXPECT_EQ(all_policies.value().size(), 2);
}

// ============================================================================
// Transaction Tests
// ============================================================================

TEST_F(DatabaseManagerTest, Transaction_Commit) {
    ASSERT_TRUE(db_manager_->begin_transaction());

    DbUser user;
    user.access_key = "tx-user";
    user.secret_key = "secret";
    user.account_name = "User";
    user.status = "active";
    user.is_admin = false;
    user.created_at = std::time(nullptr);
    user.updated_at = user.created_at;
    user.metadata = "{}";

    ASSERT_TRUE(db_manager_->create_user(user));
    ASSERT_TRUE(db_manager_->commit_transaction());

    // Verify user exists after commit
    auto exists = db_manager_->user_exists("tx-user");
    ASSERT_TRUE(exists);
    EXPECT_TRUE(exists.value());
}

TEST_F(DatabaseManagerTest, Transaction_Rollback) {
    ASSERT_TRUE(db_manager_->begin_transaction());

    DbUser user;
    user.access_key = "rollback-user";
    user.secret_key = "secret";
    user.account_name = "User";
    user.status = "active";
    user.is_admin = false;
    user.created_at = std::time(nullptr);
    user.updated_at = user.created_at;
    user.metadata = "{}";

    ASSERT_TRUE(db_manager_->create_user(user));
    ASSERT_TRUE(db_manager_->rollback_transaction());

    // Verify user does not exist after rollback
    auto exists = db_manager_->user_exists("rollback-user");
    ASSERT_TRUE(exists);
    EXPECT_FALSE(exists.value());
}

// ============================================================================
// Async Tests
// ============================================================================

TEST_F(DatabaseManagerTest, CreateUserAsync_Success) {
    DbUser user;
    user.access_key = "async-user";
    user.secret_key = "secret";
    user.account_name = "Async User";
    user.status = "active";
    user.is_admin = false;
    user.created_at = std::time(nullptr);
    user.updated_at = user.created_at;
    user.metadata = "{}";

    bool callback_called = false;
    bool success = false;

    db_manager_->create_user_async(user, [&](Result<void, String> result) {
        callback_called = true;
        success = static_cast<bool>(result);
    });

    // Wait for async operation to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_TRUE(callback_called);
    EXPECT_TRUE(success);

    // Verify user was created
    auto get_result = db_manager_->get_user("async-user");
    ASSERT_TRUE(get_result);
}

// ============================================================================
// Utility Tests
// ============================================================================

TEST_F(DatabaseManagerTest, GetCounts_Success) {
    // Create some entities
    DbUser user;
    user.access_key = "count-user";
    user.secret_key = "secret";
    user.account_name = "User";
    user.status = "active";
    user.is_admin = false;
    user.created_at = std::time(nullptr);
    user.updated_at = user.created_at;
    user.metadata = "{}";
    ASSERT_TRUE(db_manager_->create_user(user));

    DbGroup group;
    group.name = "count-group";
    group.description = "Group";
    group.status = "active";
    group.created_at = std::time(nullptr);
    group.updated_at = group.created_at;
    group.metadata = "{}";
    ASSERT_TRUE(db_manager_->create_group(group));

    DbPolicy policy;
    policy.name = "count-policy";
    policy.version = "2012-10-17";
    policy.document = "{}";
    policy.description = "Policy";
    policy.created_at = std::time(nullptr);
    policy.updated_at = policy.created_at;
    policy.metadata = "{}";
    ASSERT_TRUE(db_manager_->create_policy(policy));

    // Get counts
    auto user_count = db_manager_->get_user_count();
    auto group_count = db_manager_->get_group_count();
    auto policy_count = db_manager_->get_policy_count();

    ASSERT_TRUE(user_count);
    ASSERT_TRUE(group_count);
    ASSERT_TRUE(policy_count);

    EXPECT_GE(user_count.value(), 1); // At least our test user + default admin
    EXPECT_GE(group_count.value(), 1);
    EXPECT_GE(policy_count.value(), 1);
}

TEST_F(DatabaseManagerTest, IsReady_AfterInitialization) {
    EXPECT_TRUE(db_manager_->is_ready());
}
