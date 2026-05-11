#include "console/api/DashboardController.hpp"

#include "console/common/Logger.hpp"
#include "console/common/ServiceLocator.hpp"
#include "console/storage/DatabaseManager.hpp"

#include <chrono>
#include <json/json.h>
#include <random>

namespace console::api {

UserInfo
DashboardController::get_user_from_request(const drogon::HttpRequestPtr& req) {
    try {
        return req->attributes()->get<UserInfo>("user_info");
    } catch (...) {
        CONSOLE_LOG_ERROR("No user_info found in request attributes");
        return UserInfo{};
    }
}

std::string
DashboardController::generate_uuid() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);

    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 8; i++)
        ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; i++)
        ss << dis(gen);
    ss << "-4";
    for (int i = 0; i < 3; i++)
        ss << dis(gen);
    ss << "-";
    ss << dis2(gen);
    for (int i = 0; i < 3; i++)
        ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 12; i++)
        ss << dis(gen);
    return ss.str();
}

// ============================================================================
// Board Handlers
// ============================================================================

void
DashboardController::list_boards(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto user_info = get_user_from_request(req);

    try {
        auto db_manager = ServiceLocator::database();
        if (!db_manager) {
            Json::Value error;
            error["error"] = "Database not available";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        auto boards_result = db_manager->list_dashboard_boards(user_info.access_key);
        if (!boards_result) {
            Json::Value error;
            error["error"] = boards_result.error();
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        Json::Value response(Json::arrayValue);
        for (const auto& board : boards_result.value()) {
            Json::Value board_json;
            board_json["id"] = board.id;
            board_json["name"] = board.name;
            board_json["order_index"] = board.order_index;
            board_json["created_at"] = Json::Int64(board.created_at);
            board_json["updated_at"] = Json::Int64(board.updated_at);
            response.append(board_json);
        }

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        callback(resp);

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Error listing dashboard boards: {}", e.what());
        Json::Value error;
        error["error"] = "Internal server error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

void
DashboardController::create_board(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto user_info = get_user_from_request(req);

    try {
        auto json = req->getJsonObject();
        if (!json) {
            Json::Value error;
            error["error"] = "Invalid JSON";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k400BadRequest);
            callback(resp);
            return;
        }

        auto db_manager = ServiceLocator::database();
        if (!db_manager) {
            Json::Value error;
            error["error"] = "Database not available";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        // Get current timestamp
        auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                       .count();

        // Create new board
        storage::DbDashboardBoard board;
        board.id = generate_uuid();
        board.user_id = user_info.access_key;
        board.name = (*json).get("name", "New Dashboard").asString();
        board.order_index = (*json).get("order_index", 0).asInt();
        board.created_at = now;
        board.updated_at = now;

        auto create_result = db_manager->create_dashboard_board(board);
        if (!create_result) {
            Json::Value error;
            error["error"] = create_result.error();
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        // Create default widgets for the new board
        std::vector<storage::DbDashboardWidget> default_widgets = {
            {generate_uuid(), board.id, "capacity", 0, 0, 4, 2, true, 30, "{}", 0},
            {generate_uuid(), board.id, "servers", 4, 0, 4, 2, true, 30, "{}", 1},
            {generate_uuid(), board.id, "drives", 8, 0, 4, 2, true, 30, "{}", 2},
            {generate_uuid(), board.id, "buckets", 0, 2, 4, 2, true, 30, "{}", 3},
            {generate_uuid(), board.id, "api_errors", 4, 2, 4, 2, true, 60, "{}", 4},
            {generate_uuid(), board.id, "throughput", 8, 2, 4, 2, true, 60, "{}", 5},
            {generate_uuid(), board.id, "encryption", 0, 4, 4, 2, true, 120, "{}", 6},
            {generate_uuid(), board.id, "quick_actions", 4, 4, 8, 1, true, 0, "{}", 7}};

        for (const auto& widget : default_widgets) {
            db_manager->create_dashboard_widget(widget);
        }

        // Return created board
        Json::Value response;
        response["id"] = board.id;
        response["name"] = board.name;
        response["order_index"] = board.order_index;
        response["created_at"] = Json::Int64(board.created_at);
        response["updated_at"] = Json::Int64(board.updated_at);

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k201Created);
        callback(resp);

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Error creating dashboard board: {}", e.what());
        Json::Value error;
        error["error"] = "Internal server error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

void
DashboardController::get_board(const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                               const std::string& id) {
    auto user_info = get_user_from_request(req);

    try {
        auto db_manager = ServiceLocator::database();
        if (!db_manager) {
            Json::Value error;
            error["error"] = "Database not available";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        auto board_result = db_manager->get_dashboard_board(id);
        if (!board_result) {
            Json::Value error;
            error["error"] = "Board not found";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k404NotFound);
            callback(resp);
            return;
        }

        const auto& board = board_result.value();

        // Check ownership
        if (board.user_id != user_info.access_key && !user_info.is_admin) {
            Json::Value error;
            error["error"] = "Access denied";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k403Forbidden);
            callback(resp);
            return;
        }

        Json::Value response;
        response["id"] = board.id;
        response["name"] = board.name;
        response["order_index"] = board.order_index;
        response["created_at"] = Json::Int64(board.created_at);
        response["updated_at"] = Json::Int64(board.updated_at);

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        callback(resp);

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Error getting dashboard board: {}", e.what());
        Json::Value error;
        error["error"] = "Internal server error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

void
DashboardController::update_board(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                  const std::string& id) {
    auto user_info = get_user_from_request(req);

    try {
        auto json = req->getJsonObject();
        if (!json) {
            Json::Value error;
            error["error"] = "Invalid JSON";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k400BadRequest);
            callback(resp);
            return;
        }

        auto db_manager = ServiceLocator::database();
        if (!db_manager) {
            Json::Value error;
            error["error"] = "Database not available";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        // Get existing board
        auto board_result = db_manager->get_dashboard_board(id);
        if (!board_result) {
            Json::Value error;
            error["error"] = "Board not found";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k404NotFound);
            callback(resp);
            return;
        }

        auto board = board_result.value();

        // Check ownership
        if (board.user_id != user_info.access_key && !user_info.is_admin) {
            Json::Value error;
            error["error"] = "Access denied";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k403Forbidden);
            callback(resp);
            return;
        }

        // Update fields
        if (json->isMember("name")) {
            board.name = (*json)["name"].asString();
        }
        if (json->isMember("order_index")) {
            board.order_index = (*json)["order_index"].asInt();
        }

        board.updated_at = std::chrono::duration_cast<std::chrono::seconds>(
                               std::chrono::system_clock::now().time_since_epoch())
                               .count();

        auto update_result = db_manager->update_dashboard_board(board);
        if (!update_result) {
            Json::Value error;
            error["error"] = update_result.error();
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        Json::Value response;
        response["id"] = board.id;
        response["name"] = board.name;
        response["order_index"] = board.order_index;
        response["created_at"] = Json::Int64(board.created_at);
        response["updated_at"] = Json::Int64(board.updated_at);

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        callback(resp);

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Error updating dashboard board: {}", e.what());
        Json::Value error;
        error["error"] = "Internal server error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

void
DashboardController::delete_board(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                  const std::string& id) {
    auto user_info = get_user_from_request(req);

    try {
        auto db_manager = ServiceLocator::database();
        if (!db_manager) {
            Json::Value error;
            error["error"] = "Database not available";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        // Get existing board to check ownership
        auto board_result = db_manager->get_dashboard_board(id);
        if (!board_result) {
            Json::Value error;
            error["error"] = "Board not found";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k404NotFound);
            callback(resp);
            return;
        }

        const auto& board = board_result.value();

        // Check ownership
        if (board.user_id != user_info.access_key && !user_info.is_admin) {
            Json::Value error;
            error["error"] = "Access denied";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k403Forbidden);
            callback(resp);
            return;
        }

        auto delete_result = db_manager->delete_dashboard_board(id);
        if (!delete_result) {
            Json::Value error;
            error["error"] = delete_result.error();
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k204NoContent);
        callback(resp);

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Error deleting dashboard board: {}", e.what());
        Json::Value error;
        error["error"] = "Internal server error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

// ============================================================================
// Widget Handlers
// ============================================================================

void
DashboardController::list_widgets(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                  const std::string& board_id) {
    auto user_info = get_user_from_request(req);

    try {
        auto db_manager = ServiceLocator::database();
        if (!db_manager) {
            Json::Value error;
            error["error"] = "Database not available";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        // Check board ownership
        auto board_result = db_manager->get_dashboard_board(board_id);
        if (!board_result) {
            Json::Value error;
            error["error"] = "Board not found";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k404NotFound);
            callback(resp);
            return;
        }

        const auto& board = board_result.value();
        if (board.user_id != user_info.access_key && !user_info.is_admin) {
            Json::Value error;
            error["error"] = "Access denied";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k403Forbidden);
            callback(resp);
            return;
        }

        auto widgets_result = db_manager->list_dashboard_widgets(board_id);
        if (!widgets_result) {
            Json::Value error;
            error["error"] = widgets_result.error();
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        Json::Value response(Json::arrayValue);
        for (const auto& widget : widgets_result.value()) {
            Json::Value widget_json;
            widget_json["id"] = widget.id;
            widget_json["board_id"] = widget.board_id;
            widget_json["widget_type"] = widget.widget_type;
            widget_json["position"]["x"] = widget.position_x;
            widget_json["position"]["y"] = widget.position_y;
            widget_json["size"]["w"] = widget.width;
            widget_json["size"]["h"] = widget.height;
            widget_json["visible"] = widget.visible;
            widget_json["refresh_interval"] = widget.refresh_interval;
            widget_json["order_index"] = widget.order_index;

            // Parse settings JSON
            Json::CharReaderBuilder builder;
            std::istringstream stream(widget.settings_json);
            std::string errs;
            Json::Value settings;
            if (Json::parseFromStream(builder, stream, &settings, &errs)) {
                widget_json["settings"] = settings;
            } else {
                widget_json["settings"] = Json::Value(Json::objectValue);
            }

            response.append(widget_json);
        }

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        callback(resp);

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Error listing dashboard widgets: {}", e.what());
        Json::Value error;
        error["error"] = "Internal server error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

void
DashboardController::save_widgets(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                  const std::string& board_id) {
    auto user_info = get_user_from_request(req);

    try {
        auto json = req->getJsonObject();
        if (!json || !json->isArray()) {
            Json::Value error;
            error["error"] = "Invalid JSON - expected array of widgets";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k400BadRequest);
            callback(resp);
            return;
        }

        auto db_manager = ServiceLocator::database();
        if (!db_manager) {
            Json::Value error;
            error["error"] = "Database not available";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        // Check board ownership
        auto board_result = db_manager->get_dashboard_board(board_id);
        if (!board_result) {
            Json::Value error;
            error["error"] = "Board not found";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k404NotFound);
            callback(resp);
            return;
        }

        const auto& board = board_result.value();
        if (board.user_id != user_info.access_key && !user_info.is_admin) {
            Json::Value error;
            error["error"] = "Access denied";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k403Forbidden);
            callback(resp);
            return;
        }

        // Parse widgets from JSON
        std::vector<storage::DbDashboardWidget> widgets;
        for (const auto& widget_json : *json) {
            storage::DbDashboardWidget widget;
            widget.id = widget_json.get("id", generate_uuid()).asString();
            widget.board_id = board_id;
            widget.widget_type = widget_json.get("widget_type", "").asString();

            if (widget_json.isMember("position")) {
                widget.position_x = widget_json["position"].get("x", 0).asInt();
                widget.position_y = widget_json["position"].get("y", 0).asInt();
            }

            if (widget_json.isMember("size")) {
                widget.width = widget_json["size"].get("w", 4).asInt();
                widget.height = widget_json["size"].get("h", 2).asInt();
            }

            widget.visible = widget_json.get("visible", true).asBool();
            widget.refresh_interval = widget_json.get("refresh_interval", 30).asInt();
            widget.order_index = widget_json.get("order_index", 0).asInt();

            // Serialize settings back to JSON string
            if (widget_json.isMember("settings")) {
                Json::StreamWriterBuilder builder;
                builder["indentation"] = "";
                widget.settings_json = Json::writeString(builder, widget_json["settings"]);
            } else {
                widget.settings_json = "{}";
            }

            widgets.push_back(widget);
        }

        auto save_result = db_manager->save_dashboard_widgets(board_id, widgets);
        if (!save_result) {
            Json::Value error;
            error["error"] = save_result.error();
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        Json::Value response;
        response["message"] = "Widgets saved successfully";
        response["count"] = static_cast<int>(widgets.size());

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        callback(resp);

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Error saving dashboard widgets: {}", e.what());
        Json::Value error;
        error["error"] = "Internal server error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

void
DashboardController::update_widget(const drogon::HttpRequestPtr& req,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                   const std::string& id) {
    auto user_info = get_user_from_request(req);

    try {
        auto json = req->getJsonObject();
        if (!json) {
            Json::Value error;
            error["error"] = "Invalid JSON";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k400BadRequest);
            callback(resp);
            return;
        }

        auto db_manager = ServiceLocator::database();
        if (!db_manager) {
            Json::Value error;
            error["error"] = "Database not available";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        // Get existing widget
        auto widget_result = db_manager->get_dashboard_widget(id);
        if (!widget_result) {
            Json::Value error;
            error["error"] = "Widget not found";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k404NotFound);
            callback(resp);
            return;
        }

        auto widget = widget_result.value();

        // Check board ownership
        auto board_result = db_manager->get_dashboard_board(widget.board_id);
        if (!board_result) {
            Json::Value error;
            error["error"] = "Board not found";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k404NotFound);
            callback(resp);
            return;
        }

        const auto& board = board_result.value();
        if (board.user_id != user_info.access_key && !user_info.is_admin) {
            Json::Value error;
            error["error"] = "Access denied";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k403Forbidden);
            callback(resp);
            return;
        }

        // Update fields
        if (json->isMember("position")) {
            widget.position_x = (*json)["position"].get("x", widget.position_x).asInt();
            widget.position_y = (*json)["position"].get("y", widget.position_y).asInt();
        }
        if (json->isMember("size")) {
            widget.width = (*json)["size"].get("w", widget.width).asInt();
            widget.height = (*json)["size"].get("h", widget.height).asInt();
        }
        if (json->isMember("visible")) {
            widget.visible = (*json)["visible"].asBool();
        }
        if (json->isMember("refresh_interval")) {
            widget.refresh_interval = (*json)["refresh_interval"].asInt();
        }
        if (json->isMember("order_index")) {
            widget.order_index = (*json)["order_index"].asInt();
        }
        if (json->isMember("settings")) {
            Json::StreamWriterBuilder builder;
            builder["indentation"] = "";
            widget.settings_json = Json::writeString(builder, (*json)["settings"]);
        }

        auto update_result = db_manager->update_dashboard_widget(widget);
        if (!update_result) {
            Json::Value error;
            error["error"] = update_result.error();
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        Json::Value response;
        response["id"] = widget.id;
        response["board_id"] = widget.board_id;
        response["widget_type"] = widget.widget_type;
        response["position"]["x"] = widget.position_x;
        response["position"]["y"] = widget.position_y;
        response["size"]["w"] = widget.width;
        response["size"]["h"] = widget.height;
        response["visible"] = widget.visible;
        response["refresh_interval"] = widget.refresh_interval;
        response["order_index"] = widget.order_index;

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        callback(resp);

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Error updating dashboard widget: {}", e.what());
        Json::Value error;
        error["error"] = "Internal server error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

} // namespace console::api
