#include "console/api/UsersController.hpp"

#include "console/common/Logger.hpp"
#include "console/common/ServiceLocator.hpp"
#include "console/services/UserService.hpp"

#include <json/json.h>

namespace console::api {

void
UsersController::set_user_service(std::shared_ptr<services::IUserService> service) {
    user_service_ = service;
}

void
UsersController::list(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto admin_info = get_user_from_request(req);
    auto user_service = ServiceLocator::user_service();

    if (!user_service) {
        CONSOLE_LOG_ERROR("UserService not initialized");
        Json::Value error;
        error["error"] = "Service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = user_service->list_users(admin_info);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    response["users"] = Json::Value(Json::arrayValue);

    for (const auto& user : result.value()) {
        response["users"].append(user.to_json());
    }

    response["total"] = static_cast<int>(result.value().size());

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
UsersController::get(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                     const String& access_key) {
    auto admin_info = get_user_from_request(req);

    auto result = ServiceLocator::user_service()->get_user(admin_info, access_key);

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
UsersController::create(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto admin_info = get_user_from_request(req);

    Json::Value request_body = *req->getJsonObject();

    String access_key = request_body.get("access_key", "").asString();
    String secret_key = request_body.get("secret_key", "").asString();

    if (access_key.empty() || secret_key.empty()) {
        Json::Value error;
        error["code"] = 400;
        error["message"] = "access_key and secret_key are required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    Vector<String> policies;
    if (request_body.isMember("policies") && request_body["policies"].isArray()) {
        for (const auto& policy_json : request_body["policies"]) {
            policies.push_back(policy_json.asString());
        }
    }

    auto result = ServiceLocator::user_service()->create_user(admin_info, access_key, secret_key, policies);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(result.value().to_json());
    resp->setStatusCode(drogon::k201Created);
    callback(resp);
}

void
UsersController::update(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                        const String& access_key) {
    auto admin_info = get_user_from_request(req);

    Json::Value request_body = *req->getJsonObject();

    Optional<String> new_secret_key;
    if (request_body.isMember("secret_key")) {
        new_secret_key = request_body["secret_key"].asString();
    }

    Optional<Vector<String>> policies;
    if (request_body.isMember("policies") && request_body["policies"].isArray()) {
        Vector<String> policy_list;
        for (const auto& policy_json : request_body["policies"]) {
            policy_list.push_back(policy_json.asString());
        }
        policies = policy_list;
    }

    auto result = ServiceLocator::user_service()->update_user(admin_info, access_key, new_secret_key, policies);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k204NoContent);
    callback(resp);
}

void
UsersController::remove(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                        const String& access_key) {
    auto admin_info = get_user_from_request(req);

    auto result = ServiceLocator::user_service()->delete_user(admin_info, access_key);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k204NoContent);
    callback(resp);
}

void
UsersController::list_policies(const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                               const String& access_key) {
    auto admin_info = get_user_from_request(req);

    auto result = ServiceLocator::user_service()->list_user_policies(admin_info, access_key);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    response["policies"] = Json::Value(Json::arrayValue);

    for (const auto& policy : result.value()) {
        response["policies"].append(policy);
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
UsersController::attach_policy(const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                               const String& access_key,
                               const String& policy_name) {
    auto admin_info = get_user_from_request(req);

    auto result = ServiceLocator::user_service()->attach_user_policy(admin_info, access_key, policy_name);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k204NoContent);
    callback(resp);
}

void
UsersController::detach_policy(const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                               const String& access_key,
                               const String& policy_name) {
    auto admin_info = get_user_from_request(req);

    auto result = ServiceLocator::user_service()->detach_user_policy(admin_info, access_key, policy_name);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k204NoContent);
    callback(resp);
}

void
UsersController::add_to_group(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                              const String& access_key,
                              const String& group_name) {
    auto admin_info = get_user_from_request(req);

    auto result = ServiceLocator::user_service()->add_user_to_group(admin_info, access_key, group_name);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k204NoContent);
    callback(resp);
}

void
UsersController::remove_from_group(const drogon::HttpRequestPtr& req,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                   const String& access_key,
                                   const String& group_name) {
    auto admin_info = get_user_from_request(req);

    auto result = ServiceLocator::user_service()->remove_user_from_group(admin_info, access_key, group_name);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k204NoContent);
    callback(resp);
}

UserInfo
UsersController::get_user_from_request(const drogon::HttpRequestPtr& req) {
    try {
        auto user_info = req->attributes()->get<UserInfo>("user_info");
        CONSOLE_LOG_INFO("get_user_from_request: SUCCESS - extracted user '{}', access_key='{}', is_admin={}",
                         user_info.account_name,
                         user_info.access_key,
                         user_info.is_admin);
        return user_info;
    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("No user_info found in request attributes: {}", e.what());
        return UserInfo{};
    } catch (...) {
        CONSOLE_LOG_ERROR("No user_info found in request attributes: unknown exception");
        return UserInfo{};
    }
}

} // namespace console::api
