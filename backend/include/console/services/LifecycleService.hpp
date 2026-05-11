#pragma once

#include "console/clients/LocalStorageClient.hpp"
#include "console/common/Types.hpp"

#include <json/json.h>
#include <memory>

namespace console::services {

/**
 * @brief Lifecycle rule filter
 */
struct LifecycleFilter {
    String prefix;                        // Filter by object key prefix
    StringMap tags;                       // Filter by object tags
    int64_t object_size_greater_than{-1}; // Filter by minimum size (-1 = not set)
    int64_t object_size_less_than{-1};    // Filter by maximum size (-1 = not set)

    Json::Value to_json() const {
        Json::Value json;
        if (!prefix.empty()) {
            json["prefix"] = prefix;
        }
        if (!tags.empty()) {
            Json::Value tags_json(Json::objectValue);
            for (const auto& [k, v] : tags) {
                tags_json[k] = v;
            }
            json["tags"] = tags_json;
        }
        if (object_size_greater_than >= 0) {
            json["object_size_greater_than"] = static_cast<Json::Int64>(object_size_greater_than);
        }
        if (object_size_less_than >= 0) {
            json["object_size_less_than"] = static_cast<Json::Int64>(object_size_less_than);
        }
        return json;
    }

    static LifecycleFilter from_json(const Json::Value& json) {
        LifecycleFilter filter;
        filter.prefix = json.get("prefix", "").asString();
        if (json.isMember("tags")) {
            for (const auto& k : json["tags"].getMemberNames()) {
                filter.tags[k] = json["tags"][k].asString();
            }
        }
        filter.object_size_greater_than = json.get("object_size_greater_than", -1).asInt64();
        filter.object_size_less_than = json.get("object_size_less_than", -1).asInt64();
        return filter;
    }
};

/**
 * @brief Lifecycle rule action
 */
struct LifecycleAction {
    enum class Type {
        Expiration,                     // Delete objects after N days
        AbortIncompleteMultipartUpload, // Clean up incomplete uploads
        NoncurrentVersionExpiration,    // Delete old versions
        Transition                      // Move to different storage class
    };

    Type type{Type::Expiration};
    int days{0};            // Days after creation/modification
    String storage_class;   // For Transition action
    int noncurrent_days{0}; // For noncurrent version expiration

    Json::Value to_json() const {
        Json::Value json;
        switch (type) {
            case Type::Expiration:
                json["type"] = "Expiration";
                json["days"] = days;
                break;
            case Type::AbortIncompleteMultipartUpload:
                json["type"] = "AbortIncompleteMultipartUpload";
                json["days_after_initiation"] = days;
                break;
            case Type::NoncurrentVersionExpiration:
                json["type"] = "NoncurrentVersionExpiration";
                json["noncurrent_days"] = noncurrent_days;
                break;
            case Type::Transition:
                json["type"] = "Transition";
                json["days"] = days;
                json["storage_class"] = storage_class;
                break;
        }
        return json;
    }

    static LifecycleAction from_json(const Json::Value& json) {
        LifecycleAction action;
        String type_str = json.get("type", "Expiration").asString();

        if (type_str == "Expiration") {
            action.type = Type::Expiration;
            action.days = json.get("days", 0).asInt();
        } else if (type_str == "AbortIncompleteMultipartUpload") {
            action.type = Type::AbortIncompleteMultipartUpload;
            action.days = json.get("days_after_initiation", 7).asInt();
        } else if (type_str == "NoncurrentVersionExpiration") {
            action.type = Type::NoncurrentVersionExpiration;
            action.noncurrent_days = json.get("noncurrent_days", 30).asInt();
        } else if (type_str == "Transition") {
            action.type = Type::Transition;
            action.days = json.get("days", 0).asInt();
            action.storage_class = json.get("storage_class", "GLACIER").asString();
        }
        return action;
    }
};

/**
 * @brief Lifecycle rule
 */
struct LifecycleRule {
    String id;
    String status{"Enabled"}; // Enabled or Disabled
    LifecycleFilter filter;
    Vector<LifecycleAction> actions;

    Json::Value to_json() const {
        Json::Value json;
        json["id"] = id;
        json["status"] = status;
        json["filter"] = filter.to_json();

        Json::Value actions_json(Json::arrayValue);
        for (const auto& action : actions) {
            actions_json.append(action.to_json());
        }
        json["actions"] = actions_json;
        return json;
    }

    static LifecycleRule from_json(const Json::Value& json) {
        LifecycleRule rule;
        rule.id = json.get("id", "").asString();
        rule.status = json.get("status", "Enabled").asString();
        rule.filter = LifecycleFilter::from_json(json["filter"]);

        if (json.isMember("actions")) {
            for (const auto& action_json : json["actions"]) {
                rule.actions.push_back(LifecycleAction::from_json(action_json));
            }
        }
        return rule;
    }

    bool is_enabled() const { return status == "Enabled"; }
};

/**
 * @brief Bucket lifecycle configuration
 */
struct LifecycleConfiguration {
    Vector<LifecycleRule> rules;

    Json::Value to_json() const {
        Json::Value json;
        Json::Value rules_json(Json::arrayValue);
        for (const auto& rule : rules) {
            rules_json.append(rule.to_json());
        }
        json["rules"] = rules_json;
        return json;
    }

    static LifecycleConfiguration from_json(const Json::Value& json) {
        LifecycleConfiguration config;
        if (json.isMember("rules")) {
            for (const auto& rule_json : json["rules"]) {
                config.rules.push_back(LifecycleRule::from_json(rule_json));
            }
        }
        return config;
    }
};

/**
 * @brief Lifecycle Service
 *
 * Manages bucket lifecycle rules and executes lifecycle policies.
 */
class LifecycleService {
  public:
    explicit LifecycleService(std::shared_ptr<clients::LocalStorageClient> storage_client);
    ~LifecycleService() = default;

    /**
     * @brief Set bucket lifecycle configuration
     */
    Result<void, String> set_bucket_lifecycle(const String& bucket_name, const LifecycleConfiguration& config);

    /**
     * @brief Get bucket lifecycle configuration
     */
    Result<LifecycleConfiguration, String> get_bucket_lifecycle(const String& bucket_name);

    /**
     * @brief Delete bucket lifecycle configuration
     */
    Result<void, String> delete_bucket_lifecycle(const String& bucket_name);

    /**
     * @brief Execute lifecycle rules for a bucket (typically called by background job)
     *
     * @param bucket_name Bucket to process
     * @return Number of objects affected
     */
    Result<int, String> execute_lifecycle_rules(const String& bucket_name);

    /**
     * @brief Execute lifecycle rules for all buckets
     *
     * @return Total number of objects affected
     */
    Result<int, String> execute_all_lifecycle_rules();

  private:
    std::shared_ptr<clients::LocalStorageClient> storage_client_;

    // Check if object matches filter
    bool object_matches_filter(const models::Object& object, const LifecycleFilter& filter);

    // Apply action to object
    Result<bool, String> apply_action(const String& bucket_name,
                                      const models::Object& object,
                                      const LifecycleAction& action);
};

} // namespace console::services
