
//
#include "console/models/Object.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace console::models {

String
Object::extension() const {
    const auto& key = info_.key;
    auto pos = key.find_last_of('.');
    if (pos != String::npos && pos < key.length() - 1) {
        return key.substr(pos + 1);
    }
    return "";
}

bool
Object::is_directory() const {
    return info_.key.ends_with('/');
}

String
Object::human_readable_size() const {
    const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int unit_index = 0;
    double size = static_cast<double>(info_.size);

    while (size >= 1024.0 && unit_index < 5) {
        size /= 1024.0;
        ++unit_index;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
    return oss.str();
}

Json::Value
Object::to_json() const {
    Json::Value json;
    json["key"] = info_.key;
    json["bucket"] = info_.bucket;
    json["size"] = Json::Int64(info_.size);

    auto time_t = std::chrono::system_clock::to_time_t(info_.last_modified);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    json["last_modified"] = oss.str();

    json["etag"] = info_.etag;
    json["content_type"] = info_.content_type;
    json["storage_class"] = info_.storage_class;

    Json::Value metadata_obj;
    for (const auto& [key, value] : info_.metadata) {
        metadata_obj[key] = value;
    }
    json["metadata"] = metadata_obj;

    Json::Value user_metadata_obj;
    for (const auto& [key, value] : info_.user_metadata) {
        user_metadata_obj[key] = value;
    }
    json["user_metadata"] = user_metadata_obj;

    return json;
}

Object
Object::from_json(const Json::Value& json) {
    ObjectInfo info;
    info.key = json.get("key", "").asString();
    info.bucket = json.get("bucket", "").asString();
    info.size = json.get("size", 0).asInt64();
    info.etag = json.get("etag", "").asString();
    info.content_type = json.get("content_type", "").asString();
    info.storage_class = json.get("storage_class", "STANDARD").asString();

    // Parse metadata
    if (json.isMember("metadata")) {
        const auto& metadata = json["metadata"];
        for (const auto& key : metadata.getMemberNames()) {
            info.metadata[key] = metadata[key].asString();
        }
    }

    if (json.isMember("user_metadata")) {
        const auto& user_metadata = json["user_metadata"];
        for (const auto& key : user_metadata.getMemberNames()) {
            info.user_metadata[key] = user_metadata[key].asString();
        }
    }

    return Object(info);
}

Json::Value
ListObjectsResponse::to_json() const {
    Json::Value json;

    Json::Value objects_array(Json::arrayValue);
    for (const auto& obj : objects) {
        objects_array.append(obj.to_json());
    }
    json["objects"] = objects_array;

    json["continuation_token"] = continuation_token;
    json["is_truncated"] = is_truncated;
    json["total_count"] = Json::Int64(total_count);

    return json;
}

UploadObjectRequest
UploadObjectRequest::from_json(const Json::Value& json) {
    UploadObjectRequest req;
    req.bucket = json.get("bucket", "").asString();
    req.key = json.get("key", "").asString();
    req.content_type = json.get("content_type", "application/octet-stream").asString();

    if (json.isMember("metadata")) {
        const auto& metadata = json["metadata"];
        for (const auto& key : metadata.getMemberNames()) {
            req.metadata[key] = metadata[key].asString();
        }
    }

    if (json.isMember("user_metadata")) {
        const auto& user_metadata = json["user_metadata"];
        for (const auto& key : user_metadata.getMemberNames()) {
            req.user_metadata[key] = user_metadata[key].asString();
        }
    }

    if (json.isMember("storage_class")) {
        req.storage_class = json["storage_class"].asString();
    }

    return req;
}

Json::Value
ObjectTags::to_json() const {
    Json::Value json;
    for (const auto& [key, value] : tags) {
        json[key] = value;
    }
    return json;
}

ObjectTags
ObjectTags::from_json(const Json::Value& json) {
    ObjectTags tags_obj;
    for (const auto& key : json.getMemberNames()) {
        tags_obj.tags[key] = json[key].asString();
    }
    return tags_obj;
}

Json::Value
ObjectRetention::to_json() const {
    Json::Value json;
    json["mode"] = (mode == Mode::Governance) ? "GOVERNANCE" : "COMPLIANCE";

    auto time_t = std::chrono::system_clock::to_time_t(retain_until_date);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    json["retain_until_date"] = oss.str();

    return json;
}

ObjectRetention
ObjectRetention::from_json(const Json::Value& json) {
    ObjectRetention retention;

    String mode_str = json.get("mode", "GOVERNANCE").asString();
    retention.mode = (mode_str == "COMPLIANCE") ? Mode::Compliance : Mode::Governance;

    // TODO(Nice0Man): Parse date string to TimePoint

    return retention;
}

Json::Value
LegalHold::to_json() const {
    Json::Value json;
    json["status"] = status ? "ON" : "OFF";
    return json;
}

LegalHold
LegalHold::from_json(const Json::Value& json) {
    LegalHold hold;
    String status_str = json.get("status", "OFF").asString();
    hold.status = (status_str == "ON");
    return hold;
}

} // namespace console::models
