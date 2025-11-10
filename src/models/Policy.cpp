// Copyright 2025 OpenMaxIO Contributors
// Licensed under AGPL-3.0
//
#include "console/models/Policy.hpp"

#include <algorithm>
#include <json/json.h>

namespace console::models {

Json::Value
PolicyStatement::to_json() const {
    Json::Value json;
    json["Effect"] = (effect == Effect::Allow) ? "Allow" : "Deny";

    Json::Value actions_array(Json::arrayValue);
    for (const auto& action : actions) {
        actions_array.append(action);
    }
    json["Action"] = actions_array;

    Json::Value resources_array(Json::arrayValue);
    for (const auto& resource : resources) {
        resources_array.append(resource);
    }
    json["Resource"] = resources_array;

    if (conditions) {
        json["Condition"] = *conditions;
    }

    return json;
}

PolicyStatement
PolicyStatement::from_json(const Json::Value& json) {
    PolicyStatement statement;

    String effect_str = json.get("Effect", "Allow").asString();
    statement.effect = (effect_str == "Deny") ? Effect::Deny : Effect::Allow;

    // Parse actions (can be string or array)
    if (json.isMember("Action")) {
        if (json["Action"].isString()) {
            statement.actions.push_back(json["Action"].asString());
        } else if (json["Action"].isArray()) {
            for (const auto& action : json["Action"]) {
                statement.actions.push_back(action.asString());
            }
        }
    }

    // Parse resources (can be string or array)
    if (json.isMember("Resource")) {
        if (json["Resource"].isString()) {
            statement.resources.push_back(json["Resource"].asString());
        } else if (json["Resource"].isArray()) {
            for (const auto& resource : json["Resource"]) {
                statement.resources.push_back(resource.asString());
            }
        }
    }

    if (json.isMember("Condition")) {
        statement.conditions = json["Condition"];
    }

    return statement;
}

bool
Policy::allows_action(const String& action, const String& resource) const {
    for (const auto& statement : statements_) {
        if (statement.effect != PolicyStatement::Effect::Allow) {
            continue;
        }

        // Check if action matches
        bool action_matches = false;
        for (const auto& stmt_action : statement.actions) {
            if (stmt_action == "*" || stmt_action == action) {
                action_matches = true;
                break;
            }
            // Support wildcard matching
            if (stmt_action.back() == '*') {
                String prefix = stmt_action.substr(0, stmt_action.length() - 1);
                if (action.starts_with(prefix)) {
                    action_matches = true;
                    break;
                }
            }
        }

        if (!action_matches) {
            continue;
        }

        // Check if resource matches
        for (const auto& stmt_resource : statement.resources) {
            if (stmt_resource == "*" || stmt_resource == resource) {
                return true;
            }
            // Support wildcard matching
            if (stmt_resource.back() == '*') {
                String prefix = stmt_resource.substr(0, stmt_resource.length() - 1);
                if (resource.starts_with(prefix)) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool
Policy::denies_action(const String& action, const String& resource) const {
    for (const auto& statement : statements_) {
        if (statement.effect != PolicyStatement::Effect::Deny) {
            continue;
        }

        // Check if action matches
        bool action_matches = false;
        for (const auto& stmt_action : statement.actions) {
            if (stmt_action == "*" || stmt_action == action) {
                action_matches = true;
                break;
            }
        }

        if (!action_matches) {
            continue;
        }

        // Check if resource matches
        for (const auto& stmt_resource : statement.resources) {
            if (stmt_resource == "*" || stmt_resource == resource) {
                return true;
            }
        }
    }

    return false;
}

Json::Value
Policy::to_json() const {
    Json::Value json;
    json["Version"] = version_;

    Json::Value statements_array(Json::arrayValue);
    for (const auto& statement : statements_) {
        statements_array.append(statement.to_json());
    }
    json["Statement"] = statements_array;

    return json;
}

String
Policy::to_json_string() const {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "  ";
    return Json::writeString(builder, to_json());
}

Policy
Policy::from_json(const Json::Value& json) {
    Policy policy;
    policy.version_ = json.get("Version", "2012-10-17").asString();

    if (json.isMember("Statement") && json["Statement"].isArray()) {
        for (const auto& stmt_json : json["Statement"]) {
            policy.statements_.push_back(PolicyStatement::from_json(stmt_json));
        }
    }

    return policy;
}

Policy
Policy::from_json_string(const String& json_str) {
    Json::CharReaderBuilder builder;
    Json::Value json;
    String errors;

    std::istringstream iss(json_str);
    if (!Json::parseFromStream(builder, iss, &json, &errors)) {
        throw std::runtime_error("Failed to parse policy JSON: " + errors);
    }

    return from_json(json);
}

CreatePolicyRequest
CreatePolicyRequest::from_json(const Json::Value& json) {
    CreatePolicyRequest req;
    req.name = json.get("name", "").asString();

    if (json.isMember("description")) {
        req.description = json["description"].asString();
    }

    if (json.isMember("statements") && json["statements"].isArray()) {
        for (const auto& stmt_json : json["statements"]) {
            req.statements.push_back(PolicyStatement::from_json(stmt_json));
        }
    }

    return req;
}

UpdatePolicyRequest
UpdatePolicyRequest::from_json(const Json::Value& json) {
    UpdatePolicyRequest req;

    if (json.isMember("description")) {
        req.description = json["description"].asString();
    }

    if (json.isMember("statements") && json["statements"].isArray()) {
        Vector<PolicyStatement> statements;
        for (const auto& stmt_json : json["statements"]) {
            statements.push_back(PolicyStatement::from_json(stmt_json));
        }
        req.statements = statements;
    }

    return req;
}

Json::Value
ListPoliciesResponse::to_json() const {
    Json::Value json;

    Json::Value policies_array(Json::arrayValue);
    for (const auto& policy : policies) {
        Json::Value policy_obj;
        policy_obj["name"] = policy.name();
        policy_obj["version"] = policy.version();
        policy_obj["description"] = policy.description();
        policies_array.append(policy_obj);
    }
    json["policies"] = policies_array;

    json["total_count"] = Json::Int64(total_count);

    return json;
}

AttachPolicyRequest
AttachPolicyRequest::from_json(const Json::Value& json) {
    AttachPolicyRequest req;
    req.policy_name = json.get("policy_name", "").asString();
    req.entity_name = json.get("entity_name", "").asString();
    req.is_group = json.get("is_group", false).asBool();
    return req;
}

} // namespace console::models
