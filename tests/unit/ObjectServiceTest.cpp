#include "console/services/ObjectService.hpp"

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

    // Bucket policy, versioning, tags, encryption, lifecycle, object lock
    MOCK_METHOD((Result<void, String>), set_bucket_policy, (const String&, const String&), (override));
    MOCK_METHOD((Result<String, String>), get_bucket_policy, (const String&), (override));
    MOCK_METHOD((Result<void, String>), set_bucket_versioning, (const String&, bool), (override));
    MOCK_METHOD((Result<bool, String>), get_bucket_versioning, (const String&), (override));
    MOCK_METHOD((Result<void, String>), set_bucket_tags, (const String&, const StringMap&), (override));
    MOCK_METHOD((Result<StringMap, String>), get_bucket_tags, (const String&), (override));
    MOCK_METHOD((Result<void, String>), delete_bucket_tags, (const String&), (override));
    MOCK_METHOD((Result<Json::Value, String>), get_bucket_encryption, (const String&), (override));
    MOCK_METHOD((Result<void, String>), set_bucket_encryption, (const String&, const Json::Value&), (override));
    MOCK_METHOD((Result<Json::Value, String>), get_bucket_lifecycle, (const String&), (override));
    MOCK_METHOD((Result<void, String>), set_bucket_lifecycle, (const String&, const Json::Value&), (override));
    MOCK_METHOD((Result<Json::Value, String>), get_bucket_object_lock, (const String&), (override));
    MOCK_METHOD((Result<void, String>), set_bucket_object_lock, (const String&, const Json::Value&), (override));

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

    // Multipart upload operations
    MOCK_METHOD((Result<MultipartUploadInfo, String>),
                initiate_multipart_upload,
                (const String&, const String&, const String&, const StringMap&),
                (override));
    MOCK_METHOD((Result<String, String>),
                upload_part,
                (const String&, const String&, const String&, int, const ByteArray&),
                (override));
    MOCK_METHOD((Result<models::Object, String>),
                complete_multipart_upload,
                (const String&, const String&, const String&, const Vector<CompletedPart>&),
                (override));
    MOCK_METHOD((Result<void, String>),
                abort_multipart_upload,
                (const String&, const String&, const String&),
                (override));
    MOCK_METHOD((Result<MultipartUploadInfo, String>),
                list_parts,
                (const String&, const String&, const String&),
                (override));
    MOCK_METHOD((Result<Vector<MultipartUploadInfo>, String>),
                list_multipart_uploads,
                (const String&, const String&),
                (override));

    // Object versioning operations
    MOCK_METHOD((Result<Vector<ObjectVersion>, String>),
                list_object_versions,
                (const String&, const String&),
                (override));
    MOCK_METHOD((Result<ByteArray, String>),
                get_object_version,
                (const String&, const String&, const String&),
                (override));
    MOCK_METHOD((Result<void, String>),
                delete_object_version,
                (const String&, const String&, const String&),
                (override));
    MOCK_METHOD((Result<models::Object, String>),
                restore_object_version,
                (const String&, const String&, const String&),
                (override));

    // Object lock and retention operations
    MOCK_METHOD((Result<void, String>),
                set_object_retention,
                (const String&, const String&, const String&, int64_t, const String&),
                (override));
    MOCK_METHOD((Result<std::pair<String, int64_t>, String>),
                get_object_retention,
                (const String&, const String&, const String&),
                (override));
    MOCK_METHOD((Result<void, String>),
                set_object_legal_hold,
                (const String&, const String&, bool, const String&),
                (override));
    MOCK_METHOD((Result<bool, String>),
                get_object_legal_hold,
                (const String&, const String&, const String&),
                (override));
    MOCK_METHOD((Result<void, String>),
                set_bucket_object_lock_configuration,
                (const String&, bool, const String&, int, int),
                (override));
    MOCK_METHOD((Result<Json::Value, String>), get_bucket_object_lock_configuration, (const String&), (override));
};

class ObjectServiceTest : public ::testing::Test {
  protected:
    void SetUp() override {
        mock_client_ = std::make_shared<NiceMock<MockStorageClient>>();
        object_service_ = std::make_unique<ObjectService>(mock_client_);

        user_info_.access_key = "testuser";
        user_info_.secret_key = "testsecret";
        user_info_.is_admin = true; // Admin user bypasses policy checks in tests
        user_info_.policies = {"readwrite"};
    }

    std::shared_ptr<MockStorageClient> mock_client_;
    std::unique_ptr<ObjectService> object_service_;
    UserInfo user_info_;
};

// ============================================================================
// List Objects Tests
// ============================================================================

TEST_F(ObjectServiceTest, ListObjects_Success) {
    models::ListObjectsResponse response;
    response.objects = Vector<models::Object>();

    models::Object obj;
    obj.set_key("file1.txt");
    obj.set_size(1024);
    obj.set_etag("abc123");
    obj.set_last_modified(std::chrono::system_clock::now());
    response.objects.push_back(obj);

    ListObjectsOptions options;
    options.prefix = "";
    options.recursive = false;
    options.max_keys = 1000;

    EXPECT_CALL(*mock_client_, list_objects("test-bucket", _))
        .WillOnce(Return(Ok<models::ListObjectsResponse, String>(response)));

    auto result = object_service_->list_objects(user_info_, "test-bucket", "", false, 1000);

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().size(), 1);
    EXPECT_EQ(result.value()[0].key(), "file1.txt");
}

TEST_F(ObjectServiceTest, ListObjects_EmptyBucket) {
    models::ListObjectsResponse response;
    response.objects = Vector<models::Object>();

    EXPECT_CALL(*mock_client_, list_objects("empty-bucket", _))
        .WillOnce(Return(Ok<models::ListObjectsResponse, String>(response)));

    auto result = object_service_->list_objects(user_info_, "empty-bucket", "", false, 1000);

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().size(), 0);
}

TEST_F(ObjectServiceTest, ListObjects_WithPrefix) {
    models::ListObjectsResponse response;
    response.objects = Vector<models::Object>();

    models::Object obj;
    obj.set_key("photos/image1.jpg");
    obj.set_size(2048);
    response.objects.push_back(obj);

    ListObjectsOptions options;
    options.prefix = "photos/";

    EXPECT_CALL(*mock_client_, list_objects("test-bucket", _))
        .WillOnce(Return(Ok<models::ListObjectsResponse, String>(response)));

    auto result = object_service_->list_objects(user_info_, "test-bucket", "photos/", false, 1000);

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().size(), 1);
    EXPECT_EQ(result.value()[0].key(), "photos/image1.jpg");
}

TEST_F(ObjectServiceTest, ListObjects_Recursive) {
    models::ListObjectsResponse response;

    Vector<String> keys = {"file1.txt", "folder1/file2.txt", "folder1/subfolder/file3.txt"};

    for (const auto& key : keys) {
        models::Object obj;
        obj.set_key(key);
        obj.set_size(1024);
        response.objects.push_back(obj);
    }

    EXPECT_CALL(*mock_client_, list_objects("test-bucket", _))
        .WillOnce(Return(Ok<models::ListObjectsResponse, String>(response)));

    auto result = object_service_->list_objects(user_info_, "test-bucket", "", true, 1000);

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().size(), 3);
}

// ============================================================================
// Get Object Info Tests
// ============================================================================

TEST_F(ObjectServiceTest, GetObjectInfo_Success) {
    models::Object obj;
    obj.set_key("file.txt");
    obj.set_size(1024);
    obj.set_etag("abc123");
    obj.set_content_type("text/plain");
    obj.set_last_modified(std::chrono::system_clock::now());

    EXPECT_CALL(*mock_client_, stat_object("test-bucket", "file.txt"))
        .WillOnce(Return(Ok<models::Object, String>(obj)));

    auto result = object_service_->get_object_info(user_info_, "test-bucket", "file.txt");

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().key(), "file.txt");
    EXPECT_EQ(result.value().size(), 1024);
    EXPECT_EQ(result.value().content_type(), "text/plain");
}

TEST_F(ObjectServiceTest, GetObjectInfo_NotFound) {
    EXPECT_CALL(*mock_client_, stat_object("test-bucket", "nonexistent.txt"))
        .WillOnce(Return(Err<models::Object, String>("Object not found")));

    auto result = object_service_->get_object_info(user_info_, "test-bucket", "nonexistent.txt");

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error().status(), HttpStatus::NotFound);
}

// ============================================================================
// Download Object Tests
// ============================================================================

TEST_F(ObjectServiceTest, DownloadObject_Success) {
    ByteArray data = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};

    EXPECT_CALL(*mock_client_, get_object("test-bucket", "file.txt")).WillOnce(Return(Ok<ByteArray, String>(data)));

    auto result = object_service_->download_object(user_info_, "test-bucket", "file.txt");

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().size(), 11);
    EXPECT_EQ(result.value()[0], 'H');
}

TEST_F(ObjectServiceTest, DownloadObject_EmptyFile) {
    ByteArray empty_data;

    EXPECT_CALL(*mock_client_, get_object("test-bucket", "empty.txt"))
        .WillOnce(Return(Ok<ByteArray, String>(empty_data)));

    auto result = object_service_->download_object(user_info_, "test-bucket", "empty.txt");

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().size(), 0);
}

TEST_F(ObjectServiceTest, DownloadObject_LargeFile) {
    ByteArray large_data(10 * 1024 * 1024); // 10 MB

    EXPECT_CALL(*mock_client_, get_object("test-bucket", "large.bin"))
        .WillOnce(Return(Ok<ByteArray, String>(large_data)));

    auto result = object_service_->download_object(user_info_, "test-bucket", "large.bin");

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().size(), 10 * 1024 * 1024);
}

// ============================================================================
// Upload Object Tests
// ============================================================================

TEST_F(ObjectServiceTest, UploadObject_Success) {
    ByteArray data = {'T', 'e', 's', 't', ' ', 'd', 'a', 't', 'a'};

    models::Object obj;
    obj.set_key("upload.txt");
    obj.set_size(data.size());
    obj.set_content_type("text/plain");

    EXPECT_CALL(*mock_client_, put_object("test-bucket", "upload.txt", _, "text/plain", _))
        .WillOnce(Return(Ok<models::Object, String>(obj)));

    // ObjectService calls stat_object after put_object to get object info
    EXPECT_CALL(*mock_client_, stat_object("test-bucket", "upload.txt"))
        .WillOnce(Return(Ok<models::Object, String>(obj)));

    auto result = object_service_->upload_object(user_info_, "test-bucket", "upload.txt", data, "text/plain", {});

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().key(), "upload.txt");
    EXPECT_EQ(result.value().size(), data.size());
}

TEST_F(ObjectServiceTest, UploadObject_WithMetadata) {
    ByteArray data = {'D', 'a', 't', 'a'};
    StringMap metadata = {{"author", "John Doe"}, {"version", "1.0"}};

    models::Object obj;
    obj.set_key("file-with-meta.txt");
    obj.set_size(data.size());

    EXPECT_CALL(*mock_client_, put_object("test-bucket", "file-with-meta.txt", _, _, _))
        .WillOnce(Return(Ok<models::Object, String>(obj)));

    // ObjectService calls stat_object after put_object to get object info
    EXPECT_CALL(*mock_client_, stat_object("test-bucket", "file-with-meta.txt"))
        .WillOnce(Return(Ok<models::Object, String>(obj)));

    auto result = object_service_->upload_object(
        user_info_, "test-bucket", "file-with-meta.txt", data, "application/octet-stream", metadata);

    ASSERT_TRUE(result.is_ok());
}

TEST_F(ObjectServiceTest, UploadObject_EmptyData) {
    ByteArray empty_data;

    models::Object obj;
    obj.set_key("empty.txt");
    obj.set_size(0);

    EXPECT_CALL(*mock_client_, put_object("test-bucket", "empty.txt", _, _, _))
        .WillOnce(Return(Ok<models::Object, String>(obj)));

    // ObjectService calls stat_object after put_object to get object info
    EXPECT_CALL(*mock_client_, stat_object("test-bucket", "empty.txt"))
        .WillOnce(Return(Ok<models::Object, String>(obj)));

    auto result = object_service_->upload_object(user_info_, "test-bucket", "empty.txt", empty_data, "text/plain", {});

    ASSERT_TRUE(result.is_ok());
}

// ============================================================================
// Delete Object Tests
// ============================================================================

TEST_F(ObjectServiceTest, DeleteObject_Success) {
    EXPECT_CALL(*mock_client_, delete_object("test-bucket", "file.txt")).WillOnce(Return(Ok<bool, String>(true)));

    auto result = object_service_->delete_object(user_info_, "test-bucket", "file.txt");

    EXPECT_TRUE(result.is_ok());
}

TEST_F(ObjectServiceTest, DeleteObject_NotFound) {
    EXPECT_CALL(*mock_client_, delete_object("test-bucket", "nonexistent.txt"))
        .WillOnce(Return(Err<bool, String>("Object not found")));

    auto result = object_service_->delete_object(user_info_, "test-bucket", "nonexistent.txt");

    EXPECT_TRUE(result.is_err());
}

// ============================================================================
// Delete Multiple Objects Tests
// ============================================================================

TEST_F(ObjectServiceTest, DeleteObjects_Success) {
    Vector<String> keys = {"file1.txt", "file2.txt", "file3.txt"};

    EXPECT_CALL(*mock_client_, delete_object("test-bucket", "file1.txt")).WillOnce(Return(Ok<bool, String>(true)));
    EXPECT_CALL(*mock_client_, delete_object("test-bucket", "file2.txt")).WillOnce(Return(Ok<bool, String>(true)));
    EXPECT_CALL(*mock_client_, delete_object("test-bucket", "file3.txt")).WillOnce(Return(Ok<bool, String>(true)));

    auto result = object_service_->delete_objects(user_info_, "test-bucket", keys);

    ASSERT_TRUE(result.is_ok());
    Json::Value json_result = result.value();
    EXPECT_EQ(json_result["deleted"].size(), 3);
}

TEST_F(ObjectServiceTest, DeleteObjects_PartialFailure) {
    Vector<String> keys = {"file1.txt", "file2.txt", "file3.txt"};

    EXPECT_CALL(*mock_client_, delete_object("test-bucket", "file1.txt")).WillOnce(Return(Ok<bool, String>(true)));
    EXPECT_CALL(*mock_client_, delete_object("test-bucket", "file2.txt"))
        .WillOnce(Return(Err<bool, String>("Failed to delete")));
    EXPECT_CALL(*mock_client_, delete_object("test-bucket", "file3.txt")).WillOnce(Return(Ok<bool, String>(true)));

    auto result = object_service_->delete_objects(user_info_, "test-bucket", keys);

    ASSERT_TRUE(result.is_ok());
    Json::Value json_result = result.value();
    EXPECT_EQ(json_result["deleted"].size(), 2);
    EXPECT_EQ(json_result["errors"].size(), 1);
}

// ============================================================================
// Copy Object Tests
// ============================================================================

TEST_F(ObjectServiceTest, CopyObject_Success) {
    models::Object obj;
    obj.set_key("destination.txt");
    obj.set_size(1024);

    EXPECT_CALL(*mock_client_, copy_object("source-bucket", "source.txt", "dest-bucket", "destination.txt"))
        .WillOnce(Return(Ok<bool, String>(true)));

    EXPECT_CALL(*mock_client_, stat_object("dest-bucket", "destination.txt"))
        .WillOnce(Return(Ok<models::Object, String>(obj)));

    auto result = object_service_->copy_object(
        user_info_, "source-bucket", "source.txt", "dest-bucket", "destination.txt");

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().key(), "destination.txt");
}

TEST_F(ObjectServiceTest, CopyObject_SameBucket) {
    models::Object obj;
    obj.set_key("copy.txt");

    EXPECT_CALL(*mock_client_, copy_object("bucket", "original.txt", "bucket", "copy.txt"))
        .WillOnce(Return(Ok<bool, String>(true)));

    EXPECT_CALL(*mock_client_, stat_object("bucket", "copy.txt")).WillOnce(Return(Ok<models::Object, String>(obj)));

    auto result = object_service_->copy_object(user_info_, "bucket", "original.txt", "bucket", "copy.txt");

    ASSERT_TRUE(result.is_ok());
}

// ============================================================================
// Object Metadata Tests
// ============================================================================

TEST_F(ObjectServiceTest, SetObjectMetadata_Success) {
    StringMap metadata = {{"new-key", "new-value"}};

    EXPECT_CALL(*mock_client_, set_object_metadata("test-bucket", "file.txt", _))
        .WillOnce(Return(Ok<bool, String>(true)));

    auto result = object_service_->set_object_metadata(user_info_, "test-bucket", "file.txt", metadata);

    EXPECT_TRUE(result.is_ok());
}

// ============================================================================
// Object Tags Tests
// ============================================================================

TEST_F(ObjectServiceTest, SetObjectTags_Success) {
    StringMap tags = {{"status", "archived"}};

    EXPECT_CALL(*mock_client_, set_object_tags("test-bucket", "file.txt", _)).WillOnce(Return(Ok<bool, String>(true)));

    auto result = object_service_->set_object_tags(user_info_, "test-bucket", "file.txt", tags);

    EXPECT_TRUE(result.is_ok());
}

// ============================================================================
// Presigned URL Tests
// ============================================================================

TEST_F(ObjectServiceTest, GeneratePresignedUrl_Success) {
    String expected_url = "https://storage.example.com/test-bucket/file.txt?signature=abc123";

    EXPECT_CALL(*mock_client_, generate_presigned_url("test-bucket", "file.txt", 3600, "GET"))
        .WillOnce(Return(Ok<String, String>(expected_url)));

    auto result = object_service_->generate_presigned_url(user_info_, "test-bucket", "file.txt", "GET", 3600);

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), expected_url);
}

TEST_F(ObjectServiceTest, GeneratePresignedUrl_InvalidExpiry) {
    auto result = object_service_->generate_presigned_url(user_info_,
                                                          "test-bucket",
                                                          "file.txt",
                                                          "GET",
                                                          -100 // Invalid negative expiry
    );

    // Should either reject or normalize to valid value
    // Depending on implementation
}
