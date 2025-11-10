
//
#include "console/models/Group.hpp"

#include <algorithm>

namespace console::models {

void
Group::remove_member(const String& member) {
    auto it = std::find(members_.begin(), members_.end(), member);
    if (it != members_.end()) {
        members_.erase(it);
    }
}

bool
Group::has_member(const String& member) const {
    return std::find(members_.begin(), members_.end(), member) != members_.end();
}

bool
Group::has_policy(const String& policy) const {
    return std::find(policies_.begin(), policies_.end(), policy) != policies_.end();
}

Json::Value
Group::to_json() const {
    Json::Value json;
    json["name"] = name_;
    json["description"] = description_;

    Json::Value members_array(Json::arrayValue);
    for (const auto& member : members_) {
        members_array.append(member);
    }
    json["members"] = members_array;

    Json::Value policies_array(Json::arrayValue);
    for (const auto& policy : policies_) {
        policies_array.append(policy);
    }
    json["policies"] = policies_array;

    auto time_t = std::chrono::system_clock::to_time_t(created_at_);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    json["created_at"] = oss.str();

    return json;
}

Group
Group::from_json(const Json::Value& json) {
    Group group;
    group.name_ = json.get("name", "").asString();
    group.description_ = json.get("description", "").asString();

    if (json.isMember("members") && json["members"].isArray()) {
        for (const auto& member : json["members"]) {
            group.members_.push_back(member.asString());
        }
    }

    if (json.isMember("policies") && json["policies"].isArray()) {
        for (const auto& policy : json["policies"]) {
            group.policies_.push_back(policy.asString());
        }
    }

    // TODO(Nice0Man): Parse created_at date

    return group;
}

CreateGroupRequest
CreateGroupRequest::from_json(const Json::Value& json) {
    CreateGroupRequest req;
    req.name = json.get("name", "").asString();

    if (json.isMember("description")) {
        req.description = json["description"].asString();
    }

    if (json.isMember("members") && json["members"].isArray()) {
        for (const auto& member : json["members"]) {
            req.members.push_back(member.asString());
        }
    }

    if (json.isMember("policies") && json["policies"].isArray()) {
        for (const auto& policy : json["policies"]) {
            req.policies.push_back(policy.asString());
        }
    }

    return req;
}

UpdateGroupRequest
UpdateGroupRequest::from_json(const Json::Value& json) {
    UpdateGroupRequest req;

    if (json.isMember("description")) {
        req.description = json["description"].asString();
    }

    if (json.isMember("members") && json["members"].isArray()) {
        Vector<String> members;
        for (const auto& member : json["members"]) {
            members.push_back(member.asString());
        }
        req.members = members;
    }

    if (json.isMember("policies") && json["policies"].isArray()) {
        Vector<String> policies;
        for (const auto& policy : json["policies"]) {
            policies.push_back(policy.asString());
        }
        req.policies = policies;
    }

    return req;
}

UpdateGroupMembersRequest
UpdateGroupMembersRequest::from_json(const Json::Value& json) {
    UpdateGroupMembersRequest req;

    if (json.isMember("members_to_add") && json["members_to_add"].isArray()) {
        for (const auto& member : json["members_to_add"]) {
            req.members_to_add.push_back(member.asString());
        }
    }

    if (json.isMember("members_to_remove") && json["members_to_remove"].isArray()) {
        for (const auto& member : json["members_to_remove"]) {
            req.members_to_remove.push_back(member.asString());
        }
    }

    return req;
}

Json::Value
ListGroupsResponse::to_json() const {
    Json::Value json;

    Json::Value groups_array(Json::arrayValue);
    for (const auto& group : groups) {
        groups_array.append(group.to_json());
    }
    json["groups"] = groups_array;

    json["total_count"] = Json::Int64(total_count);

    return json;
}

} // namespace console::models
