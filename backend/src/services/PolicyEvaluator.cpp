#include "console/services/PolicyEvaluator.hpp"

#include "console/common/Logger.hpp"
#include "console/common/ServiceLocator.hpp"

#include <regex>

namespace console::services {

EvaluationResult
PolicyEvaluator::evaluate(const Vector<models::Policy>& policies, const String& action, const String& resource) const {
    EvaluationResult result;
    result.decision = PolicyDecision::NoMatch;

    // First pass: check for explicit Deny
    for (const auto& policy : policies) {
        for (const auto& statement : policy.statements()) {
            if (statement.effect == models::PolicyStatement::Effect::Deny) {
                auto decision = evaluate_statement(statement, action, resource);
                if (decision == PolicyDecision::Deny) {
                    result.decision = PolicyDecision::Deny;
                    result.matched_policy = policy.name();
                    result.reason = "Explicit Deny in policy";
                    CONSOLE_LOG_DEBUG("Policy {} explicitly denies action {} on {}", policy.name(), action, resource);
                    return result;
                }
            }
        }
    }

    // Second pass: check for explicit Allow
    for (const auto& policy : policies) {
        for (const auto& statement : policy.statements()) {
            if (statement.effect == models::PolicyStatement::Effect::Allow) {
                auto decision = evaluate_statement(statement, action, resource);
                if (decision == PolicyDecision::Allow) {
                    result.decision = PolicyDecision::Allow;
                    result.matched_policy = policy.name();
                    result.reason = "Explicit Allow in policy";
                    CONSOLE_LOG_DEBUG("Policy {} explicitly allows action {} on {}", policy.name(), action, resource);
                    return result;
                }
            }
        }
    }

    // No matching statement found - implicit deny
    result.reason = "No matching Allow statement found (implicit deny)";
    CONSOLE_LOG_DEBUG("No policy allows action {} on {}", action, resource);
    return result;
}

EvaluationResult
PolicyEvaluator::evaluate_user_access(const UserInfo& user_info,
                                      const String& action,
                                      const String& bucket_name,
                                      const String& object_key) const {
    EvaluationResult result;

    // Admin users bypass policy checks
    if (user_info.is_admin) {
        result.decision = PolicyDecision::Allow;
        result.reason = "Admin user - full access";
        return result;
    }

    // Parse user policies (JSON inline or policy names from DB)
    Vector<models::Policy> policies;
    auto db = ServiceLocator::database();
    for (const auto& policy_str : user_info.policies) {
        try {
            auto policy = models::Policy::from_json_string(policy_str);
            policies.push_back(policy);
        } catch (const std::exception& e) {
            if (db) {
                auto db_policy = db->get_policy(policy_str);
                if (db_policy) {
                    try {
                        policies.push_back(models::Policy::from_json_string(db_policy.value().document));
                    } catch (const std::exception& parse_error) {
                        CONSOLE_LOG_WARN("Failed to parse DB policy {}: {}", policy_str, parse_error.what());
                    }
                }
            }
            CONSOLE_LOG_DEBUG("Failed to parse inline policy {}: {}", policy_str, e.what());
        }
    }

    // Role baseline (hybrid role + overrides by policies).
    if (user_info.role == "admin") {
        result.decision = PolicyDecision::Allow;
        result.reason = "Role admin - full access";
        return result;
    }
    if (user_info.role == "editor") {
        if (action.rfind("s3:", 0) == 0) {
            result.decision = PolicyDecision::Allow;
            result.reason = "Role editor baseline allow";
        }
    }
    if (user_info.role == "viewer") {
        if (action == S3Actions::ListBucket || action == S3Actions::GetObject ||
            action == S3Actions::GetObjectTagging || action == S3Actions::GetObjectVersion ||
            action == S3Actions::GetObjectRetention || action == S3Actions::GetObjectLegalHold ||
            action == S3Actions::GetBucketTagging || action == S3Actions::GetBucketVersioning ||
            action == S3Actions::GetBucketPolicy || action == S3Actions::GetBucketLocation) {
            result.decision = PolicyDecision::Allow;
            result.reason = "Role viewer baseline allow";
        }
    }

    // If no policies, role baseline decides.
    if (policies.empty()) {
        if (result.decision == PolicyDecision::Allow) {
            return result;
        }
        result.decision = PolicyDecision::Deny;
        result.reason = "No matching role/policy allow";
        return result;
    }

    // Build resource ARN
    String resource = build_resource_arn(bucket_name, object_key);

    // Evaluate policies first, then fallback to role baseline.
    auto evaluated = evaluate(policies, action, resource);
    if (evaluated.decision == PolicyDecision::NoMatch && result.decision == PolicyDecision::Allow) {
        return result;
    }
    return evaluated;
}

String
PolicyEvaluator::build_resource_arn(const String& bucket_name, const String& object_key) {
    if (object_key.empty()) {
        return "arn:aws:s3:::" + bucket_name;
    }
    return "arn:aws:s3:::" + bucket_name + "/" + object_key;
}

bool
PolicyEvaluator::action_matches(const String& pattern, const String& action) {
    // Handle exact match
    if (pattern == action) {
        return true;
    }

    // Handle wildcards: "*" matches everything
    if (pattern == "*") {
        return true;
    }

    // Handle s3:* pattern
    if (pattern == "s3:*") {
        return action.find("s3:") == 0;
    }

    // Handle prefix wildcards like "s3:Get*"
    if (pattern.back() == '*') {
        String prefix = pattern.substr(0, pattern.length() - 1);
        return action.find(prefix) == 0;
    }

    // Convert pattern to regex for more complex wildcards
    // Replace * with .* and escape other special chars
    String regex_pattern;
    for (char c : pattern) {
        switch (c) {
            case '*':
                regex_pattern += ".*";
                break;
            case '?':
                regex_pattern += ".";
                break;
            case '.':
            case '[':
            case ']':
            case '(':
            case ')':
            case '{':
            case '}':
            case '^':
            case '$':
            case '+':
            case '|':
            case '\\':
                regex_pattern += '\\';
                regex_pattern += c;
                break;
            default:
                regex_pattern += c;
        }
    }

    try {
        std::regex re(regex_pattern);
        return std::regex_match(action, re);
    } catch (const std::regex_error&) {
        return pattern == action;
    }
}

bool
PolicyEvaluator::resource_matches(const String& pattern, const String& resource) {
    // Handle exact match
    if (pattern == resource) {
        return true;
    }

    // Handle wildcards: "*" matches everything
    if (pattern == "*") {
        return true;
    }

    // Handle arn:aws:s3:::* pattern (all buckets)
    if (pattern == "arn:aws:s3:::*") {
        return resource.find("arn:aws:s3:::") == 0;
    }

    // Handle bucket/* pattern (all objects in bucket)
    size_t wildcard_pos = pattern.find("/*");
    if (wildcard_pos != String::npos && pattern.back() == '*') {
        String prefix = pattern.substr(0, wildcard_pos + 1);
        return resource.find(prefix) == 0;
    }

    // Handle trailing wildcard for bucket prefix matching
    if (pattern.back() == '*') {
        String prefix = pattern.substr(0, pattern.length() - 1);
        return resource.find(prefix) == 0;
    }

    // Convert to regex for more complex patterns
    String regex_pattern;
    for (char c : pattern) {
        switch (c) {
            case '*':
                regex_pattern += ".*";
                break;
            case '?':
                regex_pattern += ".";
                break;
            case '.':
            case '[':
            case ']':
            case '(':
            case ')':
            case '{':
            case '}':
            case '^':
            case '$':
            case '+':
            case '|':
            case '\\':
                regex_pattern += '\\';
                regex_pattern += c;
                break;
            default:
                regex_pattern += c;
        }
    }

    try {
        std::regex re(regex_pattern);
        return std::regex_match(resource, re);
    } catch (const std::regex_error&) {
        return pattern == resource;
    }
}

PolicyDecision
PolicyEvaluator::evaluate_statement(const models::PolicyStatement& statement,
                                    const String& action,
                                    const String& resource) const {
    // Check if action matches any action in the statement
    bool action_match = false;
    for (const auto& pattern : statement.actions) {
        if (action_matches(pattern, action)) {
            action_match = true;
            break;
        }
    }

    if (!action_match) {
        return PolicyDecision::NoMatch;
    }

    // Check if resource matches any resource in the statement
    bool resource_match = false;
    for (const auto& pattern : statement.resources) {
        if (resource_matches(pattern, resource)) {
            resource_match = true;
            break;
        }
    }

    if (!resource_match) {
        return PolicyDecision::NoMatch;
    }

    // Both action and resource match - return effect
    return statement.effect == models::PolicyStatement::Effect::Allow ? PolicyDecision::Allow : PolicyDecision::Deny;
}

} // namespace console::services
