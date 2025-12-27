#pragma once

#include "console/clients/StorageClient.hpp"
#include "console/services/IObjectService.hpp"

#include <memory>

namespace console::services {

/**
 * @brief Object management service implementation
 *
 * Provides business logic for object operations with validation,
 * streaming support, and error handling.
 */
class ObjectService : public IObjectService {
  public:
    /**
     * @brief Construct ObjectService with Storage client
     *
     * @param storage_client Storage S3 client for object operations
     */
    explicit ObjectService(std::shared_ptr<clients::IStorageClient> storage_client);

    ~ObjectService() override = default;

    // IObjectService implementation
    Result<Vector<models::Object>, models::ApiError> list_objects(const UserInfo& user_info,
                                                                  const String& bucket_name,
                                                                  const String& prefix,
                                                                  bool recursive,
                                                                  int max_keys) override;

    Result<models::Object, models::ApiError> get_object_info(const UserInfo& user_info,
                                                             const String& bucket_name,
                                                             const String& object_key) override;

    Result<ByteArray, models::ApiError> download_object(const UserInfo& user_info,
                                                        const String& bucket_name,
                                                        const String& object_key) override;

    Result<models::Object, models::ApiError> upload_object(const UserInfo& user_info,
                                                           const String& bucket_name,
                                                           const String& object_key,
                                                           const ByteArray& data,
                                                           const String& content_type,
                                                           const StringMap& metadata) override;

    Result<void, models::ApiError> delete_object(const UserInfo& user_info,
                                                 const String& bucket_name,
                                                 const String& object_key) override;

    Result<Json::Value, models::ApiError> delete_objects(const UserInfo& user_info,
                                                         const String& bucket_name,
                                                         const Vector<String>& object_keys) override;

    Result<models::Object, models::ApiError> copy_object(const UserInfo& user_info,
                                                         const String& source_bucket,
                                                         const String& source_key,
                                                         const String& dest_bucket,
                                                         const String& dest_key) override;

    Result<void, models::ApiError> set_object_metadata(const UserInfo& user_info,
                                                       const String& bucket_name,
                                                       const String& object_key,
                                                       const StringMap& metadata) override;

    Result<void, models::ApiError> set_object_tags(const UserInfo& user_info,
                                                   const String& bucket_name,
                                                   const String& object_key,
                                                   const StringMap& tags) override;

    Result<StringMap, models::ApiError> get_object_tags(const UserInfo& user_info,
                                                        const String& bucket_name,
                                                        const String& object_key) override;

    Result<void, models::ApiError> delete_object_tags(const UserInfo& user_info,
                                                      const String& bucket_name,
                                                      const String& object_key) override;

    Result<String, models::ApiError> generate_presigned_url(const UserInfo& user_info,
                                                            const String& bucket_name,
                                                            const String& object_key,
                                                            const String& method = "GET",
                                                            int expiry_seconds = 3600) override;

    // ========================================================================
    // Server-Side Encryption (SSE) Operations
    // ========================================================================

    /**
     * @brief Upload object with server-side encryption
     *
     * @param user_info User context
     * @param bucket_name Bucket name
     * @param object_key Object key
     * @param data Object data
     * @param content_type MIME type
     * @param metadata Custom metadata
     * @param sse_customer_key SSE-C customer key (hex-encoded, 64 chars for AES-256)
     * @return Result containing uploaded object info or error
     */
    Result<models::Object, models::ApiError> upload_object_sse(const UserInfo& user_info,
                                                               const String& bucket_name,
                                                               const String& object_key,
                                                               const ByteArray& data,
                                                               const String& content_type,
                                                               const StringMap& metadata,
                                                               const String& sse_customer_key);

    /**
     * @brief Download object with server-side decryption
     *
     * @param user_info User context
     * @param bucket_name Bucket name
     * @param object_key Object key
     * @param sse_customer_key SSE-C customer key (hex-encoded, 64 chars for AES-256)
     * @return Result containing object data or error
     */
    Result<ByteArray, models::ApiError> download_object_sse(const UserInfo& user_info,
                                                            const String& bucket_name,
                                                            const String& object_key,
                                                            const String& sse_customer_key);

    // ========================================================================
    // Multipart Upload Operations
    // ========================================================================

    /**
     * @brief Initiate a multipart upload
     */
    Result<clients::MultipartUploadInfo, models::ApiError> initiate_multipart_upload(
        const UserInfo& user_info,
        const String& bucket_name,
        const String& object_key,
        const String& content_type = "application/octet-stream",
        const StringMap& metadata = {});

    /**
     * @brief Upload a part of a multipart upload
     */
    Result<String, models::ApiError> upload_part(const UserInfo& user_info,
                                                 const String& bucket_name,
                                                 const String& object_key,
                                                 const String& upload_id,
                                                 int part_number,
                                                 const ByteArray& data);

    /**
     * @brief Complete a multipart upload
     */
    Result<models::Object, models::ApiError> complete_multipart_upload(const UserInfo& user_info,
                                                                       const String& bucket_name,
                                                                       const String& object_key,
                                                                       const String& upload_id,
                                                                       const Vector<clients::CompletedPart>& parts);

    /**
     * @brief Abort a multipart upload
     */
    Result<void, models::ApiError> abort_multipart_upload(const UserInfo& user_info,
                                                          const String& bucket_name,
                                                          const String& object_key,
                                                          const String& upload_id);

    /**
     * @brief List parts of an in-progress multipart upload
     */
    Result<clients::MultipartUploadInfo, models::ApiError> list_parts(const UserInfo& user_info,
                                                                      const String& bucket_name,
                                                                      const String& object_key,
                                                                      const String& upload_id);

    /**
     * @brief List in-progress multipart uploads
     */
    Result<Vector<clients::MultipartUploadInfo>, models::ApiError> list_multipart_uploads(const UserInfo& user_info,
                                                                                          const String& bucket_name,
                                                                                          const String& prefix = "");

  private:
    /**
     * @brief Validate object key (path)
     *
     * @param object_key Object key to validate
     * @return Optional error if validation fails
     */
    Optional<models::ApiError> validate_object_key(const String& object_key);

    /**
     * @brief Sanitize object key (remove dangerous characters)
     *
     * @param object_key Original object key
     * @return Sanitized object key
     */
    String sanitize_object_key(const String& object_key);

    /**
     * @brief Validate user access to object for given action
     *
     * @param user User to validate
     * @param bucket_name Bucket name
     * @param object_key Object key
     * @param action Action to validate
     * @return Optional error if validation fails
     */
    Optional<models::ApiError> validate_access(const UserInfo& user,
                                               const String& bucket_name,
                                               const String& object_key,
                                               const String& action);

    std::shared_ptr<clients::IStorageClient> storage_client_;
    size_t max_single_upload_size_;
    size_t multipart_threshold_;
};

} // namespace console::services
