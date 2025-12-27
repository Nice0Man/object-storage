#pragma once

#include "console/common/Types.hpp"
#include "console/models/Policy.hpp"

#include <memory>

namespace console::services {

/**
 * @brief S3 Action types for policy evaluation
 */
struct S3Actions {
    // Bucket actions
    static constexpr const char* ListBucket = "s3:ListBucket";
    static constexpr const char* CreateBucket = "s3:CreateBucket";
    static constexpr const char* DeleteBucket = "s3:DeleteBucket";
    static constexpr const char* GetBucketPolicy = "s3:GetBucketPolicy";
    static constexpr const char* PutBucketPolicy = "s3:PutBucketPolicy";
    static constexpr const char* GetBucketVersioning = "s3:GetBucketVersioning";
    static constexpr const char* PutBucketVersioning = "s3:PutBucketVersioning";
    static constexpr const char* GetBucketTagging = "s3:GetBucketTagging";
    static constexpr const char* PutBucketTagging = "s3:PutBucketTagging";
    static constexpr const char* GetBucketLocation = "s3:GetBucketLocation";

    // Object actions
    static constexpr const char* GetObject = "s3:GetObject";
    static constexpr const char* PutObject = "s3:PutObject";
    static constexpr const char* DeleteObject = "s3:DeleteObject";
    static constexpr const char* GetObjectTagging = "s3:GetObjectTagging";
    static constexpr const char* PutObjectTagging = "s3:PutObjectTagging";
    static constexpr const char* DeleteObjectTagging = "s3:DeleteObjectTagging";
    static constexpr const char* GetObjectVersion = "s3:GetObjectVersion";
    static constexpr const char* DeleteObjectVersion = "s3:DeleteObjectVersion";
    static constexpr const char* RestoreObject = "s3:RestoreObject";
    static constexpr const char* GetObjectRetention = "s3:GetObjectRetention";
    static constexpr const char* PutObjectRetention = "s3:PutObjectRetention";
    static constexpr const char* GetObjectLegalHold = "s3:GetObjectLegalHold";
    static constexpr const char* PutObjectLegalHold = "s3:PutObjectLegalHold";

    // Multipart upload actions
    static constexpr const char* ListMultipartUploadParts = "s3:ListMultipartUploadParts";
    static constexpr const char* AbortMultipartUpload = "s3:AbortMultipartUpload";
    static constexpr const char* ListBucketMultipartUploads = "s3:ListBucketMultipartUploads";

    // Wildcard for all actions
    static constexpr const char* All = "s3:*";
};

/**
 * @brief Result of policy evaluation
 */
enum class PolicyDecision {
    Allow,  // Explicitly allowed
    Deny,   // Explicitly denied
    NoMatch // No matching statement found
};

/**
 * @brief Policy evaluation result with details
 */
struct EvaluationResult {
    PolicyDecision decision{PolicyDecision::NoMatch};
    String matched_policy;
    String matched_statement;
    String reason;

    bool is_allowed() const { return decision == PolicyDecision::Allow; }
    bool is_denied() const { return decision == PolicyDecision::Deny; }
};

/**
 * @brief Policy Evaluator for IAM-style policy checking
 *
 * Evaluates policies against actions and resources following
 * AWS IAM policy evaluation logic:
 * 1. Explicit Deny overrides everything
 * 2. Explicit Allow required if no Deny
 * 3. Default is Deny (implicit)
 */
class PolicyEvaluator {
  public:
    PolicyEvaluator() = default;
    ~PolicyEvaluator() = default;

    /**
     * @brief Evaluate if an action is allowed on a resource
     *
     * @param policies List of policies to evaluate
     * @param action The S3 action being performed
     * @param resource The resource (bucket/object ARN)
     * @return EvaluationResult with decision and details
     */
    EvaluationResult evaluate(const Vector<models::Policy>& policies,
                              const String& action,
                              const String& resource) const;

    /**
     * @brief Evaluate policies for a user
     *
     * @param user_info User information including policies
     * @param action The S3 action being performed
     * @param bucket_name Bucket name
     * @param object_key Object key (optional)
     * @return EvaluationResult with decision and details
     */
    EvaluationResult evaluate_user_access(const UserInfo& user_info,
                                          const String& action,
                                          const String& bucket_name,
                                          const String& object_key = "") const;

    /**
     * @brief Build resource ARN from bucket and key
     *
     * @param bucket_name Bucket name
     * @param object_key Object key (empty for bucket-level)
     * @return ARN string like "arn:aws:s3:::bucket/key"
     */
    static String build_resource_arn(const String& bucket_name, const String& object_key = "");

    /**
     * @brief Check if action matches pattern (supports wildcards)
     *
     * @param pattern Action pattern (e.g., "s3:*", "s3:Get*")
     * @param action Actual action
     * @return true if matches
     */
    static bool action_matches(const String& pattern, const String& action);

    /**
     * @brief Check if resource matches pattern (supports wildcards)
     *
     * @param pattern Resource pattern (e.g., "arn:aws:s3:::*", "arn:aws:s3:::bucket/*")
     * @param resource Actual resource ARN
     * @return true if matches
     */
    static bool resource_matches(const String& pattern, const String& resource);

  private:
    /**
     * @brief Evaluate a single policy statement
     */
    PolicyDecision evaluate_statement(const models::PolicyStatement& statement,
                                      const String& action,
                                      const String& resource) const;
};

} // namespace console::services
