// Copyright 2025 OpenMaxIO Contributors
// Licensed under AGPL-3.0
//
#include "console/api/HealthController.hpp"

#include "console/common/Logger.hpp"

#include <chrono>
#include <json/json.h>

namespace console::api {

void
HealthController::health(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
    LOG_DEBUG("Health check requested from {}", req->getPeerAddr().toIp());

    Json::Value response;
    response["status"] = "healthy";
    response["service"] = "object-storage-console";
    response["timestamp"] = static_cast<Json::Int64>(std::chrono::system_clock::now().time_since_epoch().count());

    // TODO(Nice0Man): Add checks for S3 connectivity, database, etc.
    Json::Value checks;
    checks["s3"] = "ok";
    checks["storage"] = "ok";
    response["checks"] = checks;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

void
HealthController::ready(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
    Json::Value response;
    response["ready"] = true;
    response["service"] = "object-storage-console";

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

void
HealthController::live(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
    Json::Value response;
    response["alive"] = true;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

void
HealthController::version(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
    Json::Value response;
    response["version"] = "1.0.0";
    response["api_version"] = "v1";
    response["build_date"] = __DATE__;
    response["build_time"] = __TIME__;
    response["compiler"] =
#ifdef __clang__
        "Clang " __clang_version__;
#elif defined(__GNUC__)
        "GCC " __VERSION__;
#elif defined(_MSC_VER)
        "MSVC " + std::to_string(_MSC_VER);
#else
        "Unknown";
#endif

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

} // namespace console::api
