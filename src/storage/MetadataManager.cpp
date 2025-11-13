#include "console/storage/MetadataManager.hpp"

#include "console/common/Logger.hpp"

#include <chrono>
#include <fstream>
#include <sstream>

namespace console::storage {

// ============================================================================
// BucketMetadata
// ============================================================================

Json::Value
BucketMetadata::to_json() const {
    Json::Value json;
    json["name"] = name;
    json["creation_date"] = std::chrono::system_clock::to_time_t(creation_date);
    json["region"] = region;
    json["versioning_enabled"] = versioning_enabled;
    json["object_locking"] = object_locking;
    json["owner"] = owner;
    json["statistics"]["object_count"] = static_cast<Json::Value::UInt64>(object_count);
    json["statistics"]["total_size_bytes"] = static_cast<Json::Value::UInt64>(total_size_bytes);
    return json;
}

BucketMetadata
BucketMetadata::from_json(const Json::Value& json) {
    BucketMetadata meta;
    meta.name = json.get("name", "").asString();

    if (json.isMember("creation_date")) {
        auto time_t_val = json["creation_date"].asInt64();
        meta.creation_date = std::chrono::system_clock::from_time_t(time_t_val);
    } else {
        meta.creation_date = std::chrono::system_clock::now();
    }

    meta.region = json.get("region", "us-east-1").asString();
    meta.versioning_enabled = json.get("versioning_enabled", false).asBool();
    meta.object_locking = json.get("object_locking", false).asBool();
    meta.owner = json.get("owner", "").asString();

    if (json.isMember("statistics")) {
        auto stats = json["statistics"];
        meta.object_count = stats.get("object_count", 0).asUInt64();
        meta.total_size_bytes = stats.get("total_size_bytes", 0).asUInt64();
    }

    return meta;
}

// ============================================================================
// ObjectMetadata
// ============================================================================

Json::Value
ObjectMetadata::to_json() const {
    Json::Value json;
    json["key"] = key;
    json["bucket"] = bucket;
    json["size"] = static_cast<Json::Value::UInt64>(size);
    json["etag"] = etag;
    json["content_type"] = content_type;
    json["last_modified"] = std::chrono::system_clock::to_time_t(last_modified);
    json["version_id"] = version_id;

    // Custom metadata
    Json::Value meta_json(Json::objectValue);
    for (const auto& [key, value] : metadata) {
        meta_json[key] = value;
    }
    json["metadata"] = meta_json;

    return json;
}

ObjectMetadata
ObjectMetadata::from_json(const Json::Value& json) {
    ObjectMetadata meta;
    meta.key = json.get("key", "").asString();
    meta.bucket = json.get("bucket", "").asString();
    meta.size = json.get("size", 0).asUInt64();
    meta.etag = json.get("etag", "").asString();
    meta.content_type = json.get("content_type", "application/octet-stream").asString();

    if (json.isMember("last_modified")) {
        auto time_t_val = json["last_modified"].asInt64();
        meta.last_modified = std::chrono::system_clock::from_time_t(time_t_val);
    } else {
        meta.last_modified = std::chrono::system_clock::now();
    }

    meta.version_id = json.get("version_id", "").asString();

    // Custom metadata
    if (json.isMember("metadata")) {
        auto meta_json = json["metadata"];
        for (const auto& key : meta_json.getMemberNames()) {
            meta.metadata[key] = meta_json[key].asString();
        }
    }

    return meta;
}

// ============================================================================
// MetadataManager - Bucket metadata
// ============================================================================

Result<BucketMetadata, String>
MetadataManager::read_bucket_metadata(const std::filesystem::path& path) const {
    auto json_result = read_json_file(path);
    if (!json_result) {
        return Err<BucketMetadata, String>(json_result.error());
    }

    try {
        return Ok<BucketMetadata, String>(BucketMetadata::from_json(json_result.value()));
    } catch (const std::exception& e) {
        return Err<BucketMetadata, String>(String("Failed to parse bucket metadata: ") + e.what());
    }
}

Result<void, String>
MetadataManager::write_bucket_metadata(const std::filesystem::path& path, const BucketMetadata& metadata) const {
    try {
        auto json = metadata.to_json();
        return write_json_file(path, json);
    } catch (const std::exception& e) {
        return Result<void, String>(err_tag, String("Failed to write bucket metadata: ") + e.what());
    }
}

// ============================================================================
// MetadataManager - Object metadata
// ============================================================================

Result<ObjectMetadata, String>
MetadataManager::read_object_metadata(const std::filesystem::path& path) const {
    auto json_result = read_json_file(path);
    if (!json_result) {
        return Err<ObjectMetadata, String>(json_result.error());
    }

    try {
        return Ok<ObjectMetadata, String>(ObjectMetadata::from_json(json_result.value()));
    } catch (const std::exception& e) {
        return Err<ObjectMetadata, String>(String("Failed to parse object metadata: ") + e.what());
    }
}

Result<void, String>
MetadataManager::write_object_metadata(const std::filesystem::path& path, const ObjectMetadata& metadata) const {
    try {
        auto json = metadata.to_json();
        return write_json_file(path, json);
    } catch (const std::exception& e) {
        return Result<void, String>(err_tag, String("Failed to write object metadata: ") + e.what());
    }
}

// ============================================================================
// MetadataManager - Tags
// ============================================================================

Result<StringMap, String>
MetadataManager::read_tags(const std::filesystem::path& path) const {
    auto json_result = read_json_file(path);
    if (!json_result) {
        return Err<StringMap, String>(json_result.error());
    }

    try {
        StringMap tags;
        auto json = json_result.value();
        for (const auto& key : json.getMemberNames()) {
            tags[key] = json[key].asString();
        }
        return Ok<StringMap, String>(tags);
    } catch (const std::exception& e) {
        return Err<StringMap, String>(String("Failed to parse tags: ") + e.what());
    }
}

Result<void, String>
MetadataManager::write_tags(const std::filesystem::path& path, const StringMap& tags) const {
    try {
        Json::Value json(Json::objectValue);
        for (const auto& [key, value] : tags) {
            json[key] = value;
        }
        return write_json_file(path, json);
    } catch (const std::exception& e) {
        return Result<void, String>(err_tag, String("Failed to write tags: ") + e.what());
    }
}

// ============================================================================
// MetadataManager - Policy
// ============================================================================

Result<String, String>
MetadataManager::read_policy(const std::filesystem::path& path) const {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            return Err<String, String>("Failed to open policy file");
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return Ok<String, String>(buffer.str());
    } catch (const std::exception& e) {
        return Err<String, String>(String("Failed to read policy: ") + e.what());
    }
}

Result<void, String>
MetadataManager::write_policy(const std::filesystem::path& path, const String& policy_json) const {
    try {
        // Create parent directory if needed
        std::filesystem::create_directories(path.parent_path());

        std::ofstream file(path);
        if (!file.is_open()) {
            return Result<void, String>(err_tag, "Failed to open policy file for writing");
        }

        file << policy_json;
        file.close();

        return Ok<String>();
    } catch (const std::exception& e) {
        return Result<void, String>(err_tag, String("Failed to write policy: ") + e.what());
    }
}

// ============================================================================
// MetadataManager - Versioning
// ============================================================================

Result<bool, String>
MetadataManager::read_versioning(const std::filesystem::path& path) const {
    auto json_result = read_json_file(path);
    if (!json_result) {
        // Default to false if file doesn't exist
        return Ok<bool, String>(false);
    }

    try {
        bool enabled = json_result.value().get("enabled", false).asBool();
        return Ok<bool, String>(enabled);
    } catch (const std::exception& e) {
        return Err<bool, String>(String("Failed to parse versioning config: ") + e.what());
    }
}

Result<void, String>
MetadataManager::write_versioning(const std::filesystem::path& path, bool enabled) const {
    try {
        Json::Value json;
        json["enabled"] = enabled;
        return write_json_file(path, json);
    } catch (const std::exception& e) {
        return Result<void, String>(err_tag, String("Failed to write versioning config: ") + e.what());
    }
}

// ============================================================================
// MetadataManager - Model conversion
// ============================================================================

models::Bucket
MetadataManager::bucket_metadata_to_model(const BucketMetadata& meta) const {
    BucketInfo info;
    info.name = meta.name;
    info.region = meta.region;
    info.creation_date = meta.creation_date;
    info.versioning_enabled = meta.versioning_enabled;
    return models::Bucket(info);
}

models::Object
MetadataManager::object_metadata_to_model(const ObjectMetadata& meta) const {
    ObjectInfo info;
    info.key = meta.key;
    info.bucket = meta.bucket;
    info.size = meta.size;
    info.etag = meta.etag;
    info.last_modified = meta.last_modified;
    info.content_type = meta.content_type;
    info.metadata = meta.metadata;
    return models::Object(info);
}

// ============================================================================
// MetadataManager - Helper methods
// ============================================================================

Result<Json::Value, String>
MetadataManager::read_json_file(const std::filesystem::path& path) const {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            return Err<Json::Value, String>("Failed to open file: " + path.string());
        }

        Json::Value json;
        Json::CharReaderBuilder builder;
        String errors;

        if (!Json::parseFromStream(builder, file, &json, &errors)) {
            return Err<Json::Value, String>("JSON parse error: " + errors);
        }

        return Ok<Json::Value, String>(json);
    } catch (const std::exception& e) {
        return Err<Json::Value, String>(String("Failed to read JSON file: ") + e.what());
    }
}

Result<void, String>
MetadataManager::write_json_file(const std::filesystem::path& path, const Json::Value& json) const {
    try {
        // Create parent directory if needed
        std::filesystem::create_directories(path.parent_path());

        // Write to temporary file first (atomic operation)
        auto temp_path = path.string() + ".tmp";
        std::ofstream file(temp_path);
        if (!file.is_open()) {
            return Result<void, String>(err_tag, "Failed to open file for writing: " + path.string());
        }

        Json::StreamWriterBuilder builder;
        builder["indentation"] = "  ";
        std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
        writer->write(json, &file);
        file.close();

        // Atomic rename
        std::filesystem::rename(temp_path, path);

        return Ok<String>();
    } catch (const std::exception& e) {
        return Result<void, String>(err_tag, String("Failed to write JSON file: ") + e.what());
    }
}

} // namespace console::storage
