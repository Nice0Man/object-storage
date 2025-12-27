#include "console/clients/LocalStorageClient.hpp"

#include "console/common/Config.hpp"
#include "console/common/Logger.hpp"
#include "console/common/ServiceLocator.hpp"
#include "console/services/EncryptionService.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/md5.h>
#include <random>
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

// Helper function to check if a file is a metadata file
bool
is_metadata_file(const String& filename) {
    // Skip all metadata files: *.meta, *.meta.json, .metadata.json, encryption.json, etc.
    if (filename.ends_with(".meta") || filename.ends_with(".meta.json") || filename == ".metadata.json" ||
        filename == "encryption.json" || filename == "lifecycle.json" || filename == "versioning.json" ||
        filename == "policy.json" || filename == "tags.json") {
        return true;
    }
    return false;
}

// Bucket statistics structure
struct BucketStats {
    size_t object_size{0};  // Size of actual objects only
    size_t total_size{0};   // Total size including metadata
    size_t object_count{0}; // Number of actual objects (not metadata)
};

// Helper function to calculate bucket statistics
BucketStats
calculate_bucket_stats(const std::filesystem::path& objects_root) {
    BucketStats stats;

    if (!std::filesystem::exists(objects_root)) {
        return stats;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(objects_root)) {
        if (entry.is_regular_file()) {
            String filename = entry.path().filename().string();
            size_t file_size = entry.file_size();

            // Always add to total size (includes metadata)
            stats.total_size += file_size;

            // Only count non-metadata files for object stats
            if (!is_metadata_file(filename)) {
                stats.object_size += file_size;
                stats.object_count++;
            }
        }
    }

    return stats;
}

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
                        auto meta = meta_result.value();

                        // Calculate actual size and object count
                        auto objects_root = path_manager_->objects_root(bucket_name);
                        auto stats = calculate_bucket_stats(objects_root);
                        meta.total_size_bytes = stats.object_size;
                        meta.size_with_metadata = stats.total_size;
                        meta.object_count = stats.object_count;

                        // Check encryption status
                        auto encryption_path = entry.path() / "encryption.json";
                        if (std::filesystem::exists(encryption_path)) {
                            try {
                                std::ifstream config_file(encryption_path);
                                Json::Value config;
                                Json::CharReaderBuilder reader;
                                std::string errors;
                                if (Json::parseFromStream(reader, config_file, &config, &errors)) {
                                    meta.encryption_enabled = config.get("enabled", false).asBool();
                                    meta.encryption_type = config.get("algorithm", "").asString();
                                    if (meta.encryption_type.empty() && meta.encryption_enabled) {
                                        meta.encryption_type = "SSE-S3";
                                    }
                                }
                            } catch (...) {
                                // Ignore encryption read errors
                            }
                        }

                        buckets.push_back(metadata_manager_->bucket_metadata_to_model(meta));
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

        auto meta = meta_result.value();

        // Calculate actual size and object count
        auto objects_root = path_manager_->objects_root(name);
        auto stats = calculate_bucket_stats(objects_root);
        meta.total_size_bytes = stats.object_size;
        meta.size_with_metadata = stats.total_size;
        meta.object_count = stats.object_count;

        // Check encryption status
        auto bucket_path = path_manager_->bucket_path(name);
        auto encryption_path = bucket_path / "encryption.json";
        if (std::filesystem::exists(encryption_path)) {
            try {
                std::ifstream config_file(encryption_path);
                Json::Value config;
                Json::CharReaderBuilder reader;
                std::string errors;
                if (Json::parseFromStream(reader, config_file, &config, &errors)) {
                    meta.encryption_enabled = config.get("enabled", false).asBool();
                    meta.encryption_type = config.get("algorithm", "").asString();
                    if (meta.encryption_type.empty() && meta.encryption_enabled) {
                        meta.encryption_type = "SSE-S3";
                    }
                }
            } catch (...) {
                // Ignore encryption read errors
            }
        }

        return Ok<models::Bucket, String>(metadata_manager_->bucket_metadata_to_model(meta));

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
        metadata.owner = ""; // Owner is set by the caller context
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
        // SECURITY: Validate object key
        if (!path_manager_->is_valid_object_key(object_key)) {
            return Err<models::Object, String>("Invalid object key: " + object_key);
        }

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
    // Delegate to SSE-aware version with empty customer key
    return get_object(bucket_name, object_key, "");
}

Result<ByteArray, String>
LocalStorageClient::get_object(const String& bucket_name, const String& object_key, const String& sse_customer_key) {
    std::shared_lock lock(objects_mutex_);

    try {
        // SECURITY: Validate object key
        if (!path_manager_->is_valid_object_key(object_key)) {
            return Err<ByteArray, String>("Invalid object key: " + object_key);
        }

        auto object_path = path_manager_->object_path(bucket_name, object_key);

        if (!std::filesystem::exists(object_path)) {
            return Err<ByteArray, String>("Object not found: " + object_key);
        }

        // Read the raw data from disk
        auto read_result = atomic_read(object_path);
        if (!read_result) {
            return read_result;
        }

        ByteArray data = read_result.value();

        // Check object metadata for encryption info
        auto meta_path = path_manager_->object_metadata_path(bucket_name, object_key);
        if (!std::filesystem::exists(meta_path)) {
            // No metadata = not encrypted, return raw data
            return Ok<ByteArray, String>(std::move(data));
        }

        auto meta_result = metadata_manager_->read_object_metadata(meta_path);
        if (!meta_result) {
            // Can't read metadata, assume not encrypted
            return Ok<ByteArray, String>(std::move(data));
        }

        const auto& obj_meta = meta_result.value();

        // If object is not encrypted, return raw data
        if (!obj_meta.encrypted) {
            return Ok<ByteArray, String>(std::move(data));
        }

        // Object is encrypted - need to decrypt
        auto encryption_service = ServiceLocator::encryption_service();
        if (!encryption_service) {
            return Err<ByteArray, String>("Encryption service not available");
        }

        Result<ByteArray, String> decrypt_result = Err<ByteArray, String>("Decryption not performed");

        if (obj_meta.sse_type == "SSE-C") {
            // SSE-C: require customer key
            if (sse_customer_key.empty()) {
                return Err<ByteArray, String>(
                    "Object is encrypted with SSE-C. Customer encryption key is required to access this object.");
            }

            // Verify customer key MD5 matches
            String provided_key_md5 = compute_md5(ByteArray(sse_customer_key.begin(), sse_customer_key.end()));
            if (provided_key_md5 != obj_meta.sse_customer_key_md5) {
                return Err<ByteArray, String>(
                    "The provided encryption key does not match the key used to encrypt this object.");
            }

            decrypt_result = encryption_service->decrypt_with_customer_key(data, sse_customer_key);
        } else if (obj_meta.sse_type == "SSE-S3") {
            // SSE-S3: use server managed key
            decrypt_result = encryption_service->decrypt_from_storage(data);
        } else {
            return Err<ByteArray, String>("Unknown encryption type: " + obj_meta.sse_type);
        }

        if (!decrypt_result) {
            return Err<ByteArray, String>("Decryption failed: " + decrypt_result.error());
        }

        CONSOLE_LOG_DEBUG("Decrypted object {}/{} using {}", bucket_name, object_key, obj_meta.sse_type);

        return Ok<ByteArray, String>(decrypt_result.value());

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
    // Delegate to SSE-aware version with empty customer key
    return put_object(bucket_name, object_key, data, content_type, metadata, "");
}

Result<models::Object, String>
LocalStorageClient::put_object(const String& bucket_name,
                               const String& object_key,
                               const ByteArray& data,
                               const String& content_type,
                               const StringMap& metadata,
                               const String& sse_customer_key) {
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

        // Determine if encryption should be applied
        bool should_encrypt = false;
        String sse_type;
        String encryption_algorithm;
        String customer_key_md5;

        // Check for SSE-C (customer-provided key takes priority)
        if (!sse_customer_key.empty()) {
            if (sse_customer_key.size() != 64) { // 32 bytes = 64 hex chars
                return Err<models::Object, String>("Invalid SSE-C key size. Expected 64 hex characters for AES-256");
            }
            should_encrypt = true;
            sse_type = "SSE-C";
            encryption_algorithm = "AES256";
            // Compute MD5 of customer key for verification
            customer_key_md5 = compute_md5(ByteArray(sse_customer_key.begin(), sse_customer_key.end()));
        } else {
            // Check for bucket-level encryption (SSE-S3)
            auto encryption_result = get_bucket_encryption(bucket_name);
            if (encryption_result) {
                auto& enc_config = encryption_result.value();
                if (enc_config.get("enabled", false).asBool()) {
                    should_encrypt = true;
                    sse_type = "SSE-S3";
                    encryption_algorithm = enc_config.get("algorithm", "AES256").asString();
                }
            }
        }

        // Compute ETag from original data (before encryption)
        String etag = compute_md5(data);
        size_t original_size = data.size();

        // Data to write (may be encrypted)
        ByteArray data_to_write = data;

        // Apply encryption if needed
        if (should_encrypt) {
            auto encryption_service = ServiceLocator::encryption_service();
            if (!encryption_service) {
                return Err<models::Object, String>("Encryption service not available");
            }

            Result<ByteArray, String> encrypt_result = Err<ByteArray, String>("Encryption not performed");

            if (sse_type == "SSE-C") {
                encrypt_result = encryption_service->encrypt_with_customer_key(data, sse_customer_key);
            } else {
                // SSE-S3: use server managed key
                encrypt_result = encryption_service->encrypt_for_storage(data);
            }

            if (!encrypt_result) {
                return Err<models::Object, String>("Encryption failed: " + encrypt_result.error());
            }

            data_to_write = encrypt_result.value();
            CONSOLE_LOG_DEBUG("Encrypted object {}/{} using {} ({} -> {} bytes)",
                              bucket_name,
                              object_key,
                              sse_type,
                              original_size,
                              data_to_write.size());
        }

        // Write object data atomically
        auto write_result = atomic_write(object_path, data_to_write);
        if (!write_result) {
            return Err<models::Object, String>(write_result.error());
        }

        // Check if versioning is enabled for this bucket
        auto versioning_result = get_bucket_versioning(bucket_name);
        bool versioning_enabled = versioning_result && versioning_result.value();

        // Generate version ID if versioning is enabled
        String version_id = "null";
        if (versioning_enabled) {
            // Generate unique version ID: timestamp + random suffix
            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
            std::ostringstream vid;
            vid << std::hex << timestamp << "-" << std::setfill('0') << std::setw(8) << (std::rand() & 0xFFFFFFFF);
            version_id = vid.str();

            // Save previous version if exists
            auto existing_meta_path = path_manager_->object_metadata_path(bucket_name, object_key);
            if (std::filesystem::exists(existing_meta_path)) {
                auto versions_dir = path_manager_->versions_root(bucket_name) / object_key;
                std::filesystem::create_directories(versions_dir);

                // Read existing metadata to get its version
                auto existing_meta = metadata_manager_->read_object_metadata(existing_meta_path);
                if (existing_meta) {
                    String old_version = existing_meta.value().version_id;
                    if (old_version == "null") {
                        old_version = "initial";
                    }
                    // Copy existing object to versions directory
                    auto existing_obj_path = path_manager_->object_path(bucket_name, object_key);
                    if (std::filesystem::exists(existing_obj_path)) {
                        std::filesystem::copy(existing_obj_path,
                                              versions_dir / old_version,
                                              std::filesystem::copy_options::overwrite_existing);
                    }
                }
            }
        }

        // Create object metadata
        storage::ObjectMetadata obj_meta;
        obj_meta.key = object_key;
        obj_meta.bucket = bucket_name;
        obj_meta.size = data_to_write.size();   // Size on disk (encrypted)
        obj_meta.original_size = original_size; // Original size before encryption
        obj_meta.etag = etag;                   // ETag from original data
        obj_meta.content_type = content_type;
        obj_meta.last_modified = std::chrono::system_clock::now();
        obj_meta.version_id = version_id;
        obj_meta.metadata = metadata;

        // Set encryption metadata
        obj_meta.encrypted = should_encrypt;
        obj_meta.encryption_algorithm = encryption_algorithm;
        obj_meta.sse_type = sse_type;
        obj_meta.sse_customer_key_md5 = customer_key_md5;

        auto meta_path = path_manager_->object_metadata_path(bucket_name, object_key);
        auto meta_write_result = metadata_manager_->write_object_metadata(meta_path, obj_meta);

        if (!meta_write_result) {
            // Cleanup on failure
            std::filesystem::remove(object_path);
            return Err<models::Object, String>(meta_write_result.error());
        }

        CONSOLE_LOG_INFO("Put object: {}/{} ({} bytes{})",
                         bucket_name,
                         object_key,
                         original_size,
                         should_encrypt ? ", encrypted with " + sse_type : "");

        return Ok<models::Object, String>(metadata_manager_->object_metadata_to_model(obj_meta));

    } catch (const std::exception& e) {
        return Err<models::Object, String>(String("Failed to put object: ") + e.what());
    }
}

Result<bool, String>
LocalStorageClient::delete_object(const String& bucket_name, const String& object_key) {
    std::unique_lock lock(objects_mutex_);

    try {
        // SECURITY: Validate object key
        if (!path_manager_->is_valid_object_key(object_key)) {
            return Err<bool, String>("Invalid object key: " + object_key);
        }

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
        // Format: /api/v1/objects/{bucket}/{key}?expires={timestamp}&method={method}&signature={signature}
        auto expires_at = std::chrono::system_clock::now() + std::chrono::seconds(expires_in_seconds);
        auto timestamp = std::chrono::system_clock::to_time_t(expires_at);

        // Build string to sign: bucket/key\nexpires\nmethod
        std::ostringstream string_to_sign;
        string_to_sign << bucket_name << "/" << object_key << "\n" << timestamp << "\n" << method;

        // Get secret key from config - SECURITY: no fallback to hardcoded secrets
        auto& config = Config::instance();
        String secret_key = config.get<String>("presigned_url.secret_key").value_or(config.auth().jwt_secret);

        // SECURITY: Fail if no secret is configured
        if (secret_key.empty()) {
            CONSOLE_LOG_ERROR("Security: Presigned URL generation failed - no secret configured");
            return Err<String, String>("Presigned URLs are not configured - set auth.jwt_secret in config");
        }

        // Generate HMAC-SHA256 signature
        unsigned char hmac_result[EVP_MAX_MD_SIZE];
        unsigned int hmac_len = 0;

        HMAC(EVP_sha256(),
             secret_key.c_str(),
             static_cast<int>(secret_key.length()),
             reinterpret_cast<const unsigned char*>(string_to_sign.str().c_str()),
             string_to_sign.str().length(),
             hmac_result,
             &hmac_len);

        // Convert HMAC to hex string
        std::ostringstream signature;
        signature << std::hex << std::setfill('0');
        for (unsigned int i = 0; i < hmac_len; i++) {
            signature << std::setw(2) << static_cast<unsigned>(hmac_result[i]);
        }

        // Build final URL
        std::ostringstream url;
        url << "/api/v1/objects/" << bucket_name << "/" << object_key << "?expires=" << timestamp
            << "&method=" << method << "&signature=" << signature.str();

        CONSOLE_LOG_DEBUG("Generated presigned URL for {}/{} expires at {}", bucket_name, object_key, timestamp);
        return Ok<String, String>(url.str());

    } catch (const std::exception& e) {
        return Err<String, String>(String("Failed to generate presigned URL: ") + e.what());
    }
}

// ============================================================================
// Bucket versioning operations
// ============================================================================

Result<void, String>
LocalStorageClient::set_bucket_versioning(const String& bucket_name, bool enabled) {
    std::unique_lock lock(buckets_mutex_);

    try {
        // Check bucket exists
        auto bucket_path = path_manager_->bucket_path(bucket_name);
        if (!std::filesystem::exists(bucket_path)) {
            return Result<void, String>(err_tag, "Bucket not found: " + bucket_name);
        }

        // Read current bucket metadata
        auto metadata_path = path_manager_->bucket_metadata_path(bucket_name);
        auto read_result = metadata_manager_->read_bucket_metadata(metadata_path);
        if (!read_result) {
            return Result<void, String>(err_tag, "Failed to read bucket metadata: " + read_result.error());
        }

        // Update versioning
        auto metadata = read_result.value();
        metadata.versioning_enabled = enabled;

        // Write updated metadata
        auto write_result = metadata_manager_->write_bucket_metadata(metadata_path, metadata);
        if (!write_result) {
            return Result<void, String>(err_tag, "Failed to write bucket metadata: " + write_result.error());
        }

        CONSOLE_LOG_INFO("Bucket {} versioning set to {}", bucket_name, enabled);
        return Ok<String>();

    } catch (const std::exception& e) {
        return Result<void, String>(err_tag, String("Failed to set bucket versioning: ") + e.what());
    }
}

Result<bool, String>
LocalStorageClient::get_bucket_versioning(const String& bucket_name) {
    std::shared_lock lock(buckets_mutex_);

    try {
        // Check bucket exists
        auto bucket_path = path_manager_->bucket_path(bucket_name);
        if (!std::filesystem::exists(bucket_path)) {
            return Err<bool, String>("Bucket not found: " + bucket_name);
        }

        // Read bucket metadata
        auto metadata_path = path_manager_->bucket_metadata_path(bucket_name);
        auto read_result = metadata_manager_->read_bucket_metadata(metadata_path);
        if (!read_result) {
            return Err<bool, String>("Failed to read bucket metadata: " + read_result.error());
        }

        return Ok<bool, String>(read_result.value().versioning_enabled);

    } catch (const std::exception& e) {
        return Err<bool, String>(String("Failed to get bucket versioning: ") + e.what());
    }
}

// ============================================================================
// Bucket policy operations
// ============================================================================

Result<void, String>
LocalStorageClient::set_bucket_policy(const String& bucket_name, const String& policy_json) {
    std::unique_lock lock(buckets_mutex_);

    try {
        // Check bucket exists
        auto bucket_path = path_manager_->bucket_path(bucket_name);
        if (!std::filesystem::exists(bucket_path)) {
            return Result<void, String>(err_tag, "Bucket not found: " + bucket_name);
        }

        // Write policy to bucket policy file
        auto policy_path = bucket_path / "policy.json";
        auto write_result = metadata_manager_->write_policy(policy_path, policy_json);
        if (!write_result) {
            return Result<void, String>(err_tag, "Failed to write bucket policy: " + write_result.error());
        }

        CONSOLE_LOG_INFO("Bucket {} policy updated", bucket_name);
        return Ok<String>();

    } catch (const std::exception& e) {
        return Result<void, String>(err_tag, String("Failed to set bucket policy: ") + e.what());
    }
}

Result<String, String>
LocalStorageClient::get_bucket_policy(const String& bucket_name) {
    std::shared_lock lock(buckets_mutex_);

    try {
        // Check bucket exists
        auto bucket_path = path_manager_->bucket_path(bucket_name);
        if (!std::filesystem::exists(bucket_path)) {
            return Err<String, String>("Bucket not found: " + bucket_name);
        }

        // Read policy from bucket policy file
        auto policy_path = bucket_path / "policy.json";
        if (!std::filesystem::exists(policy_path)) {
            // Return empty policy if no policy file exists
            return Ok<String, String>("{}");
        }

        auto read_result = metadata_manager_->read_policy(policy_path);
        if (!read_result) {
            return Err<String, String>("Failed to read bucket policy: " + read_result.error());
        }

        return Ok<String, String>(read_result.value());

    } catch (const std::exception& e) {
        return Err<String, String>(String("Failed to get bucket policy: ") + e.what());
    }
}

// ============================================================================
// Bucket Tags Operations
// ============================================================================

Result<void, String>
LocalStorageClient::set_bucket_tags(const String& bucket_name, const StringMap& tags) {
    std::unique_lock lock(buckets_mutex_);

    try {
        auto bucket_path = path_manager_->bucket_path(bucket_name);
        if (!std::filesystem::exists(bucket_path)) {
            return Result<void, String>(err_tag, "Bucket not found: " + bucket_name);
        }

        // Write tags to bucket tags file
        auto tags_path = bucket_path / "tags.json";
        auto write_result = metadata_manager_->write_tags(tags_path, tags);
        if (!write_result) {
            return Result<void, String>(err_tag, "Failed to write bucket tags: " + write_result.error());
        }

        CONSOLE_LOG_INFO("Set {} tags for bucket {}", tags.size(), bucket_name);
        return Ok<String>();

    } catch (const std::exception& e) {
        return Result<void, String>(err_tag, String("Failed to set bucket tags: ") + e.what());
    }
}

Result<StringMap, String>
LocalStorageClient::get_bucket_tags(const String& bucket_name) {
    std::shared_lock lock(buckets_mutex_);

    try {
        auto bucket_path = path_manager_->bucket_path(bucket_name);
        if (!std::filesystem::exists(bucket_path)) {
            return Err<StringMap, String>("Bucket not found: " + bucket_name);
        }

        auto tags_path = bucket_path / "tags.json";
        if (!std::filesystem::exists(tags_path)) {
            // Return empty tags if no tags file exists
            return Ok<StringMap, String>(StringMap{});
        }

        auto read_result = metadata_manager_->read_tags(tags_path);
        if (!read_result) {
            return Err<StringMap, String>("Failed to read bucket tags: " + read_result.error());
        }

        return Ok<StringMap, String>(read_result.value());

    } catch (const std::exception& e) {
        return Err<StringMap, String>(String("Failed to get bucket tags: ") + e.what());
    }
}

Result<void, String>
LocalStorageClient::delete_bucket_tags(const String& bucket_name) {
    std::unique_lock lock(buckets_mutex_);

    try {
        auto bucket_path = path_manager_->bucket_path(bucket_name);
        if (!std::filesystem::exists(bucket_path)) {
            return Result<void, String>(err_tag, "Bucket not found: " + bucket_name);
        }

        auto tags_path = bucket_path / "tags.json";
        if (std::filesystem::exists(tags_path)) {
            std::filesystem::remove(tags_path);
        }

        CONSOLE_LOG_INFO("Deleted tags for bucket {}", bucket_name);
        return Ok<String>();

    } catch (const std::exception& e) {
        return Result<void, String>(err_tag, String("Failed to delete bucket tags: ") + e.what());
    }
}

// ============================================================================
// Bucket Encryption (SSE) Operations
// ============================================================================

Result<void, String>
LocalStorageClient::set_bucket_encryption(const String& bucket_name,
                                          bool enabled,
                                          const String& algorithm,
                                          const String& kms_key_id) {
    std::unique_lock lock(buckets_mutex_);

    try {
        auto bucket_path = path_manager_->bucket_path(bucket_name);
        if (!std::filesystem::exists(bucket_path)) {
            return Result<void, String>(err_tag, "Bucket not found: " + bucket_name);
        }

        // Validate algorithm
        if (algorithm != "AES256" && algorithm != "aws:kms") {
            return Result<void, String>(err_tag, "Invalid encryption algorithm. Use AES256 or aws:kms");
        }

        // Create encryption configuration
        Json::Value config;
        config["enabled"] = enabled;
        config["algorithm"] = algorithm;
        if (!kms_key_id.empty()) {
            config["kms_master_key_id"] = kms_key_id;
        }

        // Write configuration
        auto encryption_path = bucket_path / "encryption.json";
        Json::StreamWriterBuilder writer;
        std::ofstream config_file(encryption_path);
        config_file << Json::writeString(writer, config);
        config_file.close();

        CONSOLE_LOG_INFO("Set encryption for bucket {}: enabled={}, algorithm={}", bucket_name, enabled, algorithm);
        return Ok<String>();

    } catch (const std::exception& e) {
        return Result<void, String>(err_tag, String("Failed to set bucket encryption: ") + e.what());
    }
}

Result<Json::Value, String>
LocalStorageClient::get_bucket_encryption(const String& bucket_name) {
    std::shared_lock lock(buckets_mutex_);

    try {
        auto bucket_path = path_manager_->bucket_path(bucket_name);
        if (!std::filesystem::exists(bucket_path)) {
            return Err<Json::Value, String>("Bucket not found: " + bucket_name);
        }

        auto encryption_path = bucket_path / "encryption.json";
        if (!std::filesystem::exists(encryption_path)) {
            // Return default (disabled) configuration
            Json::Value config;
            config["enabled"] = false;
            config["algorithm"] = "AES256";
            return Ok<Json::Value, String>(config);
        }

        std::ifstream config_file(encryption_path);
        Json::Value config;
        Json::CharReaderBuilder reader;
        std::string errors;
        if (!Json::parseFromStream(reader, config_file, &config, &errors)) {
            return Err<Json::Value, String>("Failed to parse encryption configuration");
        }

        return Ok<Json::Value, String>(config);

    } catch (const std::exception& e) {
        return Err<Json::Value, String>(String("Failed to get bucket encryption: ") + e.what());
    }
}

Result<void, String>
LocalStorageClient::delete_bucket_encryption(const String& bucket_name) {
    std::unique_lock lock(buckets_mutex_);

    try {
        auto bucket_path = path_manager_->bucket_path(bucket_name);
        if (!std::filesystem::exists(bucket_path)) {
            return Result<void, String>(err_tag, "Bucket not found: " + bucket_name);
        }

        auto encryption_path = bucket_path / "encryption.json";
        if (std::filesystem::exists(encryption_path)) {
            std::filesystem::remove(encryption_path);
        }

        CONSOLE_LOG_INFO("Deleted encryption configuration for bucket {}", bucket_name);
        return Ok<String>();

    } catch (const std::exception& e) {
        return Result<void, String>(err_tag, String("Failed to delete bucket encryption: ") + e.what());
    }
}

// ============================================================================
// Multipart Upload Operations
// ============================================================================

Result<MultipartUploadInfo, String>
LocalStorageClient::initiate_multipart_upload(const String& bucket_name,
                                              const String& object_key,
                                              const String& content_type,
                                              const StringMap& metadata) {
    std::unique_lock lock(multipart_mutex_);

    try {
        // Check bucket exists
        auto exists_result = bucket_exists(bucket_name);
        if (!exists_result || !exists_result.value()) {
            return Err<MultipartUploadInfo, String>("Bucket not found: " + bucket_name);
        }

        // Validate object key
        if (!path_manager_->is_valid_object_key(object_key)) {
            return Err<MultipartUploadInfo, String>("Invalid object key: " + object_key);
        }

        // Generate upload ID
        String upload_id = generate_upload_id();

        // Create multipart upload directory
        auto upload_path = get_multipart_upload_path(bucket_name, upload_id);
        std::filesystem::create_directories(upload_path);

        // Create upload metadata
        MultipartUploadInfo info;
        info.upload_id = upload_id;
        info.bucket = bucket_name;
        info.key = object_key;
        info.content_type = content_type;
        info.metadata = metadata;
        info.initiated = std::time(nullptr);

        // Write upload info to metadata file
        Json::Value meta_json;
        meta_json["upload_id"] = upload_id;
        meta_json["bucket"] = bucket_name;
        meta_json["key"] = object_key;
        meta_json["content_type"] = content_type;
        meta_json["initiated"] = static_cast<Json::Int64>(info.initiated);

        Json::Value meta_obj(Json::objectValue);
        for (const auto& [k, v] : metadata) {
            meta_obj[k] = v;
        }
        meta_json["metadata"] = meta_obj;

        auto meta_path = upload_path / "upload.json";
        Json::StreamWriterBuilder writer;
        std::ofstream meta_file(meta_path);
        meta_file << Json::writeString(writer, meta_json);
        meta_file.close();

        CONSOLE_LOG_INFO("Initiated multipart upload: {} for {}/{}", upload_id, bucket_name, object_key);
        return Ok<MultipartUploadInfo, String>(info);

    } catch (const std::exception& e) {
        return Err<MultipartUploadInfo, String>(String("Failed to initiate multipart upload: ") + e.what());
    }
}

Result<String, String>
LocalStorageClient::upload_part(const String& bucket_name,
                                const String& object_key,
                                const String& upload_id,
                                int part_number,
                                const ByteArray& data) {
    std::unique_lock lock(multipart_mutex_);

    try {
        // Validate part number (1-10000)
        if (part_number < 1 || part_number > 10000) {
            return Err<String, String>("Part number must be between 1 and 10000");
        }

        // Check upload exists
        auto upload_path = get_multipart_upload_path(bucket_name, upload_id);
        if (!std::filesystem::exists(upload_path)) {
            return Err<String, String>("Upload not found: " + upload_id);
        }

        // Compute ETag (MD5) for the part
        String etag = compute_md5(data);

        // Write part data
        auto part_path = upload_path / ("part_" + std::to_string(part_number));
        auto write_result = atomic_write(part_path, data);
        if (!write_result) {
            return Err<String, String>("Failed to write part: " + write_result.error());
        }

        // Write part metadata
        Json::Value part_meta;
        part_meta["part_number"] = part_number;
        part_meta["etag"] = etag;
        part_meta["size"] = static_cast<Json::Int64>(data.size());
        part_meta["last_modified"] = static_cast<Json::Int64>(std::time(nullptr));

        auto part_meta_path = upload_path / ("part_" + std::to_string(part_number) + ".meta");
        Json::StreamWriterBuilder writer;
        std::ofstream meta_file(part_meta_path);
        meta_file << Json::writeString(writer, part_meta);
        meta_file.close();

        CONSOLE_LOG_DEBUG("Uploaded part {} for upload {} ({} bytes)", part_number, upload_id, data.size());
        return Ok<String, String>(etag);

    } catch (const std::exception& e) {
        return Err<String, String>(String("Failed to upload part: ") + e.what());
    }
}

Result<models::Object, String>
LocalStorageClient::complete_multipart_upload(const String& bucket_name,
                                              const String& object_key,
                                              const String& upload_id,
                                              const Vector<CompletedPart>& parts) {
    std::unique_lock lock(multipart_mutex_);

    try {
        // Check upload exists
        auto upload_path = get_multipart_upload_path(bucket_name, upload_id);
        if (!std::filesystem::exists(upload_path)) {
            return Err<models::Object, String>("Upload not found: " + upload_id);
        }

        // Read upload metadata
        auto meta_path = upload_path / "upload.json";
        std::ifstream meta_file(meta_path);
        Json::Value meta_json;
        Json::CharReaderBuilder reader;
        std::string errors;
        if (!Json::parseFromStream(reader, meta_file, &meta_json, &errors)) {
            return Err<models::Object, String>("Failed to read upload metadata");
        }
        meta_file.close();

        String content_type = meta_json.get("content_type", "application/octet-stream").asString();
        StringMap metadata;
        if (meta_json.isMember("metadata")) {
            for (const auto& key : meta_json["metadata"].getMemberNames()) {
                metadata[key] = meta_json["metadata"][key].asString();
            }
        }

        // Verify all parts exist and ETags match
        for (const auto& completed_part : parts) {
            auto part_path = upload_path / ("part_" + std::to_string(completed_part.part_number));
            auto part_meta_path = upload_path / ("part_" + std::to_string(completed_part.part_number) + ".meta");

            if (!std::filesystem::exists(part_path)) {
                return Err<models::Object, String>("Part not found: " + std::to_string(completed_part.part_number));
            }

            // Verify ETag
            std::ifstream part_meta_file(part_meta_path);
            Json::Value part_meta;
            if (Json::parseFromStream(reader, part_meta_file, &part_meta, &errors)) {
                String stored_etag = part_meta.get("etag", "").asString();
                if (stored_etag != completed_part.etag) {
                    return Err<models::Object, String>("ETag mismatch for part " +
                                                       std::to_string(completed_part.part_number));
                }
            }
            part_meta_file.close();
        }

        // Sort parts by part number
        auto sorted_parts = parts;
        std::sort(sorted_parts.begin(), sorted_parts.end(), [](const CompletedPart& a, const CompletedPart& b) {
            return a.part_number < b.part_number;
        });

        // Concatenate all parts
        ByteArray final_data;
        for (const auto& completed_part : sorted_parts) {
            auto part_path = upload_path / ("part_" + std::to_string(completed_part.part_number));
            auto part_result = atomic_read(part_path);
            if (!part_result) {
                return Err<models::Object, String>("Failed to read part: " + part_result.error());
            }
            final_data.insert(final_data.end(), part_result.value().begin(), part_result.value().end());
        }

        // Release lock to call put_object (which acquires objects_mutex_)
        lock.unlock();

        // Write the final object using put_object
        auto put_result = put_object(bucket_name, object_key, final_data, content_type, metadata);

        // Re-acquire lock to cleanup
        lock.lock();

        if (!put_result) {
            return Err<models::Object, String>("Failed to create final object: " + put_result.error());
        }

        // Cleanup multipart upload directory
        std::filesystem::remove_all(upload_path);

        CONSOLE_LOG_INFO("Completed multipart upload: {} for {}/{} ({} bytes)",
                         upload_id,
                         bucket_name,
                         object_key,
                         final_data.size());
        return put_result;

    } catch (const std::exception& e) {
        return Err<models::Object, String>(String("Failed to complete multipart upload: ") + e.what());
    }
}

Result<void, String>
LocalStorageClient::abort_multipart_upload(const String& bucket_name,
                                           const String& object_key,
                                           const String& upload_id) {
    std::unique_lock lock(multipart_mutex_);

    try {
        auto upload_path = get_multipart_upload_path(bucket_name, upload_id);
        if (!std::filesystem::exists(upload_path)) {
            return Result<void, String>(err_tag, "Upload not found: " + upload_id);
        }

        // Remove the upload directory and all parts
        std::filesystem::remove_all(upload_path);

        CONSOLE_LOG_INFO("Aborted multipart upload: {} for {}/{}", upload_id, bucket_name, object_key);
        return Ok<String>();

    } catch (const std::exception& e) {
        return Result<void, String>(err_tag, String("Failed to abort multipart upload: ") + e.what());
    }
}

Result<MultipartUploadInfo, String>
LocalStorageClient::list_parts(const String& bucket_name, const String& object_key, const String& upload_id) {
    std::shared_lock lock(multipart_mutex_);

    try {
        auto upload_path = get_multipart_upload_path(bucket_name, upload_id);
        if (!std::filesystem::exists(upload_path)) {
            return Err<MultipartUploadInfo, String>("Upload not found: " + upload_id);
        }

        // Read upload metadata
        auto meta_path = upload_path / "upload.json";
        std::ifstream meta_file(meta_path);
        Json::Value meta_json;
        Json::CharReaderBuilder reader;
        std::string errors;
        if (!Json::parseFromStream(reader, meta_file, &meta_json, &errors)) {
            return Err<MultipartUploadInfo, String>("Failed to read upload metadata");
        }
        meta_file.close();

        MultipartUploadInfo info;
        info.upload_id = upload_id;
        info.bucket = bucket_name;
        info.key = meta_json.get("key", object_key).asString();
        info.content_type = meta_json.get("content_type", "application/octet-stream").asString();
        info.initiated = meta_json.get("initiated", 0).asInt64();

        if (meta_json.isMember("metadata")) {
            for (const auto& key : meta_json["metadata"].getMemberNames()) {
                info.metadata[key] = meta_json["metadata"][key].asString();
            }
        }

        // List all parts
        for (const auto& entry : std::filesystem::directory_iterator(upload_path)) {
            String filename = entry.path().filename().string();
            if (filename.starts_with("part_") && filename.ends_with(".meta")) {
                std::ifstream part_meta_file(entry.path());
                Json::Value part_meta;
                if (Json::parseFromStream(reader, part_meta_file, &part_meta, &errors)) {
                    UploadPart part;
                    part.part_number = part_meta.get("part_number", 0).asInt();
                    part.etag = part_meta.get("etag", "").asString();
                    part.size = part_meta.get("size", 0).asInt64();
                    part.last_modified = part_meta.get("last_modified", 0).asInt64();
                    info.parts.push_back(part);
                }
                part_meta_file.close();
            }
        }

        // Sort parts by part number
        std::sort(info.parts.begin(), info.parts.end(), [](const UploadPart& a, const UploadPart& b) {
            return a.part_number < b.part_number;
        });

        return Ok<MultipartUploadInfo, String>(info);

    } catch (const std::exception& e) {
        return Err<MultipartUploadInfo, String>(String("Failed to list parts: ") + e.what());
    }
}

Result<Vector<MultipartUploadInfo>, String>
LocalStorageClient::list_multipart_uploads(const String& bucket_name, const String& prefix) {
    std::shared_lock lock(multipart_mutex_);

    try {
        Vector<MultipartUploadInfo> uploads;

        auto multipart_root = path_manager_->multipart_root(bucket_name);
        if (!std::filesystem::exists(multipart_root)) {
            return Ok<Vector<MultipartUploadInfo>, String>(uploads);
        }

        Json::CharReaderBuilder reader;
        std::string errors;

        for (const auto& entry : std::filesystem::directory_iterator(multipart_root)) {
            if (!entry.is_directory())
                continue;

            auto meta_path = entry.path() / "upload.json";
            if (!std::filesystem::exists(meta_path))
                continue;

            std::ifstream meta_file(meta_path);
            Json::Value meta_json;
            if (!Json::parseFromStream(reader, meta_file, &meta_json, &errors))
                continue;
            meta_file.close();

            String key = meta_json.get("key", "").asString();

            // Apply prefix filter
            if (!prefix.empty() && key.find(prefix) != 0)
                continue;

            MultipartUploadInfo info;
            info.upload_id = meta_json.get("upload_id", "").asString();
            info.bucket = bucket_name;
            info.key = key;
            info.content_type = meta_json.get("content_type", "").asString();
            info.initiated = meta_json.get("initiated", 0).asInt64();

            uploads.push_back(info);
        }

        return Ok<Vector<MultipartUploadInfo>, String>(uploads);

    } catch (const std::exception& e) {
        return Err<Vector<MultipartUploadInfo>, String>(String("Failed to list multipart uploads: ") + e.what());
    }
}

String
LocalStorageClient::generate_upload_id() const {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::ostringstream ss;
    for (int i = 0; i < 32; i++) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

std::filesystem::path
LocalStorageClient::get_multipart_upload_path(const String& bucket_name, const String& upload_id) const {
    return path_manager_->multipart_root(bucket_name) / upload_id;
}

// ============================================================================
// Object Versioning Operations
// ============================================================================

Result<Vector<ObjectVersion>, String>
LocalStorageClient::list_object_versions(const String& bucket_name, const String& object_key) {
    std::shared_lock lock(objects_mutex_);

    try {
        Vector<ObjectVersion> versions;

        // Check if object exists
        auto object_path = path_manager_->object_path(bucket_name, object_key);
        auto meta_path = path_manager_->object_metadata_path(bucket_name, object_key);

        // Get current version (latest)
        if (std::filesystem::exists(meta_path)) {
            auto meta_result = metadata_manager_->read_object_metadata(meta_path);
            if (meta_result) {
                ObjectVersion current;
                current.version_id = meta_result.value().version_id;
                current.key = object_key;
                current.bucket = bucket_name;
                current.size = meta_result.value().size;
                current.last_modified = std::chrono::system_clock::to_time_t(meta_result.value().last_modified);
                current.etag = meta_result.value().etag;
                current.is_latest = true;
                current.is_delete_marker = false;
                versions.push_back(current);
            }
        }

        // Get historical versions
        auto versions_dir = path_manager_->versions_root(bucket_name) / object_key;
        if (std::filesystem::exists(versions_dir)) {
            for (const auto& entry : std::filesystem::directory_iterator(versions_dir)) {
                if (!entry.is_regular_file())
                    continue;

                ObjectVersion version;
                version.version_id = entry.path().filename().string();
                version.key = object_key;
                version.bucket = bucket_name;
                version.size = std::filesystem::file_size(entry.path());

                auto ftime = std::filesystem::last_write_time(entry.path());
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
                version.last_modified = std::chrono::system_clock::to_time_t(sctp);

                version.is_latest = false;
                version.is_delete_marker = false;
                versions.push_back(version);
            }
        }

        // Sort by last_modified descending (newest first)
        std::sort(versions.begin(), versions.end(), [](const ObjectVersion& a, const ObjectVersion& b) {
            return a.last_modified > b.last_modified;
        });

        CONSOLE_LOG_DEBUG("Listed {} versions for {}/{}", versions.size(), bucket_name, object_key);
        return Ok<Vector<ObjectVersion>, String>(versions);

    } catch (const std::exception& e) {
        return Err<Vector<ObjectVersion>, String>(String("Failed to list object versions: ") + e.what());
    }
}

Result<ByteArray, String>
LocalStorageClient::get_object_version(const String& bucket_name, const String& object_key, const String& version_id) {
    std::shared_lock lock(objects_mutex_);

    try {
        // If version is "null" or "latest", get current version
        if (version_id == "null" || version_id == "latest") {
            lock.unlock();
            return get_object(bucket_name, object_key);
        }

        // Check in versions directory
        auto version_path = path_manager_->versions_root(bucket_name) / object_key / version_id;
        if (!std::filesystem::exists(version_path)) {
            // Maybe version_id is the current version
            auto meta_path = path_manager_->object_metadata_path(bucket_name, object_key);
            if (std::filesystem::exists(meta_path)) {
                auto meta_result = metadata_manager_->read_object_metadata(meta_path);
                if (meta_result && meta_result.value().version_id == version_id) {
                    lock.unlock();
                    return get_object(bucket_name, object_key);
                }
            }
            return Err<ByteArray, String>("Version not found: " + version_id);
        }

        return atomic_read(version_path);

    } catch (const std::exception& e) {
        return Err<ByteArray, String>(String("Failed to get object version: ") + e.what());
    }
}

Result<void, String>
LocalStorageClient::delete_object_version(const String& bucket_name,
                                          const String& object_key,
                                          const String& version_id) {
    std::unique_lock lock(objects_mutex_);

    try {
        // If deleting current version
        auto meta_path = path_manager_->object_metadata_path(bucket_name, object_key);
        if (std::filesystem::exists(meta_path)) {
            auto meta_result = metadata_manager_->read_object_metadata(meta_path);
            if (meta_result && meta_result.value().version_id == version_id) {
                // Delete the current version
                auto object_path = path_manager_->object_path(bucket_name, object_key);
                auto tags_path = path_manager_->object_tags_path(bucket_name, object_key);

                if (std::filesystem::exists(object_path)) {
                    std::filesystem::remove(object_path);
                }
                std::filesystem::remove(meta_path);
                if (std::filesystem::exists(tags_path)) {
                    std::filesystem::remove(tags_path);
                }

                CONSOLE_LOG_INFO("Deleted current version {} of {}/{}", version_id, bucket_name, object_key);
                return Ok<String>();
            }
        }

        // Delete from versions directory
        auto version_path = path_manager_->versions_root(bucket_name) / object_key / version_id;
        if (!std::filesystem::exists(version_path)) {
            return Result<void, String>(err_tag, "Version not found: " + version_id);
        }

        std::filesystem::remove(version_path);

        // Also remove version metadata if exists
        auto version_meta_path = version_path.string() + ".meta";
        if (std::filesystem::exists(version_meta_path)) {
            std::filesystem::remove(version_meta_path);
        }

        CONSOLE_LOG_INFO("Deleted version {} of {}/{}", version_id, bucket_name, object_key);
        return Ok<String>();

    } catch (const std::exception& e) {
        return Result<void, String>(err_tag, String("Failed to delete object version: ") + e.what());
    }
}

Result<models::Object, String>
LocalStorageClient::restore_object_version(const String& bucket_name,
                                           const String& object_key,
                                           const String& version_id) {
    std::unique_lock lock(objects_mutex_);

    try {
        // Get version data
        auto version_path = path_manager_->versions_root(bucket_name) / object_key / version_id;
        if (!std::filesystem::exists(version_path)) {
            return Err<models::Object, String>("Version not found: " + version_id);
        }

        auto version_data = atomic_read(version_path);
        if (!version_data) {
            return Err<models::Object, String>("Failed to read version data: " + version_data.error());
        }

        // Get current metadata for content type
        String content_type = "application/octet-stream";
        auto current_meta_path = path_manager_->object_metadata_path(bucket_name, object_key);
        if (std::filesystem::exists(current_meta_path)) {
            auto meta_result = metadata_manager_->read_object_metadata(current_meta_path);
            if (meta_result) {
                content_type = meta_result.value().content_type;
            }
        }

        // Release lock and use put_object to create new version
        lock.unlock();

        // Put will handle versioning automatically
        auto result = put_object(bucket_name, object_key, version_data.value(), content_type, {});
        if (!result) {
            return result;
        }

        CONSOLE_LOG_INFO("Restored version {} of {}/{}", version_id, bucket_name, object_key);
        return result;

    } catch (const std::exception& e) {
        return Err<models::Object, String>(String("Failed to restore object version: ") + e.what());
    }
}

// ============================================================================
// Object Lock and Retention Operations
// ============================================================================

Result<void, String>
LocalStorageClient::set_object_retention(const String& bucket_name,
                                         const String& object_key,
                                         const String& mode,
                                         int64_t retain_until_date,
                                         const String& version_id) {
    std::unique_lock lock(objects_mutex_);

    try {
        // Validate mode
        if (mode != "GOVERNANCE" && mode != "COMPLIANCE") {
            return Result<void, String>(err_tag, "Invalid retention mode. Must be GOVERNANCE or COMPLIANCE");
        }

        // Check object exists
        auto object_path = path_manager_->object_path(bucket_name, object_key);
        if (!std::filesystem::exists(object_path)) {
            return Result<void, String>(err_tag, "Object not found: " + object_key);
        }

        // Read current metadata
        auto meta_path = path_manager_->object_metadata_path(bucket_name, object_key);
        auto meta_result = metadata_manager_->read_object_metadata(meta_path);
        if (!meta_result) {
            return Result<void, String>(err_tag, "Failed to read object metadata");
        }

        auto& obj_meta = meta_result.value();

        // Check if object is locked in COMPLIANCE mode
        if (obj_meta.retention_mode == "COMPLIANCE" && obj_meta.retention_until > std::time(nullptr)) {
            return Result<void, String>(err_tag, "Cannot modify retention on COMPLIANCE locked object");
        }

        // Update retention
        obj_meta.retention_mode = mode;
        obj_meta.retention_until = retain_until_date;

        // Write updated metadata
        auto write_result = metadata_manager_->write_object_metadata(meta_path, obj_meta);
        if (!write_result) {
            return Result<void, String>(err_tag, "Failed to write object metadata: " + write_result.error());
        }

        CONSOLE_LOG_INFO("Set retention for {}/{}: {} until {}", bucket_name, object_key, mode, retain_until_date);
        return Ok<String>();

    } catch (const std::exception& e) {
        return Result<void, String>(err_tag, String("Failed to set object retention: ") + e.what());
    }
}

Result<std::pair<String, int64_t>, String>
LocalStorageClient::get_object_retention(const String& bucket_name,
                                         const String& object_key,
                                         const String& version_id) {
    std::shared_lock lock(objects_mutex_);

    try {
        auto meta_path = path_manager_->object_metadata_path(bucket_name, object_key);
        if (!std::filesystem::exists(meta_path)) {
            return Err<std::pair<String, int64_t>, String>("Object not found: " + object_key);
        }

        auto meta_result = metadata_manager_->read_object_metadata(meta_path);
        if (!meta_result) {
            return Err<std::pair<String, int64_t>, String>("Failed to read object metadata");
        }

        const auto& obj_meta = meta_result.value();
        return Ok<std::pair<String, int64_t>, String>(
            std::make_pair(obj_meta.retention_mode, obj_meta.retention_until));

    } catch (const std::exception& e) {
        return Err<std::pair<String, int64_t>, String>(String("Failed to get object retention: ") + e.what());
    }
}

Result<void, String>
LocalStorageClient::set_object_legal_hold(const String& bucket_name,
                                          const String& object_key,
                                          bool enabled,
                                          const String& version_id) {
    std::unique_lock lock(objects_mutex_);

    try {
        // Check object exists
        auto object_path = path_manager_->object_path(bucket_name, object_key);
        if (!std::filesystem::exists(object_path)) {
            return Result<void, String>(err_tag, "Object not found: " + object_key);
        }

        // Read current metadata
        auto meta_path = path_manager_->object_metadata_path(bucket_name, object_key);
        auto meta_result = metadata_manager_->read_object_metadata(meta_path);
        if (!meta_result) {
            return Result<void, String>(err_tag, "Failed to read object metadata");
        }

        auto& obj_meta = meta_result.value();
        obj_meta.legal_hold = enabled;

        // Write updated metadata
        auto write_result = metadata_manager_->write_object_metadata(meta_path, obj_meta);
        if (!write_result) {
            return Result<void, String>(err_tag, "Failed to write object metadata: " + write_result.error());
        }

        CONSOLE_LOG_INFO("Set legal hold for {}/{}: {}", bucket_name, object_key, enabled);
        return Ok<String>();

    } catch (const std::exception& e) {
        return Result<void, String>(err_tag, String("Failed to set legal hold: ") + e.what());
    }
}

Result<bool, String>
LocalStorageClient::get_object_legal_hold(const String& bucket_name,
                                          const String& object_key,
                                          const String& version_id) {
    std::shared_lock lock(objects_mutex_);

    try {
        auto meta_path = path_manager_->object_metadata_path(bucket_name, object_key);
        if (!std::filesystem::exists(meta_path)) {
            return Err<bool, String>("Object not found: " + object_key);
        }

        auto meta_result = metadata_manager_->read_object_metadata(meta_path);
        if (!meta_result) {
            return Err<bool, String>("Failed to read object metadata");
        }

        return Ok<bool, String>(meta_result.value().legal_hold);

    } catch (const std::exception& e) {
        return Err<bool, String>(String("Failed to get legal hold: ") + e.what());
    }
}

Result<void, String>
LocalStorageClient::set_bucket_object_lock_configuration(const String& bucket_name,
                                                         bool enabled,
                                                         const String& default_mode,
                                                         int default_days,
                                                         int default_years) {
    std::unique_lock lock(buckets_mutex_);

    try {
        auto bucket_path = path_manager_->bucket_path(bucket_name);
        if (!std::filesystem::exists(bucket_path)) {
            return Result<void, String>(err_tag, "Bucket not found: " + bucket_name);
        }

        // Create lock configuration
        Json::Value config;
        config["object_lock_enabled"] = enabled;
        if (!default_mode.empty()) {
            config["default_retention"]["mode"] = default_mode;
            if (default_days > 0) {
                config["default_retention"]["days"] = default_days;
            }
            if (default_years > 0) {
                config["default_retention"]["years"] = default_years;
            }
        }

        // Write configuration to bucket
        auto config_path = bucket_path / "object_lock.json";
        Json::StreamWriterBuilder writer;
        std::ofstream config_file(config_path);
        config_file << Json::writeString(writer, config);
        config_file.close();

        CONSOLE_LOG_INFO("Set object lock configuration for bucket: {}", bucket_name);
        return Ok<String>();

    } catch (const std::exception& e) {
        return Result<void, String>(err_tag, String("Failed to set object lock configuration: ") + e.what());
    }
}

Result<Json::Value, String>
LocalStorageClient::get_bucket_object_lock_configuration(const String& bucket_name) {
    std::shared_lock lock(buckets_mutex_);

    try {
        auto bucket_path = path_manager_->bucket_path(bucket_name);
        if (!std::filesystem::exists(bucket_path)) {
            return Err<Json::Value, String>("Bucket not found: " + bucket_name);
        }

        auto config_path = bucket_path / "object_lock.json";
        if (!std::filesystem::exists(config_path)) {
            // Return default (disabled) configuration
            Json::Value config;
            config["object_lock_enabled"] = false;
            return Ok<Json::Value, String>(config);
        }

        std::ifstream config_file(config_path);
        Json::Value config;
        Json::CharReaderBuilder reader;
        std::string errors;
        if (!Json::parseFromStream(reader, config_file, &config, &errors)) {
            return Err<Json::Value, String>("Failed to parse object lock configuration");
        }

        return Ok<Json::Value, String>(config);

    } catch (const std::exception& e) {
        return Err<Json::Value, String>(String("Failed to get object lock configuration: ") + e.what());
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

// ============================================================================
// JSON-based bucket configuration methods (API wrappers)
// ============================================================================

Result<void, String>
LocalStorageClient::set_bucket_encryption(const String& bucket_name, const Json::Value& config) {
    bool enabled = config.get("enabled", false).asBool();
    String algorithm = config.get("algorithm", "AES256").asString();
    String kms_key_id = config.get("kms_master_key_id", "").asString();

    return set_bucket_encryption(bucket_name, enabled, algorithm, kms_key_id);
}

Result<void, String>
LocalStorageClient::set_bucket_lifecycle(const String& bucket_name, const Json::Value& rules) {
    std::unique_lock lock(buckets_mutex_);

    auto bucket_meta_path = path_manager_->bucket_path(bucket_name) / ".lifecycle.json";

    try {
        // Write lifecycle rules to JSON file
        Json::StreamWriterBuilder writer;
        String json_str = Json::writeString(writer, rules);

        std::ofstream file(bucket_meta_path);
        if (!file.is_open()) {
            return Err<void, String>("Failed to write lifecycle configuration");
        }
        file << json_str;
        file.close();

        CONSOLE_LOG_INFO("Set lifecycle rules for bucket: {}", bucket_name);
        return Ok<String>();
    } catch (const std::exception& e) {
        return Err<void, String>(String("Failed to set lifecycle: ") + e.what());
    }
}

Result<Json::Value, String>
LocalStorageClient::get_bucket_lifecycle(const String& bucket_name) {
    std::shared_lock lock(buckets_mutex_);

    auto bucket_meta_path = path_manager_->bucket_path(bucket_name) / ".lifecycle.json";

    if (!std::filesystem::exists(bucket_meta_path)) {
        // Return empty rules if not configured
        Json::Value result;
        result["rules"] = Json::Value(Json::arrayValue);
        return Ok<Json::Value, String>(result);
    }

    try {
        std::ifstream file(bucket_meta_path);
        if (!file.is_open()) {
            return Err<Json::Value, String>("Failed to read lifecycle configuration");
        }

        Json::Value result;
        Json::CharReaderBuilder reader;
        String errors;
        if (!Json::parseFromStream(reader, file, &result, &errors)) {
            return Err<Json::Value, String>("Failed to parse lifecycle JSON: " + errors);
        }

        return Ok<Json::Value, String>(result);
    } catch (const std::exception& e) {
        return Err<Json::Value, String>(String("Failed to get lifecycle: ") + e.what());
    }
}

Result<void, String>
LocalStorageClient::set_bucket_object_lock(const String& bucket_name, const Json::Value& config) {
    bool enabled = config.get("object_lock_enabled", false).asBool();
    String default_mode = "";
    int default_days = 0;
    int default_years = 0;

    if (config.isMember("default_retention")) {
        const auto& retention = config["default_retention"];
        default_mode = retention.get("mode", "").asString();
        default_days = retention.get("days", 0).asInt();
        default_years = retention.get("years", 0).asInt();
    }

    return set_bucket_object_lock_configuration(bucket_name, enabled, default_mode, default_days, default_years);
}

Result<Json::Value, String>
LocalStorageClient::get_bucket_object_lock(const String& bucket_name) {
    return get_bucket_object_lock_configuration(bucket_name);
}

} // namespace console::clients
