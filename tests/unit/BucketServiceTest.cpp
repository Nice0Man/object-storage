#include "console/services/BucketService.hpp"

#include "console/clients/StorageClient.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace console;
using namespace console::services;
using namespace console::clients;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

// Mock StorageClient
class MockStorageClient : public IStorageClient {
  public:
    MOCK_METHOD((Result<bool, String>), is_connected, (), (override));
    MOCK_METHOD((Result<Vector<models::Bucket>, String>), list_buckets, (), (override));
    MOCK_METHOD((Result<models::Bucket, String>), get_bucket, (const String&), (override));
    MOCK_METHOD((Result<bool, String>), create_bucket, (const String&, const String&), (override));
    MOCK_METHOD((Result<bool, String>), delete_bucket, (const String&), (override));
    MOCK_METHOD((Result<bool, String>), bucket_exists, (const String&), (override));
    MOCK_METHOD((Result<models::ListObjectsResponse, String>),
                list_objects,
                (const String&, const ListObjectsOptions&),
                (override));
    MOCK_METHOD((Result<models::Object, String>), stat_object, (const String&, const String&), (override));
    MOCK_METHOD((Result<ByteArray, String>), get_object, (const String&, const String&), (override));
    MOCK_METHOD((Result<models::Object, String>),
                put_object,
                (const String&, const String&, const ByteArray&, const String&, const StringMap&),
                (override));
    MOCK_METHOD((Result<bool, String>), delete_object, (const String&, const String&), (override));
    MOCK_METHOD((Result<bool, String>),
                copy_object,
                (const String&, const String&, const String&, const String&),
                (override));
    MOCK_METHOD((Result<StringMap, String>), get_object_metadata, (const String&, const String&), (override));
    MOCK_METHOD((Result<bool, String>),
                set_object_metadata,
                (const String&, const String&, const StringMap&),
                (override));
    MOCK_METHOD((Result<StringMap, String>), get_object_tags, (const String&, const String&), (override));
    MOCK_METHOD((Result<bool, String>), set_object_tags, (const String&, const String&, const StringMap&), (override));
    MOCK_METHOD((Result<String, String>),
                generate_presigned_url,
                (const String&, const String&, int64_t, const String&),
                (override));
};

class BucketServiceTest : public ::testing::Test {
  protected:
    void SetUp() override {
        mock_client_ = std::make_shared<NiceMock<MockStorageClient>>();
        bucket_service_ = std::make_unique<BucketService>(mock_client_);

        user_info_.access_key = "testuser";
        user_info_.secret_key = "testsecret";
        user_info_.is_admin = false;

        admin_info_.access_key = "admin";
        admin_info_.secret_key = "adminsecret";
        admin_info_.is_admin = true;
    }

    std::shared_ptr<MockStorageClient> mock_client_;
    std::unique_ptr<BucketService> bucket_service_;
    UserInfo user_info_;
    UserInfo admin_info_;
};

// ============================================================================
// List Buckets Tests
// ============================================================================

TEST_F(BucketServiceTest, ListBuckets_Success) {
    Vector<models::Bucket> expected_buckets;
    BucketInfo info1;
    info1.name = "bucket1";
    info1.creation_date = std::chrono::system_clock::now();
    expected_buckets.push_back(models::Bucket(info1));

    BucketInfo info2;
    info2.name = "bucket2";
    info2.creation_date = std::chrono::system_clock::now();
    expected_buckets.push_back(models::Bucket(info2));

    EXPECT_CALL(*mock_client_, list_buckets()).WillOnce(Return(Ok<Vector<models::Bucket>, String>(expected_buckets)));

    auto result = bucket_service_->list_buckets(user_info_);

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().size(), 2);
    EXPECT_EQ(result.value()[0].name(), "bucket1");
    EXPECT_EQ(result.value()[1].name(), "bucket2");
}

TEST_F(BucketServiceTest, ListBuckets_EmptyList) {
    Vector<models::Bucket> empty_buckets;

    EXPECT_CALL(*mock_client_, list_buckets()).WillOnce(Return(Ok<Vector<models::Bucket>, String>(empty_buckets)));

    auto result = bucket_service_->list_buckets(user_info_);

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().size(), 0);
}

TEST_F(BucketServiceTest, ListBuckets_ClientError) {
    EXPECT_CALL(*mock_client_, list_buckets())
        .WillOnce(Return(Err<Vector<models::Bucket>, String>("Connection failed")));

    auto result = bucket_service_->list_buckets(user_info_);

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error().status(), HttpStatus::InternalServerError);
}

// ============================================================================
// Create Bucket Tests
// ============================================================================

TEST_F(BucketServiceTest, CreateBucket_Success) {
    BucketInfo info;
    info.name = "valid-bucket-name";
    info.region = "us-east-1";
    models::Bucket expected_bucket(info);

    EXPECT_CALL(*mock_client_, create_bucket("valid-bucket-name", "us-east-1"))
        .WillOnce(Return(Ok<bool, String>(true)));

    EXPECT_CALL(*mock_client_, get_bucket("valid-bucket-name"))
        .WillOnce(Return(Ok<models::Bucket, String>(expected_bucket)));

    auto result = bucket_service_->create_bucket(user_info_, "valid-bucket-name", "us-east-1", false);

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().name(), "valid-bucket-name");
}

TEST_F(BucketServiceTest, CreateBucket_TooShort) {
    auto result = bucket_service_->create_bucket(user_info_, "ab", "us-east-1", false);

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error().status(), HttpStatus::BadRequest);
}

TEST_F(BucketServiceTest, CreateBucket_TooLong) {
    String long_name(64, 'a');

    auto result = bucket_service_->create_bucket(user_info_, long_name, "us-east-1", false);

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error().status(), HttpStatus::BadRequest);
}

TEST_F(BucketServiceTest, CreateBucket_InvalidCharacters) {
    auto result = bucket_service_->create_bucket(user_info_, "INVALID_BUCKET", "us-east-1", false);

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error().status(), HttpStatus::BadRequest);
}

TEST_F(BucketServiceTest, CreateBucket_StartsWithDash) {
    auto result = bucket_service_->create_bucket(user_info_, "-invalid", "us-east-1", false);

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error().status(), HttpStatus::BadRequest);
}

TEST_F(BucketServiceTest, CreateBucket_IPAddress) {
    auto result = bucket_service_->create_bucket(user_info_, "192.168.1.1", "us-east-1", false);

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error().status(), HttpStatus::BadRequest);
}

TEST_F(BucketServiceTest, CreateBucket_AlreadyExists) {
    EXPECT_CALL(*mock_client_, create_bucket("existing-bucket", "us-east-1"))
        .WillOnce(Return(Err<bool, String>("Bucket already exists")));

    auto result = bucket_service_->create_bucket(user_info_, "existing-bucket", "us-east-1", false);

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error().status(), HttpStatus::InternalServerError);
}

// ============================================================================
// Get Bucket Tests
// ============================================================================

TEST_F(BucketServiceTest, GetBucket_Success) {
    BucketInfo info;
    info.name = "test-bucket";
    info.region = "us-east-1";
    models::Bucket expected_bucket(info);

    EXPECT_CALL(*mock_client_, get_bucket("test-bucket")).WillOnce(Return(Ok<models::Bucket, String>(expected_bucket)));

    auto result = bucket_service_->get_bucket_info(user_info_, "test-bucket");

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().name(), "test-bucket");
}

TEST_F(BucketServiceTest, GetBucket_NotFound) {
    EXPECT_CALL(*mock_client_, get_bucket("nonexistent"))
        .WillOnce(Return(Err<models::Bucket, String>("Bucket not found")));

    auto result = bucket_service_->get_bucket_info(user_info_, "nonexistent");

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error().status(), HttpStatus::InternalServerError);
}

// ============================================================================
// Delete Bucket Tests
// ============================================================================

TEST_F(BucketServiceTest, DeleteBucket_Success) {
    EXPECT_CALL(*mock_client_, delete_bucket("test-bucket")).WillOnce(Return(Ok<bool, String>(true)));

    auto result = bucket_service_->delete_bucket(user_info_, "test-bucket");

    EXPECT_TRUE(result.is_ok());
}

TEST_F(BucketServiceTest, DeleteBucket_NotEmpty) {
    EXPECT_CALL(*mock_client_, delete_bucket("test-bucket")).WillOnce(Return(Err<bool, String>("Bucket not empty")));

    auto result = bucket_service_->delete_bucket(user_info_, "test-bucket");

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().status(), HttpStatus::InternalServerError);
}

TEST_F(BucketServiceTest, DeleteBucket_NotFound) {
    EXPECT_CALL(*mock_client_, delete_bucket("nonexistent")).WillOnce(Return(Err<bool, String>("Bucket not found")));

    auto result = bucket_service_->delete_bucket(user_info_, "nonexistent");

    EXPECT_TRUE(result.is_err());
}

// ============================================================================
// Bucket Existence Tests
// ============================================================================

TEST_F(BucketServiceTest, BucketExists_True) {
    EXPECT_CALL(*mock_client_, bucket_exists("test-bucket")).WillOnce(Return(Ok<bool, String>(true)));

    auto result = bucket_service_->bucket_exists(user_info_, "test-bucket");

    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(result.value());
}

TEST_F(BucketServiceTest, BucketExists_False) {
    EXPECT_CALL(*mock_client_, bucket_exists("nonexistent")).WillOnce(Return(Ok<bool, String>(false)));

    auto result = bucket_service_->bucket_exists(user_info_, "nonexistent");

    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value());
}

// ============================================================================
// Bucket Name Validation Tests
// ============================================================================

TEST_F(BucketServiceTest, ValidateBucketName_ValidNames) {
    Vector<String> valid_names = {"mybucket", "my-bucket", "my.bucket", "my-bucket-123", "bucket123", "a-b-c"};

    for (const auto& name : valid_names) {
        BucketInfo info;
        info.name = name;
        models::Bucket expected_bucket(info);

        EXPECT_CALL(*mock_client_, create_bucket(name, "")).WillOnce(Return(Ok<bool, String>(true)));
        EXPECT_CALL(*mock_client_, get_bucket(name)).WillOnce(Return(Ok<models::Bucket, String>(expected_bucket)));

        auto result = bucket_service_->create_bucket(user_info_, name, "", false);
        EXPECT_TRUE(result.is_ok()) << "Name: " << name << " should be valid";
    }
}

TEST_F(BucketServiceTest, ValidateBucketName_InvalidNames) {
    Vector<String> invalid_names = {
        "ab",            // too short
        String(64, 'a'), // too long
        "MyBucket",      // uppercase
        "my_bucket",     // underscore
        "-mybucket",     // starts with dash
        "mybucket-",     // ends with dash
        ".mybucket",     // starts with dot
        "my..bucket",    // consecutive dots
        "192.168.1.1",   // IP address
        "my bucket"      // space
    };

    for (const auto& name : invalid_names) {
        auto result = bucket_service_->create_bucket(user_info_, name, "", false);
        EXPECT_TRUE(result.is_err()) << "Name: " << name << " should be invalid";
        EXPECT_EQ(result.error().status(), HttpStatus::BadRequest);
    }
}
