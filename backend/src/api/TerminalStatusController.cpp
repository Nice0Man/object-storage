#include "console/api/TerminalStatusController.hpp"

#include "console/common/Config.hpp"
#include "console/common/Types.hpp"

#include <json/json.h>

namespace console::api {

namespace {

UserInfo
get_user_from_request(const drogon::HttpRequestPtr& req) {
    try {
        return req->attributes()->get<UserInfo>("user_info");
    } catch (...) {
        return UserInfo{};
    }
}

} // namespace

void
TerminalStatusController::status(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
    const auto user_info = get_user_from_request(req);
    const auto& terminal_cfg = Config::instance().terminal();
    const bool is_admin = user_info.is_admin || user_info.role == "admin";

    Json::Value response;
    response["enabled"] = terminal_cfg.enabled;
    response["allowed"] = terminal_cfg.enabled && is_admin;
    response["shell"] = terminal_cfg.shell;
    response["max_sessions"] = terminal_cfg.max_sessions;
    response["ws_path"] = "/api/v1/ws/terminal";

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

} // namespace console::api
