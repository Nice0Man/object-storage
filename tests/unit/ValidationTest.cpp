#include "console/common/Types.hpp"

#include <gtest/gtest.h>
#include <regex>

using namespace console;

// ============================================================================
// Bucket Name Validation Tests
// ============================================================================

class BucketNameValidationTest : public ::testing::Test {
  protected:
    bool is_valid_bucket_name(const String& name) {
        // Based on AWS S3 bucket naming rules
        if (name.length() < 3 || name.length() > 63) {
            return false;
        }

        // Must start with lowercase letter or number
        if (!std::isalnum(name[0]) || std::isupper(name[0])) {
            return false;
        }

        // Must end with lowercase letter or number
        if (!std::isalnum(name.back()) || std::isupper(name.back())) {
            return false;
        }

        // Check for invalid characters and patterns
        std::regex valid_pattern("^[a-z0-9][a-z0-9.-]*[a-z0-9]$");
        if (!std::regex_match(name, valid_pattern)) {
            return false;
        }

        // No consecutive dots
        if (name.find("..") != String::npos) {
            return false;
        }

        // Not an IP address
        std::regex ip_pattern("^[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+$");
        if (std::regex_match(name, ip_pattern)) {
            return false;
        }

        return true;
    }
};

TEST_F(BucketNameValidationTest, ValidNames) {
    Vector<String> valid_names = {
        "mybucket",
        "my-bucket",
        "my.bucket",
        "my-bucket-123",
        "bucket123",
        "a-b-c",
        "test.bucket.name",
        "bucket-with-many-dashes",
        "bucket.with.many.dots",
        String(63, 'a') // Max length
    };

    for (const auto& name : valid_names) {
        EXPECT_TRUE(is_valid_bucket_name(name)) << "Name should be valid: " << name;
    }
}

TEST_F(BucketNameValidationTest, InvalidNames_TooShort) {
    Vector<String> invalid_names = {"", "a", "ab"};

    for (const auto& name : invalid_names) {
        EXPECT_FALSE(is_valid_bucket_name(name)) << "Name should be invalid (too short): " << name;
    }
}

TEST_F(BucketNameValidationTest, InvalidNames_TooLong) {
    String too_long(64, 'a');
    EXPECT_FALSE(is_valid_bucket_name(too_long));

    String much_too_long(100, 'a');
    EXPECT_FALSE(is_valid_bucket_name(much_too_long));
}

TEST_F(BucketNameValidationTest, InvalidNames_UpperCase) {
    Vector<String> invalid_names = {"MyBucket", "MYBUCKET", "myBucket", "my-Bucket"};

    for (const auto& name : invalid_names) {
        EXPECT_FALSE(is_valid_bucket_name(name)) << "Name should be invalid (uppercase): " << name;
    }
}

TEST_F(BucketNameValidationTest, InvalidNames_SpecialCharacters) {
    Vector<String> invalid_names = {
        "my_bucket",   // underscore
        "my bucket",   // space
        "my@bucket",   // @
        "my#bucket",   // #
        "my$bucket",   // $
        "my%bucket",   // %
        "my&bucket",   // &
        "my*bucket",   // *
        "my(bucket)",  // parentheses
        "my[bucket]",  // brackets
        "my{bucket}",  // braces
        "my|bucket",   // pipe
        "my\\bucket",  // backslash
        "my/bucket",   // forward slash
        "my:bucket",   // colon
        "my;bucket",   // semicolon
        "my?bucket",   // question mark
        "my<bucket>",  // angle brackets
        "my'bucket'",  // quotes
        "my\"bucket\"" // double quotes
    };

    for (const auto& name : invalid_names) {
        EXPECT_FALSE(is_valid_bucket_name(name)) << "Name should be invalid (special chars): " << name;
    }
}

TEST_F(BucketNameValidationTest, InvalidNames_StartOrEndPattern) {
    Vector<String> invalid_names = {
        "-mybucket",   // starts with dash
        "mybucket-",   // ends with dash
        ".mybucket",   // starts with dot
        "mybucket.",   // ends with dot
        "-my-bucket-", // starts and ends with dash
        ".my.bucket."  // starts and ends with dot
    };

    for (const auto& name : invalid_names) {
        EXPECT_FALSE(is_valid_bucket_name(name)) << "Name should be invalid (start/end): " << name;
    }
}

TEST_F(BucketNameValidationTest, InvalidNames_ConsecutiveDots) {
    Vector<String> invalid_names = {"my..bucket", "bucket..name", "my...bucket", "a..b..c"};

    for (const auto& name : invalid_names) {
        EXPECT_FALSE(is_valid_bucket_name(name)) << "Name should be invalid (consecutive dots): " << name;
    }
}

TEST_F(BucketNameValidationTest, InvalidNames_IPAddress) {
    Vector<String> invalid_names = {"192.168.1.1", "10.0.0.1", "255.255.255.255", "1.2.3.4"};

    for (const auto& name : invalid_names) {
        EXPECT_FALSE(is_valid_bucket_name(name)) << "Name should be invalid (IP address): " << name;
    }
}

// ============================================================================
// Object Key Validation Tests
// ============================================================================

class ObjectKeyValidationTest : public ::testing::Test {
  protected:
    bool is_valid_object_key(const String& key) {
        // Object keys are more permissive than bucket names
        if (key.empty() || key.length() > 1024) {
            return false;
        }

        // Should not contain null bytes
        if (key.find('\0') != String::npos) {
            return false;
        }

        // Should not start with '/' (optional, depending on implementation)
        // if (key[0] == '/') {
        //     return false;
        // }

        return true;
    }
};

TEST_F(ObjectKeyValidationTest, ValidKeys) {
    Vector<String> valid_keys = {
        "file.txt",
        "folder/file.txt",
        "deep/nested/folder/structure/file.txt",
        "file with spaces.txt",
        "file_with_underscores.txt",
        "file-with-dashes.txt",
        "file.with.dots.txt",
        "UPPERCASE.TXT",
        "123456.bin",
        "файл.txt", // Unicode
        "file@example.com",
        "file+tag.txt",
        String(1024, 'a') // Max length
    };

    for (const auto& key : valid_keys) {
        EXPECT_TRUE(is_valid_object_key(key)) << "Key should be valid: " << key;
    }
}

TEST_F(ObjectKeyValidationTest, InvalidKeys_Empty) {
    EXPECT_FALSE(is_valid_object_key(""));
}

TEST_F(ObjectKeyValidationTest, InvalidKeys_TooLong) {
    String too_long(1025, 'a');
    EXPECT_FALSE(is_valid_object_key(too_long));

    String much_too_long(10000, 'a');
    EXPECT_FALSE(is_valid_object_key(much_too_long));
}

// ============================================================================
// Access Key Validation Tests
// ============================================================================

class AccessKeyValidationTest : public ::testing::Test {
  protected:
    bool is_valid_access_key(const String& key) {
        if (key.length() < 3 || key.length() > 128) {
            return false;
        }

        // Only alphanumeric and limited special characters
        std::regex valid_pattern("^[A-Za-z0-9_.-]+$");
        return std::regex_match(key, valid_pattern);
    }
};

TEST_F(AccessKeyValidationTest, ValidAccessKeys) {
    Vector<String> valid_keys = {
        "admin", "user123", "test-user", "test_user", "test.user", "USER-123_test.key", String(128, 'a') // Max length
    };

    for (const auto& key : valid_keys) {
        EXPECT_TRUE(is_valid_access_key(key)) << "Access key should be valid: " << key;
    }
}

TEST_F(AccessKeyValidationTest, InvalidAccessKeys_TooShort) {
    Vector<String> invalid_keys = {"", "a", "ab"};

    for (const auto& key : invalid_keys) {
        EXPECT_FALSE(is_valid_access_key(key)) << "Access key should be invalid (too short): " << key;
    }
}

TEST_F(AccessKeyValidationTest, InvalidAccessKeys_TooLong) {
    String too_long(129, 'a');
    EXPECT_FALSE(is_valid_access_key(too_long));
}

TEST_F(AccessKeyValidationTest, InvalidAccessKeys_SpecialCharacters) {
    Vector<String> invalid_keys = {"user@example",
                                   "user#123",
                                   "user$name",
                                   "user%test",
                                   "user&name",
                                   "user*name",
                                   "user name",
                                   "user/name",
                                   "user\\name"};

    for (const auto& key : invalid_keys) {
        EXPECT_FALSE(is_valid_access_key(key)) << "Access key should be invalid (special chars): " << key;
    }
}

// ============================================================================
// Secret Key Validation Tests
// ============================================================================

class SecretKeyValidationTest : public ::testing::Test {
  protected:
    bool is_valid_secret_key(const String& key) {
        // Secret keys have minimum length requirement for security
        return key.length() >= 8 && key.length() <= 256;
    }

    bool is_strong_password(const String& password) {
        if (password.length() < 8) {
            return false;
        }

        bool has_upper = false;
        bool has_lower = false;
        bool has_digit = false;
        bool has_special = false;

        for (char c : password) {
            if (std::isupper(c))
                has_upper = true;
            if (std::islower(c))
                has_lower = true;
            if (std::isdigit(c))
                has_digit = true;
            if (std::ispunct(c))
                has_special = true;
        }

        // At least 3 out of 4 categories
        int categories = has_upper + has_lower + has_digit + has_special;
        return categories >= 3;
    }
};

TEST_F(SecretKeyValidationTest, ValidSecretKeys) {
    Vector<String> valid_keys = {
        "password123",
        "MySecurePassword!",
        "P@ssw0rd",
        "VeryLongSecretKeyWithManyCharacters123!@#",
        String(256, 'a') // Max length
    };

    for (const auto& key : valid_keys) {
        EXPECT_TRUE(is_valid_secret_key(key)) << "Secret key should be valid: " << key;
    }
}

TEST_F(SecretKeyValidationTest, InvalidSecretKeys_TooShort) {
    Vector<String> invalid_keys = {
        "",
        "a",
        "pass",
        "1234567" // 7 characters
    };

    for (const auto& key : invalid_keys) {
        EXPECT_FALSE(is_valid_secret_key(key)) << "Secret key should be invalid (too short): " << key;
    }
}

TEST_F(SecretKeyValidationTest, InvalidSecretKeys_TooLong) {
    String too_long(257, 'a');
    EXPECT_FALSE(is_valid_secret_key(too_long));
}

TEST_F(SecretKeyValidationTest, StrongPasswords) {
    Vector<String> strong_passwords = {"P@ssw0rd", "MySecure123!", "Str0ng_P4ss", "C0mplex!Pass"};

    for (const auto& password : strong_passwords) {
        EXPECT_TRUE(is_strong_password(password)) << "Password should be strong: " << password;
    }
}

TEST_F(SecretKeyValidationTest, WeakPasswords) {
    Vector<String> weak_passwords = {
        "password",    // no upper, no digit, no special (only 1 category)
        "12345678",    // only digits (only 1 category)
        "ABCDEFGH",    // only uppercase (only 1 category)
        "abcdefgh",    // only lowercase (only 1 category)
        "PASSWORD123", // only uppercase + digits (only 2 categories)
        "password123"  // only lowercase + digits (only 2 categories)
    };

    for (const auto& password : weak_passwords) {
        EXPECT_FALSE(is_strong_password(password)) << "Password should be weak: " << password;
    }
}

// ============================================================================
// Content Type Validation Tests
// ============================================================================

class ContentTypeValidationTest : public ::testing::Test {
  protected:
    bool is_valid_content_type(const String& content_type) {
        if (content_type.empty()) {
            return false;
        }

        // Basic MIME type pattern: type/subtype
        std::regex mime_pattern("^[a-zA-Z0-9]+/[a-zA-Z0-9.+-]+");
        return std::regex_match(content_type, mime_pattern);
    }
};

TEST_F(ContentTypeValidationTest, ValidContentTypes) {
    Vector<String> valid_types = {"text/plain",
                                  "text/html",
                                  "application/json",
                                  "application/octet-stream",
                                  "image/jpeg",
                                  "image/png",
                                  "video/mp4",
                                  "audio/mpeg",
                                  "application/pdf",
                                  "application/xml",
                                  "text/csv",
                                  "application/zip",
                                  "multipart/form-data"};

    for (const auto& type : valid_types) {
        EXPECT_TRUE(is_valid_content_type(type)) << "Content type should be valid: " << type;
    }
}

TEST_F(ContentTypeValidationTest, InvalidContentTypes) {
    Vector<String> invalid_types = {"", "text", "/plain", "text/", "invalid", "text plain", "text\\plain"};

    for (const auto& type : invalid_types) {
        EXPECT_FALSE(is_valid_content_type(type)) << "Content type should be invalid: " << type;
    }
}

// ============================================================================
// Region Validation Tests
// ============================================================================

class RegionValidationTest : public ::testing::Test {
  protected:
    bool is_valid_region(const String& region) {
        if (region.empty() || region.length() > 64) {
            return false;
        }

        // Regions are typically lowercase with dashes
        std::regex region_pattern("^[a-z0-9-]+$");
        return std::regex_match(region, region_pattern);
    }
};

TEST_F(RegionValidationTest, ValidRegions) {
    Vector<String> valid_regions = {
        "us-east-1", "us-west-2", "eu-central-1", "ap-southeast-1", "local", "custom-region", "region123"};

    for (const auto& region : valid_regions) {
        EXPECT_TRUE(is_valid_region(region)) << "Region should be valid: " << region;
    }
}

TEST_F(RegionValidationTest, InvalidRegions) {
    Vector<String> invalid_regions = {
        "",
        "US-EAST-1",    // uppercase
        "us_east_1",    // underscores
        "us east 1",    // spaces
        "us.east.1",    // dots
        String(65, 'a') // too long
    };

    for (const auto& region : invalid_regions) {
        EXPECT_FALSE(is_valid_region(region)) << "Region should be invalid: " << region;
    }
}
