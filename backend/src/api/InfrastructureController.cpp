#include "console/api/InfrastructureController.hpp"

#include "console/common/Logger.hpp"
#include "console/common/ServiceLocator.hpp"
#include "console/services/InfrastructureService.hpp"

#include <json/json.h>

namespace console::api {

UserInfo
InfrastructureController::get_user_from_request(const drogon::HttpRequestPtr& req) {
    try {
        return req->attributes()->get<UserInfo>("user_info");
    } catch (...) {
        CONSOLE_LOG_ERROR("No user_info found in request attributes");
        return UserInfo{};
    }
}

// ============================================================================
// Summary
// ============================================================================

void
InfrastructureController::get_summary(const drogon::HttpRequestPtr& req,
                                      std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto user_info = get_user_from_request(req);
    auto infra_service = ServiceLocator::infrastructure_service();

    if (!infra_service) {
        Json::Value error;
        error["error"] = "Infrastructure service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = infra_service->get_summary(user_info);

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

// ============================================================================
// Server Handlers
// ============================================================================

void
InfrastructureController::list_servers(const drogon::HttpRequestPtr& req,
                                       std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto user_info = get_user_from_request(req);
    auto infra_service = ServiceLocator::infrastructure_service();

    if (!infra_service) {
        Json::Value error;
        error["error"] = "Infrastructure service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = infra_service->list_servers(user_info);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    Json::Value servers(Json::arrayValue);
    int online = 0, offline = 0;

    for (const auto& server : result.value()) {
        servers.append(server.to_json());
        if (server.status == "online")
            online++;
        else
            offline++;
    }

    response["servers"] = servers;
    response["total"] = static_cast<int>(result.value().size());
    response["online"] = online;
    response["offline"] = offline;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
InfrastructureController::get_server(const drogon::HttpRequestPtr& req,
                                     std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                     const String& id) {
    auto user_info = get_user_from_request(req);
    auto infra_service = ServiceLocator::infrastructure_service();

    if (!infra_service) {
        Json::Value error;
        error["error"] = "Infrastructure service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = infra_service->get_server(user_info, id);

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
InfrastructureController::add_server(const drogon::HttpRequestPtr& req,
                                     std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto user_info = get_user_from_request(req);
    auto infra_service = ServiceLocator::infrastructure_service();

    if (!infra_service) {
        Json::Value error;
        error["error"] = "Infrastructure service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json) {
        Json::Value error;
        error["error"] = "Invalid JSON request body";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    services::AddServerRequest request;
    request.name = (*json).get("name", "").asString();
    request.endpoint = (*json).get("endpoint", "").asString();
    request.region = (*json).get("region", "").asString();

    if (json->isMember("drive_paths") && (*json)["drive_paths"].isArray()) {
        for (const auto& path : (*json)["drive_paths"]) {
            request.drive_paths.push_back(path.asString());
        }
    }

    auto result = infra_service->add_server(user_info, request);

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
InfrastructureController::remove_server(const drogon::HttpRequestPtr& req,
                                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                        const String& id) {
    auto user_info = get_user_from_request(req);
    auto infra_service = ServiceLocator::infrastructure_service();

    if (!infra_service) {
        Json::Value error;
        error["error"] = "Infrastructure service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = infra_service->remove_server(user_info, id);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    response["message"] = "Server removed successfully";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
InfrastructureController::check_server_health(const drogon::HttpRequestPtr& req,
                                              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                              const String& id) {
    auto user_info = get_user_from_request(req);
    auto infra_service = ServiceLocator::infrastructure_service();

    if (!infra_service) {
        Json::Value error;
        error["error"] = "Infrastructure service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = infra_service->check_server_health(user_info, id);

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

// ============================================================================
// Drive Handlers
// ============================================================================

void
InfrastructureController::list_drives(const drogon::HttpRequestPtr& req,
                                      std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto user_info = get_user_from_request(req);
    auto infra_service = ServiceLocator::infrastructure_service();

    if (!infra_service) {
        Json::Value error;
        error["error"] = "Infrastructure service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    String server_id = req->getParameter("server_id");
    auto result = infra_service->list_drives(user_info, server_id);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    Json::Value drives(Json::arrayValue);
    int online = 0, offline = 0;

    for (const auto& drive : result.value()) {
        drives.append(drive.to_json());
        if (drive.status == "online")
            online++;
        else
            offline++;
    }

    response["drives"] = drives;
    response["total"] = static_cast<int>(result.value().size());
    response["online"] = online;
    response["offline"] = offline;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
InfrastructureController::get_drive(const drogon::HttpRequestPtr& req,
                                    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                    const String& id) {
    auto user_info = get_user_from_request(req);
    auto infra_service = ServiceLocator::infrastructure_service();

    if (!infra_service) {
        Json::Value error;
        error["error"] = "Infrastructure service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = infra_service->get_drive(user_info, id);

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
InfrastructureController::add_drive(const drogon::HttpRequestPtr& req,
                                    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto user_info = get_user_from_request(req);
    auto infra_service = ServiceLocator::infrastructure_service();

    if (!infra_service) {
        Json::Value error;
        error["error"] = "Infrastructure service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json) {
        Json::Value error;
        error["error"] = "Invalid JSON request body";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    services::AddDriveRequest request;
    request.server_id = (*json).get("server_id", "").asString();
    request.path = (*json).get("path", "").asString();
    request.capacity = (*json).get("capacity", 0).asInt64();

    auto result = infra_service->add_drive(user_info, request);

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
InfrastructureController::remove_drive(const drogon::HttpRequestPtr& req,
                                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                       const String& id) {
    auto user_info = get_user_from_request(req);
    auto infra_service = ServiceLocator::infrastructure_service();

    if (!infra_service) {
        Json::Value error;
        error["error"] = "Infrastructure service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = infra_service->remove_drive(user_info, id);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    response["message"] = "Drive removed successfully";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
InfrastructureController::set_drive_status(const drogon::HttpRequestPtr& req,
                                           std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                           const String& id) {
    auto user_info = get_user_from_request(req);
    auto infra_service = ServiceLocator::infrastructure_service();

    if (!infra_service) {
        Json::Value error;
        error["error"] = "Infrastructure service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("status")) {
        Json::Value error;
        error["error"] = "Status is required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    String status = (*json)["status"].asString();
    auto result = infra_service->set_drive_status(user_info, id, status);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    response["message"] = "Drive status updated successfully";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

// ============================================================================
// Pool Handlers
// ============================================================================

void
InfrastructureController::list_pools(const drogon::HttpRequestPtr& req,
                                     std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto user_info = get_user_from_request(req);
    auto infra_service = ServiceLocator::infrastructure_service();

    if (!infra_service) {
        Json::Value error;
        error["error"] = "Infrastructure service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = infra_service->list_pools(user_info);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    Json::Value pools(Json::arrayValue);

    for (const auto& pool : result.value()) {
        pools.append(pool.to_json());
    }

    response["pools"] = pools;
    response["total"] = static_cast<int>(result.value().size());

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void
InfrastructureController::get_pool(const drogon::HttpRequestPtr& req,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                   const String& id) {
    auto user_info = get_user_from_request(req);
    auto infra_service = ServiceLocator::infrastructure_service();

    if (!infra_service) {
        Json::Value error;
        error["error"] = "Infrastructure service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = infra_service->get_pool(user_info, id);

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
InfrastructureController::configure_pool(const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto user_info = get_user_from_request(req);
    auto infra_service = ServiceLocator::infrastructure_service();

    if (!infra_service) {
        Json::Value error;
        error["error"] = "Infrastructure service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json) {
        Json::Value error;
        error["error"] = "Invalid JSON request body";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    services::ConfigurePoolRequest request;
    request.name = (*json).get("name", "").asString();
    request.erasure_data_shards = (*json).get("erasure_data_shards", 8).asInt();
    request.erasure_parity_shards = (*json).get("erasure_parity_shards", 4).asInt();

    if (json->isMember("server_ids") && (*json)["server_ids"].isArray()) {
        for (const auto& sid : (*json)["server_ids"]) {
            request.server_ids.push_back(sid.asString());
        }
    }

    auto result = infra_service->configure_pool(user_info, request);

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
InfrastructureController::decommission_pool(const drogon::HttpRequestPtr& req,
                                            std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                            const String& id) {
    auto user_info = get_user_from_request(req);
    auto infra_service = ServiceLocator::infrastructure_service();

    if (!infra_service) {
        Json::Value error;
        error["error"] = "Infrastructure service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = infra_service->decommission_pool(user_info, id);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    response["message"] = "Pool decommissioned successfully";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

// ============================================================================
// Healing Handlers
// ============================================================================

void
InfrastructureController::get_heal_status(const drogon::HttpRequestPtr& req,
                                          std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto user_info = get_user_from_request(req);
    auto infra_service = ServiceLocator::infrastructure_service();

    if (!infra_service) {
        Json::Value error;
        error["error"] = "Infrastructure service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = infra_service->get_heal_status(user_info);

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
InfrastructureController::start_heal(const drogon::HttpRequestPtr& req,
                                     std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto user_info = get_user_from_request(req);
    auto infra_service = ServiceLocator::infrastructure_service();

    if (!infra_service) {
        Json::Value error;
        error["error"] = "Infrastructure service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    String pool_id;
    auto json = req->getJsonObject();
    if (json && json->isMember("pool_id")) {
        pool_id = (*json)["pool_id"].asString();
    }

    auto result = infra_service->start_heal(user_info, pool_id);

    if (!result) {
        auto error_json = result.error().to_json();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.error().status()));
        callback(resp);
        return;
    }

    Json::Value response;
    response["message"] = "Heal process started";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

} // namespace console::api
