#pragma once

#include "console/common/Types.hpp"
#include "console/websocket/PtySession.hpp"

#include <drogon/WebSocketController.h>

#include <json/json.h>
#include <map>
#include <memory>
#include <mutex>

namespace console::websocket {

/**
 * @brief Admin-only interactive shell over WebSocket (PTY bridge).
 *
 * Protocol:
 * - Binary frames: stdin/stdout raw bytes
 * - Text JSON: {"type":"resize","cols":80,"rows":24}
 */
class TerminalController : public drogon::WebSocketController<TerminalController> {
  public:
    void handleNewMessage(const drogon::WebSocketConnectionPtr& conn,
                          std::string&& message,
                          const drogon::WebSocketMessageType& type) override;

    void handleNewConnection(const drogon::HttpRequestPtr& req, const drogon::WebSocketConnectionPtr& conn) override;

    void handleConnectionClosed(const drogon::WebSocketConnectionPtr& conn) override;

    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/api/v1/ws/terminal", drogon::Get);
    WS_PATH_LIST_END

  private:
    struct SessionState {
        UserInfo user_info;
        std::unique_ptr<PtySession> pty;
    };

    static bool is_terminal_allowed(const UserInfo& user_info);
    static size_t active_session_count();
    static void send_text(const drogon::WebSocketConnectionPtr& conn, const Json::Value& payload);
    static void send_binary_safe(const drogon::WebSocketConnectionPtr& conn, const char* data, size_t len);

    static std::map<drogon::WebSocketConnectionPtr, SessionState> sessions_;
    static std::mutex sessions_mutex_;
};

} // namespace console::websocket
