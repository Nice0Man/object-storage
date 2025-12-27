#pragma once

#include "console/clients/StorageClient.hpp"
#include "console/common/Types.hpp"

#include <drogon/HttpController.h>

#include <memory>

namespace console::api {

/**
 * @brief Presigned URL Controller
 *
 * Handles public access to objects via presigned URLs with HMAC signature validation.
 * No authentication middleware - signature validation is performed manually.
 */
class PresignedController : public drogon::HttpController<PresignedController> {
  public:
    METHOD_LIST_BEGIN

    // Access object via presigned URL (no auth middleware - public access)
    ADD_METHOD_TO(PresignedController::access_object, "/api/v1/objects/{bucket}/{key:.*}", drogon::Get);

    // Upload object via presigned URL (PUT for upload)
    ADD_METHOD_TO(PresignedController::upload_object, "/api/v1/objects/{bucket}/{key:.*}", drogon::Put);

    METHOD_LIST_END

    /**
     * @brief Access object via presigned URL
     *
     * Validates HMAC signature and returns object data if valid.
     * Supports GET method for downloads.
     */
    void access_object(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       const String& bucket,
                       const String& key);

    /**
     * @brief Upload object via presigned URL
     *
     * Validates HMAC signature and uploads object data if valid.
     * Supports PUT method for uploads.
     */
    void upload_object(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       const String& bucket,
                       const String& key);

  private:
    /**
     * @brief Validate presigned URL signature
     *
     * @param bucket Bucket name
     * @param key Object key
     * @param expires Expiration timestamp
     * @param method HTTP method (GET, PUT)
     * @param signature Provided signature
     * @return true if signature is valid and not expired
     */
    bool validate_signature(const String& bucket,
                            const String& key,
                            int64_t expires,
                            const String& method,
                            const String& signature);

    /**
     * @brief Generate HMAC-SHA256 signature
     */
    String generate_signature(const String& bucket, const String& key, int64_t expires, const String& method);
};

} // namespace console::api
