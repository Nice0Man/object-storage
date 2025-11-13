#include "console/clients/LocalStorageClient.hpp"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

using namespace console;
using namespace console::clients;
namespace fs = std::filesystem;

class LocalStorageClientTest : public ::testing::Test {
  protected:
    void SetUp() override {
        test_storage_root_ = "/tmp/test_storage_" + std::to_string(time(nullptr));

        // Clean up if exists
        if (fs::exists(test_storage_root_)) {
            fs::remove_all(test_storage_root_);
        }

        client_ = std::make_unique<LocalStorageClient>(test_storage_root_);
    }

    void TearDown() override {
        // Clean up test directory
        if (fs::exists(test_storage_root_)) {
            fs::remove_all(test_storage_root_);
        }
    }

    std::unique_ptr<LocalStorageClient> client_;
    String test_storage_root_;
};

// ============================================================================
// Connection Tests
// ============================================================================

TEST_F(LocalStorageClientTest, IsConnected_AlwaysTrue) {
    auto result = client_->is_connected();

    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(result.value());
}

// ============================================================================
// Bucket Operations Tests
// ============================================================================

TEST_F(LocalStorageClientTest, CreateBucket_Success) {
    auto result = client_->create_bucket("test-bucket", "us-east-1");

    ASSERT_TRUE(result.is_ok()) << "Failed to create bucket: " << (result.is_err() ? result.error() : "unknown");
    EXPECT_TRUE(result.value());

    // Verify directory was created (LocalStorageClient uses buckets/ subdirectory)
    EXPECT_TRUE(fs::exists(test_storage_root_ + "/buckets/test-bucket"));
}

TEST_F(LocalStorageClientTest, CreateBucket_AlreadyExists) {
    auto result1 = client_->create_bucket("existing-bucket", "");
    ASSERT_TRUE(result1.is_ok());

    auto result2 = client_->create_bucket("existing-bucket", "");

    // Should return error for duplicate
    ASSERT_TRUE(result2.is_err());
}

TEST_F(LocalStorageClientTest, ListBuckets_Empty) {
    auto result = client_->list_buckets();

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().size(), 0);
}

TEST_F(LocalStorageClientTest, ListBuckets_Multiple) {
    client_->create_bucket("bucket1", "");
    client_->create_bucket("bucket2", "");
    client_->create_bucket("bucket3", "");

    auto result = client_->list_buckets();

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().size(), 3);

    Vector<String> bucket_names;
    for (const auto& bucket : result.value()) {
        bucket_names.push_back(bucket.name());
    }

    EXPECT_NE(std::find(bucket_names.begin(), bucket_names.end(), "bucket1"), bucket_names.end());
    EXPECT_NE(std::find(bucket_names.begin(), bucket_names.end(), "bucket2"), bucket_names.end());
    EXPECT_NE(std::find(bucket_names.begin(), bucket_names.end(), "bucket3"), bucket_names.end());
}

TEST_F(LocalStorageClientTest, GetBucket_Success) {
    client_->create_bucket("test-bucket", "us-west-2");

    auto result = client_->get_bucket("test-bucket");

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().name(), "test-bucket");
}

TEST_F(LocalStorageClientTest, GetBucket_NotFound) {
    auto result = client_->get_bucket("nonexistent");

    ASSERT_TRUE(result.is_err());
}

TEST_F(LocalStorageClientTest, BucketExists_True) {
    client_->create_bucket("existing", "");

    auto result = client_->bucket_exists("existing");

    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(result.value());
}

TEST_F(LocalStorageClientTest, BucketExists_False) {
    auto result = client_->bucket_exists("nonexistent");

    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value());
}

TEST_F(LocalStorageClientTest, DeleteBucket_Empty) {
    client_->create_bucket("empty-bucket", "");

    auto result = client_->delete_bucket("empty-bucket");

    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(result.value());

    // Verify it's gone
    auto exists = client_->bucket_exists("empty-bucket");
    EXPECT_FALSE(exists.value());
}

TEST_F(LocalStorageClientTest, DeleteBucket_NotEmpty) {
    client_->create_bucket("nonempty-bucket", "");

    ByteArray data = {'t', 'e', 's', 't'};
    client_->put_object("nonempty-bucket", "file.txt", data, "text/plain", {});

    auto result = client_->delete_bucket("nonempty-bucket");

    // Should fail because bucket is not empty
    ASSERT_TRUE(result.is_err());
}

// ============================================================================
// Object Operations Tests
// ============================================================================

TEST_F(LocalStorageClientTest, PutObject_Success) {
    client_->create_bucket("test-bucket", "");

    ByteArray data = {'H', 'e', 'l', 'l', 'o'};

    auto result = client_->put_object("test-bucket", "hello.txt", data, "text/plain", {});

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().key(), "hello.txt");
    EXPECT_EQ(result.value().size(), 5);
}

TEST_F(LocalStorageClientTest, PutObject_WithMetadata) {
    client_->create_bucket("test-bucket", "");

    ByteArray data = {'D', 'a', 't', 'a'};
    StringMap metadata = {{"author", "John"}, {"version", "1.0"}};

    auto result = client_->put_object("test-bucket", "file.txt", data, "application/octet-stream", metadata);

    ASSERT_TRUE(result.is_ok());
}

TEST_F(LocalStorageClientTest, GetObject_Success) {
    client_->create_bucket("test-bucket", "");

    ByteArray original_data = {'T', 'e', 's', 't', ' ', 'd', 'a', 't', 'a'};
    client_->put_object("test-bucket", "test.txt", original_data, "text/plain", {});

    auto result = client_->get_object("test-bucket", "test.txt");

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), original_data);
}

TEST_F(LocalStorageClientTest, GetObject_NotFound) {
    client_->create_bucket("test-bucket", "");

    auto result = client_->get_object("test-bucket", "nonexistent.txt");

    ASSERT_TRUE(result.is_err());
}

TEST_F(LocalStorageClientTest, StatObject_Success) {
    client_->create_bucket("test-bucket", "");

    ByteArray data = {'D', 'a', 't', 'a'};
    client_->put_object("test-bucket", "file.txt", data, "text/plain", {});

    auto result = client_->stat_object("test-bucket", "file.txt");

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().key(), "file.txt");
    EXPECT_EQ(result.value().size(), 4);
}

TEST_F(LocalStorageClientTest, DeleteObject_Success) {
    client_->create_bucket("test-bucket", "");

    ByteArray data = {'D', 'a', 't', 'a'};
    client_->put_object("test-bucket", "delete-me.txt", data, "text/plain", {});

    auto result = client_->delete_object("test-bucket", "delete-me.txt");

    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(result.value());

    // Verify it's gone
    auto get_result = client_->get_object("test-bucket", "delete-me.txt");
    EXPECT_TRUE(get_result.is_err());
}

TEST_F(LocalStorageClientTest, ListObjects_Empty) {
    client_->create_bucket("empty-bucket", "");

    ListObjectsOptions options;
    auto result = client_->list_objects("empty-bucket", options);

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().objects.size(), 0);
}

TEST_F(LocalStorageClientTest, ListObjects_Multiple) {
    client_->create_bucket("test-bucket", "");

    ByteArray data = {'D', 'a', 't', 'a'};
    client_->put_object("test-bucket", "file1.txt", data, "text/plain", {});
    client_->put_object("test-bucket", "file2.txt", data, "text/plain", {});
    client_->put_object("test-bucket", "file3.txt", data, "text/plain", {});

    ListObjectsOptions options;
    auto result = client_->list_objects("test-bucket", options);

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().objects.size(), 3);
}

TEST_F(LocalStorageClientTest, ListObjects_WithPrefix) {
    client_->create_bucket("test-bucket", "");

    ByteArray data = {'D', 'a', 't', 'a'};
    client_->put_object("test-bucket", "photos/pic1.jpg", data, "image/jpeg", {});
    client_->put_object("test-bucket", "photos/pic2.jpg", data, "image/jpeg", {});
    client_->put_object("test-bucket", "documents/doc1.pdf", data, "application/pdf", {});

    ListObjectsOptions options;
    options.prefix = "photos/";
    auto result = client_->list_objects("test-bucket", options);

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().objects.size(), 2);

    for (const auto& obj : result.value().objects) {
        EXPECT_TRUE(obj.key().find("photos/") == 0);
    }
}

TEST_F(LocalStorageClientTest, CopyObject_Success) {
    auto bucket1 = client_->create_bucket("source-bucket", "");
    ASSERT_TRUE(bucket1.is_ok()) << "Failed to create source bucket: " << bucket1.error();

    auto bucket2 = client_->create_bucket("dest-bucket", "");
    ASSERT_TRUE(bucket2.is_ok()) << "Failed to create dest bucket: " << bucket2.error();

    ByteArray data = {'C', 'o', 'p', 'y', ' ', 'm', 'e'};
    auto put_result = client_->put_object("source-bucket", "original.txt", data, "text/plain", {});
    ASSERT_TRUE(put_result.is_ok()) << "Failed to put object: " << put_result.error();

    auto result = client_->copy_object("source-bucket", "original.txt", "dest-bucket", "copied.txt");

    ASSERT_TRUE(result.is_ok()) << "Failed to copy object: " << (result.is_err() ? result.error() : "unknown");

    // Verify copy exists
    auto get_result = client_->get_object("dest-bucket", "copied.txt");
    ASSERT_TRUE(get_result.is_ok()) << "Failed to get copied object: " << get_result.error();
    EXPECT_EQ(get_result.value(), data);
}

TEST_F(LocalStorageClientTest, CopyObject_SameBucket) {
    client_->create_bucket("bucket", "");

    ByteArray data = {'D', 'a', 't', 'a'};
    client_->put_object("bucket", "original.txt", data, "text/plain", {});

    auto result = client_->copy_object("bucket", "original.txt", "bucket", "copy.txt");

    ASSERT_TRUE(result.is_ok());

    // Both should exist
    auto orig = client_->get_object("bucket", "original.txt");
    auto copy = client_->get_object("bucket", "copy.txt");

    EXPECT_TRUE(orig.is_ok());
    EXPECT_TRUE(copy.is_ok());
}

// ============================================================================
// Object Metadata Tests
// ============================================================================

TEST_F(LocalStorageClientTest, GetObjectMetadata_Success) {
    client_->create_bucket("test-bucket", "");

    ByteArray data = {'D', 'a', 't', 'a'};
    StringMap metadata = {{"key1", "value1"}, {"key2", "value2"}};
    client_->put_object("test-bucket", "file.txt", data, "text/plain", metadata);

    auto result = client_->get_object_metadata("test-bucket", "file.txt");

    ASSERT_TRUE(result.is_ok());
    // Metadata should include what we set
}

TEST_F(LocalStorageClientTest, SetObjectMetadata_Success) {
    client_->create_bucket("test-bucket", "");

    ByteArray data = {'D', 'a', 't', 'a'};
    client_->put_object("test-bucket", "file.txt", data, "text/plain", {});

    StringMap new_metadata = {{"updated", "true"}};

    auto result = client_->set_object_metadata("test-bucket", "file.txt", new_metadata);

    ASSERT_TRUE(result.is_ok());
}

// ============================================================================
// Object Tags Tests
// ============================================================================

TEST_F(LocalStorageClientTest, SetAndGetObjectTags) {
    client_->create_bucket("test-bucket", "");

    ByteArray data = {'D', 'a', 't', 'a'};
    client_->put_object("test-bucket", "file.txt", data, "text/plain", {});

    StringMap tags = {{"environment", "production"}, {"team", "backend"}};

    auto set_result = client_->set_object_tags("test-bucket", "file.txt", tags);
    ASSERT_TRUE(set_result.is_ok());

    auto get_result = client_->get_object_tags("test-bucket", "file.txt");
    ASSERT_TRUE(get_result.is_ok());
    EXPECT_EQ(get_result.value().size(), 2);
}

// ============================================================================
// Presigned URL Tests
// ============================================================================

TEST_F(LocalStorageClientTest, GeneratePresignedUrl_Success) {
    client_->create_bucket("test-bucket", "");

    ByteArray data = {'D', 'a', 't', 'a'};
    client_->put_object("test-bucket", "file.txt", data, "text/plain", {});

    auto result = client_->generate_presigned_url("test-bucket", "file.txt", 3600, "GET");

    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value().empty());
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

TEST_F(LocalStorageClientTest, PutObject_LargeFile) {
    client_->create_bucket("test-bucket", "");

    // 10 MB file
    ByteArray large_data(10 * 1024 * 1024, 'X');

    auto result = client_->put_object("test-bucket", "large.bin", large_data, "application/octet-stream", {});

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().size(), 10 * 1024 * 1024);
}

TEST_F(LocalStorageClientTest, PutObject_EmptyFile) {
    client_->create_bucket("test-bucket", "");

    ByteArray empty_data;

    auto result = client_->put_object("test-bucket", "empty.txt", empty_data, "text/plain", {});

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().size(), 0);
}

TEST_F(LocalStorageClientTest, ObjectWithSpecialCharacters) {
    client_->create_bucket("test-bucket", "");

    ByteArray data = {'D', 'a', 't', 'a'};

    // Test various special characters in object key
    Vector<String> special_keys = {"file with spaces.txt",
                                   "file-with-dashes.txt",
                                   "file_with_underscores.txt",
                                   "file.with.dots.txt",
                                   "folder/subfolder/file.txt"};

    for (const auto& key : special_keys) {
        auto result = client_->put_object("test-bucket", key, data, "text/plain", {});
        EXPECT_TRUE(result.is_ok()) << "Failed for key: " << key;

        auto get_result = client_->get_object("test-bucket", key);
        EXPECT_TRUE(get_result.is_ok()) << "Failed to get key: " << key;
    }
}

TEST_F(LocalStorageClientTest, ConcurrentBucketOperations) {
    // Create multiple buckets rapidly
    for (int i = 0; i < 10; ++i) {
        String bucket_name = "bucket-" + std::to_string(i);
        auto result = client_->create_bucket(bucket_name, "");
        EXPECT_TRUE(result.is_ok());
    }

    auto list_result = client_->list_buckets();
    ASSERT_TRUE(list_result.is_ok());
    EXPECT_EQ(list_result.value().size(), 10);
}

TEST_F(LocalStorageClientTest, NestedFolderStructure) {
    client_->create_bucket("test-bucket", "");

    ByteArray data = {'D', 'a', 't', 'a'};

    // Create nested structure
    client_->put_object("test-bucket", "level1/level2/level3/file.txt", data, "text/plain", {});

    auto result = client_->get_object("test-bucket", "level1/level2/level3/file.txt");
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), data);
}
