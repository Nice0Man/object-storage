#include "console/websocket/EventsController.hpp"
#include "console/common/Logger.hpp"
#include "console/utils/JWT.hpp"
#include "console/common/Config.hpp"

namespace console::websocket {

using namespace console::utils;

// Static members initialization
std::set<drogon::WebSocketConnectionPtr> EventsController::connections_;
std::mutex EventsController::connections_mutex_;
std::map<drogon::WebSocketConnectionPtr, EventsController::ConnectionMetadata> 
    EventsController::metadata_;

void EventsController::handleNewConnection(
    const drogon::HttpRequestPtr& req,
    const drogon::WebSocketConnectionPtr& conn
) {
    CONSOLE_LOG_INFO("New WebSocket connection from: {}", req->getPeerAddr().toIp());
    
    // Authenticate connection
    UserInfo user_info;
    if (!authenticate_connection(req, user_info)) {
        CONSOLE_LOG_WARN("Unauthenticated WebSocket connection attempt");
        conn->shutdown();
        return;
    }
    
    // Add connection to active connections
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.insert(conn);
        
        // Store metadata
        ConnectionMetadata metadata;
        metadata.user_info = user_info;
        metadata.subscribed_events = {"bucket", "object", "server"};  // Default subscriptions
        metadata_[conn] = metadata;
    }
    
    // Send welcome message
    Json::Value welcome;
    welcome["type"] = "connection";
    welcome["status"] = "connected";
    welcome["message"] = "WebSocket connection established";
    welcome["user"] = user_info.access_key;
    
    send_event(conn, welcome);
    
    CONSOLE_LOG_INFO("WebSocket connection established for user: {}", 
             user_info.access_key);
}

void EventsController::handleNewMessage(
    const drogon::WebSocketConnectionPtr& conn,
    std::string&& message,
    const drogon::WebSocketMessageType& type
) {
    if (type == drogon::WebSocketMessageType::Text) {
        CONSOLE_LOG_DEBUG("Received WebSocket message: {}", message);
        
        // Parse JSON message
        Json::Value json_message;
        Json::CharReaderBuilder builder;
        std::istringstream stream(message);
        String errors;
        
        if (!Json::parseFromStream(builder, stream, &json_message, &errors)) {
            CONSOLE_LOG_WARN("Invalid JSON in WebSocket message: {}", errors);
            
            Json::Value error;
            error["type"] = "error";
            error["message"] = "Invalid JSON format";
            send_event(conn, error);
            return;
        }
        
        handle_client_message(conn, json_message);
    }
}

void EventsController::handleConnectionClosed(
    const drogon::WebSocketConnectionPtr& conn
) {
    CONSOLE_LOG_INFO("WebSocket connection closed");
    
    // Remove connection
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.erase(conn);
        metadata_.erase(conn);
    }
}

void EventsController::broadcast_event(const Json::Value& event) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    String event_type = event.get("type", "").asString();
    
    // Convert JSON to string once
    Json::StreamWriterBuilder writer;
    String event_str = Json::writeString(writer, event);
    
    for (const auto& conn : connections_) {
        // Check if client subscribed to this event type
        auto it = metadata_.find(conn);
        if (it != metadata_.end()) {
            const auto& subscriptions = it->second.subscribed_events;
            if (subscriptions.find(event_type) != subscriptions.end() ||
                subscriptions.find("*") != subscriptions.end()) {
                conn->send(event_str);
            }
        }
    }
    
    CONSOLE_LOG_DEBUG("Broadcasted event type '{}' to {} connections", 
              event_type, connections_.size());
}

void EventsController::send_event(
    const drogon::WebSocketConnectionPtr& conn,
    const Json::Value& event
) {
    Json::StreamWriterBuilder writer;
    String event_str = Json::writeString(writer, event);
    conn->send(event_str);
}

void EventsController::handle_client_message(
    const drogon::WebSocketConnectionPtr& conn,
    const Json::Value& message
) {
    String message_type = message.get("type", "").asString();
    
    if (message_type == "ping") {
        // Respond to ping
        Json::Value pong;
        pong["type"] = "pong";
        pong["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
        send_event(conn, pong);
        
    } else if (message_type == "subscribe") {
        // Subscribe to specific events
        Vector<String> event_types;
        if (message.isMember("events") && message["events"].isArray()) {
            for (const auto& event : message["events"]) {
                event_types.push_back(event.asString());
            }
        }
        
        subscribe_to_events(conn, event_types);
        
        Json::Value response;
        response["type"] = "subscribed";
        response["events"] = message["events"];
        send_event(conn, response);
        
    } else if (message_type == "unsubscribe") {
        // Unsubscribe from events
        std::lock_guard<std::mutex> lock(connections_mutex_);
        auto it = metadata_.find(conn);
        if (it != metadata_.end() && message.isMember("events")) {
            for (const auto& event : message["events"]) {
                it->second.subscribed_events.erase(event.asString());
            }
        }
        
        Json::Value response;
        response["type"] = "unsubscribed";
        response["events"] = message["events"];
        send_event(conn, response);
        
    } else {
        CONSOLE_LOG_WARN("Unknown message type: {}", message_type);
        
        Json::Value error;
        error["type"] = "error";
        error["message"] = "Unknown message type: " + message_type;
        send_event(conn, error);
    }
}

bool EventsController::authenticate_connection(
    const drogon::HttpRequestPtr& req,
    UserInfo& user_info
) {
    // Try to get token from query parameter
    String token = req->getParameter("token");
    
    if (token.empty()) {
        // Try Authorization header
        auto auth_header = req->getHeader("Authorization");
        if (auth_header.find("Bearer ") == 0) {
            token = auth_header.substr(7);
        }
    }
    
    if (token.empty()) {
        return false;
    }
    
    // Validate JWT token
    auto config = std::make_shared<Config>();
    config->load("config.json");
    auto jwt_secret = config->get_string("auth.jwt_secret", "");
    
    auto user_info_opt = JWT::validate_token(token, jwt_secret);
    if (!user_info_opt) {
        return false;
    }
    
    user_info = *user_info_opt;
    return true;
}

void EventsController::subscribe_to_events(
    const drogon::WebSocketConnectionPtr& conn,
    const Vector<String>& event_types
) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    auto it = metadata_.find(conn);
    if (it != metadata_.end()) {
        for (const auto& event_type : event_types) {
            it->second.subscribed_events.insert(event_type);
        }
    }
}

} // namespace console::websocket

