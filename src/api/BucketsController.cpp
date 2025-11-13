
//
#include "console/api/BucketsController.hpp"

#include "console/common/Logger.hpp"
#include "console/common/ServiceLocator.hpp"
#include "console/services/BucketService.hpp"

#include <json/json.h>

namespace console::api {

void
BucketsController::list(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto user_info = get_user_from_request(req);
    auto bucket_service = ServiceLocator::bucket_service();

    if (!bucket_service) {
        CONSOLE_LOG_ERROR("BucketService not initialized");
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    CONSOLE_LOG_DEBUG("List buckets requested by user: {}", user_info.access_key);

    auto result = bucket_service->list_buckets(user_info);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    response["buckets"] = Json::Value(Json::arrayValue);

    for (const auto& bucket : result.value()) {
        response["buckets"].append(bucket.to_json());
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
BucketsController::get(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       const String& bucket_name) {
    auto user_info = get_user_from_request(req);
    auto bucket_service = ServiceLocator::bucket_service();

    if (!bucket_service) {
        CONSOLE_LOG_ERROR("BucketService not initialized");
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    CONSOLE_LOG_DEBUG("Get bucket info: {}", bucket_name);

    auto result = bucket_service->get_bucket_info(user_info, bucket_name);

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
BucketsController::create(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto user_info = get_user_from_request(req);
    auto bucket_service = ServiceLocator::bucket_service();

    if (!bucket_service) {
        CONSOLE_LOG_ERROR("BucketService not initialized");
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("name")) {
        Json::Value error;
        error["error"] = "Bucket name is required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    String bucket_name = (*json)["name"].asString();
    String region = (*json).get("region", "us-east-1").asString();
    bool object_locking = (*json).get("object_locking", false).asBool();

    CONSOLE_LOG_INFO("Creating bucket: {} in region: {}", bucket_name, region);

    auto result = bucket_service->create_bucket(user_info, bucket_name, region, object_locking);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    response["message"] = "Bucket created successfully";
    response["bucket"] = bucket_name;
    response["region"] = region;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k201Created);
    callback(resp);
}

void
BucketsController::remove(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                          const String& bucket_name) {
    auto user_info = get_user_from_request(req);
    auto bucket_service = ServiceLocator::bucket_service();

    if (!bucket_service) {
        CONSOLE_LOG_ERROR("BucketService not initialized");
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    CONSOLE_LOG_INFO("Deleting bucket: {}", bucket_name);

    auto result = bucket_service->delete_bucket(user_info, bucket_name);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    response["message"] = "Bucket deleted successfully";
    response["bucket"] = bucket_name;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
BucketsController::getPolicy(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                             const String& bucket_name) {
    auto user_info = get_user_from_request(req);
    auto bucket_service = ServiceLocator::bucket_service();

    if (!bucket_service) {
        CONSOLE_LOG_ERROR("BucketService not initialized");
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    CONSOLE_LOG_DEBUG("Get bucket policy: {}", bucket_name);

    auto result = bucket_service->get_bucket_policy(user_info, bucket_name);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    response["bucket"] = bucket_name;
    response["policy"] = result.value();

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
BucketsController::setPolicy(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                             const String& bucket_name) {
    auto user_info = get_user_from_request(req);
    auto bucket_service = ServiceLocator::bucket_service();

    if (!bucket_service) {
        CONSOLE_LOG_ERROR("BucketService not initialized");
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("policy")) {
        Json::Value error;
        error["error"] = "Policy is required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    String policy = (*json)["policy"].asString();

    CONSOLE_LOG_INFO("Set bucket policy: {}", bucket_name);

    auto result = bucket_service->set_bucket_policy(user_info, bucket_name, policy);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    response["message"] = "Policy updated successfully";
    response["bucket"] = bucket_name;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

UserInfo
BucketsController::get_user_from_request(const drogon::HttpRequestPtr& req) {
    try {
        return req->attributes()->get<UserInfo>("user_info");
    } catch (...) {
        CONSOLE_LOG_ERROR("No user_info found in request attributes");
        return UserInfo{};
    }
}

} // namespace console::api
