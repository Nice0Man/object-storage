#pragma once

#include "console/common/Types.hpp"

#include <json/json.h>

namespace console::websocket {

/**
 * @brief Event broadcaster for WebSocket notifications
 *
 * Provides helper methods to broadcast various events to WebSocket clients.
 */
class EventBroadcaster {
  public:
    /**
     * @brief Broadcast bucket created event
     */
    static void broadcast_bucket_created(const String& bucket_name, const String& region, const String& user);

    /**
     * @brief Broadcast bucket deleted event
     */
    static void broadcast_bucket_deleted(const String& bucket_name, const String& user);

    /**
     * @brief Broadcast object uploaded event
     */
    static void broadcast_object_uploaded(const String& bucket_name,
                                          const String& object_key,
                                          int64_t size,
                                          const String& user);

    /**
     * @brief Broadcast object deleted event
     */
    static void broadcast_object_deleted(const String& bucket_name, const String& object_key, const String& user);

    /**
     * @brief Broadcast upload progress event
     */
    static void broadcast_upload_progress(const String& upload_id,
                                          const String& bucket_name,
                                          const String& object_key,
                                          int64_t bytes_uploaded,
                                          int64_t total_bytes,
                                          double percent);

    /**
     * @brief Broadcast server notification
     */
    static void broadcast_server_notification(const String& level, // info, warning, error
                                              const String& message);

  private:
    /**
     * @brief Create base event JSON
     */
    static Json::Value create_base_event(const String& event_type, const String& category);

    /**
     * @brief Broadcast event via EventsController
     */
    static void broadcast(const Json::Value& event);
};

} // namespace console::websocket
