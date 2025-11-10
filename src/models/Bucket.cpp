
//
#include "console/models/Bucket.hpp"

namespace console::models {

Json::Value
Bucket::to_json() const {
    Json::Value json;
    json["name"] = info_.name;
    json["region"] = info_.region;
    json["size_bytes"] = static_cast<Json::Int64>(info_.size_bytes);
    json["object_count"] = static_cast<Json::Int64>(info_.object_count);
    json["versioning_enabled"] = info_.versioning_enabled;

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
    info.object_count = json.get("object_count", 0).asInt64();
    info.versioning_enabled = json.get("versioning_enabled", false).asBool();

    // Parse tags
    if (json.isMember("tags") && json["tags"].isObject()) {
        for (const auto& key : json["tags"].getMemberNames()) {
            info.tags[key] = json["tags"][key].asString();
        }
    }

    return Bucket(info);
}

} // namespace console::models
