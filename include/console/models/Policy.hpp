#pragma once

#include "console/common/Types.hpp"

#include <json/json.h>

namespace console::models {

/**
 * @brief IAM Policy Statement
 */
struct PolicyStatement {
    enum class Effect {
        Allow,
        Deny
    };

    Effect effect{Effect::Allow};
    Vector<String> actions;
    Vector<String> resources;
    Optional<Json::Value> conditions;

    Json::Value to_json() const;
    static PolicyStatement from_json(const Json::Value& json);
};

/**
 * @brief IAM Policy
 */
class Policy {
  public:
    Policy() = default;
    explicit Policy(String name) : name_(std::move(name)) {}

    // Getters
    const String& name() const { return name_; }
    const String& version() const { return version_; }
    const Vector<PolicyStatement>& statements() const { return statements_; }
    const TimePoint& created_at() const { return created_at_; }
    const String& description() const { return description_; }

    // Setters
    void set_name(const String& name) { name_ = name; }
    void set_version(const String& version) { version_ = version; }
    void set_statements(const Vector<PolicyStatement>& statements) { statements_ = statements; }
    void add_statement(const PolicyStatement& statement) { statements_.push_back(statement); }
    void set_created_at(const TimePoint& time) { created_at_ = time; }
    void set_description(const String& desc) { description_ = desc; }

    // Utilities
    bool allows_action(const String& action, const String& resource) const;
    bool denies_action(const String& action, const String& resource) const;

    // Serialization
    Json::Value to_json() const;
    String to_json_string() const;
    static Policy from_json(const Json::Value& json);
    static Policy from_json_string(const String& json_str);

  private:
    String name_;
    String version_{"2012-10-17"}; // AWS IAM policy version
    Vector<PolicyStatement> statements_;
    TimePoint created_at_;
    String description_;
};

/**
 * @brief Create policy request
 */
struct CreatePolicyRequest {
    String name;
    Vector<PolicyStatement> statements;
    Optional<String> description;

    static CreatePolicyRequest from_json(const Json::Value& json);
};

/**
 * @brief Update policy request
 */
struct UpdatePolicyRequest {
    Optional<Vector<PolicyStatement>> statements;
    Optional<String> description;

    static UpdatePolicyRequest from_json(const Json::Value& json);
};

/**
 * @brief List policies response
 */
struct ListPoliciesResponse {
    Vector<Policy> policies;
    int64_t total_count{0};

    Json::Value to_json() const;
};

/**
 * @brief Attach/Detach policy request
 */
struct AttachPolicyRequest {
    String policy_name;
    String entity_name; // User or Group name
    bool is_group{false};

    static AttachPolicyRequest from_json(const Json::Value& json);
};

/**
 * @brief Built-in policies
 */
namespace BuiltInPolicies {
const char* const ReadOnly = R"({
  "Version": "2012-10-17",
  "Statement": [{
    "Effect": "Allow",
    "Action": ["s3:GetObject", "s3:ListBucket"],
    "Resource": ["arn:aws:s3:::*"]
  }]
})";

const char* const ReadWrite = R"({
  "Version": "2012-10-17",
  "Statement": [{
    "Effect": "Allow",
    "Action": ["s3:*"],
    "Resource": ["arn:aws:s3:::*"]
  }]
})";

const char* const WriteOnly = R"({
  "Version": "2012-10-17",
  "Statement": [{
    "Effect": "Allow",
    "Action": ["s3:PutObject", "s3:DeleteObject"],
    "Resource": ["arn:aws:s3:::*"]
  }]
})";

const char* const AdminAccess = R"({
  "Version": "2012-10-17",
  "Statement": [{
    "Effect": "Allow",
    "Action": ["*"],
    "Resource": ["*"]
  }]
})";
} // namespace BuiltInPolicies

} // namespace console::models
