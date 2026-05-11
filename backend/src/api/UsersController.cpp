#include "console/api/UsersController.hpp"

#include "console/common/Logger.hpp"
#include "console/common/ServiceLocator.hpp"
#include "console/services/UserService.hpp"
#include "console/storage/DatabaseManager.hpp"

#include <ctime>
#include <json/json.h>
#include <sstream>

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

    // Support both swagger format (username/password) and legacy format (access_key/secret_key)
    String username = request_body.get("username", "").asString();
    String password = request_body.get("password", "").asString();

    // Fallback to legacy format if username/password not provided
    if (username.empty()) {
        username = request_body.get("access_key", "").asString();
    }
    if (password.empty()) {
        password = request_body.get("secret_key", "").asString();
    }

    bool is_admin = request_body.get("is_admin", false).asBool();

    if (username.empty() || password.empty()) {
        Json::Value error;
        error["error"] = "username and password are required";
        error["code"] = 400;
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

    auto result = ServiceLocator::user_service()->create_user(admin_info, username, password, policies, is_admin);

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

    // Support both 'password' (swagger) and 'secret_key' (legacy) field names
    Optional<String> new_secret_key;
    if (request_body.isMember("password")) {
        new_secret_key = request_body["password"].asString();
    } else if (request_body.isMember("secret_key")) {
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

    // Return updated user info per swagger.json (GET user response)
    auto user_result = ServiceLocator::user_service()->get_user(admin_info, access_key);
    if (user_result) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(user_result.value().to_json());
        resp->setStatusCode(drogon::k200OK);
        callback(resp);
    } else {
        // Fallback if user fetch fails
        Json::Value response;
        response["message"] = "User updated successfully";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k200OK);
        callback(resp);
    }
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

    // Return 200 with message per swagger.json
    Json::Value response;
    response["message"] = "User deleted successfully";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
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

    // Format per swagger.json: { "policies": [{ "name": "...", "policy": "..." }, ...] }
    Json::Value response;
    response["policies"] = Json::Value(Json::arrayValue);

    auto db = ServiceLocator::database();
    for (const auto& policy_name : result.value()) {
        Json::Value policy_obj;
        policy_obj["name"] = policy_name;

        // Fetch actual policy document from database
        if (db) {
            auto policy_result = db->get_policy(policy_name);
            if (policy_result) {
                policy_obj["policy"] = policy_result.value().document;
            } else {
                // Fallback if policy document not found
                policy_obj["policy"] = "{\"Version\":\"2012-10-17\",\"Statement\":[]}";
            }
        } else {
            policy_obj["policy"] = "{\"Version\":\"2012-10-17\",\"Statement\":[]}";
        }
        response["policies"].append(policy_obj);
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

    // Per swagger.json, body should contain { "policy": "<JSON document>" }
    auto json = req->getJsonObject();
    if (!json || !json->isMember("policy")) {
        Json::Value error;
        error["error"] = "'policy' JSON document is required in request body";
        error["code"] = 400;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    String policy_document = (*json)["policy"].asString();

    // Validate that policy_document is valid JSON
    Json::CharReaderBuilder builder;
    Json::Value policy_json;
    std::istringstream stream(policy_document);
    std::string parse_errors;
    if (!Json::parseFromStream(builder, stream, &policy_json, &parse_errors)) {
        Json::Value error;
        error["error"] = "Invalid policy JSON document: " + parse_errors;
        error["code"] = 400;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    // Create or update the policy document in database
    auto db = ServiceLocator::database();
    if (db) {
        auto policy_exists = db->policy_exists(policy_name);
        storage::DbPolicy db_policy;
        db_policy.name = policy_name;
        db_policy.version = policy_json.get("Version", "2012-10-17").asString();
        db_policy.document = policy_document;
        db_policy.description = "User policy for " + access_key;
        db_policy.updated_at = std::time(nullptr);

        if (policy_exists && policy_exists.value()) {
            // Update existing policy
            auto update_result = db->update_policy(db_policy);
            if (!update_result) {
                CONSOLE_LOG_WARN("Failed to update policy {}: {}", policy_name, update_result.error());
            } else {
                CONSOLE_LOG_INFO("Policy {} updated with new document", policy_name);
            }
        } else {
            // Create new policy
            db_policy.created_at = std::time(nullptr);
            auto create_result = db->create_policy(db_policy);
            if (!create_result) {
                CONSOLE_LOG_WARN("Failed to create policy {}: {}", policy_name, create_result.error());
            } else {
                CONSOLE_LOG_INFO("Policy {} created", policy_name);
            }
        }
    }

    // Attach the policy to user
    auto result = ServiceLocator::user_service()->attach_user_policy(admin_info, access_key, policy_name);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    // Return 200 with message per swagger.json
    Json::Value response;
    response["message"] = "Policy assigned successfully";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
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

    // Return 200 with message per swagger.json
    Json::Value response;
    response["message"] = "Policy removed successfully";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
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

    // Return 200 with message per swagger.json
    Json::Value response;
    response["message"] = "User added to group successfully";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
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

    // Return 200 with message per swagger.json
    Json::Value response;
    response["message"] = "User removed from group successfully";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

UserInfo
UsersController::get_user_from_request(const drogon::HttpRequestPtr& req) {
    CONSOLE_LOG_DEBUG("get_user_from_request: checking attributes for user_info at path: {}", req->getPath());

    auto attrs = req->attributes();
    if (!attrs) {
        CONSOLE_LOG_ERROR("get_user_from_request: attributes is nullptr!");
        return UserInfo{};
    }

    try {
        auto user_info = attrs->get<UserInfo>("user_info");
        CONSOLE_LOG_INFO("get_user_from_request: SUCCESS - extracted user '{}', access_key='{}', is_admin={}",
                         user_info.account_name,
                         user_info.access_key,
                         user_info.is_admin);
        return user_info;
    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("get_user_from_request: No user_info in attributes: {} (path: {})", e.what(), req->getPath());
        return UserInfo{};
    } catch (...) {
        CONSOLE_LOG_ERROR("No user_info found in request attributes: unknown exception");
        return UserInfo{};
    }
}

} // namespace console::api
