#pragma once

#include "console/common/Types.hpp"

#include <json/json.h>

namespace console {
namespace models {

/**
 * @brief Server information model
 */
class ServerInfo {
  public:
    ServerInfo() = default;

    // Getters
    const String& version() const { return version_; }
    const String& region() const { return region_; }
    const String& deployment_id() const { return deployment_id_; }
    uint64_t uptime() const { return uptime_; }
    uint64_t total_storage() const { return total_storage_; }
    uint64_t used_storage() const { return used_storage_; }
    const StringMap& backend() const { return backend_; }

    // Setters
    void set_version(const String& version) { version_ = version; }
    void set_region(const String& region) { region_ = region; }
    void set_deployment_id(const String& id) { deployment_id_ = id; }
    void set_uptime(uint64_t uptime) { uptime_ = uptime; }
    void set_total_storage(uint64_t total) { total_storage_ = total; }
    void set_used_storage(uint64_t used) { used_storage_ = used; }
    void set_backend(const StringMap& backend) { backend_ = backend; }

    // Serialization
    Json::Value to_json() const;
    static ServerInfo from_json(const Json::Value& json);

  private:
    String version_;
    String region_;
    String deployment_id_;
    uint64_t uptime_{0};
    uint64_t total_storage_{0};
    uint64_t used_storage_{0};
    StringMap backend_;
};

} // namespace models
} // namespace console
