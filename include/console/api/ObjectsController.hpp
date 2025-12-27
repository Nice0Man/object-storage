#pragma once

#include "console/services/IObjectService.hpp"

#include <drogon/HttpController.h>

#include <memory>

namespace console::api {

/**
 * @brief Objects API Controller
 *
 * Handles object operations (list, upload, download, delete, etc.)
 */
class ObjectsController : public drogon::HttpController<ObjectsController> {
  public:
    METHOD_LIST_BEGIN

    // List objects in bucket
    ADD_METHOD_TO(ObjectsController::list,
                  "/api/v1/buckets/{bucket}/objects",
                  drogon::Get,
                  "console::middleware::AuthMiddleware");

    // Get object info
    ADD_METHOD_TO(ObjectsController::get_info,
                  "/api/v1/buckets/{bucket}/objects/{key}/info",
                  drogon::Get,
                  "console::middleware::AuthMiddleware");

    // Get object (download without explicit /download in path - alias)
    ADD_METHOD_TO(ObjectsController::get_object,
                  "/api/v1/buckets/{bucket}/objects/{key}",
                  drogon::Get,
                  "console::middleware::AuthMiddleware");

    // Download object (explicit download endpoint)
    ADD_METHOD_TO(ObjectsController::download,
                  "/api/v1/buckets/{bucket}/objects/{key}/download",
                  drogon::Get,
                  "console::middleware::AuthMiddleware");

    // Upload object
    ADD_METHOD_TO(ObjectsController::upload,
                  "/api/v1/buckets/{bucket}/objects",
                  drogon::Post,
                  "console::middleware::AuthMiddleware");

    // Delete object
    ADD_METHOD_TO(ObjectsController::delete_object,
                  "/api/v1/buckets/{bucket}/objects/{key}",
                  drogon::Delete,
                  "console::middleware::AuthMiddleware");

    // Batch delete objects
    ADD_METHOD_TO(ObjectsController::batch_delete,
                  "/api/v1/buckets/{bucket}/objects/batch-delete",
                  drogon::Post,
                  "console::middleware::AuthMiddleware");

    // Copy object
    ADD_METHOD_TO(ObjectsController::copy,
                  "/api/v1/buckets/{bucket}/objects/{key}/copy",
                  drogon::Post,
                  "console::middleware::AuthMiddleware");

    // Get presigned URL (key:path supports slashes in object key)
    ADD_METHOD_TO(ObjectsController::get_presigned_url,
                  "/api/v1/buckets/{bucket}/objects/{key:path}/presigned-url",
                  drogon::Get,
                  "console::middleware::AuthMiddleware");

    // Object tags
    ADD_METHOD_TO(ObjectsController::get_tags,
                  "/api/v1/buckets/{bucket}/objects/{key}/tags",
                  drogon::Get,
                  "console::middleware::AuthMiddleware");
    ADD_METHOD_TO(ObjectsController::set_tags,
                  "/api/v1/buckets/{bucket}/objects/{key}/tags",
                  drogon::Put,
                  "console::middleware::AuthMiddleware");

    // Multipart upload operations
    ADD_METHOD_TO(ObjectsController::initiate_multipart_upload,
                  "/api/v1/buckets/{bucket}/uploads",
                  drogon::Post,
                  "console::middleware::AuthMiddleware");
    ADD_METHOD_TO(ObjectsController::upload_part,
                  "/api/v1/buckets/{bucket}/uploads/{uploadId}/parts/{partNumber}",
                  drogon::Put,
                  "console::middleware::AuthMiddleware");
    ADD_METHOD_TO(ObjectsController::complete_multipart_upload,
                  "/api/v1/buckets/{bucket}/uploads/{uploadId}/complete",
                  drogon::Post,
                  "console::middleware::AuthMiddleware");
    ADD_METHOD_TO(ObjectsController::abort_multipart_upload,
                  "/api/v1/buckets/{bucket}/uploads/{uploadId}",
                  drogon::Delete,
                  "console::middleware::AuthMiddleware");
    ADD_METHOD_TO(ObjectsController::list_parts,
                  "/api/v1/buckets/{bucket}/uploads/{uploadId}/parts",
                  drogon::Get,
                  "console::middleware::AuthMiddleware");
    ADD_METHOD_TO(ObjectsController::list_multipart_uploads,
                  "/api/v1/buckets/{bucket}/uploads",
                  drogon::Get,
                  "console::middleware::AuthMiddleware");

    // Object versioning operations
    ADD_METHOD_TO(ObjectsController::list_object_versions,
                  "/api/v1/buckets/{bucket}/objects/{key}/versions",
                  drogon::Get,
                  "console::middleware::AuthMiddleware");
    ADD_METHOD_TO(ObjectsController::get_object_version,
                  "/api/v1/buckets/{bucket}/objects/{key}/versions/{versionId}",
                  drogon::Get,
                  "console::middleware::AuthMiddleware");
    ADD_METHOD_TO(ObjectsController::delete_object_version,
                  "/api/v1/buckets/{bucket}/objects/{key}/versions/{versionId}",
                  drogon::Delete,
                  "console::middleware::AuthMiddleware");
    ADD_METHOD_TO(ObjectsController::restore_object_version,
                  "/api/v1/buckets/{bucket}/objects/{key}/versions/{versionId}/restore",
                  drogon::Post,
                  "console::middleware::AuthMiddleware");

    // Object retention and legal hold
    ADD_METHOD_TO(ObjectsController::get_object_retention,
                  "/api/v1/buckets/{bucket}/objects/{key}/retention",
                  drogon::Get,
                  "console::middleware::AuthMiddleware");
    ADD_METHOD_TO(ObjectsController::set_object_retention,
                  "/api/v1/buckets/{bucket}/objects/{key}/retention",
                  drogon::Put,
                  "console::middleware::AuthMiddleware");
    ADD_METHOD_TO(ObjectsController::get_object_legal_hold,
                  "/api/v1/buckets/{bucket}/objects/{key}/legal-hold",
                  drogon::Get,
                  "console::middleware::AuthMiddleware");
    ADD_METHOD_TO(ObjectsController::set_object_legal_hold,
                  "/api/v1/buckets/{bucket}/objects/{key}/legal-hold",
                  drogon::Put,
                  "console::middleware::AuthMiddleware");

    METHOD_LIST_END

    // Constructor
    void set_object_service(std::shared_ptr<services::IObjectService> service);

    // Handlers
    void list(const drogon::HttpRequestPtr& req,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const String& bucket);

    void get_info(const drogon::HttpRequestPtr& req,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                  const String& bucket,
                  const String& key);

    void get_object(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                    const String& bucket,
                    const String& key);

    void download(const drogon::HttpRequestPtr& req,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                  const String& bucket,
                  const String& key);

    void upload(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                const String& bucket);

    void delete_object(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       const String& bucket,
                       const String& key);

    void batch_delete(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                      const String& bucket);

    void copy(const drogon::HttpRequestPtr& req,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const String& bucket,
              const String& key);

    void get_presigned_url(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                           const String& bucket,
                           const String& key);

    void get_tags(const drogon::HttpRequestPtr& req,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                  const String& bucket,
                  const String& key);

    void set_tags(const drogon::HttpRequestPtr& req,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                  const String& bucket,
                  const String& key);

    // Multipart upload handlers
    void initiate_multipart_upload(const drogon::HttpRequestPtr& req,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                   const String& bucket);

    void upload_part(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                     const String& bucket,
                     const String& uploadId,
                     int partNumber);

    void complete_multipart_upload(const drogon::HttpRequestPtr& req,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                   const String& bucket,
                                   const String& uploadId);

    void abort_multipart_upload(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                const String& bucket,
                                const String& uploadId);

    void list_parts(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                    const String& bucket,
                    const String& uploadId);

    void list_multipart_uploads(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                const String& bucket);

    // Object versioning handlers
    void list_object_versions(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                              const String& bucket,
                              const String& key);

    void get_object_version(const drogon::HttpRequestPtr& req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                            const String& bucket,
                            const String& key,
                            const String& versionId);

    void delete_object_version(const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                               const String& bucket,
                               const String& key,
                               const String& versionId);

    void restore_object_version(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                const String& bucket,
                                const String& key,
                                const String& versionId);

    // Object retention and legal hold handlers
    void get_object_retention(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                              const String& bucket,
                              const String& key);

    void set_object_retention(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                              const String& bucket,
                              const String& key);

    void get_object_legal_hold(const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                               const String& bucket,
                               const String& key);

    void set_object_legal_hold(const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                               const String& bucket,
                               const String& key);

  private:
    UserInfo get_user_from_request(const drogon::HttpRequestPtr& req);

    std::shared_ptr<services::IObjectService> object_service_;
};

} // namespace console::api
