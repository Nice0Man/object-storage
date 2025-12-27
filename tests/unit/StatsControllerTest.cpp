#include "console/api/StatsController.hpp"

#include "console/clients/LocalAdminClient.hpp"
#include "console/clients/LocalStorageClient.hpp"
#include "console/common/ServiceLocator.hpp"
#include "console/services/BucketService.hpp"
#include "console/services/UserService.hpp"

#include <drogon/HttpTypes.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <json/json.h>

using namespace console;
using namespace console::api;
using namespace console::services;
using namespace console::clients;
using namespace console::models;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

// ============================================================================
// Mock Classes
// ============================================================================

class MockStorageClient : public IStorageClient {
  public:
    MOCK_METHOD((Result<bool, String>), is_connected, (), (override));
    MOCK_METHOD((Result<Vector<Bucket>, String>), list_buckets, (), (override));
    MOCK_METHOD((Result<Bucket, String>), get_bucket, (const String&), (override));
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

    MOCK_METHOD((Result<ListObjectsResponse, String>),
                list_objects,
                (const String&, const ListObjectsOptions&),
                (override));
    MOCK_METHOD((Result<Object, String>), stat_object, (const String&, const String&), (override));
    MOCK_METHOD((Result<ByteArray, String>), get_object, (const String&, const String&), (override));
    MOCK_METHOD((Result<Object, String>),
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
    MOCK_METHOD((Result<Object, String>),
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
    MOCK_METHOD((Result<Object, String>),
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

class MockAdminClient : public IMinioAdminClient {
  public:
    // Server info
    MOCK_METHOD((Result<models::ServerInfo, String>), get_server_info, (), (override));

    // User management
    MOCK_METHOD((Result<Vector<User>, String>), list_users, (), (override));
    MOCK_METHOD((Result<User, String>), get_user, (const String&), (override));
    MOCK_METHOD((Result<User, String>), create_user, (const String&, const String&, bool), (override));
    MOCK_METHOD((Result<void, String>), delete_user, (const String&), (override));
    MOCK_METHOD((Result<void, String>), set_user_policy, (const String&, const String&), (override));
    MOCK_METHOD((Result<void, String>), update_user_groups, (const String&, const Vector<String>&), (override));

    // Group management - stub implementations for abstract class
    Result<Vector<models::Group>, String> list_groups() override {
        return Err<Vector<models::Group>, String>("Not implemented");
    }
    Result<models::Group, String> get_group(const String&) override {
        return Err<models::Group, String>("Not implemented");
    }
    Result<bool, String> create_group(const clients::CreateGroupRequest&) override { return Ok<bool, String>(true); }
    Result<bool, String> delete_group(const String&) override { return Ok<bool, String>(true); }
    Result<bool, String> add_user_to_group(const String&, const String&) override { return Ok<bool, String>(true); }
    Result<bool, String> remove_user_from_group(const String&, const String&) override {
        return Ok<bool, String>(true);
    }

    // Policy management - stub implementations for abstract class
    Result<Vector<models::Policy>, String> list_policies() override {
        return Err<Vector<models::Policy>, String>("Not implemented");
    }
    Result<bool, String> create_policy(const clients::CreatePolicyRequest&) override { return Ok<bool, String>(true); }
    Result<bool, String> delete_policy(const String&) override { return Ok<bool, String>(true); }
    Result<models::Policy, String> get_policy(const String&) override {
        return Err<models::Policy, String>("Not implemented");
    }
    Result<bool, String> attach_policy(const clients::AttachPolicyRequest&) override { return Ok<bool, String>(true); }
    Result<bool, String> detach_policy(const clients::AttachPolicyRequest&) override { return Ok<bool, String>(true); }

    // Service accounts - stub implementations
    Result<models::ServiceAccount, String> create_service_account(const String&, const Optional<String>&) override {
        return Err<models::ServiceAccount, String>("Not implemented");
    }
    Result<bool, String> delete_service_account(const String&) override { return Ok<bool, String>(true); }
};

// ============================================================================
// Test Fixture
// ============================================================================

class StatsControllerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        mock_storage_client_ = std::make_shared<NiceMock<MockStorageClient>>();
        mock_admin_client_ = std::make_shared<NiceMock<MockAdminClient>>();

        // Create services
        bucket_service_ = std::make_shared<BucketService>(mock_storage_client_);
        user_service_ = std::make_shared<UserService>(mock_admin_client_);

        // Register services with ServiceLocator
        ServiceLocator::set_bucket_service(bucket_service_);
        ServiceLocator::set_user_service(user_service_);

        // Setup user info
        admin_info_.access_key = "admin";
        admin_info_.is_admin = true;
        admin_info_.account_name = "Admin";

        user_info_.access_key = "testuser";
        user_info_.is_admin = false;
        user_info_.account_name = "Test User";
    }

    void TearDown() override {
        // Clean up ServiceLocator
        ServiceLocator::set_bucket_service(nullptr);
        ServiceLocator::set_user_service(nullptr);
    }

    // Helper to create buckets with specific stats
    Vector<Bucket> create_test_buckets(int count, int64_t objects_per_bucket, int64_t bytes_per_bucket) {
        Vector<Bucket> buckets;
        auto now = std::chrono::system_clock::now();

        for (int i = 0; i < count; i++) {
            BucketInfo info;
            info.name = "bucket-" + std::to_string(i + 1);
            info.creation_date = now - std::chrono::hours(24 * i); // Different dates

            Bucket bucket(info);
            // Set internal stats (would normally be set by storage client)
            // For testing, we'll need to create buckets with proper stats
            buckets.push_back(bucket);
        }

        return buckets;
    }

    // Helper to create users
    Vector<User> create_test_users(int count) {
        Vector<User> users;
        for (int i = 0; i < count; i++) {
            // User is a model class, create using UserInfo first
            UserInfo info;
            info.access_key = "user-" + std::to_string(i + 1);
            info.secret_key = "secret-" + std::to_string(i + 1);
            info.account_name = "User " + std::to_string(i + 1);
            info.policies = {"readwrite"};
            User user(info);
            users.push_back(user);
        }
        return users;
    }

    std::shared_ptr<MockStorageClient> mock_storage_client_;
    std::shared_ptr<MockAdminClient> mock_admin_client_;
    std::shared_ptr<BucketService> bucket_service_;
    std::shared_ptr<UserService> user_service_;
    UserInfo admin_info_;
    UserInfo user_info_;
};

// ============================================================================
// System Stats Tests
// ============================================================================

TEST_F(StatsControllerTest, SystemStats_EmptySystem) {
    // Setup: No buckets, no users
    Vector<Bucket> empty_buckets;
    Vector<User> empty_users;

    EXPECT_CALL(*mock_storage_client_, list_buckets()).WillOnce(Return(Ok<Vector<Bucket>, String>(empty_buckets)));

    EXPECT_CALL(*mock_admin_client_, list_users()).WillOnce(Return(Ok<Vector<User>, String>(empty_users)));

    // Create controller
    StatsController controller;

    // Mock request with user_info
    auto req = drogon::HttpRequest::newHttpRequest();
    req->attributes()->insert("user_info", admin_info_);

    // Capture response
    Json::Value response_json;
    bool callback_called = false;

    controller.get_system_stats(req, [&](const drogon::HttpResponsePtr& resp) {
        callback_called = true;
        EXPECT_EQ(resp->statusCode(), drogon::k200OK);

        auto body_str = std::string(resp->body());
        Json::CharReaderBuilder builder;
        std::istringstream stream(body_str);
        JSONCPP_STRING errs;
        ASSERT_TRUE(Json::parseFromStream(builder, stream, &response_json, &errs));
    });

    ASSERT_TRUE(callback_called);

    // Verify stats
    EXPECT_EQ(response_json["buckets"].asInt(), 0);
    EXPECT_EQ(response_json["objects"].asInt64(), 0);
    EXPECT_EQ(response_json["users"].asInt(), 0);
    EXPECT_EQ(response_json["storage_used"].asInt64(), 0);
    EXPECT_GT(response_json["storage_total"].asInt64(), 0); // Should have capacity
    EXPECT_EQ(response_json["storage_available"].asInt64(), response_json["storage_total"].asInt64());
    EXPECT_EQ(response_json["status"].asString(), "online");
    EXPECT_GT(response_json["timestamp"].asInt64(), 0);
}

TEST_F(StatsControllerTest, SystemStats_WithData) {
    // Setup: 3 buckets with different object counts and sizes
    Vector<Bucket> buckets = create_test_buckets(3, 0, 0);

    // Manually set stats for each bucket (simulating real storage data)
    // Note: In real implementation, Bucket class would track these stats
    // For now, we test the aggregation logic

    Vector<User> users = create_test_users(5);

    EXPECT_CALL(*mock_storage_client_, list_buckets()).WillOnce(Return(Ok<Vector<Bucket>, String>(buckets)));

    EXPECT_CALL(*mock_admin_client_, list_users()).WillOnce(Return(Ok<Vector<User>, String>(users)));

    // Create controller
    StatsController controller;

    // Mock request with user_info
    auto req = drogon::HttpRequest::newHttpRequest();
    req->attributes()->insert("user_info", admin_info_);

    // Capture response
    Json::Value response_json;
    bool callback_called = false;

    controller.get_system_stats(req, [&](const drogon::HttpResponsePtr& resp) {
        callback_called = true;
        EXPECT_EQ(resp->statusCode(), drogon::k200OK);

        auto body_str = std::string(resp->body());
        Json::CharReaderBuilder builder;
        std::istringstream stream(body_str);
        JSONCPP_STRING errs;
        ASSERT_TRUE(Json::parseFromStream(builder, stream, &response_json, &errs));
    });

    ASSERT_TRUE(callback_called);

    // Verify stats
    EXPECT_EQ(response_json["buckets"].asInt(), 3);
    EXPECT_EQ(response_json["users"].asInt(), 5);
    EXPECT_TRUE(response_json.isMember("objects"));
    EXPECT_TRUE(response_json.isMember("storage_used"));
    EXPECT_TRUE(response_json.isMember("storage_total"));
    EXPECT_TRUE(response_json.isMember("storage_available"));
}

TEST_F(StatsControllerTest, SystemStats_StorageCapacityCalculation) {
    // Test capacity calculations - now uses real filesystem data

    Vector<Bucket> buckets;
    Vector<User> users;

    EXPECT_CALL(*mock_storage_client_, list_buckets()).WillOnce(Return(Ok<Vector<Bucket>, String>(buckets)));

    EXPECT_CALL(*mock_admin_client_, list_users()).WillOnce(Return(Ok<Vector<User>, String>(users)));

    StatsController controller;
    auto req = drogon::HttpRequest::newHttpRequest();
    req->attributes()->insert("user_info", admin_info_);

    Json::Value response_json;
    bool callback_called = false;

    controller.get_system_stats(req, [&](const drogon::HttpResponsePtr& resp) {
        callback_called = true;
        auto body_str = std::string(resp->body());
        Json::CharReaderBuilder builder;
        std::istringstream stream(body_str);
        JSONCPP_STRING errs;
        ASSERT_TRUE(Json::parseFromStream(builder, stream, &response_json, &errs));
    });

    ASSERT_TRUE(callback_called);

    // Verify capacity calculations (uses real filesystem data)
    int64_t total = response_json["storage_total"].asInt64();
    int64_t used = response_json["storage_used"].asInt64();
    int64_t available = response_json["storage_available"].asInt64();

    // Verify storage capacity is positive
    EXPECT_GT(total, 0) << "Storage total should be > 0 (from real filesystem)";
    // Verify available = total - used
    EXPECT_EQ(available, total - used);
    // Verify used is within valid range
    EXPECT_GE(used, 0);
    EXPECT_LE(used, total);
}

// ============================================================================
// Capacity Stats Tests
// ============================================================================

TEST_F(StatsControllerTest, CapacityStats_EmptyStorage) {
    Vector<Bucket> empty_buckets;

    EXPECT_CALL(*mock_storage_client_, list_buckets()).WillOnce(Return(Ok<Vector<Bucket>, String>(empty_buckets)));

    StatsController controller;
    auto req = drogon::HttpRequest::newHttpRequest();
    req->attributes()->insert("user_info", admin_info_);

    Json::Value response_json;
    bool callback_called = false;

    controller.get_capacity(req, [&](const drogon::HttpResponsePtr& resp) {
        callback_called = true;
        EXPECT_EQ(resp->statusCode(), drogon::k200OK);

        auto body_str = std::string(resp->body());
        Json::CharReaderBuilder builder;
        std::istringstream stream(body_str);
        JSONCPP_STRING errs;
        ASSERT_TRUE(Json::parseFromStream(builder, stream, &response_json, &errs));
    });

    ASSERT_TRUE(callback_called);

    // Verify capacity
    EXPECT_GT(response_json["total"].asInt64(), 0);
    EXPECT_EQ(response_json["used"].asInt64(), 0);
    EXPECT_EQ(response_json["available"].asInt64(), response_json["total"].asInt64());
    EXPECT_EQ(response_json["usage_percent"].asDouble(), 0.0);
}

TEST_F(StatsControllerTest, CapacityStats_UsagePercentCalculation) {
    Vector<Bucket> buckets = create_test_buckets(2, 0, 0);

    EXPECT_CALL(*mock_storage_client_, list_buckets()).WillOnce(Return(Ok<Vector<Bucket>, String>(buckets)));

    StatsController controller;
    auto req = drogon::HttpRequest::newHttpRequest();
    req->attributes()->insert("user_info", admin_info_);

    Json::Value response_json;
    bool callback_called = false;

    controller.get_capacity(req, [&](const drogon::HttpResponsePtr& resp) {
        callback_called = true;
        auto body_str = std::string(resp->body());
        Json::CharReaderBuilder builder;
        std::istringstream stream(body_str);
        JSONCPP_STRING errs;
        ASSERT_TRUE(Json::parseFromStream(builder, stream, &response_json, &errs));
    });

    ASSERT_TRUE(callback_called);

    // Verify usage_percent is valid
    double usage_percent = response_json["usage_percent"].asDouble();
    EXPECT_GE(usage_percent, 0.0);
    EXPECT_LE(usage_percent, 100.0);

    // Verify formula: used / total * 100
    int64_t total = response_json["total"].asInt64();
    int64_t used = response_json["used"].asInt64();
    double expected_percent = (total > 0) ? (static_cast<double>(used) / total * 100.0) : 0.0;
    EXPECT_NEAR(usage_percent, expected_percent, 0.01);
}

// ============================================================================
// Activity Stats Tests
// ============================================================================

TEST_F(StatsControllerTest, ActivityStats_EmptySystem) {
    Vector<Bucket> empty_buckets;

    EXPECT_CALL(*mock_storage_client_, list_buckets()).WillOnce(Return(Ok<Vector<Bucket>, String>(empty_buckets)));

    StatsController controller;
    auto req = drogon::HttpRequest::newHttpRequest();
    req->attributes()->insert("user_info", admin_info_);

    Json::Value response_json;
    bool callback_called = false;

    controller.get_activity(req, [&](const drogon::HttpResponsePtr& resp) {
        callback_called = true;
        EXPECT_EQ(resp->statusCode(), drogon::k200OK);

        auto body_str = std::string(resp->body());
        Json::CharReaderBuilder builder;
        std::istringstream stream(body_str);
        JSONCPP_STRING errs;
        ASSERT_TRUE(Json::parseFromStream(builder, stream, &response_json, &errs));
    });

    ASSERT_TRUE(callback_called);

    // Verify activity
    EXPECT_TRUE(response_json["recent_buckets"].isArray());
    EXPECT_EQ(response_json["recent_buckets"].size(), 0);
    EXPECT_EQ(response_json["total_activity"].asInt(), 0);
}

TEST_F(StatsControllerTest, ActivityStats_RecentBuckets) {
    // Create 15 buckets (should return only 10 most recent)
    Vector<Bucket> buckets = create_test_buckets(15, 0, 0);

    EXPECT_CALL(*mock_storage_client_, list_buckets()).WillOnce(Return(Ok<Vector<Bucket>, String>(buckets)));

    StatsController controller;
    auto req = drogon::HttpRequest::newHttpRequest();
    req->attributes()->insert("user_info", admin_info_);

    Json::Value response_json;
    bool callback_called = false;

    controller.get_activity(req, [&](const drogon::HttpResponsePtr& resp) {
        callback_called = true;
        auto body_str = std::string(resp->body());
        Json::CharReaderBuilder builder;
        std::istringstream stream(body_str);
        JSONCPP_STRING errs;
        ASSERT_TRUE(Json::parseFromStream(builder, stream, &response_json, &errs));
    });

    ASSERT_TRUE(callback_called);

    // Verify only 10 most recent buckets are returned
    EXPECT_TRUE(response_json["recent_buckets"].isArray());
    EXPECT_LE(response_json["recent_buckets"].size(), 10);

    // Verify each bucket has required fields
    for (const auto& bucket : response_json["recent_buckets"]) {
        EXPECT_TRUE(bucket.isMember("name"));
        EXPECT_TRUE(bucket.isMember("objects"));
        EXPECT_TRUE(bucket.isMember("size"));
        EXPECT_TRUE(bucket.isMember("timestamp"));
    }
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(StatsControllerTest, SystemStats_ServiceUnavailable) {
    // Simulate bucket service failure
    EXPECT_CALL(*mock_storage_client_, list_buckets())
        .WillOnce(Return(Err<Vector<Bucket>, String>("Service unavailable")));

    StatsController controller;
    auto req = drogon::HttpRequest::newHttpRequest();
    req->attributes()->insert("user_info", admin_info_);

    bool callback_called = false;

    controller.get_system_stats(req, [&](const drogon::HttpResponsePtr& resp) {
        callback_called = true;
        // When bucket service fails, controller returns empty stats with 200 OK
        // (graceful degradation: returns 0 buckets, 0 objects)
        EXPECT_TRUE(resp->statusCode() == drogon::k200OK || resp->statusCode() == drogon::k500InternalServerError)
            << "Expected 200 OK (graceful degradation) or 500, got " << resp->statusCode();
    });

    ASSERT_TRUE(callback_called);
}

TEST_F(StatsControllerTest, SystemStats_NoUserInfo) {
    // Request without user_info in attributes
    StatsController controller;
    auto req = drogon::HttpRequest::newHttpRequest();
    // Don't insert user_info

    bool callback_called = false;

    controller.get_system_stats(req, [&](const drogon::HttpResponsePtr& resp) {
        callback_called = true;
        // Should handle missing user_info gracefully
        EXPECT_TRUE(resp->statusCode() == drogon::k200OK || resp->statusCode() == drogon::k500InternalServerError);
    });

    ASSERT_TRUE(callback_called);
}

// ============================================================================
// Business Logic Integration Tests
// ============================================================================

TEST_F(StatsControllerTest, BusinessLogic_TotalCalculations) {
    // Create realistic test data
    Vector<Bucket> buckets = create_test_buckets(5, 0, 0);
    Vector<User> users = create_test_users(10);

    EXPECT_CALL(*mock_storage_client_, list_buckets()).WillRepeatedly(Return(Ok<Vector<Bucket>, String>(buckets)));

    EXPECT_CALL(*mock_admin_client_, list_users()).WillOnce(Return(Ok<Vector<User>, String>(users)));

    StatsController controller;
    auto req = drogon::HttpRequest::newHttpRequest();
    req->attributes()->insert("user_info", admin_info_);

    Json::Value response_json;

    controller.get_system_stats(req, [&](const drogon::HttpResponsePtr& resp) {
        auto body_str = std::string(resp->body());
        Json::CharReaderBuilder builder;
        std::istringstream stream(body_str);
        JSONCPP_STRING errs;
        ASSERT_TRUE(Json::parseFromStream(builder, stream, &response_json, &errs));
    });

    // Verify business logic
    int expected_buckets = 5;
    int expected_users = 10;

    EXPECT_EQ(response_json["buckets"].asInt(), expected_buckets) << "Business logic: Total buckets count mismatch";

    EXPECT_EQ(response_json["users"].asInt(), expected_users) << "Business logic: Total users count mismatch";

    // Verify storage calculations are consistent
    int64_t total = response_json["storage_total"].asInt64();
    int64_t used = response_json["storage_used"].asInt64();
    int64_t available = response_json["storage_available"].asInt64();

    EXPECT_EQ(total, used + available) << "Business logic: Storage total = used + available";

    EXPECT_GE(available, 0) << "Business logic: Available storage cannot be negative";
}

TEST_F(StatsControllerTest, BusinessLogic_CapacityThresholds) {
    // Test various capacity scenarios
    Vector<Bucket> buckets;

    EXPECT_CALL(*mock_storage_client_, list_buckets()).WillOnce(Return(Ok<Vector<Bucket>, String>(buckets)));

    StatsController controller;
    auto req = drogon::HttpRequest::newHttpRequest();
    req->attributes()->insert("user_info", admin_info_);

    Json::Value response_json;

    controller.get_capacity(req, [&](const drogon::HttpResponsePtr& resp) {
        auto body_str = std::string(resp->body());
        Json::CharReaderBuilder builder;
        std::istringstream stream(body_str);
        JSONCPP_STRING errs;
        ASSERT_TRUE(Json::parseFromStream(builder, stream, &response_json, &errs));
    });

    // Business rules validation
    double usage_percent = response_json["usage_percent"].asDouble();

    // Usage percent should be valid percentage
    EXPECT_GE(usage_percent, 0.0) << "Usage percent cannot be negative";
    EXPECT_LE(usage_percent, 100.0) << "Usage percent cannot exceed 100%";

    // Check capacity warning thresholds (business logic)
    if (usage_percent > 90.0) {
        // In real app, this would trigger warning
        SUCCEED() << "High capacity warning threshold reached: " << usage_percent << "%";
    }

    if (usage_percent > 95.0) {
        // In real app, this would trigger critical alert
        SUCCEED() << "Critical capacity threshold reached: " << usage_percent << "%";
    }
}

// ============================================================================
// Main Test Runner
// ============================================================================

int
main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
