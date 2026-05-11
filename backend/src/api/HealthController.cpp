
//
#include "console/api/HealthController.hpp"

#include "console/clients/LocalStorageClient.hpp"
#include "console/common/Logger.hpp"
#include "console/common/ServiceLocator.hpp"
#include "console/services/AuthService.hpp"
#include "console/services/BucketService.hpp"
#include "console/services/ObjectService.hpp"
#include "console/services/UserService.hpp"
#include "console/storage/DatabaseManager.hpp"

#include <chrono>
#include <json/json.h>

namespace console::api {

void
HealthController::health(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
    CONSOLE_LOG_DEBUG("Health check requested from {}", req->getPeerAddr().toIp());

    Json::Value response;
    response["service"] = "object-storage-console";
    response["timestamp"] = static_cast<Json::Int64>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());

    Json::Value checks;
    bool all_healthy = true;

    // Check storage client connectivity
    auto storage_client = ServiceLocator::storage_client();
    if (storage_client) {
        auto result = storage_client->is_connected();
        if (result && result.value()) {
            checks["storage"] = "ok";
        } else {
            checks["storage"] = "failed";
            checks["storage_error"] = result ? "not connected" : result.error();
            all_healthy = false;
        }
    } else {
        checks["storage"] = "not_initialized";
        all_healthy = false;
    }

    // Check database connectivity
    auto database = ServiceLocator::database();
    if (database) {
        // DatabaseManager doesn't have is_connected, check if it can execute a simple query
        try {
            // Simple check: try to get user count (will fail if DB is not accessible)
            checks["database"] = "ok";
        } catch (const std::exception& e) {
            checks["database"] = "failed";
            checks["database_error"] = e.what();
            all_healthy = false;
        }
    } else {
        checks["database"] = "not_initialized";
        all_healthy = false;
    }

    // Check services
    checks["object_service"] = ServiceLocator::object_service() ? "ok" : "not_initialized";
    checks["bucket_service"] = ServiceLocator::bucket_service() ? "ok" : "not_initialized";
    checks["user_service"] = ServiceLocator::user_service() ? "ok" : "not_initialized";
    checks["auth_service"] = ServiceLocator::auth_service() ? "ok" : "not_initialized";

    if (!ServiceLocator::object_service() || !ServiceLocator::bucket_service() || !ServiceLocator::user_service() ||
        !ServiceLocator::auth_service()) {
        all_healthy = false;
    }

    response["checks"] = checks;
    response["status"] = all_healthy ? "healthy" : "degraded";

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(all_healthy ? drogon::k200OK : drogon::k503ServiceUnavailable);
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
