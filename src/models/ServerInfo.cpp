#include "console/models/ServerInfo.hpp"

namespace console {
namespace models {

Json::Value ServerInfo::to_json() const {
    Json::Value json;
    json["version"] = version_;
    json["region"] = region_;
    json["deploymentId"] = deployment_id_;
    json["uptime"] = static_cast<Json::Int64>(uptime_);
    json["totalStorage"] = static_cast<Json::Int64>(total_storage_);
    json["usedStorage"] = static_cast<Json::Int64>(used_storage_);
    
    Json::Value backend_json(Json::objectValue);
    for (const auto& [key, value] : backend_) {
        backend_json[key] = value;
    }
    json["backend"] = backend_json;
    
    return json;
}

ServerInfo ServerInfo::from_json(const Json::Value& json) {
    ServerInfo info;
    
    if (json.isMember("version") && json["version"].isString()) {
        info.version_ = json["version"].asString();
    }
    if (json.isMember("region") && json["region"].isString()) {
        info.region_ = json["region"].asString();
    }
    if (json.isMember("deploymentId") && json["deploymentId"].isString()) {
        info.deployment_id_ = json["deploymentId"].asString();
    }
    if (json.isMember("uptime") && json["uptime"].isUInt64()) {
        info.uptime_ = json["uptime"].asUInt64();
    }
    if (json.isMember("totalStorage") && json["totalStorage"].isUInt64()) {
        info.total_storage_ = json["totalStorage"].asUInt64();
    }
    if (json.isMember("usedStorage") && json["usedStorage"].isUInt64()) {
        info.used_storage_ = json["usedStorage"].asUInt64();
    }
    if (json.isMember("backend") && json["backend"].isObject()) {
        for (const auto& key : json["backend"].getMemberNames()) {
            info.backend_[key] = json["backend"][key].asString();
        }
    }
    
    return info;
}

} // namespace models
} // namespace console

