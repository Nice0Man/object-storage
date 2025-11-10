#include "console/websocket/EventBroadcaster.hpp"
#include "console/websocket/EventsController.hpp"
#include "console/common/Logger.hpp"
#include <chrono>

namespace console::websocket {

using namespace console::utils;

void EventBroadcaster::broadcast_bucket_created(
    const String& bucket_name,
    const String& region,
    const String& user
) {
    auto event = create_base_event("bucket_created", "bucket");
    event["bucket_name"] = bucket_name;
    event["region"] = region;
    event["user"] = user;
    
    broadcast(event);
    CONSOLE_LOG_INFO("Broadcast: Bucket '{}' created by {}", bucket_name, user);
}

void EventBroadcaster::broadcast_bucket_deleted(
    const String& bucket_name,
    const String& user
) {
    auto event = create_base_event("bucket_deleted", "bucket");
    event["bucket_name"] = bucket_name;
    event["user"] = user;
    
    broadcast(event);
    CONSOLE_LOG_INFO("Broadcast: Bucket '{}' deleted by {}", bucket_name, user);
}

void EventBroadcaster::broadcast_object_uploaded(
    const String& bucket_name,
    const String& object_key,
    int64_t size,
    const String& user
) {
    auto event = create_base_event("object_uploaded", "object");
    event["bucket_name"] = bucket_name;
    event["object_key"] = object_key;
    event["size"] = size;
    event["user"] = user;
    
    broadcast(event);
    CONSOLE_LOG_INFO("Broadcast: Object '{}' uploaded to '{}' by {}", 
             object_key, bucket_name, user);
}

void EventBroadcaster::broadcast_object_deleted(
    const String& bucket_name,
    const String& object_key,
    const String& user
) {
    auto event = create_base_event("object_deleted", "object");
    event["bucket_name"] = bucket_name;
    event["object_key"] = object_key;
    event["user"] = user;
    
    broadcast(event);
    CONSOLE_LOG_INFO("Broadcast: Object '{}' deleted from '{}' by {}", 
             object_key, bucket_name, user);
}

void EventBroadcaster::broadcast_upload_progress(
    const String& upload_id,
    const String& bucket_name,
    const String& object_key,
    int64_t bytes_uploaded,
    int64_t total_bytes,
    double percent
) {
    auto event = create_base_event("upload_progress", "object");
    event["upload_id"] = upload_id;
    event["bucket_name"] = bucket_name;
    event["object_key"] = object_key;
    event["bytes_uploaded"] = bytes_uploaded;
    event["total_bytes"] = total_bytes;
    event["percent"] = percent;
    
    broadcast(event);
}

void EventBroadcaster::broadcast_server_notification(
    const String& level,
    const String& message
) {
    auto event = create_base_event("server_notification", "server");
    event["level"] = level;
    event["message"] = message;
    
    broadcast(event);
    CONSOLE_LOG_INFO("Broadcast: Server notification [{}]: {}", level, message);
}

Json::Value EventBroadcaster::create_base_event(
    const String& event_type,
    const String& category
) {
    Json::Value event;
    event["type"] = event_type;
    event["category"] = category;
    event["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    return event;
}

void EventBroadcaster::broadcast(const Json::Value& event) {
    EventsController::broadcast_event(event);
}

} // namespace console::websocket

