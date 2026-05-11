#pragma once

#include "console/common/Types.hpp"

#include <drogon/HttpController.h>

namespace console::api {

/**
 * @brief Buckets management controller
 *
 * Handles S3 bucket operations: list, create, delete, configure
 */
class BucketsController : public drogon::HttpController<BucketsController> {
  public:
    METHOD_LIST_BEGIN
    // List all buckets
    ADD_METHOD_TO(BucketsController::list, "/api/v1/buckets", drogon::Get, "AuthFilter");
    // Get bucket info
    ADD_METHOD_TO(BucketsController::get, "/api/v1/buckets/{1}", drogon::Get, "AuthFilter");
    // Create bucket
    ADD_METHOD_TO(BucketsController::create, "/api/v1/buckets", drogon::Post, "AuthFilter");
    // Delete bucket
    ADD_METHOD_TO(BucketsController::remove, "/api/v1/buckets/{1}", drogon::Delete, "AuthFilter");
    // Get bucket policy
    ADD_METHOD_TO(BucketsController::getPolicy, "/api/v1/buckets/{1}/policy", drogon::Get, "AuthFilter");
    // Set bucket policy
    ADD_METHOD_TO(BucketsController::setPolicy, "/api/v1/buckets/{1}/policy", drogon::Put, "AuthFilter");

    // Bucket versioning
    ADD_METHOD_TO(BucketsController::getVersioning, "/api/v1/buckets/{1}/versioning", drogon::Get, "AuthFilter");
    ADD_METHOD_TO(BucketsController::setVersioning, "/api/v1/buckets/{1}/versioning", drogon::Put, "AuthFilter");

    // Bucket tags
    ADD_METHOD_TO(BucketsController::getTags, "/api/v1/buckets/{1}/tags", drogon::Get, "AuthFilter");
    ADD_METHOD_TO(BucketsController::setTags, "/api/v1/buckets/{1}/tags", drogon::Put, "AuthFilter");
    ADD_METHOD_TO(BucketsController::deleteTags, "/api/v1/buckets/{1}/tags", drogon::Delete, "AuthFilter");

    // Bucket encryption
    ADD_METHOD_TO(BucketsController::getEncryption, "/api/v1/buckets/{1}/encryption", drogon::Get, "AuthFilter");
    ADD_METHOD_TO(BucketsController::setEncryption, "/api/v1/buckets/{1}/encryption", drogon::Put, "AuthFilter");
    ADD_METHOD_TO(BucketsController::deleteEncryption, "/api/v1/buckets/{1}/encryption", drogon::Delete, "AuthFilter");

    // Bucket lifecycle
    ADD_METHOD_TO(BucketsController::getLifecycle, "/api/v1/buckets/{1}/lifecycle", drogon::Get, "AuthFilter");
    ADD_METHOD_TO(BucketsController::setLifecycle, "/api/v1/buckets/{1}/lifecycle", drogon::Put, "AuthFilter");
    ADD_METHOD_TO(BucketsController::deleteLifecycle, "/api/v1/buckets/{1}/lifecycle", drogon::Delete, "AuthFilter");

    // Bucket object lock configuration
    ADD_METHOD_TO(BucketsController::getObjectLock, "/api/v1/buckets/{1}/object-lock", drogon::Get, "AuthFilter");
    ADD_METHOD_TO(BucketsController::setObjectLock, "/api/v1/buckets/{1}/object-lock", drogon::Put, "AuthFilter");
    METHOD_LIST_END

    void list(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void get(const drogon::HttpRequestPtr& req,
             std::function<void(const drogon::HttpResponsePtr&)>&& callback,
             const String& bucket_name);

    void create(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void remove(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                const String& bucket_name);

    void getPolicy(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const String& bucket_name);

    void setPolicy(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const String& bucket_name);

    // Versioning
    void getVersioning(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       const String& bucket_name);

    void setVersioning(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       const String& bucket_name);

    // Tags
    void getTags(const drogon::HttpRequestPtr& req,
                 std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                 const String& bucket_name);

    void setTags(const drogon::HttpRequestPtr& req,
                 std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                 const String& bucket_name);

    void deleteTags(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                    const String& bucket_name);

    // Encryption
    void getEncryption(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       const String& bucket_name);

    void setEncryption(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       const String& bucket_name);

    void deleteEncryption(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                          const String& bucket_name);

    // Lifecycle
    void getLifecycle(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                      const String& bucket_name);

    void setLifecycle(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                      const String& bucket_name);

    void deleteLifecycle(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                         const String& bucket_name);

    // Object Lock
    void getObjectLock(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       const String& bucket_name);

    void setObjectLock(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       const String& bucket_name);

  private:
    UserInfo get_user_from_request(const drogon::HttpRequestPtr& req);
};

} // namespace console::api
