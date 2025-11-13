#include "console/clients/LocalStorageClient.hpp"

#include "console/common/Logger.hpp"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <openssl/md5.h>
#include <sstream>

namespace console::clients {

LocalStorageClient::LocalStorageClient(const String& storage_root)
    : path_manager_(std::make_unique<storage::PathManager>(storage_root)),
      metadata_manager_(std::make_unique<storage::MetadataManager>()) {
    CONSOLE_LOG_INFO("LocalStorageClient initialized with root: {}", storage_root);
    initialize_storage();
}

// ============================================================================
// Connection
// ============================================================================

Result<bool, String>
LocalStorageClient::is_connected() {
    // Check if storage root exists and is accessible
    try {
        auto root = path_manager_->storage_root();
        return Ok<bool, String>(std::filesystem::exists(root));
    } catch (const std::exception& e) {
        return Err<bool, String>(String("Storage check failed: ") + e.what());
    }
}

// ============================================================================
// Bucket operations
// ============================================================================

Result<Vector<models::Bucket>, String>
LocalStorageClient::list_buckets() {
    std::shared_lock lock(buckets_mutex_);

    try {
        Vector<models::Bucket> buckets;
        auto buckets_root = path_manager_->buckets_root();

        if (!std::filesystem::exists(buckets_root)) {
            return Ok<Vector<models::Bucket>, String>(buckets);
        }

        for (const auto& entry : std::filesystem::directory_iterator(buckets_root)) {
            if (entry.is_directory()) {
                String bucket_name = entry.path().filename().string();
                auto metadata_path = path_manager_->bucket_metadata_path(bucket_name);

                if (std::filesystem::exists(metadata_path)) {
                    auto meta_result = metadata_manager_->read_bucket_metadata(metadata_path);
                    if (meta_result) {
                        buckets.push_back(metadata_manager_->bucket_metadata_to_model(meta_result.value()));
                    }
                }
            }
        }

        CONSOLE_LOG_DEBUG("Listed {} buckets", buckets.size());
        return Ok<Vector<models::Bucket>, String>(buckets);

    } catch (const std::exception& e) {
        return Err<Vector<models::Bucket>, String>(String("Failed to list buckets: ") + e.what());
    }
}

Result<models::Bucket, String>
LocalStorageClient::get_bucket(const String& name) {
    std::shared_lock lock(buckets_mutex_);

    try {
        auto metadata_path = path_manager_->bucket_metadata_path(name);

        if (!std::filesystem::exists(metadata_path)) {
            return Err<models::Bucket, String>("Bucket not found: " + name);
        }

        auto meta_result = metadata_manager_->read_bucket_metadata(metadata_path);
        if (!meta_result) {
            return Err<models::Bucket, String>(meta_result.error());
        }

        return Ok<models::Bucket, String>(metadata_manager_->bucket_metadata_to_model(meta_result.value()));

    } catch (const std::exception& e) {
        return Err<models::Bucket, String>(String("Failed to get bucket: ") + e.what());
    }
}

Result<bool, String>
LocalStorageClient::create_bucket(const String& name, const String& region) {
    std::unique_lock lock(buckets_mutex_);

    try {
        // Validate bucket name
        if (!path_manager_->is_valid_bucket_name(name)) {
            return Err<bool, String>("Invalid bucket name: " + name);
        }

        auto bucket_path = path_manager_->bucket_path(name);

        // Check if bucket already exists
        if (std::filesystem::exists(bucket_path)) {
            return Err<bool, String>("Bucket already exists: " + name);
        }

        // Create bucket directory structure
        std::filesystem::create_directories(bucket_path);
        std::filesystem::create_directories(path_manager_->objects_root(name));
        std::filesystem::create_directories(path_manager_->versions_root(name));
        std::filesystem::create_directories(path_manager_->multipart_root(name));

        // Create bucket metadata
        storage::BucketMetadata metadata;
        metadata.name = name;
        metadata.creation_date = std::chrono::system_clock::now();
        metadata.region = region.empty() ? "us-east-1" : region;
        metadata.versioning_enabled = false;
        metadata.object_locking = false;
        metadata.owner = "admin"; // TODO: Get from current user
        metadata.object_count = 0;
        metadata.total_size_bytes = 0;

        auto metadata_path = path_manager_->bucket_metadata_path(name);
        auto write_result = metadata_manager_->write_bucket_metadata(metadata_path, metadata);

        if (!write_result) {
            // Cleanup on failure
            std::filesystem::remove_all(bucket_path);
            return Err<bool, String>(write_result.error());
        }

        CONSOLE_LOG_INFO("Created bucket: {}", name);
        return Ok<bool, String>(true);

    } catch (const std::exception& e) {
        return Err<bool, String>(String("Failed to create bucket: ") + e.what());
    }
}

Result<bool, String>
LocalStorageClient::delete_bucket(const String& name) {
    std::unique_lock lock(buckets_mutex_);

    try {
        auto bucket_path = path_manager_->bucket_path(name);

        if (!std::filesystem::exists(bucket_path)) {
            return Err<bool, String>("Bucket not found: " + name);
        }

        // Check if bucket is empty
        auto objects_root = path_manager_->objects_root(name);
        if (std::filesystem::exists(objects_root) && !std::filesystem::is_empty(objects_root)) {
            return Err<bool, String>("Bucket is not empty: " + name);
        }

        // Delete bucket and all metadata
        std::filesystem::remove_all(bucket_path);

        CONSOLE_LOG_INFO("Deleted bucket: {}", name);
        return Ok<bool, String>(true);

    } catch (const std::exception& e) {
        return Err<bool, String>(String("Failed to delete bucket: ") + e.what());
    }
}

Result<bool, String>
LocalStorageClient::bucket_exists(const String& name) {
    std::shared_lock lock(buckets_mutex_);

    try {
        auto bucket_path = path_manager_->bucket_path(name);
        return Ok<bool, String>(std::filesystem::exists(bucket_path));
    } catch (const std::exception& e) {
        return Err<bool, String>(String("Failed to check bucket existence: ") + e.what());
    }
}

// ============================================================================
// Object operations
// ============================================================================

Result<models::ListObjectsResponse, String>
LocalStorageClient::list_objects(const String& bucket_name, const ListObjectsOptions& options) {
    std::shared_lock lock(objects_mutex_);

    try {
        // Check bucket exists
        auto exists_result = bucket_exists(bucket_name);
        if (!exists_result || !exists_result.value()) {
            return Err<models::ListObjectsResponse, String>("Bucket not found: " + bucket_name);
        }

        models::ListObjectsResponse response;

        auto objects_root = path_manager_->objects_root(bucket_name);
        if (!std::filesystem::exists(objects_root)) {
            return Ok<models::ListObjectsResponse, String>(response);
        }

        // Iterate through object files
        for (const auto& entry : std::filesystem::recursive_directory_iterator(objects_root)) {
            if (entry.is_regular_file() && entry.path().extension() != ".meta" && entry.path().extension() != ".tags") {
                // Get relative path from objects_root
                auto relative_path = std::filesystem::relative(entry.path(), objects_root);
                String key = relative_path.string();

                // Apply prefix filter
                if (!options.prefix.empty() && key.find(options.prefix) != 0) {
                    continue;
                }

                // Read object metadata
                auto meta_path = path_manager_->object_metadata_path(bucket_name, key);
                if (std::filesystem::exists(meta_path)) {
                    auto meta_result = metadata_manager_->read_object_metadata(meta_path);
                    if (meta_result) {
                        response.objects.push_back(metadata_manager_->object_metadata_to_model(meta_result.value()));
                    }
                }

                // Limit results
                if (response.objects.size() >= static_cast<size_t>(options.max_keys)) {
                    response.is_truncated = true;
                    break;
                }
            }
        }

        CONSOLE_LOG_DEBUG("Listed {} objects in bucket {}", response.objects.size(), bucket_name);
        return Ok<models::ListObjectsResponse, String>(response);

    } catch (const std::exception& e) {
        return Err<models::ListObjectsResponse, String>(String("Failed to list objects: ") + e.what());
    }
}

Result<models::Object, String>
LocalStorageClient::stat_object(const String& bucket_name, const String& object_key) {
    std::shared_lock lock(objects_mutex_);

    try {
        auto meta_path = path_manager_->object_metadata_path(bucket_name, object_key);

        if (!std::filesystem::exists(meta_path)) {
            return Err<models::Object, String>("Object not found: " + object_key);
        }

        auto meta_result = metadata_manager_->read_object_metadata(meta_path);
        if (!meta_result) {
            return Err<models::Object, String>(meta_result.error());
        }

        return Ok<models::Object, String>(metadata_manager_->object_metadata_to_model(meta_result.value()));

    } catch (const std::exception& e) {
        return Err<models::Object, String>(String("Failed to stat object: ") + e.what());
    }
}

Result<ByteArray, String>
LocalStorageClient::get_object(const String& bucket_name, const String& object_key) {
    std::shared_lock lock(objects_mutex_);

    try {
        auto object_path = path_manager_->object_path(bucket_name, object_key);

        if (!std::filesystem::exists(object_path)) {
            return Err<ByteArray, String>("Object not found: " + object_key);
        }

        return atomic_read(object_path);

    } catch (const std::exception& e) {
        return Err<ByteArray, String>(String("Failed to get object: ") + e.what());
    }
}

Result<models::Object, String>
LocalStorageClient::put_object(const String& bucket_name,
                               const String& object_key,
                               const ByteArray& data,
                               const String& content_type,
                               const StringMap& metadata) {
    std::unique_lock lock(objects_mutex_);

    try {
        // Validate object key
        if (!path_manager_->is_valid_object_key(object_key)) {
            return Err<models::Object, String>("Invalid object key: " + object_key);
        }

        // Check bucket exists
        auto exists_result = bucket_exists(bucket_name);
        if (!exists_result || !exists_result.value()) {
            return Err<models::Object, String>("Bucket not found: " + bucket_name);
        }

        auto object_path = path_manager_->object_path(bucket_name, object_key);

        // Create parent directories
        std::filesystem::create_directories(object_path.parent_path());

        // Write object data atomically
        auto write_result = atomic_write(object_path, data);
        if (!write_result) {
            return Err<models::Object, String>(write_result.error());
        }

        // Compute ETag (MD5)
        String etag = compute_md5(data);

        // Create object metadata
        storage::ObjectMetadata obj_meta;
        obj_meta.key = object_key;
        obj_meta.bucket = bucket_name;
        obj_meta.size = data.size();
        obj_meta.etag = etag;
        obj_meta.content_type = content_type;
        obj_meta.last_modified = std::chrono::system_clock::now();
        obj_meta.version_id = "null"; // TODO: Implement versioning
        obj_meta.metadata = metadata;

        auto meta_path = path_manager_->object_metadata_path(bucket_name, object_key);
        auto meta_write_result = metadata_manager_->write_object_metadata(meta_path, obj_meta);

        if (!meta_write_result) {
            // Cleanup on failure
            std::filesystem::remove(object_path);
            return Err<models::Object, String>(meta_write_result.error());
        }

        CONSOLE_LOG_INFO("Put object: {}/{} ({} bytes)", bucket_name, object_key, data.size());

        return Ok<models::Object, String>(metadata_manager_->object_metadata_to_model(obj_meta));

    } catch (const std::exception& e) {
        return Err<models::Object, String>(String("Failed to put object: ") + e.what());
    }
}

Result<bool, String>
LocalStorageClient::delete_object(const String& bucket_name, const String& object_key) {
    std::unique_lock lock(objects_mutex_);

    try {
        auto object_path = path_manager_->object_path(bucket_name, object_key);
        auto meta_path = path_manager_->object_metadata_path(bucket_name, object_key);
        auto tags_path = path_manager_->object_tags_path(bucket_name, object_key);

        if (!std::filesystem::exists(object_path)) {
            return Err<bool, String>("Object not found: " + object_key);
        }

        // Delete object file and metadata
        std::filesystem::remove(object_path);
        if (std::filesystem::exists(meta_path)) {
            std::filesystem::remove(meta_path);
        }
        if (std::filesystem::exists(tags_path)) {
            std::filesystem::remove(tags_path);
        }

        CONSOLE_LOG_INFO("Deleted object: {}/{}", bucket_name, object_key);
        return Ok<bool, String>(true);

    } catch (const std::exception& e) {
        return Err<bool, String>(String("Failed to delete object: ") + e.what());
    }
}

Result<bool, String>
LocalStorageClient::copy_object(const String& source_bucket,
                                const String& source_key,
                                const String& dest_bucket,
                                const String& dest_key) {
    try {
        // Get source object (get_object has its own lock)
        auto get_result = get_object(source_bucket, source_key);
        if (!get_result) {
            return Err<bool, String>(get_result.error());
        }

        // Get source metadata (needs lock for filesystem access)
        storage::ObjectMetadata source_meta;
        {
            std::shared_lock lock(objects_mutex_);
            auto meta_path = path_manager_->object_metadata_path(source_bucket, source_key);
            auto meta_result = metadata_manager_->read_object_metadata(meta_path);
            if (!meta_result) {
                return Err<bool, String>(meta_result.error());
            }
            source_meta = meta_result.value();
        }

        // Put object in destination (put_object has its own lock)
        auto put_result = put_object(
            dest_bucket, dest_key, get_result.value(), source_meta.content_type, source_meta.metadata);

        if (!put_result) {
            return Err<bool, String>(put_result.error());
        }

        // Copy tags if exist
        {
            std::shared_lock lock(objects_mutex_);
            auto tags_path = path_manager_->object_tags_path(source_bucket, source_key);
            if (std::filesystem::exists(tags_path)) {
                auto tags_result = metadata_manager_->read_tags(tags_path);
                if (tags_result) {
                    auto dest_tags_path = path_manager_->object_tags_path(dest_bucket, dest_key);
                    metadata_manager_->write_tags(dest_tags_path, tags_result.value());
                }
            }
        }

        CONSOLE_LOG_INFO("Copied object from {}/{} to {}/{}", source_bucket, source_key, dest_bucket, dest_key);
        return Ok<bool, String>(true);

    } catch (const std::exception& e) {
        return Err<bool, String>(String("Failed to copy object: ") + e.what());
    }
}

// ============================================================================
// Object metadata
// ============================================================================

Result<StringMap, String>
LocalStorageClient::get_object_metadata(const String& bucket_name, const String& object_key) {
    std::shared_lock lock(objects_mutex_);

    try {
        auto meta_path = path_manager_->object_metadata_path(bucket_name, object_key);

        if (!std::filesystem::exists(meta_path)) {
            return Err<StringMap, String>("Object not found: " + object_key);
        }

        auto meta_result = metadata_manager_->read_object_metadata(meta_path);
        if (!meta_result) {
            return Err<StringMap, String>(meta_result.error());
        }

        return Ok<StringMap, String>(meta_result.value().metadata);

    } catch (const std::exception& e) {
        return Err<StringMap, String>(String("Failed to get object metadata: ") + e.what());
    }
}

Result<bool, String>
LocalStorageClient::set_object_metadata(const String& bucket_name,
                                        const String& object_key,
                                        const StringMap& metadata) {
    std::unique_lock lock(objects_mutex_);

    try {
        auto meta_path = path_manager_->object_metadata_path(bucket_name, object_key);

        if (!std::filesystem::exists(meta_path)) {
            return Err<bool, String>("Object not found: " + object_key);
        }

        auto meta_result = metadata_manager_->read_object_metadata(meta_path);
        if (!meta_result) {
            return Err<bool, String>(meta_result.error());
        }

        auto obj_meta = meta_result.value();
        obj_meta.metadata = metadata;
        obj_meta.last_modified = std::chrono::system_clock::now();

        auto write_result = metadata_manager_->write_object_metadata(meta_path, obj_meta);
        if (!write_result) {
            return Err<bool, String>(write_result.error());
        }

        return Ok<bool, String>(true);

    } catch (const std::exception& e) {
        return Err<bool, String>(String("Failed to set object metadata: ") + e.what());
    }
}

// ============================================================================
// Object tags
// ============================================================================

Result<StringMap, String>
LocalStorageClient::get_object_tags(const String& bucket_name, const String& object_key) {
    std::shared_lock lock(objects_mutex_);

    try {
        auto tags_path = path_manager_->object_tags_path(bucket_name, object_key);

        if (!std::filesystem::exists(tags_path)) {
            // Return empty tags if file doesn't exist
            return Ok<StringMap, String>(StringMap{});
        }

        return metadata_manager_->read_tags(tags_path);

    } catch (const std::exception& e) {
        return Err<StringMap, String>(String("Failed to get object tags: ") + e.what());
    }
}

Result<bool, String>
LocalStorageClient::set_object_tags(const String& bucket_name, const String& object_key, const StringMap& tags) {
    std::unique_lock lock(objects_mutex_);

    try {
        // Check object exists
        auto object_path = path_manager_->object_path(bucket_name, object_key);
        if (!std::filesystem::exists(object_path)) {
            return Err<bool, String>("Object not found: " + object_key);
        }

        auto tags_path = path_manager_->object_tags_path(bucket_name, object_key);
        auto write_result = metadata_manager_->write_tags(tags_path, tags);

        if (!write_result) {
            return Err<bool, String>(write_result.error());
        }

        return Ok<bool, String>(true);

    } catch (const std::exception& e) {
        return Err<bool, String>(String("Failed to set object tags: ") + e.what());
    }
}

// ============================================================================
// Presigned URLs
// ============================================================================

Result<String, String>
LocalStorageClient::generate_presigned_url(const String& bucket_name,
                                           const String& object_key,
                                           int64_t expires_in_seconds,
                                           const String& method) {
    try {
        // Check object exists
        auto object_path = path_manager_->object_path(bucket_name, object_key);
        if (!std::filesystem::exists(object_path)) {
            return Err<String, String>("Object not found: " + object_key);
        }

        // Generate presigned URL
        // Format: /api/v1/objects/{bucket}/{key}?expires={timestamp}&signature={signature}
        auto expires_at = std::chrono::system_clock::now() + std::chrono::seconds(expires_in_seconds);
        auto timestamp = std::chrono::system_clock::to_time_t(expires_at);

        std::ostringstream url;
        url << "/api/v1/objects/" << bucket_name << "/" << object_key << "?expires=" << timestamp
            << "&method=" << method;

        // TODO: Add HMAC signature for security

        return Ok<String, String>(url.str());

    } catch (const std::exception& e) {
        return Err<String, String>(String("Failed to generate presigned URL: ") + e.what());
    }
}

// ============================================================================
// Helper methods
// ============================================================================

void
LocalStorageClient::initialize_storage() {
    try {
        // Create base directory structure
        std::filesystem::create_directories(path_manager_->buckets_root());
        std::filesystem::create_directories(path_manager_->users_root());
        std::filesystem::create_directories(path_manager_->groups_root());
        std::filesystem::create_directories(path_manager_->policies_root());
        std::filesystem::create_directories(path_manager_->config_root());

        CONSOLE_LOG_INFO("Storage structure initialized");
    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Failed to initialize storage: {}", e.what());
    }
}

String
LocalStorageClient::compute_md5(const ByteArray& data) const {
    unsigned char hash[MD5_DIGEST_LENGTH];
    MD5(data.data(), data.size(), hash);

    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        ss << std::setw(2) << static_cast<unsigned>(hash[i]);
    }
    return ss.str();
}

Result<void, String>
LocalStorageClient::atomic_write(const std::filesystem::path& path, const ByteArray& data) {
    try {
        // Write to temporary file
        auto temp_path = path.string() + ".tmp";
        std::ofstream file(temp_path, std::ios::binary);
        if (!file.is_open()) {
            return Result<void, String>(err_tag, "Failed to open file for writing");
        }

        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        file.close();

        // Atomic rename
        std::filesystem::rename(temp_path, path);

        return Ok<String>();
    } catch (const std::exception& e) {
        return Result<void, String>(err_tag, String("Atomic write failed: ") + e.what());
    }
}

Result<ByteArray, String>
LocalStorageClient::atomic_read(const std::filesystem::path& path) {
    try {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return Err<ByteArray, String>("Failed to open file for reading");
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        ByteArray buffer(size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            return Err<ByteArray, String>("Failed to read file");
        }

        return Ok<ByteArray, String>(buffer);
    } catch (const std::exception& e) {
        return Err<ByteArray, String>(String("Atomic read failed: ") + e.what());
    }
}

bool
LocalStorageClient::is_valid_bucket_name(const String& name) const {
    return path_manager_->is_valid_bucket_name(name);
}

bool
LocalStorageClient::is_valid_object_key(const String& key) const {
    return path_manager_->is_valid_object_key(key);
}

} // namespace console::clients
