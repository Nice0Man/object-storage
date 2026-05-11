#include "console/api/ObjectsController.hpp"

#include "console/common/Logger.hpp"
#include "console/common/ServiceLocator.hpp"
#include "console/services/ObjectService.hpp"
#include "console/storage/DatabaseManager.hpp"

#include <chrono>
#include <ctime>
#include <json/json.h>

namespace console::api {

void
ObjectsController::set_object_service(std::shared_ptr<services::IObjectService> service) {
    object_service_ = service;
}

void
ObjectsController::list(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                        const String& bucket) {
    auto user_info = get_user_from_request(req);
    auto object_service = ServiceLocator::object_service();

    if (!object_service) {
        CONSOLE_LOG_ERROR("ObjectService not initialized");
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    // Get query parameters
    String prefix = req->getParameter("prefix");
    bool recursive = req->getParameter("recursive") == "true";
    int max_keys = 1000;

    try {
        max_keys = std::stoi(req->getParameter("max_keys"));
    } catch (...) {
        // Use default
    }

    auto result = object_service->list_objects(user_info, bucket, prefix, recursive, max_keys);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    // Build response
    Json::Value response;
    response["objects"] = Json::Value(Json::arrayValue);

    for (const auto& obj : result.value()) {
        response["objects"].append(obj.to_json());
    }

    response["total"] = static_cast<int>(result.value().size());
    response["bucket"] = bucket;
    response["prefix"] = prefix;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
ObjectsController::get_info(const drogon::HttpRequestPtr& req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                            const String& bucket,
                            const String& key) {
    auto user_info = get_user_from_request(req);
    auto object_service = ServiceLocator::object_service();

    if (!object_service) {
        CONSOLE_LOG_ERROR("ObjectService not initialized");
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = object_service->get_object_info(user_info, bucket, key);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(result.value().to_json());
    callback(resp);
}

void
ObjectsController::get_object(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                              const String& bucket,
                              const String& key) {
    // This is an alias for download() as per swagger.json specification
    // GET /api/v1/buckets/{bucket}/objects/{objectKey} should return binary content
    download(req, std::move(callback), bucket, key);
}

void
ObjectsController::download(const drogon::HttpRequestPtr& req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                            const String& bucket,
                            const String& key) {
    auto user_info = get_user_from_request(req);

    // Get ObjectService - need concrete type for SSE methods
    auto object_service = std::dynamic_pointer_cast<services::ObjectService>(ServiceLocator::object_service());

    if (!object_service) {
        CONSOLE_LOG_ERROR("ObjectService not initialized");
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    // Check for SSE-C customer key header
    String sse_customer_key = req->getHeader("x-amz-server-side-encryption-customer-key");

    Result<ByteArray, models::ApiError> result = Err<ByteArray>(
        models::ApiError(HttpStatus::InternalServerError, "Not executed"));

    if (!sse_customer_key.empty()) {
        // Use SSE-aware download
        result = object_service->download_object_sse(user_info, bucket, key, sse_customer_key);
    } else {
        // Standard download
        result = object_service->download_object(user_info, bucket, key);
    }

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    const auto& data = result.value();
    int64_t total_size = static_cast<int64_t>(data.size());

    // Parse Range header for partial content support
    String range_header = req->getHeader("Range");
    int64_t start = 0;
    int64_t end = total_size - 1;
    bool has_range = false;

    if (!range_header.empty() && range_header.find("bytes=") == 0) {
        // Parse "bytes=start-end" format
        String range_spec = range_header.substr(6); // Remove "bytes="
        size_t dash_pos = range_spec.find('-');

        if (dash_pos != String::npos) {
            String start_str = range_spec.substr(0, dash_pos);
            String end_str = range_spec.substr(dash_pos + 1);

            if (!start_str.empty()) {
                start = std::stoll(start_str);
            }
            if (!end_str.empty()) {
                end = std::stoll(end_str);
            } else if (start_str.empty() && !end_str.empty()) {
                // Suffix range: bytes=-500 means last 500 bytes
                int64_t suffix_length = std::stoll(end_str);
                start = total_size - suffix_length;
                if (start < 0)
                    start = 0;
                end = total_size - 1;
            }

            // Validate range
            if (start >= 0 && end >= start && end < total_size) {
                has_range = true;
            } else if (start >= total_size) {
                // Range not satisfiable
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k416RequestedRangeNotSatisfiable);
                resp->addHeader("Content-Range", "bytes */" + std::to_string(total_size));
                callback(resp);
                return;
            }
        }
    }

    // Record throughput stats
    int64_t data_size = has_range ? (end - start + 1) : total_size;
    auto db = ServiceLocator::database();
    if (db) {
        storage::DataThroughputStats throughput;
        throughput.timestamp = std::time(nullptr);
        throughput.read_bytes = data_size;
        throughput.write_bytes = 0;
        throughput.total_bytes = data_size;
        db->add_throughput_stat(throughput);
        CONSOLE_LOG_DEBUG("Recorded download throughput: {} bytes", data_size);
    }

    // Create response
    auto resp = drogon::HttpResponse::newHttpResponse();

    if (has_range) {
        // Partial content response (HTTP 206)
        resp->setStatusCode(drogon::k206PartialContent);
        resp->setBody(String(data.begin() + start, data.begin() + end + 1));
        resp->addHeader("Content-Range",
                        "bytes " + std::to_string(start) + "-" + std::to_string(end) + "/" +
                            std::to_string(total_size));
        CONSOLE_LOG_DEBUG("Serving partial content: bytes {}-{}/{}", start, end, total_size);
    } else {
        // Full content response (HTTP 200)
        resp->setBody(String(data.begin(), data.end()));
    }

    resp->setContentTypeCode(drogon::CT_APPLICATION_OCTET_STREAM);
    resp->addHeader("Accept-Ranges", "bytes");
    resp->addHeader("Content-Length", std::to_string(data_size));

    // Set Content-Disposition for download
    size_t last_slash = key.find_last_of('/');
    String filename = (last_slash != String::npos) ? key.substr(last_slash + 1) : key;
    resp->addHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");

    callback(resp);
}

void
ObjectsController::upload(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                          const String& bucket) {
    auto user_info = get_user_from_request(req);

    // Get object key from form data or query parameter or from multipart
    String key = req->getParameter("key");

    // Check for multipart form-data upload
    bool is_multipart = (req->getHeader("Content-Type").find("multipart/form-data") != std::string::npos);

    if (!is_multipart) {
        // Fallback to raw body upload
        if (key.empty()) {
            Json::Value error;
            error["code"] = 400;
            error["message"] = "Object key is required";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k400BadRequest);
            callback(resp);
            return;
        }

        // Get content type
        String content_type = req->getHeader("Content-Type");
        if (content_type.empty()) {
            content_type = "application/octet-stream";
        }

        // Get body as ByteArray
        auto body = req->getBody();
        ByteArray data(body.begin(), body.end());

        // Empty metadata map
        StringMap metadata;

        // Check for SSE-C customer key header
        String sse_customer_key = req->getHeader("x-amz-server-side-encryption-customer-key");

        // Get ObjectService - need concrete type for SSE methods
        auto object_service = std::dynamic_pointer_cast<services::ObjectService>(ServiceLocator::object_service());
        if (!object_service) {
            Json::Value error;
            error["error"] = "Service not available";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        Result<models::Object, models::ApiError> result = Err<models::Object>(
            models::ApiError(HttpStatus::InternalServerError, "Not executed"));

        if (!sse_customer_key.empty()) {
            // Use SSE-aware upload
            result = object_service->upload_object_sse(
                user_info, bucket, key, data, content_type, metadata, sse_customer_key);
        } else {
            // Standard upload (may still use SSE-S3 if bucket encryption is enabled)
            result = object_service->upload_object(user_info, bucket, key, data, content_type, metadata);
        }

        if (!result) {
            auto error_json = result.error().to_json();
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
            resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
            callback(resp);
            return;
        }

        // Record throughput stats for successful upload
        int64_t data_size = static_cast<int64_t>(data.size());
        auto db = ServiceLocator::database();
        if (db) {
            storage::DataThroughputStats throughput;
            throughput.timestamp = std::time(nullptr);
            throughput.read_bytes = 0;
            throughput.write_bytes = data_size;
            throughput.total_bytes = data_size;
            db->add_throughput_stat(throughput);
            CONSOLE_LOG_DEBUG("Recorded upload throughput: {} bytes", data_size);
        }

        // Success response for raw body upload
        auto resp = drogon::HttpResponse::newHttpJsonResponse(result.value().to_json());
        resp->setStatusCode(drogon::k201Created);
        callback(resp);
        return;
    }

    // Handle multipart/form-data upload
    // For now, log and return error - multipart will be handled when we properly integrate FileUpload
    CONSOLE_LOG_ERROR("Multipart upload not yet fully implemented");
    Json::Value error;
    error["code"] = 501;
    error["message"] = "Multipart upload temporarily disabled. Please use raw binary upload with ?key parameter";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(drogon::k501NotImplemented);
    callback(resp);
}

void
ObjectsController::delete_object(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                 const String& bucket,
                                 const String& key) {
    auto user_info = get_user_from_request(req);
    auto object_service = ServiceLocator::object_service();

    if (!object_service) {
        CONSOLE_LOG_ERROR("ObjectService not initialized");
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = object_service->delete_object(user_info, bucket, key);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    // Return 200 with message per swagger.json
    Json::Value response;
    response["message"] = "Object deleted successfully";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

void
ObjectsController::batch_delete(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                const String& bucket) {
    auto user_info = get_user_from_request(req);

    auto json = req->getJsonObject();
    if (!json) {
        Json::Value error;
        error["code"] = 400;
        error["message"] = "Invalid JSON request body";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    Json::Value request_body = *json;

    if (!request_body.isMember("keys") || !request_body["keys"].isArray()) {
        Json::Value error;
        error["code"] = 400;
        error["message"] = "'keys' array is required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    Vector<String> keys;
    for (const auto& key_json : request_body["keys"]) {
        keys.push_back(key_json.asString());
    }

    auto result = ServiceLocator::object_service()->delete_objects(user_info, bucket, keys);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(result.value());
    callback(resp);
}

void
ObjectsController::copy(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                        const String& bucket,
                        const String& key) {
    auto user_info = get_user_from_request(req);

    auto json = req->getJsonObject();
    if (!json) {
        Json::Value error;
        error["code"] = 400;
        error["message"] = "Invalid JSON request body";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    Json::Value request_body = *json;

    String dest_bucket = request_body.get("destination_bucket", bucket).asString();
    String dest_key = request_body.get("destination_key", "").asString();

    if (dest_key.empty()) {
        Json::Value error;
        error["code"] = 400;
        error["message"] = "destination_key is required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    auto result = ServiceLocator::object_service()->copy_object(user_info, bucket, key, dest_bucket, dest_key);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(result.value().to_json());
    callback(resp);
}

void
ObjectsController::get_presigned_url(const drogon::HttpRequestPtr& req,
                                     std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                     const String& bucket,
                                     const String& key) {
    auto user_info = get_user_from_request(req);

    int expiry_seconds = 3600; // Default 1 hour
    // Support both 'expires_in' (preferred), 'expires' (legacy), and 'expiry' (legacy) parameter names.
    String expires_param = req->getParameter("expires_in");
    if (expires_param.empty()) {
        expires_param = req->getParameter("expires");
    }
    if (expires_param.empty()) {
        expires_param = req->getParameter("expiry");
    }
    if (!expires_param.empty()) {
        try {
            expiry_seconds = std::stoi(expires_param);
        } catch (...) {
            // Use default
        }
    }

    auto result = ServiceLocator::object_service()->generate_presigned_url(
        user_info, bucket, key, "GET", expiry_seconds);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    // Calculate expires_at as ISO 8601 datetime per swagger.json
    auto expires_at = std::chrono::system_clock::now() + std::chrono::seconds(expiry_seconds);
    auto expires_time = std::chrono::system_clock::to_time_t(expires_at);
    std::tm tm = *std::gmtime(&expires_time);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);

    Json::Value response;
    response["url"] = result.value();
    response["expires_at"] = buffer;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
ObjectsController::get_tags(const drogon::HttpRequestPtr& req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                            const String& bucket,
                            const String& key) {
    auto user_info = get_user_from_request(req);

    auto result = ServiceLocator::object_service()->get_object_tags(user_info, bucket, key);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    // Format response per swagger.json: { "tags": { "key": "value", ... } }
    Json::Value response;
    Json::Value tags_obj(Json::objectValue);
    for (const auto& [k, v] : result.value()) {
        tags_obj[k] = v;
    }
    response["tags"] = tags_obj;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
ObjectsController::set_tags(const drogon::HttpRequestPtr& req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                            const String& bucket,
                            const String& key) {
    auto user_info = get_user_from_request(req);

    auto json = req->getJsonObject();
    if (!json) {
        Json::Value error;
        error["error"] = "Invalid JSON request body";
        error["code"] = 400;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    Json::Value request_body = *json;

    if (!request_body.isMember("tags") || !request_body["tags"].isObject()) {
        Json::Value error;
        error["error"] = "'tags' object is required";
        error["code"] = 400;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    StringMap tags;
    const Json::Value& tags_obj = request_body["tags"];
    for (const auto& tag_key : tags_obj.getMemberNames()) {
        tags[tag_key] = tags_obj[tag_key].asString();
    }

    auto result = ServiceLocator::object_service()->set_object_tags(user_info, bucket, key, tags);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    // Return success response per swagger.json
    Json::Value response;
    response["message"] = "Tags updated successfully";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

// ============================================================================
// Multipart Upload Handlers
// ============================================================================

void
ObjectsController::initiate_multipart_upload(const drogon::HttpRequestPtr& req,
                                             std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                             const String& bucket) {
    auto user_info = get_user_from_request(req);
    auto object_service = std::dynamic_pointer_cast<services::ObjectService>(ServiceLocator::object_service());

    if (!object_service) {
        CONSOLE_LOG_ERROR("ObjectService not initialized");
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    // Parse request body
    auto json = req->getJsonObject();
    if (!json || !json->isMember("key")) {
        Json::Value error;
        error["error"] = "Object key is required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    String key = (*json)["key"].asString();
    String content_type = (*json).get("content_type", "application/octet-stream").asString();

    StringMap metadata;
    if (json->isMember("metadata") && (*json)["metadata"].isObject()) {
        for (const auto& k : (*json)["metadata"].getMemberNames()) {
            metadata[k] = (*json)["metadata"][k].asString();
        }
    }

    auto result = object_service->initiate_multipart_upload(user_info, bucket, key, content_type, metadata);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(result.value().to_json());
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

void
ObjectsController::upload_part(const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                               const String& bucket,
                               const String& uploadId,
                               int partNumber) {
    auto user_info = get_user_from_request(req);
    auto object_service = std::dynamic_pointer_cast<services::ObjectService>(ServiceLocator::object_service());

    if (!object_service) {
        CONSOLE_LOG_ERROR("ObjectService not initialized");
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    // Get object key from query parameter
    String key = req->getParameter("key");
    if (key.empty()) {
        Json::Value error;
        error["error"] = "Object key is required (query parameter 'key')";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    // Get body as binary data
    auto body = req->getBody();
    ByteArray data(body.begin(), body.end());

    if (data.empty()) {
        Json::Value error;
        error["error"] = "Part data is required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    auto result = object_service->upload_part(user_info, bucket, key, uploadId, partNumber, data);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    response["part_number"] = partNumber;
    response["etag"] = result.value();
    response["size"] = static_cast<Json::Int64>(data.size());

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

void
ObjectsController::complete_multipart_upload(const drogon::HttpRequestPtr& req,
                                             std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                             const String& bucket,
                                             const String& uploadId) {
    auto user_info = get_user_from_request(req);
    auto object_service = std::dynamic_pointer_cast<services::ObjectService>(ServiceLocator::object_service());

    if (!object_service) {
        CONSOLE_LOG_ERROR("ObjectService not initialized");
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    // Parse request body
    auto json = req->getJsonObject();
    if (!json || !json->isMember("key") || !json->isMember("parts")) {
        Json::Value error;
        error["error"] = "Object key and parts are required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    String key = (*json)["key"].asString();

    Vector<clients::CompletedPart> parts;
    for (const auto& part_json : (*json)["parts"]) {
        clients::CompletedPart part;
        part.part_number = part_json["part_number"].asInt();
        part.etag = part_json["etag"].asString();
        parts.push_back(part);
    }

    auto result = object_service->complete_multipart_upload(user_info, bucket, key, uploadId, parts);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(result.value().to_json());
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

void
ObjectsController::abort_multipart_upload(const drogon::HttpRequestPtr& req,
                                          std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                          const String& bucket,
                                          const String& uploadId) {
    auto user_info = get_user_from_request(req);
    auto object_service = std::dynamic_pointer_cast<services::ObjectService>(ServiceLocator::object_service());

    if (!object_service) {
        CONSOLE_LOG_ERROR("ObjectService not initialized");
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    // Get object key from query parameter
    String key = req->getParameter("key");
    if (key.empty()) {
        Json::Value error;
        error["error"] = "Object key is required (query parameter 'key')";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    auto result = object_service->abort_multipart_upload(user_info, bucket, key, uploadId);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    response["message"] = "Multipart upload aborted successfully";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

void
ObjectsController::list_parts(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                              const String& bucket,
                              const String& uploadId) {
    auto user_info = get_user_from_request(req);
    auto object_service = std::dynamic_pointer_cast<services::ObjectService>(ServiceLocator::object_service());

    if (!object_service) {
        CONSOLE_LOG_ERROR("ObjectService not initialized");
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    // Get object key from query parameter
    String key = req->getParameter("key");
    if (key.empty()) {
        Json::Value error;
        error["error"] = "Object key is required (query parameter 'key')";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    auto result = object_service->list_parts(user_info, bucket, key, uploadId);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(result.value().to_json());
    callback(resp);
}

void
ObjectsController::list_multipart_uploads(const drogon::HttpRequestPtr& req,
                                          std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                          const String& bucket) {
    auto user_info = get_user_from_request(req);
    auto object_service = std::dynamic_pointer_cast<services::ObjectService>(ServiceLocator::object_service());

    if (!object_service) {
        CONSOLE_LOG_ERROR("ObjectService not initialized");
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    String prefix = req->getParameter("prefix");

    auto result = object_service->list_multipart_uploads(user_info, bucket, prefix);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    Json::Value uploads(Json::arrayValue);
    for (const auto& upload : result.value()) {
        uploads.append(upload.to_json());
    }
    response["uploads"] = uploads;
    response["bucket"] = bucket;
    response["prefix"] = prefix;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

// ============================================================================
// Object Versioning Handlers
// ============================================================================

void
ObjectsController::list_object_versions(const drogon::HttpRequestPtr& req,
                                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                        const String& bucket,
                                        const String& key) {
    auto user_info = get_user_from_request(req);
    auto storage_client = ServiceLocator::storage_client();

    if (!storage_client) {
        CONSOLE_LOG_ERROR("Storage client not initialized");
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = storage_client->list_object_versions(bucket, key);

    if (!result) {
        Json::Value error;
        error["error"] = result.error();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k404NotFound);
        callback(resp);
        return;
    }

    Json::Value response;
    Json::Value versions(Json::arrayValue);
    for (const auto& version : result.value()) {
        versions.append(version.to_json());
    }
    response["versions"] = versions;
    response["bucket"] = bucket;
    response["key"] = key;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
ObjectsController::get_object_version(const drogon::HttpRequestPtr& req,
                                      std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                      const String& bucket,
                                      const String& key,
                                      const String& versionId) {
    auto user_info = get_user_from_request(req);
    auto storage_client = ServiceLocator::storage_client();

    if (!storage_client) {
        CONSOLE_LOG_ERROR("Storage client not initialized");
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = storage_client->get_object_version(bucket, key, versionId);

    if (!result) {
        Json::Value error;
        error["error"] = result.error();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k404NotFound);
        callback(resp);
        return;
    }

    // Get object metadata for content type
    auto stat_result = storage_client->stat_object(bucket, key);
    String content_type = "application/octet-stream";
    if (stat_result) {
        content_type = stat_result.value().content_type();
    }

    // Return object data
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setBody(String(result.value().begin(), result.value().end()));
    resp->setContentTypeString(content_type);
    resp->addHeader("X-Version-Id", versionId);
    callback(resp);
}

void
ObjectsController::delete_object_version(const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                         const String& bucket,
                                         const String& key,
                                         const String& versionId) {
    auto user_info = get_user_from_request(req);
    auto storage_client = ServiceLocator::storage_client();

    if (!storage_client) {
        CONSOLE_LOG_ERROR("Storage client not initialized");
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = storage_client->delete_object_version(bucket, key, versionId);

    if (!result) {
        Json::Value error;
        error["error"] = result.error();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k404NotFound);
        callback(resp);
        return;
    }

    Json::Value response;
    response["message"] = "Version deleted successfully";
    response["version_id"] = versionId;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

void
ObjectsController::restore_object_version(const drogon::HttpRequestPtr& req,
                                          std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                          const String& bucket,
                                          const String& key,
                                          const String& versionId) {
    auto user_info = get_user_from_request(req);
    auto storage_client = ServiceLocator::storage_client();

    if (!storage_client) {
        CONSOLE_LOG_ERROR("Storage client not initialized");
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = storage_client->restore_object_version(bucket, key, versionId);

    if (!result) {
        Json::Value error;
        error["error"] = result.error();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k404NotFound);
        callback(resp);
        return;
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(result.value().to_json());
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

void
ObjectsController::get_object_retention(const drogon::HttpRequestPtr& req,
                                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                        const String& bucket,
                                        const String& key) {
    auto user_info = get_user_from_request(req);
    auto storage_client = ServiceLocator::storage_client();

    if (!storage_client) {
        CONSOLE_LOG_ERROR("Storage client not initialized");
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    String version_id = req->getParameter("versionId");
    if (version_id.empty())
        version_id = "null";

    auto result = storage_client->get_object_retention(bucket, key, version_id);

    if (!result) {
        Json::Value error;
        error["error"] = result.error();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k404NotFound);
        callback(resp);
        return;
    }

    // Result is std::pair<String, int64_t> = {mode, retain_until_date}
    Json::Value response;
    response["mode"] = result.value().first;
    response["retain_until_date"] = static_cast<Json::Int64>(result.value().second);
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

void
ObjectsController::set_object_retention(const drogon::HttpRequestPtr& req,
                                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                        const String& bucket,
                                        const String& key) {
    auto user_info = get_user_from_request(req);
    auto storage_client = ServiceLocator::storage_client();

    if (!storage_client) {
        CONSOLE_LOG_ERROR("Storage client not initialized");
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("mode") || !json->isMember("retain_until_date")) {
        Json::Value error;
        error["error"] = "Retention mode and retain_until_date are required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    String mode = (*json)["mode"].asString();
    int64_t retain_until = (*json)["retain_until_date"].asInt64();
    String version_id = req->getParameter("versionId");
    if (version_id.empty())
        version_id = "null";

    // Note: order is bucket, key, mode, retain_until, version_id
    auto result = storage_client->set_object_retention(bucket, key, mode, retain_until, version_id);

    if (!result) {
        Json::Value error;
        error["error"] = result.error();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    Json::Value response;
    response["message"] = "Retention set successfully";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

void
ObjectsController::get_object_legal_hold(const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                         const String& bucket,
                                         const String& key) {
    auto user_info = get_user_from_request(req);
    auto storage_client = ServiceLocator::storage_client();

    if (!storage_client) {
        CONSOLE_LOG_ERROR("Storage client not initialized");
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    String version_id = req->getParameter("versionId");
    if (version_id.empty())
        version_id = "null";

    auto result = storage_client->get_object_legal_hold(bucket, key, version_id);

    if (!result) {
        Json::Value error;
        error["error"] = result.error();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k404NotFound);
        callback(resp);
        return;
    }

    Json::Value response;
    response["status"] = result.value();
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

void
ObjectsController::set_object_legal_hold(const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                         const String& bucket,
                                         const String& key) {
    auto user_info = get_user_from_request(req);
    auto storage_client = ServiceLocator::storage_client();

    if (!storage_client) {
        CONSOLE_LOG_ERROR("Storage client not initialized");
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("status")) {
        Json::Value error;
        error["error"] = "Legal hold status is required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    bool enabled = (*json)["status"].asBool();
    String version_id = req->getParameter("versionId");
    if (version_id.empty())
        version_id = "null";

    // Note: order is bucket, key, enabled, version_id
    auto result = storage_client->set_object_legal_hold(bucket, key, enabled, version_id);

    if (!result) {
        Json::Value error;
        error["error"] = result.error();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    Json::Value response;
    response["message"] = "Legal hold updated successfully";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

UserInfo
ObjectsController::get_user_from_request(const drogon::HttpRequestPtr& req) {
    try {
        return req->attributes()->get<UserInfo>("user_info");
    } catch (...) {
        // If no user in attributes, return empty UserInfo
        // This should not happen if AuthMiddleware is properly configured
        CONSOLE_LOG_ERROR("No user_info found in request attributes");
        return UserInfo{};
    }
}

} // namespace console::api
