#include "console/utils/JWT.hpp"

#include "console/common/Types.hpp"

#include <chrono>
#include <gtest/gtest.h>

using namespace console;
using namespace console::utils;

class JWTTest : public ::testing::Test {
  protected:
    void SetUp() override {
        secret_ = "test-secret-key-for-jwt-testing";

        // Initialize JWT globally for tests
        JWT::initialize(secret_, "test-passphrase", "test-salt");

        user_info_.access_key = "testuser";
        user_info_.secret_key = "testsecret";
        user_info_.session_token = "session123";
        user_info_.account_name = "Test User";
        user_info_.is_admin = false;
        user_info_.created_at = std::chrono::system_clock::now();
    }

    String secret_;
    UserInfo user_info_;

    JWTClaims userinfo_to_claims(const UserInfo& user_info, std::chrono::seconds expiry) {
        JWTClaims claims;
        claims.subject = user_info.access_key;
        claims.issuer = "object-storage-console"; // Must match the issuer in JWT::validate_token
        claims.audience = "object-storage-api";
        claims.issued_at = std::chrono::system_clock::now();
        claims.expires_at = claims.issued_at + expiry;
        claims.account_name = user_info.account_name;
        claims.sts_session_token = user_info.session_token;
        claims.custom_fields["is_admin"] = user_info.is_admin ? "true" : "false";
        return claims;
    }
};

TEST_F(JWTTest, GenerateToken_Success) {
    auto claims = userinfo_to_claims(user_info_, std::chrono::hours(1));
    auto result = JWT::generate_token(claims);

    ASSERT_TRUE(result.is_ok());
    auto token = result.value();
    EXPECT_FALSE(token.empty());
    EXPECT_GT(token.length(), 50); // JWT should be reasonably long
}

TEST_F(JWTTest, ValidateToken_Success) {
    // Generate token
    auto claims = userinfo_to_claims(user_info_, std::chrono::hours(1));
    auto token_result = JWT::generate_token(claims);
    ASSERT_TRUE(token_result.is_ok()) << "Failed to generate token: " << token_result.error();

    // Validate token
    auto result = JWT::validate_token(token_result.value());

    ASSERT_TRUE(result.is_ok()) << "Failed to validate token: " << result.error();
    auto validated_claims = result.value();
    EXPECT_EQ(validated_claims.subject, user_info_.access_key);
    EXPECT_EQ(validated_claims.account_name, user_info_.account_name);
    EXPECT_EQ(validated_claims.custom_fields["is_admin"], user_info_.is_admin ? "true" : "false");
}

TEST_F(JWTTest, ValidateToken_InvalidSecret) {
    auto claims = userinfo_to_claims(user_info_, std::chrono::hours(1));
    auto token_result = JWT::generate_token(claims);
    ASSERT_TRUE(token_result.is_ok());

    // Reinitialize with wrong secret
    JWT::initialize("wrong-secret", "test-passphrase", "test-salt");

    auto result = JWT::validate_token(token_result.value());

    EXPECT_TRUE(result.is_err());

    // Reinitialize with correct secret for other tests
    JWT::initialize(secret_, "test-passphrase", "test-salt");
}

TEST_F(JWTTest, ValidateToken_ExpiredToken) {
    // Generate token with very short expiry
    auto claims = userinfo_to_claims(user_info_, std::chrono::seconds(1));
    auto token_result = JWT::generate_token(claims);
    ASSERT_TRUE(token_result.is_ok());

    // Wait for token to expire
    std::this_thread::sleep_for(std::chrono::seconds(2));

    auto result = JWT::validate_token(token_result.value());

    EXPECT_TRUE(result.is_err());
}

TEST_F(JWTTest, ValidateToken_MalformedToken) {
    auto result = JWT::validate_token("malformed.token.here");

    EXPECT_TRUE(result.is_err());
}

TEST_F(JWTTest, GenerateAndValidate_AdminUser) {
    user_info_.is_admin = true;

    auto claims = userinfo_to_claims(user_info_, std::chrono::hours(1));
    auto token_result = JWT::generate_token(claims);
    ASSERT_TRUE(token_result.is_ok());

    auto result = JWT::validate_token(token_result.value());

    ASSERT_TRUE(result.is_ok());
    auto validated_claims = result.value();
    EXPECT_EQ(validated_claims.custom_fields["is_admin"], "true");
}
