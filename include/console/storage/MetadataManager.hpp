#pragma once

#include "console/common/Types.hpp"
#include "console/models/Bucket.hpp"
#include "console/models/Object.hpp"

#include <filesystem>
#include <json/json.h>

namespace console::storage {

/**
 * @brief Bucket metadata structure
 */
struct BucketMetadata {
    String name;
    TimePoint creation_date;
    String region;
    bool versioning_enabled{false};
    bool object_locking{false};
    bool encryption_enabled{false};
    String encryption_type; // "SSE-S3", "SSE-C", or empty
    String owner;
    size_t object_count{0};
    size_t total_size_bytes{0};   // Size of actual objects only
    size_t size_with_metadata{0}; // Total size including metadata files

    Json::Value to_json() const;
    static BucketMetadata from_json(const Json::Value& json);
};

/**
 * @brief Object metadata structure
 */
struct ObjectMetadata {
    String key;
    String bucket;
    size_t size{0};
    size_t original_size{0}; // Original size before encryption (for encrypted objects)
    String etag;
    String content_type;
    TimePoint last_modified;
    String version_id;
    StringMap metadata; // Custom user metadata

    // Object Lock and Retention fields
    String retention_mode;      // "GOVERNANCE" or "COMPLIANCE", empty if not set
    int64_t retention_until{0}; // Unix timestamp, 0 if not set
    bool legal_hold{false};     // Legal hold status

    // Server-Side Encryption fields
    bool encrypted{false};       // Whether object is encrypted
    String encryption_algorithm; // "AES256" or empty if not encrypted
    String sse_type;             // "SSE-S3" (server key) or "SSE-C" (customer key)
    String sse_customer_key_md5; // MD5 of customer key (for SSE-C verification only)

    Json::Value to_json() const;
    static ObjectMetadata from_json(const Json::Value& json);
};

/**
 * @brief Manages reading and writing of metadata files
 */
class MetadataManager {
  public:
    MetadataManager() = default;

    // Bucket metadata
    Result<BucketMetadata, String> read_bucket_metadata(const std::filesystem::path& path) const;

    Result<void, String> write_bucket_metadata(const std::filesystem::path& path, const BucketMetadata& metadata) const;

    // Object metadata
    Result<ObjectMetadata, String> read_object_metadata(const std::filesystem::path& path) const;

    Result<void, String> write_object_metadata(const std::filesystem::path& path, const ObjectMetadata& metadata) const;

    // Tags
    Result<StringMap, String> read_tags(const std::filesystem::path& path) const;

    Result<void, String> write_tags(const std::filesystem::path& path, const StringMap& tags) const;

    // Policy
    Result<String, String> read_policy(const std::filesystem::path& path) const;

    Result<void, String> write_policy(const std::filesystem::path& path, const String& policy_json) const;

    // Versioning config
    Result<bool, String> read_versioning(const std::filesystem::path& path) const;

    Result<void, String> write_versioning(const std::filesystem::path& path, bool enabled) const;

    // Convert to models
    models::Bucket bucket_metadata_to_model(const BucketMetadata& meta) const;
    models::Object object_metadata_to_model(const ObjectMetadata& meta) const;

  private:
    // Helper methods
    Result<Json::Value, String> read_json_file(const std::filesystem::path& path) const;
    Result<void, String> write_json_file(const std::filesystem::path& path, const Json::Value& json) const;
};

} // namespace console::storage
