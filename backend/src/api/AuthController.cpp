
#include "console/api/AuthController.hpp"

#include "console/common/Logger.hpp"
#include "console/common/ServiceLocator.hpp"
#include "console/services/AuthService.hpp"
#include "console/utils/JWT.hpp"

#include <json/json.h>

namespace console::api {

String
AuthController::extract_bearer_token(const drogon::HttpRequestPtr& req) {
    const auto auth_header = req->getHeader("Authorization");
    if (auth_header.empty() || auth_header.rfind("Bearer ", 0) != 0) {
        return {};
    }
    return auth_header.substr(7);
}

UserInfo
AuthController::get_user_from_request(const drogon::HttpRequestPtr& req) {
    try {
        return req->attributes()->get<UserInfo>("user_info");
    } catch (...) {
        return {};
    }
}

void
AuthController::login(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto json = req->getJsonObject();
    if (!json) {
        Json::Value error;
        error["error"] = "Invalid JSON";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    String username = (*json).get("username", "").asString();
    String password = (*json).get("password", "").asString();
    if (username.empty()) {
        username = (*json).get("accessKey", "").asString();
    }
    if (password.empty()) {
        password = (*json).get("secretKey", "").asString();
    }

    if (username.empty() || password.empty()) {
        Json::Value error;
        error["error"] = "Username and password (or accessKey and secretKey) are required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    auto auth_service = ServiceLocator::auth_service();
    if (!auth_service) {
        Json::Value error;
        error["error"] = "Authentication service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = auth_service->login(username, password);
    if (!result) {
        Json::Value error;
        error["error"] = result.error().message();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().http_status()));
        callback(resp);
        return;
    }

    const auto& login_response = result.value();
    Json::Value response;
    response["token"] = login_response.get("access_token", "").asString();
    response["refresh_token"] = login_response.get("refresh_token", "").asString();
    response["expires_in"] = login_response.get("expires_in", 0).asInt64();
    if (login_response.isMember("user")) {
        response["user"] = login_response["user"];
        if (response["user"].isMember("account_name")) {
            response["user"]["username"] = response["user"]["account_name"].asString();
        }
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

void
AuthController::logout(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    const String token = extract_bearer_token(req);

    auto auth_service = ServiceLocator::auth_service();
    if (auth_service && !token.empty()) {
        auth_service->logout(token);
    }

    Json::Value response;
    response["message"] = "Logged out successfully";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

void
AuthController::refresh(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    const String token = extract_bearer_token(req);
    if (token.empty()) {
        Json::Value error;
        error["error"] = "Missing or invalid Authorization header";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
        return;
    }

    auto auth_service = ServiceLocator::auth_service();
    if (!auth_service) {
        Json::Value error;
        error["error"] = "Authentication service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = auth_service->refresh_token(token);
    if (!result) {
        Json::Value error;
        error["error"] = result.error().message();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().http_status()));
        callback(resp);
        return;
    }

    const auto& refresh_response = result.value();
    Json::Value response;
    response["token"] = refresh_response.get("access_token", "").asString();
    response["refresh_token"] = refresh_response.get("refresh_token", "").asString();
    response["expires_in"] = refresh_response.get("expires_in", 0).asInt64();
    if (refresh_response.isMember("user")) {
        response["user"] = refresh_response["user"];
        if (response["user"].isMember("account_name")) {
            response["user"]["username"] = response["user"]["account_name"].asString();
        }
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

void
AuthController::me(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    const auto user = get_user_from_request(req);
    if (user.access_key.empty()) {
        Json::Value error;
        error["error"] = "Unauthenticated";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
        return;
    }

    Json::Value response;
    response["username"] = user.account_name;
    response["access_key"] = user.access_key;
    response["is_admin"] = user.is_admin;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
AuthController::session(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    const String token = extract_bearer_token(req);
    if (token.empty()) {
        Json::Value error;
        error["error"] = "Missing or invalid Authorization header";
        error["authenticated"] = false;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
        return;
    }

    auto auth_service = ServiceLocator::auth_service();
    if (!auth_service) {
        Json::Value error;
        error["error"] = "Authentication service not available";
        error["authenticated"] = false;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto validate_result = auth_service->validate_token(token);
    if (!validate_result) {
        Json::Value error;
        error["error"] = "Unauthenticated";
        error["authenticated"] = false;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
        return;
    }

    const auto& user = validate_result.value();
    auto claims_result = utils::JWT::decode_token_unsafe(token);
    auto expires_at = claims_result.is_ok() ? claims_result.value().expires_at
                                            : (std::chrono::system_clock::now() + std::chrono::hours(24));

    Json::Value response;
    response["authenticated"] = true;
    response["username"] = user.account_name;
    response["access_key"] = user.access_key;
    response["is_admin"] = user.is_admin;
    response["expires_at"] = format_iso8601_date(expires_at);

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

void
AuthController::change_password(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    const String token = extract_bearer_token(req);
    if (token.empty()) {
        Json::Value error;
        error["error"] = "Missing or invalid Authorization header";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json) {
        Json::Value error;
        error["error"] = "Invalid JSON";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    String old_password = (*json).get("old_password", "").asString();
    String new_password = (*json).get("new_password", "").asString();
    if (old_password.empty() || new_password.empty()) {
        Json::Value error;
        error["error"] = "Old password and new password are required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    auto auth_service = ServiceLocator::auth_service();
    if (!auth_service) {
        Json::Value error;
        error["error"] = "Authentication service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = auth_service->change_password(token, old_password, new_password);
    if (!result) {
        Json::Value error;
        error["error"] = result.error().message();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().http_status()));
        callback(resp);
        return;
    }

    Json::Value response;
    response["message"] = "Password changed successfully";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

} // namespace console::api
