#include "console/models/Bucket.hpp"
#include "console/models/Error.hpp"
#include "console/models/Group.hpp"
#include "console/models/Object.hpp"
#include "console/models/Policy.hpp"
#include "console/models/User.hpp"

#include <gtest/gtest.h>

using namespace console::models;
using console::BucketInfo;
using console::HttpStatus;
using console::ObjectInfo;
using console::UserInfo;
using console::models::ErrorCode;

class BucketModelTest : public ::testing::Test {
  protected:
    Bucket create_test_bucket() {
        BucketInfo info;
        info.name = "test-bucket";
        info.region = "us-east-1";
        info.versioning_enabled = true;
        info.creation_date = std::chrono::system_clock::now();
        return Bucket(info);
    }
};

TEST_F(BucketModelTest, ToJson_Success) {
    auto bucket = create_test_bucket();
    auto json = bucket.to_json();

    EXPECT_EQ(json["name"].asString(), "test-bucket");
    EXPECT_EQ(json["region"].asString(), "us-east-1");
    EXPECT_TRUE(json["versioning_enabled"].asBool());
}

TEST_F(BucketModelTest, FromJson_Success) {
    Json::Value json;
    json["name"] = "from-json-bucket";
    json["region"] = "eu-west-1";
    json["versioning_enabled"] = false;
    json["creation_date"] = "2025-01-01T00:00:00Z";

    auto bucket = Bucket::from_json(json);

    EXPECT_EQ(bucket.name(), "from-json-bucket");
    EXPECT_EQ(bucket.region(), "eu-west-1");
    EXPECT_FALSE(bucket.versioning_enabled());
}

class ObjectModelTest : public ::testing::Test {
  protected:
    Object create_test_object() {
        ObjectInfo info;
        info.key = "path/to/file.txt";
        info.bucket = "test-bucket";
        info.size = 1024;
        info.etag = "abc123";
        info.content_type = "text/plain";
        info.last_modified = std::chrono::system_clock::now();
        return Object(info);
    }
};

TEST_F(ObjectModelTest, ToJson_Success) {
    auto obj = create_test_object();
    auto json = obj.to_json();

    EXPECT_EQ(json["key"].asString(), "path/to/file.txt");
    EXPECT_EQ(json["bucket"].asString(), "test-bucket");
    EXPECT_EQ(json["size"].asInt64(), 1024);
    EXPECT_EQ(json["content_type"].asString(), "text/plain");
}

TEST_F(ObjectModelTest, HumanReadableSize) {
    ObjectInfo info;
    info.key = "test";
    info.bucket = "bucket";

    // Test different sizes
    info.size = 500;
    EXPECT_EQ(Object(info).human_readable_size(), "500.00 B");

    info.size = 1536; // 1.5 KB
    EXPECT_EQ(Object(info).human_readable_size(), "1.50 KB");

    info.size = 1048576; // 1 MB
    EXPECT_EQ(Object(info).human_readable_size(), "1.00 MB");

    info.size = 1073741824; // 1 GB
    EXPECT_EQ(Object(info).human_readable_size(), "1.00 GB");
}

TEST_F(ObjectModelTest, Extension) {
    ObjectInfo info;
    info.bucket = "bucket";

    info.key = "file.txt";
    EXPECT_EQ(Object(info).extension(), "txt");

    info.key = "path/to/document.pdf";
    EXPECT_EQ(Object(info).extension(), "pdf");

    info.key = "noextension";
    EXPECT_EQ(Object(info).extension(), "");
}

class UserModelTest : public ::testing::Test {
  protected:
    User create_test_user() {
        UserInfo info;
        info.access_key = "AKIAIOSFODNN7EXAMPLE";
        info.secret_key = "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY";
        info.account_name = "Test User";
        info.is_admin = false;
        info.policies = {"ReadOnly", "WriteOnly"};
        info.created_at = std::chrono::system_clock::now();
        return User(info);
    }
};

TEST_F(UserModelTest, ToJson_Success) {
    auto user = create_test_user();
    auto json = user.to_json();

    EXPECT_EQ(json["access_key"].asString(), "AKIAIOSFODNN7EXAMPLE");
    EXPECT_EQ(json["account_name"].asString(), "Test User");
    EXPECT_FALSE(json["is_admin"].asBool());
    EXPECT_EQ(json["policies"].size(), 2);
}

TEST_F(UserModelTest, FromJson_Success) {
    Json::Value json;
    json["access_key"] = "TEST123";
    json["account_name"] = "JSON User";
    json["is_admin"] = true;
    json["policies"] = Json::Value(Json::arrayValue);
    json["policies"].append("AdminPolicy");

    auto user = User::from_json(json);

    EXPECT_EQ(user.access_key(), "TEST123");
    EXPECT_EQ(user.account_name(), "JSON User");
    EXPECT_TRUE(user.is_admin());
    EXPECT_EQ(user.policies().size(), 1);
}

class ErrorModelTest : public ::testing::Test {};

TEST_F(ErrorModelTest, ApiError_Creation) {
    ApiError error(HttpStatus::BadRequest, "Test error message");

    EXPECT_EQ(error.status(), HttpStatus::BadRequest);
    EXPECT_EQ(error.message(), "Test error message");
}

TEST_F(ErrorModelTest, ApiError_ToJson) {
    ApiError error(HttpStatus::NotFound, "Resource not found", "The requested bucket does not exist");

    auto json = error.to_json();

    // Check that error code is converted properly (NotFound from HttpStatus -> ErrorCode)
    EXPECT_EQ(json["code"].asInt(), static_cast<int>(ErrorCode::NotFound));
    EXPECT_EQ(json["message"].asString(), "Resource not found");
    EXPECT_EQ(json["detail"].asString(), "The requested bucket does not exist");
    EXPECT_EQ(json["http_status"].asInt(), 404);
}

TEST_F(ErrorModelTest, ApiException_ThrowAndCatch) {
    ApiError error(HttpStatus::InternalServerError, "Server error");

    EXPECT_THROW({ throw ApiException(error); }, ApiException);

    try {
        throw ApiException(error);
    } catch (const ApiException& ex) {
        EXPECT_EQ(ex.error().status(), HttpStatus::InternalServerError);
        EXPECT_EQ(ex.error().message(), "Server error");
    }
}
