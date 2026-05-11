#pragma once

#include "console/common/Types.hpp"

#include <json/json.h>

namespace console::models {

/**
 * @brief Group model
 */
class Group {
  public:
    Group() = default;
    explicit Group(String name) : name_(std::move(name)) {}

    // Getters
    const String& name() const { return name_; }
    const Vector<String>& members() const { return members_; }
    const Vector<String>& policies() const { return policies_; }
    const TimePoint& created_at() const { return created_at_; }
    const String& description() const { return description_; }

    // Setters
    void set_name(const String& name) { name_ = name; }
    void set_members(const Vector<String>& members) { members_ = members; }
    void add_member(const String& member) { members_.push_back(member); }
    void remove_member(const String& member);
    void set_policies(const Vector<String>& policies) { policies_ = policies; }
    void add_policy(const String& policy) { policies_.push_back(policy); }
    void set_created_at(const TimePoint& time) { created_at_ = time; }
    void set_description(const String& desc) { description_ = desc; }

    // Utilities
    bool has_member(const String& member) const;
    bool has_policy(const String& policy) const;

    // Serialization
    Json::Value to_json() const;
    static Group from_json(const Json::Value& json);

  private:
    String name_;
    Vector<String> members_;
    Vector<String> policies_;
    TimePoint created_at_;
    String description_;
};

/**
 * @brief Create group request
 */
struct CreateGroupRequest {
    String name;
    Vector<String> members;
    Vector<String> policies;
    Optional<String> description;

    static CreateGroupRequest from_json(const Json::Value& json);
};

/**
 * @brief Update group request
 */
struct UpdateGroupRequest {
    Optional<Vector<String>> members;
    Optional<Vector<String>> policies;
    Optional<String> description;

    static UpdateGroupRequest from_json(const Json::Value& json);
};

/**
 * @brief Add/Remove group members request
 */
struct UpdateGroupMembersRequest {
    Vector<String> members_to_add;
    Vector<String> members_to_remove;

    static UpdateGroupMembersRequest from_json(const Json::Value& json);
};

/**
 * @brief List groups response
 */
struct ListGroupsResponse {
    Vector<Group> groups;
    int64_t total_count{0};

    Json::Value to_json() const;
};

} // namespace console::models
