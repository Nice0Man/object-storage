#pragma once

#include "console/clients/StorageClient.hpp"
#include "console/storage/MetadataManager.hpp"
#include "console/storage/PathManager.hpp"

#include <memory>
#include <shared_mutex>

namespace console::clients {

/**
 * @brief Local filesystem-based storage client implementing S3-compatible API
 *
 * Provides complete S3-compatible storage without external dependencies.
 * Data is stored in the local filesystem with metadata in JSON files.
 */
class LocalStorageClient : public IStorageClient {
  public:
    /**
     * @brief Construct LocalStorageClient
     *
     * @param storage_root Root directory for all storage data
     */
    explicit LocalStorageClient(const String& storage_root);

    ~LocalStorageClient() override = default;

    // ========================================================================
    // IStorageClient implementation
    // ========================================================================

    // Connection
    Result<bool, String> is_connected() override;

    // Bucket operations
    Result<Vector<models::Bucket>, String> list_buckets() override;
    Result<models::Bucket, String> get_bucket(const String& name) override;
    Result<bool, String> create_bucket(const String& name, const String& region) override;
    Result<bool, String> delete_bucket(const String& name) override;
    Result<bool, String> bucket_exists(const String& name) override;

    // Object operations
    Result<models::ListObjectsResponse, String> list_objects(const String& bucket_name,
                                                             const ListObjectsOptions& options) override;

    Result<models::Object, String> stat_object(const String& bucket_name, const String& object_key) override;

    Result<ByteArray, String> get_object(const String& bucket_name, const String& object_key) override;

    /**
     * @brief Get object with SSE-C decryption support
     *
     * @param bucket_name Bucket name
     * @param object_key Object key
     * @param sse_customer_key Optional customer encryption key (hex-encoded, 64 chars)
     * @return Decrypted object data or error
     */
    Result<ByteArray, String> get_object(const String& bucket_name,
                                         const String& object_key,
                                         const String& sse_customer_key);

    Result<models::Object, String> put_object(const String& bucket_name,
                                              const String& object_key,
                                              const ByteArray& data,
                                              const String& content_type,
                                              const StringMap& metadata) override;

    /**
     * @brief Put object with server-side encryption
     *
     * @param bucket_name Bucket name
     * @param object_key Object key
     * @param data Object data
     * @param content_type Content type
     * @param metadata Custom metadata
     * @param sse_customer_key Optional customer encryption key (hex-encoded, 64 chars for AES-256)
     * @return Object info or error
     */
    Result<models::Object, String> put_object(const String& bucket_name,
                                              const String& object_key,
                                              const ByteArray& data,
                                              const String& content_type,
                                              const StringMap& metadata,
                                              const String& sse_customer_key);

    Result<bool, String> delete_object(const String& bucket_name, const String& object_key) override;

    Result<bool, String> copy_object(const String& source_bucket,
                                     const String& source_key,
                                     const String& dest_bucket,
                                     const String& dest_key) override;

    // Object metadata
    Result<StringMap, String> get_object_metadata(const String& bucket_name, const String& object_key) override;

    Result<bool, String> set_object_metadata(const String& bucket_name,
                                             const String& object_key,
                                             const StringMap& metadata) override;

    // Object tags
    Result<StringMap, String> get_object_tags(const String& bucket_name, const String& object_key) override;

    Result<bool, String> set_object_tags(const String& bucket_name,
                                         const String& object_key,
                                         const StringMap& tags) override;

    // Presigned URLs
    Result<String, String> generate_presigned_url(const String& bucket_name,
                                                  const String& object_key,
                                                  int64_t expires_in_seconds,
                                                  const String& method) override;

    // Multipart upload operations
    Result<MultipartUploadInfo, String> initiate_multipart_upload(const String& bucket_name,
                                                                  const String& object_key,
                                                                  const String& content_type,
                                                                  const StringMap& metadata) override;

    Result<String, String> upload_part(const String& bucket_name,
                                       const String& object_key,
                                       const String& upload_id,
                                       int part_number,
                                       const ByteArray& data) override;

    Result<models::Object, String> complete_multipart_upload(const String& bucket_name,
                                                             const String& object_key,
                                                             const String& upload_id,
                                                             const Vector<CompletedPart>& parts) override;

    Result<void, String> abort_multipart_upload(const String& bucket_name,
                                                const String& object_key,
                                                const String& upload_id) override;

    Result<MultipartUploadInfo, String> list_parts(const String& bucket_name,
                                                   const String& object_key,
                                                   const String& upload_id) override;

    Result<Vector<MultipartUploadInfo>, String> list_multipart_uploads(const String& bucket_name,
                                                                       const String& prefix) override;

    // Object versioning operations
    Result<Vector<ObjectVersion>, String> list_object_versions(const String& bucket_name,
                                                               const String& object_key) override;

    Result<ByteArray, String> get_object_version(const String& bucket_name,
                                                 const String& object_key,
                                                 const String& version_id) override;

    Result<void, String> delete_object_version(const String& bucket_name,
                                               const String& object_key,
                                               const String& version_id) override;

    Result<models::Object, String> restore_object_version(const String& bucket_name,
                                                          const String& object_key,
                                                          const String& version_id) override;

    // Object lock and retention operations
    Result<void, String> set_object_retention(const String& bucket_name,
                                              const String& object_key,
                                              const String& mode,
                                              int64_t retain_until_date,
                                              const String& version_id) override;

    Result<std::pair<String, int64_t>, String> get_object_retention(const String& bucket_name,
                                                                    const String& object_key,
                                                                    const String& version_id) override;

    Result<void, String> set_object_legal_hold(const String& bucket_name,
                                               const String& object_key,
                                               bool enabled,
                                               const String& version_id) override;

    Result<bool, String> get_object_legal_hold(const String& bucket_name,
                                               const String& object_key,
                                               const String& version_id) override;

    Result<void, String> set_bucket_object_lock_configuration(const String& bucket_name,
                                                              bool enabled,
                                                              const String& default_mode,
                                                              int default_days,
                                                              int default_years) override;

    Result<Json::Value, String> get_bucket_object_lock_configuration(const String& bucket_name) override;

    // ========================================================================
    // Additional bucket operations (not in base interface)
    // ========================================================================

    /**
     * @brief Set bucket versioning status
     *
     * @param bucket_name Bucket name
     * @param enabled Enable or disable versioning
     * @return Result indicating success or error
     */
    Result<void, String> set_bucket_versioning(const String& bucket_name, bool enabled) override;

    /**
     * @brief Get bucket versioning status
     *
     * @param bucket_name Bucket name
     * @return Result containing versioning status or error
     */
    Result<bool, String> get_bucket_versioning(const String& bucket_name) override;

    /**
     * @brief Set bucket policy
     *
     * @param bucket_name Bucket name
     * @param policy_json Policy JSON document
     * @return Result indicating success or error
     */
    Result<void, String> set_bucket_policy(const String& bucket_name, const String& policy_json) override;

    /**
     * @brief Get bucket policy
     *
     * @param bucket_name Bucket name
     * @return Result containing policy JSON or error
     */
    Result<String, String> get_bucket_policy(const String& bucket_name) override;

    /**
     * @brief Set bucket tags
     *
     * @param bucket_name Bucket name
     * @param tags Tags to set
     * @return Result indicating success or error
     */
    Result<void, String> set_bucket_tags(const String& bucket_name, const StringMap& tags) override;

    /**
     * @brief Get bucket tags
     *
     * @param bucket_name Bucket name
     * @return Result containing tags or error
     */
    Result<StringMap, String> get_bucket_tags(const String& bucket_name) override;

    /**
     * @brief Delete bucket tags
     *
     * @param bucket_name Bucket name
     * @return Result indicating success or error
     */
    Result<void, String> delete_bucket_tags(const String& bucket_name) override;

    /**
     * @brief Set bucket encryption configuration (SSE)
     *
     * @param bucket_name Bucket name
     * @param enabled Enable encryption
     * @param algorithm Encryption algorithm (AES256 or aws:kms)
     * @param kms_key_id Optional KMS key ID
     * @return Result indicating success or error
     */
    Result<void, String> set_bucket_encryption(const String& bucket_name,
                                               bool enabled,
                                               const String& algorithm = "AES256",
                                               const String& kms_key_id = "");

    /**
     * @brief Get bucket encryption configuration
     *
     * @param bucket_name Bucket name
     * @return Result containing encryption config JSON
     */
    Result<Json::Value, String> get_bucket_encryption(const String& bucket_name) override;

    /**
     * @brief Delete bucket encryption configuration
     *
     * @param bucket_name Bucket name
     * @return Result indicating success or error
     */
    Result<void, String> delete_bucket_encryption(const String& bucket_name);

    // IStorageClient interface implementations
    Result<void, String> set_bucket_encryption(const String& bucket_name, const Json::Value& config) override;

    Result<void, String> set_bucket_lifecycle(const String& bucket_name, const Json::Value& rules) override;

    Result<Json::Value, String> get_bucket_lifecycle(const String& bucket_name) override;

    Result<void, String> set_bucket_object_lock(const String& bucket_name, const Json::Value& config) override;

    Result<Json::Value, String> get_bucket_object_lock(const String& bucket_name) override;

  private:
    // ========================================================================
    // Helper methods
    // ========================================================================

    // Initialize storage structure
    void initialize_storage();

    // Compute MD5 hash for ETag
    String compute_md5(const ByteArray& data) const;

    // Atomic file write
    Result<void, String> atomic_write(const std::filesystem::path& path, const ByteArray& data);

    // Atomic file read
    Result<ByteArray, String> atomic_read(const std::filesystem::path& path);

    // Validate bucket name (S3 rules)
    bool is_valid_bucket_name(const String& name) const;

    // Validate object key (S3 rules)
    bool is_valid_object_key(const String& key) const;

    // Generate unique upload ID for multipart
    String generate_upload_id() const;

    // Get multipart upload directory
    std::filesystem::path get_multipart_upload_path(const String& bucket_name, const String& upload_id) const;

    // ========================================================================
    // Member variables
    // ========================================================================

    std::unique_ptr<storage::PathManager> path_manager_;
    std::unique_ptr<storage::MetadataManager> metadata_manager_;

    // Thread-safe access
    mutable std::shared_mutex buckets_mutex_;
    mutable std::shared_mutex objects_mutex_;
    mutable std::shared_mutex multipart_mutex_;
};

} // namespace console::clients
