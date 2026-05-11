#pragma once

#include "console/common/Types.hpp"

#include <json/json.h>

namespace console::models {

/**
 * @brief Bucket model
 */
class Bucket {
  public:
    Bucket() = default;
    Bucket(const BucketInfo& info) : info_(info) {}

    // Getters
    const String& name() const { return info_.name; }
    const TimePoint& creation_date() const { return info_.creation_date; }
    const String& region() const { return info_.region; }
    int64_t size_bytes() const { return info_.size_bytes; }
    int64_t size_with_metadata() const { return info_.size_with_metadata; }
    int64_t object_count() const { return info_.object_count; }
    bool versioning_enabled() const { return info_.versioning_enabled; }
    bool encryption_enabled() const { return info_.encryption_enabled; }
    const String& encryption_type() const { return info_.encryption_type; }
    const StringMap& tags() const { return info_.tags; }

    // Setters
    void set_name(const String& name) { info_.name = name; }
    void set_region(const String& region) { info_.region = region; }
    void set_versioning_enabled(bool enabled) { info_.versioning_enabled = enabled; }
    void set_encryption_enabled(bool enabled) { info_.encryption_enabled = enabled; }
    void set_encryption_type(const String& type) { info_.encryption_type = type; }

    // Serialization
    Json::Value to_json() const;
    static Bucket from_json(const Json::Value& json);

  private:
    BucketInfo info_;
};

} // namespace console::models
