
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
    Vector<String> visibility_groups;
    if ((*json).isMember("visibility_groups") && (*json)["visibility_groups"].isArray()) {
        for (const auto& entry : (*json)["visibility_groups"]) {
            visibility_groups.push_back(entry.asString());
        }
    }

    CONSOLE_LOG_INFO("Creating bucket: {} in region: {}", bucket_name, region);

    auto result = bucket_service->create_bucket(user_info, bucket_name, region, object_locking);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }
    if (!visibility_groups.empty()) {
        auto visibility_result = bucket_service->set_bucket_visibility_groups(user_info,
                                                                              bucket_name,
                                                                              visibility_groups);
        if (!visibility_result) {
            auto error_json = visibility_result.error().to_json();
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
            resp->setStatusCode(static_cast<drogon::HttpStatusCode>(visibility_result.error().status()));
            callback(resp);
            return;
        }
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

void
BucketsController::getVersioning(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                 const String& bucket_name) {
    auto user_info = get_user_from_request(req);
    auto bucket_service = ServiceLocator::bucket_service();

    if (!bucket_service) {
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = bucket_service->get_bucket_versioning(user_info, bucket_name);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    response["bucket"] = bucket_name;
    response["enabled"] = result.value();

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
BucketsController::setVersioning(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                 const String& bucket_name) {
    auto user_info = get_user_from_request(req);
    auto bucket_service = ServiceLocator::bucket_service();

    if (!bucket_service) {
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("enabled")) {
        Json::Value error;
        error["error"] = "Enabled flag is required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    bool enabled = (*json)["enabled"].asBool();
    auto result = bucket_service->set_bucket_versioning(user_info, bucket_name, enabled);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    response["message"] = "Versioning updated successfully";
    response["bucket"] = bucket_name;
    response["enabled"] = enabled;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
BucketsController::getTags(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                           const String& bucket_name) {
    auto user_info = get_user_from_request(req);
    auto bucket_service = ServiceLocator::bucket_service();

    if (!bucket_service) {
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = bucket_service->get_bucket_tags(user_info, bucket_name);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    response["bucket"] = bucket_name;
    response["tags"] = Json::Value(Json::objectValue);
    for (const auto& [key, value] : result.value()) {
        response["tags"][key] = value;
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
BucketsController::setTags(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                           const String& bucket_name) {
    auto user_info = get_user_from_request(req);
    auto bucket_service = ServiceLocator::bucket_service();

    if (!bucket_service) {
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("tags")) {
        Json::Value error;
        error["error"] = "Tags object is required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    StringMap tags;
    const auto& tags_json = (*json)["tags"];
    for (const auto& key : tags_json.getMemberNames()) {
        tags[key] = tags_json[key].asString();
    }

    auto result = bucket_service->set_bucket_tags(user_info, bucket_name, tags);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    response["message"] = "Tags updated successfully";
    response["bucket"] = bucket_name;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
BucketsController::deleteTags(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                              const String& bucket_name) {
    auto user_info = get_user_from_request(req);
    auto bucket_service = ServiceLocator::bucket_service();

    if (!bucket_service) {
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    // Set empty tags to delete all
    auto result = bucket_service->set_bucket_tags(user_info, bucket_name, StringMap{});

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    response["message"] = "Tags deleted successfully";
    response["bucket"] = bucket_name;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
BucketsController::getEncryption(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                 const String& bucket_name) {
    auto user_info = get_user_from_request(req);
    auto bucket_service = ServiceLocator::bucket_service();

    if (!bucket_service) {
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = bucket_service->get_bucket_encryption(user_info, bucket_name);

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
BucketsController::setEncryption(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                 const String& bucket_name) {
    auto user_info = get_user_from_request(req);
    auto bucket_service = ServiceLocator::bucket_service();

    if (!bucket_service) {
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json) {
        Json::Value error;
        error["error"] = "Encryption configuration required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    auto result = bucket_service->set_bucket_encryption(user_info, bucket_name, *json);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    response["message"] = "Encryption updated successfully";
    response["bucket"] = bucket_name;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
BucketsController::deleteEncryption(const drogon::HttpRequestPtr& req,
                                    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                    const String& bucket_name) {
    auto user_info = get_user_from_request(req);
    auto bucket_service = ServiceLocator::bucket_service();

    if (!bucket_service) {
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    Json::Value empty_config;
    empty_config["enabled"] = false;
    auto result = bucket_service->set_bucket_encryption(user_info, bucket_name, empty_config);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    response["message"] = "Encryption removed successfully";
    response["bucket"] = bucket_name;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
BucketsController::getLifecycle(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                const String& bucket_name) {
    auto user_info = get_user_from_request(req);
    auto bucket_service = ServiceLocator::bucket_service();

    if (!bucket_service) {
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = bucket_service->get_bucket_lifecycle(user_info, bucket_name);

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
BucketsController::setLifecycle(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                const String& bucket_name) {
    auto user_info = get_user_from_request(req);
    auto bucket_service = ServiceLocator::bucket_service();

    if (!bucket_service) {
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json) {
        Json::Value error;
        error["error"] = "Lifecycle configuration required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    auto result = bucket_service->set_bucket_lifecycle(user_info, bucket_name, *json);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    response["message"] = "Lifecycle rules updated successfully";
    response["bucket"] = bucket_name;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
BucketsController::deleteLifecycle(const drogon::HttpRequestPtr& req,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                   const String& bucket_name) {
    auto user_info = get_user_from_request(req);
    auto bucket_service = ServiceLocator::bucket_service();

    if (!bucket_service) {
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    Json::Value empty_config;
    empty_config["rules"] = Json::Value(Json::arrayValue);
    auto result = bucket_service->set_bucket_lifecycle(user_info, bucket_name, empty_config);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    response["message"] = "Lifecycle rules removed successfully";
    response["bucket"] = bucket_name;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
BucketsController::getObjectLock(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                 const String& bucket_name) {
    auto user_info = get_user_from_request(req);
    auto bucket_service = ServiceLocator::bucket_service();

    if (!bucket_service) {
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = bucket_service->get_bucket_object_lock(user_info, bucket_name);

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
BucketsController::setObjectLock(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                 const String& bucket_name) {
    auto user_info = get_user_from_request(req);
    auto bucket_service = ServiceLocator::bucket_service();

    if (!bucket_service) {
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json) {
        Json::Value error;
        error["error"] = "Object lock configuration required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    auto result = bucket_service->set_bucket_object_lock(user_info, bucket_name, *json);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    response["message"] = "Object lock configuration updated successfully";
    response["bucket"] = bucket_name;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
BucketsController::getVisibility(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                 const String& bucket_name) {
    auto user_info = get_user_from_request(req);
    auto bucket_service = ServiceLocator::bucket_service();
    if (!bucket_service) {
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }
    auto result = bucket_service->get_bucket_visibility_groups(user_info, bucket_name);
    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }
    Json::Value response;
    response["bucket"] = bucket_name;
    response["groups"] = Json::Value(Json::arrayValue);
    for (const auto& group : result.value()) {
        response["groups"].append(group);
    }
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
BucketsController::setVisibility(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                 const String& bucket_name) {
    auto user_info = get_user_from_request(req);
    auto bucket_service = ServiceLocator::bucket_service();
    if (!bucket_service) {
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }
    auto json = req->getJsonObject();
    if (!json || !json->isMember("groups") || !(*json)["groups"].isArray()) {
        Json::Value error;
        error["error"] = "groups array is required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }
    Vector<String> groups;
    for (const auto& entry : (*json)["groups"]) {
        groups.push_back(entry.asString());
    }
    auto result = bucket_service->set_bucket_visibility_groups(user_info, bucket_name, groups);
    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }
    Json::Value response;
    response["message"] = "Bucket visibility updated";
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
