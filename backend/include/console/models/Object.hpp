#pragma once

#include "console/common/Types.hpp"

#include <json/json.h>

namespace console::models {

/**
 * @brief Object model for S3 objects
 */
class Object {
  public:
    Object() = default;
    explicit Object(const ObjectInfo& info) : info_(info) {}

    // Getters
    const String& key() const { return info_.key; }
    const String& bucket() const { return info_.bucket; }
    int64_t size() const { return info_.size; }
    const TimePoint& last_modified() const { return info_.last_modified; }
    const String& etag() const { return info_.etag; }
    const String& content_type() const { return info_.content_type; }
    const String& storage_class() const { return info_.storage_class; }
    const StringMap& metadata() const { return info_.metadata; }
    const StringMap& user_metadata() const { return info_.user_metadata; }
    // Encryption getters
    bool encrypted() const { return info_.encrypted; }
    const String& encryption_algorithm() const { return info_.encryption_algorithm; }
    const String& sse_type() const { return info_.sse_type; }
    const String& sse_customer_key_md5() const { return info_.sse_customer_key_md5; }
    int64_t original_size() const { return info_.original_size; }

    // Setters
    void set_key(const String& key) { info_.key = key; }
    void set_bucket(const String& bucket) { info_.bucket = bucket; }
    void set_size(int64_t size) { info_.size = size; }
    void set_last_modified(const TimePoint& time) { info_.last_modified = time; }
    void set_etag(const String& etag) { info_.etag = etag; }
    void set_content_type(const String& type) { info_.content_type = type; }
    void set_storage_class(const String& storage_class) { info_.storage_class = storage_class; }
    void set_metadata(const StringMap& metadata) { info_.metadata = metadata; }
    void set_user_metadata(const StringMap& metadata) { info_.user_metadata = metadata; }
    // Encryption setters
    void set_encrypted(bool encrypted) { info_.encrypted = encrypted; }
    void set_encryption_algorithm(const String& algo) { info_.encryption_algorithm = algo; }
    void set_sse_type(const String& type) { info_.sse_type = type; }
    void set_sse_customer_key_md5(const String& md5) { info_.sse_customer_key_md5 = md5; }
    void set_original_size(int64_t size) { info_.original_size = size; }

    // Utilities
    String extension() const;
    bool is_directory() const;
    String human_readable_size() const;

    // Serialization
    Json::Value to_json() const;
    static Object from_json(const Json::Value& json);

  private:
    ObjectInfo info_;
};

/**
 * @brief List objects response
 */
struct ListObjectsResponse {
    Vector<Object> objects;
    String continuation_token;
    bool is_truncated{false};
    int64_t total_count{0};

    Json::Value to_json() const;
};

/**
 * @brief Upload object request
 */
struct UploadObjectRequest {
    String bucket;
    String key;
    String content_type;
    StringMap metadata;
    StringMap user_metadata;
    Optional<String> storage_class;

    static UploadObjectRequest from_json(const Json::Value& json);
};

/**
 * @brief Object tags
 */
struct ObjectTags {
    StringMap tags;

    Json::Value to_json() const;
    static ObjectTags from_json(const Json::Value& json);
};

/**
 * @brief Object retention settings
 */
struct ObjectRetention {
    enum class Mode {
        Governance,
        Compliance
    };

    Mode mode;
    TimePoint retain_until_date;

    Json::Value to_json() const;
    static ObjectRetention from_json(const Json::Value& json);
};

/**
 * @brief Legal hold status
 */
struct LegalHold {
    bool status{false};

    Json::Value to_json() const;
    static LegalHold from_json(const Json::Value& json);
};

} // namespace console::models
