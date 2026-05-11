#pragma once

#include "console/common/Types.hpp"
#include "console/models/Bucket.hpp"
#include "console/models/Group.hpp"
#include "console/models/Object.hpp"
#include "console/models/Policy.hpp"
#include "console/models/ServerInfo.hpp"
#include "console/models/User.hpp"

#include <memory>
#include <vector>

namespace console {
namespace clients {

// ============================================================================
// Request/Response Types
// ============================================================================

struct ListObjectsOptions {
    String prefix;
    String delimiter;
    int max_keys{1000};
    String continuation_token;
    bool recursive{false};
};

struct CreateUserRequest {
    String access_key;
    String secret_key;
    Vector<String> policies;
    Vector<String> groups;
};

struct UpdateUserRequest {
    String access_key;
    Optional<String> secret_key;
    Optional<Vector<String>> policies;
    Optional<Vector<String>> groups;
    Optional<bool> enabled;
};

struct CreateGroupRequest {
    String name;
    Vector<String> members;
    Vector<String> policies;
};

struct CreatePolicyRequest {
    String name;
    String policy_document;
};

struct AttachPolicyRequest {
    String policy_name;
    String entity_type; // "user" or "group"
    String entity_name;
};

// ============================================================================
// Multipart Upload Types
// ============================================================================

/**
 * @brief Part info for multipart upload
 */
struct UploadPart {
    int part_number{0};
    String etag;
    int64_t size{0};
    int64_t last_modified{0};
};

/**
 * @brief Multipart upload info
 */
struct MultipartUploadInfo {
    String upload_id;
    String bucket;
    String key;
    String content_type;
    StringMap metadata;
    int64_t initiated{0};
    Vector<UploadPart> parts;

    Json::Value to_json() const {
        Json::Value json;
        json["upload_id"] = upload_id;
        json["bucket"] = bucket;
        json["key"] = key;
        json["content_type"] = content_type;
        json["initiated"] = static_cast<Json::Int64>(initiated);

        Json::Value parts_arr(Json::arrayValue);
        for (const auto& part : parts) {
            Json::Value p;
            p["part_number"] = part.part_number;
            p["etag"] = part.etag;
            p["size"] = static_cast<Json::Int64>(part.size);
            parts_arr.append(p);
        }
        json["parts"] = parts_arr;

        return json;
    }
};

/**
 * @brief Completed part for completing multipart upload
 */
struct CompletedPart {
    int part_number{0};
    String etag;
};

// ============================================================================
// Object Versioning Types
// ============================================================================

/**
 * @brief Object version info
 */
struct ObjectVersion {
    String version_id;
    String key;
    String bucket;
    int64_t size{0};
    int64_t last_modified{0};
    String etag;
    bool is_latest{false};
    bool is_delete_marker{false};

    Json::Value to_json() const {
        Json::Value json;
        json["version_id"] = version_id;
        json["key"] = key;
        json["bucket"] = bucket;
        json["size"] = static_cast<Json::Int64>(size);
        json["last_modified"] = static_cast<Json::Int64>(last_modified);
        json["etag"] = etag;
        json["is_latest"] = is_latest;
        json["is_delete_marker"] = is_delete_marker;
        return json;
    }
};

// ============================================================================
// IStorageClient - S3-compatible operations
// ============================================================================

class IStorageClient {
  public:
    virtual ~IStorageClient() = default;

    // Connection
    virtual Result<bool, String> is_connected() = 0;

    // Bucket operations
    virtual Result<Vector<models::Bucket>, String> list_buckets() = 0;
    virtual Result<models::Bucket, String> get_bucket(const String& name) = 0;
    virtual Result<bool, String> create_bucket(const String& name, const String& region = "") = 0;
    virtual Result<bool, String> delete_bucket(const String& name) = 0;
    virtual Result<bool, String> bucket_exists(const String& name) = 0;

    // Bucket policy operations
    virtual Result<void, String> set_bucket_policy(const String& name, const String& policy_json) = 0;
    virtual Result<String, String> get_bucket_policy(const String& name) = 0;

    // Bucket versioning
    virtual Result<void, String> set_bucket_versioning(const String& name, bool enabled) = 0;
    virtual Result<bool, String> get_bucket_versioning(const String& name) = 0;

    // Bucket tags
    virtual Result<void, String> set_bucket_tags(const String& name, const StringMap& tags) = 0;
    virtual Result<StringMap, String> get_bucket_tags(const String& name) = 0;
    virtual Result<void, String> delete_bucket_tags(const String& name) = 0;

    // Bucket encryption
    virtual Result<Json::Value, String> get_bucket_encryption(const String& name) = 0;
    virtual Result<void, String> set_bucket_encryption(const String& name, const Json::Value& config) = 0;

    // Bucket lifecycle
    virtual Result<Json::Value, String> get_bucket_lifecycle(const String& name) = 0;
    virtual Result<void, String> set_bucket_lifecycle(const String& name, const Json::Value& rules) = 0;

    // Bucket object lock
    virtual Result<Json::Value, String> get_bucket_object_lock(const String& name) = 0;
    virtual Result<void, String> set_bucket_object_lock(const String& name, const Json::Value& config) = 0;

    // Object operations
    virtual Result<models::ListObjectsResponse, String> list_objects(const String& bucket_name,
                                                                     const ListObjectsOptions& options) = 0;

    virtual Result<models::Object, String> stat_object(const String& bucket_name, const String& object_key) = 0;

    virtual Result<ByteArray, String> get_object(const String& bucket_name, const String& object_key) = 0;

    virtual Result<models::Object, String> put_object(const String& bucket_name,
                                                      const String& object_key,
                                                      const ByteArray& data,
                                                      const String& content_type = "application/octet-stream",
                                                      const StringMap& metadata = {}) = 0;

    virtual Result<bool, String> delete_object(const String& bucket_name, const String& object_key) = 0;

    virtual Result<bool, String> copy_object(const String& source_bucket,
                                             const String& source_key,
                                             const String& dest_bucket,
                                             const String& dest_key) = 0;

    // Object metadata
    virtual Result<StringMap, String> get_object_metadata(const String& bucket_name, const String& object_key) = 0;

    virtual Result<bool, String> set_object_metadata(const String& bucket_name,
                                                     const String& object_key,
                                                     const StringMap& metadata) = 0;

    // Object tags
    virtual Result<StringMap, String> get_object_tags(const String& bucket_name, const String& object_key) = 0;

    virtual Result<bool, String> set_object_tags(const String& bucket_name,
                                                 const String& object_key,
                                                 const StringMap& tags) = 0;

    // Presigned URLs
    virtual Result<String, String> generate_presigned_url(const String& bucket_name,
                                                          const String& object_key,
                                                          int64_t expires_in_seconds = 3600,
                                                          const String& method = "GET") = 0;

    // ========================================================================
    // Multipart Upload Operations
    // ========================================================================

    /**
     * @brief Initiate a multipart upload
     *
     * @param bucket_name Bucket name
     * @param object_key Object key
     * @param content_type MIME type
     * @param metadata Additional metadata
     * @return Result containing upload info with upload_id
     */
    virtual Result<MultipartUploadInfo, String> initiate_multipart_upload(
        const String& bucket_name,
        const String& object_key,
        const String& content_type = "application/octet-stream",
        const StringMap& metadata = {}) = 0;

    /**
     * @brief Upload a part of a multipart upload
     *
     * @param bucket_name Bucket name
     * @param object_key Object key
     * @param upload_id Upload ID from initiate_multipart_upload
     * @param part_number Part number (1-10000)
     * @param data Part data (5MB minimum except last part)
     * @return Result containing the part's ETag
     */
    virtual Result<String, String> upload_part(const String& bucket_name,
                                               const String& object_key,
                                               const String& upload_id,
                                               int part_number,
                                               const ByteArray& data) = 0;

    /**
     * @brief Complete a multipart upload
     *
     * @param bucket_name Bucket name
     * @param object_key Object key
     * @param upload_id Upload ID from initiate_multipart_upload
     * @param parts List of completed parts (part_number and etag)
     * @return Result containing the final object info
     */
    virtual Result<models::Object, String> complete_multipart_upload(const String& bucket_name,
                                                                     const String& object_key,
                                                                     const String& upload_id,
                                                                     const Vector<CompletedPart>& parts) = 0;

    /**
     * @brief Abort a multipart upload
     *
     * @param bucket_name Bucket name
     * @param object_key Object key
     * @param upload_id Upload ID from initiate_multipart_upload
     * @return Result indicating success or error
     */
    virtual Result<void, String> abort_multipart_upload(const String& bucket_name,
                                                        const String& object_key,
                                                        const String& upload_id) = 0;

    /**
     * @brief List parts of an in-progress multipart upload
     *
     * @param bucket_name Bucket name
     * @param object_key Object key
     * @param upload_id Upload ID
     * @return Result containing the upload info with parts
     */
    virtual Result<MultipartUploadInfo, String> list_parts(const String& bucket_name,
                                                           const String& object_key,
                                                           const String& upload_id) = 0;

    /**
     * @brief List in-progress multipart uploads in a bucket
     *
     * @param bucket_name Bucket name
     * @param prefix Optional prefix filter
     * @return Result containing list of multipart uploads
     */
    virtual Result<Vector<MultipartUploadInfo>, String> list_multipart_uploads(const String& bucket_name,
                                                                               const String& prefix = "") = 0;

    // ========================================================================
    // Object Versioning Operations
    // ========================================================================

    /**
     * @brief List all versions of an object
     *
     * @param bucket_name Bucket name
     * @param object_key Object key
     * @return Result containing list of object versions
     */
    virtual Result<Vector<ObjectVersion>, String> list_object_versions(const String& bucket_name,
                                                                       const String& object_key) = 0;

    /**
     * @brief Get a specific version of an object
     *
     * @param bucket_name Bucket name
     * @param object_key Object key
     * @param version_id Version ID
     * @return Result containing object data
     */
    virtual Result<ByteArray, String> get_object_version(const String& bucket_name,
                                                         const String& object_key,
                                                         const String& version_id) = 0;

    /**
     * @brief Delete a specific version of an object
     *
     * @param bucket_name Bucket name
     * @param object_key Object key
     * @param version_id Version ID
     * @return Result indicating success or error
     */
    virtual Result<void, String> delete_object_version(const String& bucket_name,
                                                       const String& object_key,
                                                       const String& version_id) = 0;

    /**
     * @brief Restore a specific version of an object (make it the latest)
     *
     * @param bucket_name Bucket name
     * @param object_key Object key
     * @param version_id Version ID to restore
     * @return Result containing the restored object info
     */
    virtual Result<models::Object, String> restore_object_version(const String& bucket_name,
                                                                  const String& object_key,
                                                                  const String& version_id) = 0;

    // ========================================================================
    // Object Lock and Retention Operations
    // ========================================================================

    /**
     * @brief Set object retention policy
     *
     * @param bucket_name Bucket name
     * @param object_key Object key
     * @param mode Retention mode (GOVERNANCE or COMPLIANCE)
     * @param retain_until_date Date until which object is retained
     * @param version_id Optional version ID
     * @return Result indicating success or error
     */
    virtual Result<void, String> set_object_retention(const String& bucket_name,
                                                      const String& object_key,
                                                      const String& mode,
                                                      int64_t retain_until_date,
                                                      const String& version_id = "") = 0;

    /**
     * @brief Get object retention policy
     *
     * @param bucket_name Bucket name
     * @param object_key Object key
     * @param version_id Optional version ID
     * @return Result containing retention info (mode and date)
     */
    virtual Result<std::pair<String, int64_t>, String> get_object_retention(const String& bucket_name,
                                                                            const String& object_key,
                                                                            const String& version_id = "") = 0;

    /**
     * @brief Set legal hold status on object
     *
     * @param bucket_name Bucket name
     * @param object_key Object key
     * @param enabled Enable or disable legal hold
     * @param version_id Optional version ID
     * @return Result indicating success or error
     */
    virtual Result<void, String> set_object_legal_hold(const String& bucket_name,
                                                       const String& object_key,
                                                       bool enabled,
                                                       const String& version_id = "") = 0;

    /**
     * @brief Get legal hold status
     *
     * @param bucket_name Bucket name
     * @param object_key Object key
     * @param version_id Optional version ID
     * @return Result containing legal hold status
     */
    virtual Result<bool, String> get_object_legal_hold(const String& bucket_name,
                                                       const String& object_key,
                                                       const String& version_id = "") = 0;

    /**
     * @brief Enable object lock on bucket (must be set at bucket creation)
     *
     * @param bucket_name Bucket name
     * @return Result indicating success or error
     */
    virtual Result<void, String> set_bucket_object_lock_configuration(const String& bucket_name,
                                                                      bool enabled,
                                                                      const String& default_mode = "",
                                                                      int default_days = 0,
                                                                      int default_years = 0) = 0;

    /**
     * @brief Get bucket object lock configuration
     *
     * @param bucket_name Bucket name
     * @return Result containing object lock configuration
     */
    virtual Result<Json::Value, String> get_bucket_object_lock_configuration(const String& bucket_name) = 0;
};

// ============================================================================
// IMinioAdminClient - Admin operations
// ============================================================================

class IMinioAdminClient {
  public:
    virtual ~IMinioAdminClient() = default;

    // Server info
    virtual Result<models::ServerInfo, String> get_server_info() = 0;

    // User management
    virtual Result<Vector<models::User>, String> list_users() = 0;
    virtual Result<models::User, String> get_user(const String& access_key) = 0;
    virtual Result<models::User, String> create_user(const String& access_key,
                                                     const String& secret_key,
                                                     bool is_admin = false) = 0;
    virtual Result<void, String> delete_user(const String& access_key) = 0;
    virtual Result<void, String> set_user_policy(const String& access_key, const String& policy_name) = 0;
    virtual Result<void, String> update_user_groups(const String& access_key, const Vector<String>& groups) = 0;

    // Group management
    virtual Result<Vector<models::Group>, String> list_groups() = 0;
    virtual Result<models::Group, String> get_group(const String& name) = 0;
    virtual Result<bool, String> create_group(const CreateGroupRequest& request) = 0;
    virtual Result<bool, String> delete_group(const String& name) = 0;
    virtual Result<bool, String> add_user_to_group(const String& user, const String& group) = 0;
    virtual Result<bool, String> remove_user_from_group(const String& user, const String& group) = 0;

    // Policy management
    virtual Result<Vector<models::Policy>, String> list_policies() = 0;
    virtual Result<bool, String> create_policy(const CreatePolicyRequest& request) = 0;
    virtual Result<bool, String> delete_policy(const String& name) = 0;
    virtual Result<models::Policy, String> get_policy(const String& name) = 0;
    virtual Result<bool, String> attach_policy(const AttachPolicyRequest& request) = 0;
    virtual Result<bool, String> detach_policy(const AttachPolicyRequest& request) = 0;

    // Service accounts
    virtual Result<models::ServiceAccount, String> create_service_account(
        const String& parent_user,
        const Optional<String>& policy = std::nullopt) = 0;

    virtual Result<bool, String> delete_service_account(const String& access_key) = 0;
};

// ============================================================================
// StorageClient Implementation
// ============================================================================

class StorageClient : public IStorageClient {
  public:
    StorageClient(const String& endpoint, const String& access_key, const String& secret_key, bool use_ssl = true);

    // IStorageClient implementation
    Result<bool, String> is_connected() override;
    Result<Vector<models::Bucket>, String> list_buckets() override;
    Result<models::Bucket, String> get_bucket(const String& name) override;
    Result<bool, String> create_bucket(const String& name, const String& region) override;
    Result<bool, String> delete_bucket(const String& name) override;
    Result<bool, String> bucket_exists(const String& name) override;

    Result<models::ListObjectsResponse, String> list_objects(const String& bucket_name,
                                                             const ListObjectsOptions& options) override;

    Result<models::Object, String> stat_object(const String& bucket_name, const String& object_key) override;

    Result<ByteArray, String> get_object(const String& bucket_name, const String& object_key) override;

    Result<models::Object, String> put_object(const String& bucket_name,
                                              const String& object_key,
                                              const ByteArray& data,
                                              const String& content_type,
                                              const StringMap& metadata) override;

    Result<bool, String> delete_object(const String& bucket_name, const String& object_key) override;

    Result<bool, String> copy_object(const String& source_bucket,
                                     const String& source_key,
                                     const String& dest_bucket,
                                     const String& dest_key) override;

    Result<StringMap, String> get_object_metadata(const String& bucket_name, const String& object_key) override;

    Result<bool, String> set_object_metadata(const String& bucket_name,
                                             const String& object_key,
                                             const StringMap& metadata) override;

    Result<StringMap, String> get_object_tags(const String& bucket_name, const String& object_key) override;

    Result<bool, String> set_object_tags(const String& bucket_name,
                                         const String& object_key,
                                         const StringMap& tags) override;

    Result<String, String> generate_presigned_url(const String& bucket_name,
                                                  const String& object_key,
                                                  int64_t expires_in_seconds,
                                                  const String& method) override;

    // Multipart upload
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

    // Object versioning
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

    // Object lock and retention
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

  private:
    String endpoint_;
    String access_key_;
    String secret_key_;
    bool use_ssl_;
};

} // namespace clients
} // namespace console
