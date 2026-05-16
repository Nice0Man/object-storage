#pragma once

#include <drogon/HttpController.h>

namespace console::api {

/**
 * @brief REST status for admin interactive terminal (WebSocket PTY).
 */
class TerminalStatusController : public drogon::HttpController<TerminalStatusController> {
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(TerminalStatusController::status, "/api/v1/system/terminal/status", drogon::Get, "AuthFilter");
    METHOD_LIST_END

    void status(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback) const;
};

} // namespace console::api
