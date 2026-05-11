#pragma once

#include "console/common/Types.hpp"

#include <json/json.h>

namespace console::models {

/**
 * @brief User model
 */
class User {
  public:
    User() = default;
    explicit User(const UserInfo& info) : info_(info) {}

    // Getters
    const String& access_key() const { return info_.access_key; }
    const String& secret_key() const { return info_.secret_key; }
    const String& account_name() const { return info_.account_name; }
    const Vector<String>& policies() const { return info_.policies; }
    const TimePoint& created_at() const { return info_.created_at; }
    bool is_admin() const { return info_.is_admin; }

    // Setters
    void set_access_key(const String& key) { info_.access_key = key; }
    void set_secret_key(const String& key) { info_.secret_key = key; }
    void set_account_name(const String& name) { info_.account_name = name; }
    void set_policies(const Vector<String>& policies) { info_.policies = policies; }
    void add_policy(const String& policy) { info_.policies.push_back(policy); }
    void set_admin(bool is_admin) { info_.is_admin = is_admin; }

    // Serialization (excludes secret_key for security)
    Json::Value to_json() const;
    static User from_json(const Json::Value& json);

  private:
    UserInfo info_;
};

/**
 * @brief Create user request
 */
struct CreateUserRequest {
    String access_key;
    String secret_key;
    Vector<String> policies;
    Vector<String> groups;

    static CreateUserRequest from_json(const Json::Value& json);
};

/**
 * @brief Update user request
 */
struct UpdateUserRequest {
    Optional<String> secret_key;
    Optional<Vector<String>> policies;
    Optional<Vector<String>> groups;
    Optional<bool> is_enabled;

    static UpdateUserRequest from_json(const Json::Value& json);
};

/**
 * @brief Change password request
 */
struct ChangePasswordRequest {
    String old_password;
    String new_password;

    static ChangePasswordRequest from_json(const Json::Value& json);
};

/**
 * @brief Service account credentials
 */
struct ServiceAccount {
    String access_key;
    String secret_key;
    String parent_user;
    Vector<String> policies;
    TimePoint created_at;
    Optional<TimePoint> expires_at;

    Json::Value to_json() const;
    static ServiceAccount from_json(const Json::Value& json);
};

/**
 * @brief List users response
 */
struct ListUsersResponse {
    Vector<User> users;
    int64_t total_count{0};

    Json::Value to_json() const;
};

} // namespace console::models
