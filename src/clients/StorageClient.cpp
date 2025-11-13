#include "console/clients/StorageClient.hpp"

#include "console/common/Logger.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <json/json.h>
#include <sstream>

namespace fs = std::filesystem;

namespace console {
namespace clients {

// ============================================================================
// File-based Storage Implementation
// ============================================================================

class FileStorage {
  public:
    explicit FileStorage(const String& base_path) : base_path_(base_path) {
        // Create base storage directory
        fs::create_directories(base_path_);
        fs::create_directories(base_path_ + "/.metadata");
        CONSOLE_LOG_INFO("FileStorage initialized at: {}", base_path_);
    }

    String bucket_path(const String& bucket_name) const { return base_path_ + "/" + bucket_name; }

    String object_path(const String& bucket_name, const String& object_key) const {
        return bucket_path(bucket_name) + "/" + object_key;
    }

    String bucket_metadata_path(const String& bucket_name) const {
        return base_path_ + "/.metadata/" + bucket_name + ".json";
    }

    bool bucket_exists(const String& bucket_name) const {
        return fs::exists(bucket_path(bucket_name)) && fs::is_directory(bucket_path(bucket_name));
    }

    bool object_exists(const String& bucket_name, const String& object_key) const {
        return fs::exists(object_path(bucket_name, object_key)) &&
               fs::is_regular_file(object_path(bucket_name, object_key));
    }

  private:
    String base_path_;
};

// Global storage instance
static std::unique_ptr<FileStorage> g_storage;

// ============================================================================
// StorageClient Implementation (File-based)
// ============================================================================

StorageClient::StorageClient(const String& endpoint, const String& access_key, const String& secret_key, bool use_ssl)
    : endpoint_(endpoint), access_key_(access_key), secret_key_(secret_key), use_ssl_(use_ssl) {
    // Initialize file storage in ./storage directory
    String storage_path = "./storage";
    if (!g_storage) {
        g_storage = std::make_unique<FileStorage>(storage_path);
    }

    CONSOLE_LOG_INFO("StorageClient initialized (FILE-BASED): storage={}", storage_path);
}

Result<bool, String>
StorageClient::is_connected() {
    // TODO: Implement real health check
    return Ok<bool, String>(true);
}

// ============================================================================
// Bucket Operations
// ============================================================================

Result<Vector<models::Bucket>, String>
StorageClient::list_buckets() {
    CONSOLE_LOG_DEBUG("Listing buckets from filesystem");

    if (!g_storage) {
        return Err<Vector<models::Bucket>, String>("Storage not initialized");
    }

    Vector<models::Bucket> buckets;

    try {
        String base_path = "./storage";
        if (!fs::exists(base_path)) {
            return Ok<Vector<models::Bucket>, String>(buckets);
        }

        for (const auto& entry : fs::directory_iterator(base_path)) {
            if (entry.is_directory() && entry.path().filename() != ".metadata") {
                String bucket_name = entry.path().filename().string();

                // Load bucket metadata
                String metadata_path = base_path + "/.metadata/" + bucket_name + ".json";
                BucketInfo info;
                info.name = bucket_name;

                if (fs::exists(metadata_path)) {
                    std::ifstream file(metadata_path);
                    Json::Value root;
                    file >> root;

                    info.region = root.get("region", "us-east-1").asString();

                    // Parse creation date
                    String date_str = root.get("creation_date", "").asString();
                    if (!date_str.empty()) {
                        std::tm tm = {};
                        std::istringstream ss(date_str);
                        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
                        info.creation_date = std::chrono::system_clock::from_time_t(std::mktime(&tm));
                    } else {
                        info.creation_date = std::chrono::system_clock::now();
                    }
                } else {
                    info.region = "us-east-1";
                    info.creation_date = std::chrono::system_clock::now();
                }

                // Calculate size and object count
                info.size_bytes = 0;
                info.object_count = 0;

                for (const auto& obj_entry : fs::recursive_directory_iterator(entry.path())) {
                    if (obj_entry.is_regular_file()) {
                        info.size_bytes += fs::file_size(obj_entry.path());
                        info.object_count++;
                    }
                }

                info.versioning_enabled = false;

                buckets.push_back(models::Bucket(info));
            }
        }

        CONSOLE_LOG_INFO("Found {} buckets", buckets.size());
        return Ok<Vector<models::Bucket>, String>(buckets);

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Failed to list buckets: {}", e.what());
        return Err<Vector<models::Bucket>, String>(String("Failed to list buckets: ") + e.what());
    }
}

Result<models::Bucket, String>
StorageClient::get_bucket(const String& name) {
    CONSOLE_LOG_DEBUG("Getting bucket: {}", name);

    if (!g_storage) {
        return Err<models::Bucket, String>("Storage not initialized");
    }

    if (!g_storage->bucket_exists(name)) {
        return Err<models::Bucket, String>("Bucket not found: " + name);
    }

    try {
        BucketInfo info;
        info.name = name;

        // Load metadata
        String metadata_path = g_storage->bucket_metadata_path(name);
        if (fs::exists(metadata_path)) {
            std::ifstream file(metadata_path);
            Json::Value root;
            file >> root;

            info.region = root.get("region", "us-east-1").asString();
            info.versioning_enabled = root.get("versioning_enabled", false).asBool();

            String date_str = root.get("creation_date", "").asString();
            if (!date_str.empty()) {
                std::tm tm = {};
                std::istringstream ss(date_str);
                ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
                info.creation_date = std::chrono::system_clock::from_time_t(std::mktime(&tm));
            } else {
                info.creation_date = std::chrono::system_clock::now();
            }
        } else {
            info.region = "us-east-1";
            info.versioning_enabled = false;
            info.creation_date = std::chrono::system_clock::now();
        }

        // Calculate size and object count
        info.size_bytes = 0;
        info.object_count = 0;

        String bucket_path = g_storage->bucket_path(name);
        if (fs::exists(bucket_path)) {
            for (const auto& entry : fs::recursive_directory_iterator(bucket_path)) {
                if (entry.is_regular_file()) {
                    info.size_bytes += fs::file_size(entry.path());
                    info.object_count++;
                }
            }
        }

        return Ok<models::Bucket, String>(models::Bucket(info));

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Failed to get bucket {}: {}", name, e.what());
        return Err<models::Bucket, String>(String("Failed to get bucket: ") + e.what());
    }
}

Result<bool, String>
StorageClient::create_bucket(const String& name, const String& region) {
    CONSOLE_LOG_INFO("Creating bucket: {} in region: {}", name, region);

    if (!g_storage) {
        return Err<bool, String>("Storage not initialized");
    }

    if (g_storage->bucket_exists(name)) {
        return Err<bool, String>("Bucket already exists: " + name);
    }

    try {
        // Create bucket directory
        String bucket_path = g_storage->bucket_path(name);
        fs::create_directories(bucket_path);

        // Save bucket metadata
        Json::Value metadata;
        metadata["name"] = name;
        metadata["region"] = region.empty() ? "us-east-1" : region;

        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now;
#ifdef _WIN32
        localtime_s(&tm_now, &time_t_now);
#else
        localtime_r(&time_t_now, &tm_now);
#endif

        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm_now);
        metadata["creation_date"] = String(buffer);
        metadata["versioning_enabled"] = false;

        String metadata_path = g_storage->bucket_metadata_path(name);
        std::ofstream file(metadata_path);
        file << metadata;
        file.close();

        CONSOLE_LOG_INFO("Bucket created: {}", name);
        return Ok<bool, String>(true);

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Failed to create bucket {}: {}", name, e.what());
        return Err<bool, String>(String("Failed to create bucket: ") + e.what());
    }
}

Result<bool, String>
StorageClient::delete_bucket(const String& name) {
    CONSOLE_LOG_INFO("Deleting bucket: {}", name);

    if (!g_storage) {
        return Err<bool, String>("Storage not initialized");
    }

    if (!g_storage->bucket_exists(name)) {
        return Err<bool, String>("Bucket not found: " + name);
    }

    try {
        String bucket_path = g_storage->bucket_path(name);

        // Check if bucket is empty
        bool is_empty = true;
        for (const auto& entry : fs::directory_iterator(bucket_path)) {
            is_empty = false;
            break;
        }

        if (!is_empty) {
            return Err<bool, String>("Bucket is not empty: " + name);
        }

        // Remove bucket directory and metadata
        fs::remove_all(bucket_path);

        String metadata_path = g_storage->bucket_metadata_path(name);
        if (fs::exists(metadata_path)) {
            fs::remove(metadata_path);
        }

        CONSOLE_LOG_INFO("Bucket deleted: {}", name);
        return Ok<bool, String>(true);

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Failed to delete bucket {}: {}", name, e.what());
        return Err<bool, String>(String("Failed to delete bucket: ") + e.what());
    }
}

Result<bool, String>
StorageClient::bucket_exists(const String& name) {
    CONSOLE_LOG_DEBUG("Checking if bucket exists: {}", name);

    if (!g_storage) {
        return Err<bool, String>("Storage not initialized");
    }

    return Ok<bool, String>(g_storage->bucket_exists(name));
}

// ============================================================================
// Object Operations
// ============================================================================

Result<models::ListObjectsResponse, String>
StorageClient::list_objects(const String& bucket_name, const ListObjectsOptions& options) {
    CONSOLE_LOG_DEBUG("Listing objects in bucket: {} with prefix: {}", bucket_name, options.prefix);
    // TODO: Implement real object listing
    return Err<models::ListObjectsResponse, String>("Not implemented: list_objects");
}

Result<models::Object, String>
StorageClient::stat_object(const String& bucket_name, const String& object_key) {
    CONSOLE_LOG_DEBUG("Getting object stat: {}/{}", bucket_name, object_key);
    // TODO: Implement real object stat
    return Err<models::Object, String>("Not implemented: stat_object");
}

Result<ByteArray, String>
StorageClient::get_object(const String& bucket_name, const String& object_key) {
    CONSOLE_LOG_DEBUG("Getting object: {}/{}", bucket_name, object_key);
    // TODO: Implement real object download
    return Err<ByteArray, String>("Not implemented: get_object");
}

Result<models::Object, String>
StorageClient::put_object(const String& bucket_name,
                          const String& object_key,
                          const ByteArray& data,
                          const String& content_type,
                          const StringMap& metadata) {
    CONSOLE_LOG_INFO("Putting object: {}/{} ({} bytes)", bucket_name, object_key, data.size());
    // TODO: Implement real object upload
    return Err<models::Object, String>("Not implemented: put_object");
}

Result<bool, String>
StorageClient::delete_object(const String& bucket_name, const String& object_key) {
    CONSOLE_LOG_INFO("Deleting object: {}/{}", bucket_name, object_key);
    // TODO: Implement real object deletion
    return Err<bool, String>("Not implemented: delete_object");
}

Result<bool, String>
StorageClient::copy_object(const String& source_bucket,
                           const String& source_key,
                           const String& dest_bucket,
                           const String& dest_key) {
    CONSOLE_LOG_INFO("Copying object: {}/{} -> {}/{}", source_bucket, source_key, dest_bucket, dest_key);
    // TODO: Implement real object copy
    return Err<bool, String>("Not implemented: copy_object");
}

// ============================================================================
// Object Metadata
// ============================================================================

Result<StringMap, String>
StorageClient::get_object_metadata(const String& bucket_name, const String& object_key) {
    CONSOLE_LOG_DEBUG("Getting metadata: {}/{}", bucket_name, object_key);
    // TODO: Implement real metadata retrieval
    return Err<StringMap, String>("Not implemented: get_object_metadata");
}

Result<bool, String>
StorageClient::set_object_metadata(const String& bucket_name, const String& object_key, const StringMap& metadata) {
    CONSOLE_LOG_DEBUG("Setting metadata: {}/{}", bucket_name, object_key);
    // TODO: Implement real metadata update
    return Err<bool, String>("Not implemented: set_object_metadata");
}

Result<StringMap, String>
StorageClient::get_object_tags(const String& bucket_name, const String& object_key) {
    CONSOLE_LOG_DEBUG("Getting tags: {}/{}", bucket_name, object_key);
    // TODO: Implement real tags retrieval
    return Err<StringMap, String>("Not implemented: get_object_tags");
}

Result<bool, String>
StorageClient::set_object_tags(const String& bucket_name, const String& object_key, const StringMap& tags) {
    CONSOLE_LOG_DEBUG("Setting tags: {}/{}", bucket_name, object_key);
    // TODO: Implement real tags update
    return Err<bool, String>("Not implemented: set_object_tags");
}

// ============================================================================
// Presigned URLs
// ============================================================================

Result<String, String>
StorageClient::generate_presigned_url(const String& bucket_name,
                                      const String& object_key,
                                      int64_t expires_in_seconds,
                                      const String& method) {
    CONSOLE_LOG_DEBUG("Generating presigned URL: {}/{}", bucket_name, object_key);
    // TODO: Implement AWS Signature V4
    return Err<String, String>("Not implemented: generate_presigned_url");
}

} // namespace clients
} // namespace console
