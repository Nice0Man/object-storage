
//
#include "console/models/User.hpp"

#include <algorithm>

namespace console::models {

Json::Value
User::to_json() const {
    Json::Value json;
    json["access_key"] = info_.access_key;
    // Note: secret_key is intentionally excluded for security
    json["account_name"] = info_.account_name;
    json["is_admin"] = info_.is_admin;

    Json::Value policies_array(Json::arrayValue);
    for (const auto& policy : info_.policies) {
        policies_array.append(policy);
    }
    json["policies"] = policies_array;

    auto time_t = std::chrono::system_clock::to_time_t(info_.created_at);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    json["created_at"] = oss.str();

    return json;
}

User
User::from_json(const Json::Value& json) {
    UserInfo info;
    info.access_key = json.get("access_key", "").asString();
    info.account_name = json.get("account_name", "").asString();
    info.is_admin = json.get("is_admin", false).asBool();

    if (json.isMember("policies") && json["policies"].isArray()) {
        for (const auto& policy : json["policies"]) {
            info.policies.push_back(policy.asString());
        }
    }

    // TODO(Nice0Man): Parse created_at date

    return User(info);
}

CreateUserRequest
CreateUserRequest::from_json(const Json::Value& json) {
    CreateUserRequest req;
    req.access_key = json.get("access_key", "").asString();
    req.secret_key = json.get("secret_key", "").asString();

    if (json.isMember("policies") && json["policies"].isArray()) {
        for (const auto& policy : json["policies"]) {
            req.policies.push_back(policy.asString());
        }
    }

    if (json.isMember("groups") && json["groups"].isArray()) {
        for (const auto& group : json["groups"]) {
            req.groups.push_back(group.asString());
        }
    }

    return req;
}

UpdateUserRequest
UpdateUserRequest::from_json(const Json::Value& json) {
    UpdateUserRequest req;

    if (json.isMember("secret_key")) {
        req.secret_key = json["secret_key"].asString();
    }

    if (json.isMember("policies") && json["policies"].isArray()) {
        Vector<String> policies;
        for (const auto& policy : json["policies"]) {
            policies.push_back(policy.asString());
        }
        req.policies = policies;
    }

    if (json.isMember("groups") && json["groups"].isArray()) {
        Vector<String> groups;
        for (const auto& group : json["groups"]) {
            groups.push_back(group.asString());
        }
        req.groups = groups;
    }

    if (json.isMember("is_enabled")) {
        req.is_enabled = json["is_enabled"].asBool();
    }

    return req;
}

ChangePasswordRequest
ChangePasswordRequest::from_json(const Json::Value& json) {
    ChangePasswordRequest req;
    req.old_password = json.get("old_password", "").asString();
    req.new_password = json.get("new_password", "").asString();
    return req;
}

Json::Value
ServiceAccount::to_json() const {
    Json::Value json;
    json["access_key"] = access_key;
    json["secret_key"] = secret_key;
    json["parent_user"] = parent_user;

    Json::Value policies_array(Json::arrayValue);
    for (const auto& policy : policies) {
        policies_array.append(policy);
    }
    json["policies"] = policies_array;

    auto time_t = std::chrono::system_clock::to_time_t(created_at);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    json["created_at"] = oss.str();

    if (expires_at) {
        auto expires_t = std::chrono::system_clock::to_time_t(*expires_at);
        std::ostringstream oss_exp;
        oss_exp << std::put_time(std::gmtime(&expires_t), "%Y-%m-%dT%H:%M:%SZ");
        json["expires_at"] = oss_exp.str();
    }

    return json;
}

ServiceAccount
ServiceAccount::from_json(const Json::Value& json) {
    ServiceAccount sa;
    sa.access_key = json.get("access_key", "").asString();
    sa.secret_key = json.get("secret_key", "").asString();
    sa.parent_user = json.get("parent_user", "").asString();

    if (json.isMember("policies") && json["policies"].isArray()) {
        for (const auto& policy : json["policies"]) {
            sa.policies.push_back(policy.asString());
        }
    }

    // TODO(Nice0Man): Parse dates

    return sa;
}

Json::Value
ListUsersResponse::to_json() const {
    Json::Value json;

    Json::Value users_array(Json::arrayValue);
    for (const auto& user : users) {
        users_array.append(user.to_json());
    }
    json["users"] = users_array;

    json["total_count"] = Json::Int64(total_count);

    return json;
}

} // namespace console::models
