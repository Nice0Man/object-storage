#pragma once

#include "console/common/Types.hpp"

#include <drogon/WebSocketController.h>

#include <json/json.h>
#include <mutex>
#include <set>

namespace console::websocket {

/**
 * @brief WebSocket controller for real-time events
 *
 * Handles WebSocket connections for:
 * - Bucket events (create, delete, etc.)
 * - Object events (upload, download, delete)
 * - Upload progress tracking
 * - Server notifications
 */
class EventsController : public drogon::WebSocketController<EventsController> {
  public:
    void handleNewMessage(const drogon::WebSocketConnectionPtr& conn,
                          std::string&& message,
                          const drogon::WebSocketMessageType& type) override;

    void handleNewConnection(const drogon::HttpRequestPtr& req, const drogon::WebSocketConnectionPtr& conn) override;

    void handleConnectionClosed(const drogon::WebSocketConnectionPtr& conn) override;

    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/api/v1/ws/events", drogon::Get);
    WS_PATH_LIST_END

    /**
     * @brief Broadcast event to all connected clients
     */
    static void broadcast_event(const Json::Value& event);

    /**
     * @brief Send event to specific connection
     */
    static void send_event(const drogon::WebSocketConnectionPtr& conn, const Json::Value& event);

  private:
    /**
     * @brief Handle client message
     */
    void handle_client_message(const drogon::WebSocketConnectionPtr& conn, const Json::Value& message);

    /**
     * @brief Authenticate WebSocket connection
     */
    bool authenticate_connection(const drogon::HttpRequestPtr& req, UserInfo& user_info);

    /**
     * @brief Subscribe client to specific event types
     */
    void subscribe_to_events(const drogon::WebSocketConnectionPtr& conn, const Vector<String>& event_types);

    // Static members for managing connections
    static std::set<drogon::WebSocketConnectionPtr> connections_;
    static std::mutex connections_mutex_;

    // Connection metadata
    struct ConnectionMetadata {
        UserInfo user_info;
        std::set<String> subscribed_events;
    };
    static std::map<drogon::WebSocketConnectionPtr, ConnectionMetadata> metadata_;
};

} // namespace console::websocket
