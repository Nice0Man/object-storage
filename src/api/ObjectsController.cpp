#include "console/api/ObjectsController.hpp"
#include "console/common/Logger.hpp"
#include <json/json.h>

namespace console::api {


void ObjectsController::set_object_service(
    std::shared_ptr<services::IObjectService> service
) {
    object_service_ = service;
}

void ObjectsController::list(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const String& bucket
) {
    auto user_info = get_user_from_request(req);
    
    // Get query parameters
    String prefix = req->getParameter("prefix");
    bool recursive = req->getParameter("recursive") == "true";
    int max_keys = 1000;
    
    try {
        max_keys = std::stoi(req->getParameter("max_keys"));
    } catch (...) {
        // Use default
    }
    
    auto result = object_service_->list_objects(
        user_info,
        bucket,
        prefix,
        recursive,
        max_keys
    );
    
    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(
            result.error().status()
        ));
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

void ObjectsController::get_info(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const String& bucket,
    const String& key
) {
    auto user_info = get_user_from_request(req);
    
    auto result = object_service_->get_object_info(user_info, bucket, key);
    
    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(
            result.error().status()
        ));
        callback(resp);
        return;
    }
    
    auto resp = drogon::HttpResponse::newHttpJsonResponse(
        result.value().to_json()
    );
    callback(resp);
}

void ObjectsController::download(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const String& bucket,
    const String& key
) {
    auto user_info = get_user_from_request(req);
    
    auto result = object_service_->download_object(user_info, bucket, key);
    
    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(
            result.error().status()
        ));
        callback(resp);
        return;
    }
    
    // Create binary response
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setBody(String(result.value().begin(), result.value().end()));
    resp->setContentTypeCode(drogon::CT_APPLICATION_OCTET_STREAM);
    
    // Set Content-Disposition for download
    size_t last_slash = key.find_last_of('/');
    String filename = (last_slash != String::npos) 
        ? key.substr(last_slash + 1) 
        : key;
    resp->addHeader("Content-Disposition", 
                    "attachment; filename=\"" + filename + "\"");
    
    callback(resp);
}

void ObjectsController::upload(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const String& bucket
) {
    auto user_info = get_user_from_request(req);
    
    // Get object key from query parameter
    String key = req->getParameter("key");
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
    
    auto result = object_service_->upload_object(
        user_info,
        bucket,
        key,
        data,
        content_type
    );
    
    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(
            result.error().status()
        ));
        callback(resp);
        return;
    }
    
    auto resp = drogon::HttpResponse::newHttpJsonResponse(
        result.value().to_json()
    );
    resp->setStatusCode(drogon::k201Created);
    callback(resp);
}

void ObjectsController::delete_object(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const String& bucket,
    const String& key
) {
    auto user_info = get_user_from_request(req);
    
    auto result = object_service_->delete_object(user_info, bucket, key);
    
    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(
            result.error().status()
        ));
        callback(resp);
        return;
    }
    
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k204NoContent);
    callback(resp);
}

void ObjectsController::batch_delete(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const String& bucket
) {
    auto user_info = get_user_from_request(req);
    
    // Parse request body
    Json::Value request_body = *req->getJsonObject();
    
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
    
    auto result = object_service_->delete_objects(user_info, bucket, keys);
    
    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(
            result.error().status()
        ));
        callback(resp);
        return;
    }
    
    auto resp = drogon::HttpResponse::newHttpJsonResponse(result.value());
    callback(resp);
}

void ObjectsController::copy(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const String& bucket,
    const String& key
) {
    auto user_info = get_user_from_request(req);
    
    // Parse request body
    Json::Value request_body = *req->getJsonObject();
    
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
    
    auto result = object_service_->copy_object(
        user_info,
        bucket,
        key,
        dest_bucket,
        dest_key
    );
    
    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(
            result.error().status()
        ));
        callback(resp);
        return;
    }
    
    auto resp = drogon::HttpResponse::newHttpJsonResponse(
        result.value().to_json()
    );
    callback(resp);
}

void ObjectsController::get_presigned_url(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const String& bucket,
    const String& key
) {
    auto user_info = get_user_from_request(req);
    
    int expiry_seconds = 3600;  // Default 1 hour
    try {
        expiry_seconds = std::stoi(req->getParameter("expiry"));
    } catch (...) {
        // Use default
    }
    
    auto result = object_service_->generate_presigned_url(
        user_info,
        bucket,
        key,
        expiry_seconds
    );
    
    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(
            result.error().status()
        ));
        callback(resp);
        return;
    }
    
    Json::Value response;
    response["url"] = result.value();
    response["expires_in"] = expiry_seconds;
    
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void ObjectsController::get_tags(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const String& bucket,
    const String& key
) {
    auto user_info = get_user_from_request(req);
    
    auto result = object_service_->get_object_tags(user_info, bucket, key);
    
    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(
            result.error().status()
        ));
        callback(resp);
        return;
    }
    
    Json::Value response;
    for (const auto& [k, v] : result.value()) {
        response[k] = v;
    }
    
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void ObjectsController::set_tags(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const String& bucket,
    const String& key
) {
    auto user_info = get_user_from_request(req);
    
    // Parse tags from request body
    Json::Value request_body = *req->getJsonObject();
    
    StringMap tags;
    for (const auto& tag_key : request_body.getMemberNames()) {
        tags[tag_key] = request_body[tag_key].asString();
    }
    
    auto result = object_service_->set_object_tags(
        user_info,
        bucket,
        key,
        tags
    );
    
    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(
            result.error().status()
        ));
        callback(resp);
        return;
    }
    
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k204NoContent);
    callback(resp);
}

UserInfo ObjectsController::get_user_from_request(
    const drogon::HttpRequestPtr& req
) {
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

