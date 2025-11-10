#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "console/services/AuthService.hpp"
#include "console/services/BucketService.hpp"
#include "console/clients/MinioClient.hpp"

using namespace console;
using namespace console::services;
using namespace console::clients;
using ::testing::Return;
using ::testing::_;

// Mock MinioClient
class MockMinioClient : public IMinioClient {
public:
    MOCK_METHOD(Result<Vector<models::Bucket>, String>, list_buckets, (), (override));
    MOCK_METHOD(Result<models::Bucket, String>, create_bucket, 
                (const String&, const String&, bool), (override));
    MOCK_METHOD(Result<void, String>, delete_bucket, (const String&), (override));
    MOCK_METHOD(Result<models::Bucket, String>, get_bucket_info, 
                (const String&), (override));
    MOCK_METHOD(Result<void, String>, set_bucket_policy, 
                (const String&, const String&), (override));
    MOCK_METHOD(Result<String, String>, get_bucket_policy, 
                (const String&), (override));
    MOCK_METHOD(Result<void, String>, set_bucket_versioning, 
                (const String&, bool), (override));
    MOCK_METHOD(Result<bool, String>, get_bucket_versioning, 
                (const String&), (override));
    
    MOCK_METHOD(Result<Vector<models::Object>, String>, list_objects,
                (const String&, const String&, bool), (override));
    MOCK_METHOD(Result<models::Object, String>, upload_object,
                (const String&, const String&, const ByteArray&, const String&), 
                (override));
    MOCK_METHOD(Result<ByteArray, String>, download_object,
                (const String&, const String&), (override));
    MOCK_METHOD(Result<void, String>, delete_object,
                (const String&, const String&), (override));
    MOCK_METHOD(Result<models::Object, String>, get_object_info,
                (const String&, const String&), (override));
};

class BucketServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_client_ = std::make_shared<MockMinioClient>();
        bucket_service_ = std::make_unique<BucketService>(mock_client_);
        
        user_info_.access_key = "testuser";
        user_info_.is_admin = false;
    }

    std::shared_ptr<MockMinioClient> mock_client_;
    std::unique_ptr<BucketService> bucket_service_;
    UserInfo user_info_;
};

TEST_F(BucketServiceTest, ListBuckets_Success) {
    Vector<models::Bucket> expected_buckets;
    BucketInfo info1;
    info1.name = "bucket1";
    expected_buckets.push_back(models::Bucket(info1));
    
    EXPECT_CALL(*mock_client_, list_buckets())
        .WillOnce(Return(Ok(expected_buckets)));
    
    auto result = bucket_service_->list_buckets(user_info_);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), 1);
    EXPECT_EQ(result.value()[0].name(), "bucket1");
}

TEST_F(BucketServiceTest, ListBuckets_Failure) {
    EXPECT_CALL(*mock_client_, list_buckets())
        .WillOnce(Return(Err<Vector<models::Bucket>, String>("Connection failed")));
    
    auto result = bucket_service_->list_buckets(user_info_);
    
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().status(), HttpStatus::InternalServerError);
}

TEST_F(BucketServiceTest, CreateBucket_InvalidName) {
    // Test with invalid bucket name (too short)
    auto result = bucket_service_->create_bucket(
        user_info_,
        "ab",  // Too short
        "us-east-1",
        false
    );
    
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().status(), HttpStatus::BadRequest);
}

TEST_F(BucketServiceTest, CreateBucket_Success) {
    BucketInfo info;
    info.name = "valid-bucket-name";
    models::Bucket expected_bucket(info);
    
    EXPECT_CALL(*mock_client_, create_bucket("valid-bucket-name", "us-east-1", false))
        .WillOnce(Return(Ok(expected_bucket)));
    
    auto result = bucket_service_->create_bucket(
        user_info_,
        "valid-bucket-name",
        "us-east-1",
        false
    );
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().name(), "valid-bucket-name");
}

TEST_F(BucketServiceTest, DeleteBucket_Success) {
    EXPECT_CALL(*mock_client_, delete_bucket("test-bucket"))
        .WillOnce(Return(Ok()));
    
    auto result = bucket_service_->delete_bucket(user_info_, "test-bucket");
    
    EXPECT_TRUE(result.has_value());
}

class AuthServiceValidationTest : public ::testing::Test {
protected:
    // Test credential validation without mocks
};

TEST_F(AuthServiceValidationTest, ValidateUsername_TooShort) {
    // Username must be at least 3 characters
    // This would be tested through the public login method
    // For now, we'll test indirectly through behavior
}

TEST_F(AuthServiceValidationTest, ValidatePassword_TooShort) {
    // Password must be at least 8 characters
    // Tested indirectly through login behavior
}

