#include <gtest/gtest.h>
#include "console/utils/JWT.hpp"
#include "console/common/Types.hpp"
#include <chrono>

using namespace console;
using namespace console::utils;

class JWTTest : public ::testing::Test {
protected:
    void SetUp() override {
        secret_ = "test-secret-key-for-jwt-testing";
        
        user_info_.access_key = "testuser";
        user_info_.secret_key = "testsecret";
        user_info_.session_token = "session123";
        user_info_.account_name = "Test User";
        user_info_.is_admin = false;
        user_info_.created_at = std::chrono::system_clock::now();
    }

    String secret_;
    UserInfo user_info_;
};

TEST_F(JWTTest, GenerateToken_Success) {
    auto token = JWT::generate_token(
        user_info_,
        secret_,
        std::chrono::hours(1)
    );
    
    EXPECT_FALSE(token.empty());
    EXPECT_GT(token.length(), 50);  // JWT should be reasonably long
}

TEST_F(JWTTest, ValidateToken_Success) {
    // Generate token
    auto token = JWT::generate_token(
        user_info_,
        secret_,
        std::chrono::hours(1)
    );
    
    // Validate token
    auto result = JWT::validate_token(token, secret_);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->access_key, user_info_.access_key);
    EXPECT_EQ(result->account_name, user_info_.account_name);
    EXPECT_EQ(result->is_admin, user_info_.is_admin);
}

TEST_F(JWTTest, ValidateToken_InvalidSecret) {
    auto token = JWT::generate_token(
        user_info_,
        secret_,
        std::chrono::hours(1)
    );
    
    auto result = JWT::validate_token(token, "wrong-secret");
    
    EXPECT_FALSE(result.has_value());
}

TEST_F(JWTTest, ValidateToken_ExpiredToken) {
    // Generate token with very short expiry
    auto token = JWT::generate_token(
        user_info_,
        secret_,
        std::chrono::seconds(1)
    );
    
    // Wait for token to expire
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    auto result = JWT::validate_token(token, secret_);
    
    EXPECT_FALSE(result.has_value());
}

TEST_F(JWTTest, ValidateToken_MalformedToken) {
    auto result = JWT::validate_token("malformed.token.here", secret_);
    
    EXPECT_FALSE(result.has_value());
}

TEST_F(JWTTest, GenerateAndValidate_AdminUser) {
    user_info_.is_admin = true;
    
    auto token = JWT::generate_token(
        user_info_,
        secret_,
        std::chrono::hours(1)
    );
    
    auto result = JWT::validate_token(token, secret_);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->is_admin);
}

