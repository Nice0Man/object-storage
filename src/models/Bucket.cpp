
//
#include "console/models/Bucket.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace console::models {

Json::Value
Bucket::to_json() const {
    Json::Value json;
    json["name"] = info_.name;
    json["region"] = info_.region;

    // Add both naming conventions for compatibility
    json["size_bytes"] = static_cast<Json::Int64>(info_.size_bytes);
    json["size"] = static_cast<Json::Int64>(info_.size_bytes);
    json["size_with_metadata"] = static_cast<Json::Int64>(info_.size_with_metadata);
    json["object_count"] = static_cast<Json::Int64>(info_.object_count);
    json["objects_count"] = static_cast<Json::Int64>(info_.object_count);
    json["versioning_enabled"] = info_.versioning_enabled;
    json["versioning"] = info_.versioning_enabled;

    // Encryption info
    json["encryption_enabled"] = info_.encryption_enabled;
    json["encryption_type"] = info_.encryption_type;

    // Format creation_date as ISO 8601 string
    auto time_t_val = std::chrono::system_clock::to_time_t(info_.creation_date);
    std::tm tm_val{};
#ifdef _WIN32
    gmtime_s(&tm_val, &time_t_val);
#else
    gmtime_r(&time_t_val, &tm_val);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_val, "%Y-%m-%dT%H:%M:%SZ");
    json["creation_date"] = oss.str();

    // Convert tags to JSON object
    Json::Value tags_json(Json::objectValue);
    for (const auto& [key, value] : info_.tags) {
        tags_json[key] = value;
    }
    json["tags"] = tags_json;

    return json;
}

Bucket
Bucket::from_json(const Json::Value& json) {
    BucketInfo info;
    info.name = json.get("name", "").asString();
    info.region = json.get("region", "").asString();
    info.size_bytes = json.get("size_bytes", 0).asInt64();
    info.size_with_metadata = json.get("size_with_metadata", 0).asInt64();
    info.object_count = json.get("object_count", 0).asInt64();
    info.versioning_enabled = json.get("versioning_enabled", false).asBool();
    info.encryption_enabled = json.get("encryption_enabled", false).asBool();
    info.encryption_type = json.get("encryption_type", "").asString();

    // Parse tags
    if (json.isMember("tags") && json["tags"].isObject()) {
        for (const auto& key : json["tags"].getMemberNames()) {
            info.tags[key] = json["tags"][key].asString();
        }
    }

    return Bucket(info);
}

} // namespace console::models
