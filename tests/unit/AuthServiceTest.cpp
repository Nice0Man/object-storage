#include "console/services/AuthService.hpp"

#include "console/clients/LocalAdminClient.hpp"
#include "console/common/Config.hpp"
#include "console/storage/DatabaseManager.hpp"
#include "console/utils/JWT.hpp"
#include "console/utils/PasswordHash.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace console;
using namespace console::services;
using namespace console::clients;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

// Mock AdminClient
class MockAdminClient : public IMinioAdminClient {
  public:
    // Server info
    MOCK_METHOD((Result<models::ServerInfo, String>), get_server_info, (), (override));

    // User management
    MOCK_METHOD((Result<Vector<models::User>, String>), list_users, (), (override));
    MOCK_METHOD((Result<models::User, String>), get_user, (const String&), (override));
    MOCK_METHOD((Result<models::User, String>), create_user, (const String&, const String&, bool), (override));
    MOCK_METHOD((Result<void, String>), delete_user, (const String&), (override));
    MOCK_METHOD((Result<void, String>), set_user_policy, (const String&, const String&), (override));
    MOCK_METHOD((Result<void, String>), update_user_groups, (const String&, const Vector<String>&), (override));

    // Group management
    MOCK_METHOD((Result<Vector<models::Group>, String>), list_groups, (), (override));
    MOCK_METHOD((Result<models::Group, String>), get_group, (const String&), (override));
    MOCK_METHOD((Result<bool, String>), create_group, (const CreateGroupRequest&), (override));
    MOCK_METHOD((Result<bool, String>), delete_group, (const String&), (override));
    MOCK_METHOD((Result<bool, String>), add_user_to_group, (const String&, const String&), (override));
    MOCK_METHOD((Result<bool, String>), remove_user_from_group, (const String&, const String&), (override));

    // Policy management
    MOCK_METHOD((Result<Vector<models::Policy>, String>), list_policies, (), (override));
    MOCK_METHOD((Result<bool, String>), create_policy, (const CreatePolicyRequest&), (override));
    MOCK_METHOD((Result<bool, String>), delete_policy, (const String&), (override));
    MOCK_METHOD((Result<models::Policy, String>), get_policy, (const String&), (override));
    MOCK_METHOD((Result<bool, String>), attach_policy, (const AttachPolicyRequest&), (override));
    MOCK_METHOD((Result<bool, String>), detach_policy, (const AttachPolicyRequest&), (override));

    // Service accounts
    MOCK_METHOD((Result<models::ServiceAccount, String>),
                create_service_account,
                (const String&, const Optional<String>&),
                (override));
    MOCK_METHOD((Result<bool, String>), delete_service_account, (const String&), (override));
};

class AuthServiceTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create temporary test database
        test_db_path_ = "/tmp/test_auth_" + std::to_string(time(nullptr)) + ".db";

        mock_admin_client_ = std::make_shared<NiceMock<MockAdminClient>>();

        // Configure the global Config instance for tests
        // This is needed because initialize() uses Config::instance()
        auto& global_config = Config::instance();
        global_config.auth().jwt_secret = "test-secret-key-for-testing-purposes-only";
        global_config.auth().token_expiry = std::chrono::hours(24);
        global_config.default_admin().enabled = true;
        global_config.default_admin().username = "admin";
        global_config.default_admin().password = "admin123";
        global_config.default_admin().account_name = "Administrator";

        // Use the global config
        config_ = std::shared_ptr<Config>(&global_config, [](Config*) {}); // Non-owning shared_ptr

        // Initialize JWT with config
        utils::JWT::initialize(config_->auth().jwt_secret, "test-encryption-passphrase", "test-encryption-salt");

        db_manager_ = std::make_shared<storage::DatabaseManager>(test_db_path_);
        db_manager_->initialize(); // Create tables and default admin user

        auth_service_ = std::make_unique<AuthService>(mock_admin_client_, config_, db_manager_);
    }

    void TearDown() override {
        auth_service_.reset();
        db_manager_.reset();

        // Clean up test database (RocksDB creates a directory, not a file)
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }

    std::shared_ptr<MockAdminClient> mock_admin_client_;
    std::shared_ptr<Config> config_;
    std::shared_ptr<storage::DatabaseManager> db_manager_;
    std::unique_ptr<AuthService> auth_service_;
    String test_db_path_;
};

// ============================================================================
// Login Tests
// ============================================================================

TEST_F(AuthServiceTest, Login_AdminSuccess) {
    auto result = auth_service_->login("admin", "admin123");

    ASSERT_TRUE(result.is_ok()) << "Login failed: "
                                << (result.is_err() ? result.error().to_json().toStyledString() : "unknown");

    Json::Value response = result.value();
    EXPECT_TRUE(response.isMember("access_token"));
    EXPECT_TRUE(response.isMember("refresh_token"));
    EXPECT_TRUE(response.isMember("expires_in"));
    EXPECT_TRUE(response.isMember("user"));

    EXPECT_FALSE(response["access_token"].asString().empty());
    EXPECT_FALSE(response["refresh_token"].asString().empty());
}

TEST_F(AuthServiceTest, Login_InvalidUsername) {
    auto result = auth_service_->login("nonexistent", "password");

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error().status(), HttpStatus::Unauthorized);
}

TEST_F(AuthServiceTest, Login_InvalidPassword) {
    auto result = auth_service_->login("admin", "wrongpassword");

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error().status(), HttpStatus::Unauthorized);
}

TEST_F(AuthServiceTest, Login_EmptyUsername) {
    auto result = auth_service_->login("", "password");

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error().status(), HttpStatus::BadRequest);
}

TEST_F(AuthServiceTest, Login_EmptyPassword) {
    auto result = auth_service_->login("admin", "");

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error().status(), HttpStatus::BadRequest);
}

TEST_F(AuthServiceTest, Login_UsernameSpecialCharacters) {
    // Test SQL injection attempt
    auto result = auth_service_->login("admin' OR '1'='1", "password");

    ASSERT_TRUE(result.is_err());
}

// ============================================================================
// Token Validation Tests
// ============================================================================

TEST_F(AuthServiceTest, ValidateToken_ValidToken) {
    // First login to get a valid token
    auto login_result = auth_service_->login("admin", "admin123");
    ASSERT_TRUE(login_result.is_ok());

    String token = login_result.value()["access_token"].asString();

    // Validate the token
    auto validate_result = auth_service_->validate_token(token);

    ASSERT_TRUE(validate_result.is_ok()) << "Validation failed: "
                                         << (validate_result.is_err()
                                                 ? validate_result.error().to_json().toStyledString()
                                                 : "unknown");
    EXPECT_EQ(validate_result.value().access_key, "admin");
    EXPECT_TRUE(validate_result.value().is_admin);
}

TEST_F(AuthServiceTest, ValidateToken_InvalidToken) {
    auto result = auth_service_->validate_token("invalid.token.here");

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error().status(), HttpStatus::Unauthorized);
}

TEST_F(AuthServiceTest, ValidateToken_ExpiredToken) {
    // This would require manipulating time or creating a token with past expiry
    // For now, just test with malformed token
    auto result = auth_service_->validate_token("expired.token.value");

    ASSERT_TRUE(result.is_err());
}

TEST_F(AuthServiceTest, ValidateToken_EmptyToken) {
    auto result = auth_service_->validate_token("");

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error().status(), HttpStatus::Unauthorized);
}

// ============================================================================
// Get Current User Tests
// ============================================================================

TEST_F(AuthServiceTest, GetCurrentUser_Success) {
    auto login_result = auth_service_->login("admin", "admin123");
    ASSERT_TRUE(login_result.is_ok());

    String token = login_result.value()["access_token"].asString();

    auto user_result = auth_service_->get_current_user(token);

    ASSERT_TRUE(user_result.is_ok());
    EXPECT_EQ(user_result.value().access_key(), "admin");
    EXPECT_TRUE(user_result.value().is_admin());
}

TEST_F(AuthServiceTest, GetCurrentUser_InvalidToken) {
    auto result = auth_service_->get_current_user("invalid.token");

    ASSERT_TRUE(result.is_err());
}

// ============================================================================
// Change Password Tests
// ============================================================================

TEST_F(AuthServiceTest, ChangePassword_Success) {
    auto login_result = auth_service_->login("admin", "admin123");
    ASSERT_TRUE(login_result.is_ok());

    String token = login_result.value()["access_token"].asString();

    auto change_result = auth_service_->change_password(token, "admin123", "newpassword123");

    ASSERT_TRUE(change_result.is_ok()) << "Change password failed: "
                                       << (change_result.is_err() ? change_result.error().to_json().toStyledString()
                                                                  : "unknown");

    // Verify can login with new password
    auto new_login = auth_service_->login("admin", "newpassword123");
    EXPECT_TRUE(new_login.is_ok());

    // Verify cannot login with old password
    auto old_login = auth_service_->login("admin", "admin123");
    EXPECT_TRUE(old_login.is_err());
}

TEST_F(AuthServiceTest, ChangePassword_WrongOldPassword) {
    auto login_result = auth_service_->login("admin", "admin123");
    ASSERT_TRUE(login_result.is_ok());

    String token = login_result.value()["access_token"].asString();

    auto result = auth_service_->change_password(token, "wrongoldpassword", "newpassword123");

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error().status(), HttpStatus::Unauthorized);
}

TEST_F(AuthServiceTest, ChangePassword_WeakNewPassword) {
    auto login_result = auth_service_->login("admin", "admin123");
    ASSERT_TRUE(login_result.is_ok());

    String token = login_result.value()["access_token"].asString();

    auto result = auth_service_->change_password(token,
                                                 "admin123",
                                                 "weak" // Too short
    );

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error().status(), HttpStatus::BadRequest);
}

TEST_F(AuthServiceTest, ChangePassword_InvalidToken) {
    auto result = auth_service_->change_password("invalid.token", "oldpass", "newpass123");

    ASSERT_TRUE(result.is_err());
}

// ============================================================================
// Logout Tests
// ============================================================================

TEST_F(AuthServiceTest, Logout_Success) {
    auto login_result = auth_service_->login("admin", "admin123");
    ASSERT_TRUE(login_result.is_ok());

    String token = login_result.value()["access_token"].asString();

    auto logout_result = auth_service_->logout(token);

    EXPECT_TRUE(logout_result.is_ok());
}

TEST_F(AuthServiceTest, Logout_InvalidToken) {
    auto result = auth_service_->logout("invalid.token");

    // Logout with invalid token should still succeed (idempotent)
    EXPECT_TRUE(result.is_ok());
}

// ============================================================================
// Refresh Token Tests
// ============================================================================

TEST_F(AuthServiceTest, RefreshToken_Success) {
    auto login_result = auth_service_->login("admin", "admin123");
    ASSERT_TRUE(login_result.is_ok());

    String refresh_token = login_result.value()["refresh_token"].asString();

    auto refresh_result = auth_service_->refresh_token(refresh_token);

    ASSERT_TRUE(refresh_result.is_ok());

    Json::Value response = refresh_result.value();
    EXPECT_TRUE(response.isMember("access_token"));
    EXPECT_TRUE(response.isMember("refresh_token"));

    // New tokens should be different
    EXPECT_NE(response["access_token"].asString(), login_result.value()["access_token"].asString());
}

TEST_F(AuthServiceTest, RefreshToken_InvalidToken) {
    auto result = auth_service_->refresh_token("invalid.refresh.token");

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error().status(), HttpStatus::Unauthorized);
}

TEST_F(AuthServiceTest, RefreshToken_EmptyToken) {
    auto result = auth_service_->refresh_token("");

    ASSERT_TRUE(result.is_err());
}

// ============================================================================
// Multiple Login Sessions Tests
// ============================================================================

TEST_F(AuthServiceTest, MultipleLoginSessions_Allowed) {
    // First login
    auto login1 = auth_service_->login("admin", "admin123");
    ASSERT_TRUE(login1.is_ok());
    String token1 = login1.value()["access_token"].asString();

    // Second login (same user)
    auto login2 = auth_service_->login("admin", "admin123");
    ASSERT_TRUE(login2.is_ok());
    String token2 = login2.value()["access_token"].asString();

    // Both tokens should be different
    EXPECT_NE(token1, token2);

    // Both tokens should be valid
    auto validate1 = auth_service_->validate_token(token1);
    auto validate2 = auth_service_->validate_token(token2);

    EXPECT_TRUE(validate1.is_ok());
    EXPECT_TRUE(validate2.is_ok());
}

// ============================================================================
// Security Tests
// ============================================================================

TEST_F(AuthServiceTest, Security_PasswordNotInResponse) {
    auto result = auth_service_->login("admin", "admin123");
    ASSERT_TRUE(result.is_ok());

    Json::Value response = result.value();
    String response_str = response.toStyledString();

    // Password should never appear in response
    EXPECT_EQ(response_str.find("admin123"), String::npos);
}

TEST_F(AuthServiceTest, Security_TokenContainsNecessaryInfo) {
    auto login_result = auth_service_->login("admin", "admin123");
    ASSERT_TRUE(login_result.is_ok());

    String token = login_result.value()["access_token"].asString();
    auto validate_result = auth_service_->validate_token(token);

    ASSERT_TRUE(validate_result.is_ok());

    UserInfo user_info = validate_result.value();
    EXPECT_FALSE(user_info.access_key.empty());
    EXPECT_TRUE(user_info.is_admin);
}

TEST_F(AuthServiceTest, Security_DifferentTokensForDifferentLogins) {
    auto login1 = auth_service_->login("admin", "admin123");
    auto login2 = auth_service_->login("admin", "admin123");

    ASSERT_TRUE(login1.is_ok());
    ASSERT_TRUE(login2.is_ok());

    String token1 = login1.value()["access_token"].asString();
    String token2 = login2.value()["access_token"].asString();

    EXPECT_NE(token1, token2);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(AuthServiceTest, EdgeCase_VeryLongUsername) {
    String long_username(1000, 'a');

    auto result = auth_service_->login(long_username, "password");

    ASSERT_TRUE(result.is_err());
}

TEST_F(AuthServiceTest, EdgeCase_VeryLongPassword) {
    String long_password(1000, 'a');

    auto result = auth_service_->login("admin", long_password);

    ASSERT_TRUE(result.is_err());
}

TEST_F(AuthServiceTest, EdgeCase_UnicodeUsername) {
    auto result = auth_service_->login("админ", "password");

    // Should handle unicode gracefully (either accept or reject cleanly)
    EXPECT_TRUE(result.is_err());
}
