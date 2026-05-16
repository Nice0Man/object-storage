#include "console/websocket/TerminalController.hpp"

#include "console/common/Config.hpp"
#include "console/common/Logger.hpp"
#include "console/websocket/WebSocketAuth.hpp"

#include <drogon/drogon.h>

namespace console::websocket {

std::map<drogon::WebSocketConnectionPtr, TerminalController::SessionState> TerminalController::sessions_;
std::mutex TerminalController::sessions_mutex_;

bool
TerminalController::is_terminal_allowed(const UserInfo& user_info) {
    const auto& terminal_cfg = Config::instance().terminal();
    if (!terminal_cfg.enabled) {
        return false;
    }
    return user_info.is_admin || user_info.role == "admin";
}

size_t
TerminalController::active_session_count() {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    return sessions_.size();
}

void
TerminalController::send_text(const drogon::WebSocketConnectionPtr& conn, const Json::Value& payload) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    conn->send(Json::writeString(builder, payload));
}

void
TerminalController::send_binary_safe(const drogon::WebSocketConnectionPtr& conn, const char* data, size_t len) {
    std::string chunk(data, len);
    drogon::app().getLoop()->queueInLoop([conn, chunk = std::move(chunk)]() {
        if (conn && conn->connected()) {
            conn->send(chunk, drogon::WebSocketMessageType::Binary);
        }
    });
}

void
TerminalController::handleNewConnection(const drogon::HttpRequestPtr& req, const drogon::WebSocketConnectionPtr& conn) {
    UserInfo user_info;
    if (!authenticate_websocket_request(req, user_info)) {
        CONSOLE_LOG_WARN("Terminal WebSocket: authentication failed");
        conn->shutdown();
        return;
    }

    if (!is_terminal_allowed(user_info)) {
        CONSOLE_LOG_WARN("Terminal WebSocket: access denied for {}", user_info.access_key);
        Json::Value error;
        error["type"] = "error";
        error["message"] = "Admin terminal is disabled or insufficient permissions";
        send_text(conn, error);
        conn->shutdown();
        return;
    }

    const auto& terminal_cfg = Config::instance().terminal();
    if (active_session_count() >= terminal_cfg.max_sessions) {
        Json::Value error;
        error["type"] = "error";
        error["message"] = "Maximum terminal sessions reached";
        send_text(conn, error);
        conn->shutdown();
        return;
    }

    auto pty = std::make_unique<PtySession>();
    auto conn_weak = std::weak_ptr<drogon::WebSocketConnection>(conn);
    bool started = pty->start(terminal_cfg.shell, [conn_weak](const char* data, size_t len) {
        if (auto locked = conn_weak.lock()) {
            if (locked->connected()) {
                send_binary_safe(locked, data, len);
            }
        }
    });

    if (!started) {
        Json::Value error;
        error["type"] = "error";
        error["message"] = "Failed to start shell session";
        send_text(conn, error);
        conn->shutdown();
        return;
    }

    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        sessions_[conn] = SessionState{user_info, std::move(pty)};
    }

    Json::Value welcome;
    welcome["type"] = "connected";
    welcome["shell"] = terminal_cfg.shell;
    welcome["user"] = user_info.access_key;
    send_text(conn, welcome);

    CONSOLE_LOG_INFO("Terminal session started for admin {}", user_info.access_key);
}

void
TerminalController::handleNewMessage(const drogon::WebSocketConnectionPtr& conn,
                                     std::string&& message,
                                     const drogon::WebSocketMessageType& type) {
    PtySession* pty = nullptr;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = sessions_.find(conn);
        if (it == sessions_.end()) {
            return;
        }
        pty = it->second.pty.get();
    }

    if (!pty || !pty->is_running()) {
        return;
    }

    if (type == drogon::WebSocketMessageType::Binary) {
        pty->write_input(message.data(), message.size());
        return;
    }

    if (type == drogon::WebSocketMessageType::Text) {
        Json::Value json_message;
        Json::CharReaderBuilder builder;
        std::istringstream stream(message);
        String errors;
        if (!Json::parseFromStream(builder, stream, &json_message, &errors)) {
            return;
        }

        const String msg_type = json_message.get("type", "").asString();
        if (msg_type == "resize") {
            const auto cols = static_cast<uint16_t>(json_message.get("cols", 80).asUInt());
            const auto rows = static_cast<uint16_t>(json_message.get("rows", 24).asUInt());
            pty->resize(cols, rows);
        } else if (msg_type == "input" && json_message.isMember("data")) {
            const String data = json_message["data"].asString();
            pty->write_input(data.data(), data.size());
        }
    }
}

void
TerminalController::handleConnectionClosed(const drogon::WebSocketConnectionPtr& conn) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = sessions_.find(conn);
    if (it != sessions_.end()) {
        if (it->second.pty) {
            it->second.pty->stop();
        }
        CONSOLE_LOG_INFO("Terminal session closed for {}", it->second.user_info.access_key);
        sessions_.erase(it);
    }
}

} // namespace console::websocket
