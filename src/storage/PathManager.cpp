#include "console/storage/PathManager.hpp"

#include "console/common/Logger.hpp"

#include <algorithm>
#include <regex>

namespace console::storage {

PathManager::PathManager(const std::filesystem::path& storage_root) : storage_root_(storage_root) {
    CONSOLE_LOG_INFO("PathManager initialized with root: {}", storage_root_.string());
}

// ============================================================================
// Bucket paths
// ============================================================================

std::filesystem::path
PathManager::buckets_root() const {
    return storage_root_ / "buckets";
}

std::filesystem::path
PathManager::bucket_path(const String& bucket_name) const {
    return buckets_root() / bucket_name;
}

std::filesystem::path
PathManager::bucket_metadata_path(const String& bucket_name) const {
    return bucket_path(bucket_name) / ".metadata.json";
}

std::filesystem::path
PathManager::bucket_policy_path(const String& bucket_name) const {
    return bucket_path(bucket_name) / ".policy.json";
}

std::filesystem::path
PathManager::bucket_versioning_path(const String& bucket_name) const {
    return bucket_path(bucket_name) / ".versioning.json";
}

std::filesystem::path
PathManager::bucket_tags_path(const String& bucket_name) const {
    return bucket_path(bucket_name) / ".tags.json";
}

// ============================================================================
// Object paths
// ============================================================================

std::filesystem::path
PathManager::objects_root(const String& bucket_name) const {
    return bucket_path(bucket_name) / "objects";
}

std::filesystem::path
PathManager::object_path(const String& bucket_name, const String& object_key) const {
    auto sanitized_key = sanitize_key(object_key);
    auto result_path = objects_root(bucket_name) / sanitized_key;

    // SECURITY: Verify the resulting path is still within the objects root
    // This is a defense-in-depth check against path traversal
    auto canonical_objects_root = std::filesystem::weakly_canonical(objects_root(bucket_name));
    auto canonical_result = std::filesystem::weakly_canonical(result_path);

    // Check that result path starts with objects root
    auto objects_root_str = canonical_objects_root.string();
    auto result_str = canonical_result.string();

    if (result_str.find(objects_root_str) != 0) {
        CONSOLE_LOG_ERROR("Security: Path traversal detected! Key '{}' resolved outside storage",
                          object_key.substr(0, 50));
        // Return a safe path that won't exist
        return objects_root(bucket_name) / "_security_violation_";
    }

    return result_path;
}

std::filesystem::path
PathManager::object_metadata_path(const String& bucket_name, const String& object_key) const {
    auto path = object_path(bucket_name, object_key);
    return path.parent_path() / (path.filename().string() + ".meta");
}

std::filesystem::path
PathManager::object_tags_path(const String& bucket_name, const String& object_key) const {
    auto path = object_path(bucket_name, object_key);
    return path.parent_path() / (path.filename().string() + ".tags");
}

// ============================================================================
// Versioning paths
// ============================================================================

std::filesystem::path
PathManager::versions_root(const String& bucket_name) const {
    return bucket_path(bucket_name) / "versions";
}

std::filesystem::path
PathManager::object_version_path(const String& bucket_name, const String& object_key, const String& version_id) const {
    auto sanitized = sanitize_key(object_key);
    return versions_root(bucket_name) / (sanitized + ".v" + version_id);
}

// ============================================================================
// Multipart uploads paths
// ============================================================================

std::filesystem::path
PathManager::multipart_root(const String& bucket_name) const {
    return bucket_path(bucket_name) / ".multipart";
}

std::filesystem::path
PathManager::multipart_upload_path(const String& bucket_name, const String& upload_id) const {
    return multipart_root(bucket_name) / upload_id;
}

// ============================================================================
// Admin paths
// ============================================================================

std::filesystem::path
PathManager::users_root() const {
    return storage_root_ / "users";
}

std::filesystem::path
PathManager::user_path(const String& access_key) const {
    return users_root() / (access_key + ".json");
}

std::filesystem::path
PathManager::groups_root() const {
    return storage_root_ / "groups";
}

std::filesystem::path
PathManager::group_path(const String& group_name) const {
    return groups_root() / (group_name + ".json");
}

std::filesystem::path
PathManager::policies_root() const {
    return storage_root_ / "policies";
}

std::filesystem::path
PathManager::policy_path(const String& policy_name) const {
    return policies_root() / (policy_name + ".json");
}

// ============================================================================
// Config paths
// ============================================================================

std::filesystem::path
PathManager::config_root() const {
    return storage_root_ / "config";
}

std::filesystem::path
PathManager::server_config_path() const {
    return config_root() / "server.json";
}

std::filesystem::path
PathManager::access_log_path() const {
    return config_root() / "access.log";
}

// ============================================================================
// Validation
// ============================================================================

bool
PathManager::is_valid_bucket_name(const String& name) const {
    // S3 bucket naming rules:
    // - 3-63 characters
    // - lowercase letters, numbers, dots, hyphens
    // - must start and end with letter or number
    // - no consecutive dots
    // - no IP address format

    if (name.length() < 3 || name.length() > 63) {
        return false;
    }

    // Check for valid characters
    std::regex valid_chars("^[a-z0-9][a-z0-9.-]*[a-z0-9]$");
    if (!std::regex_match(name, valid_chars)) {
        return false;
    }

    // Check for consecutive dots
    if (name.find("..") != String::npos) {
        return false;
    }

    // Check for IP address format
    std::regex ip_format("^[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+$");
    if (std::regex_match(name, ip_format)) {
        return false;
    }

    return true;
}

bool
PathManager::is_valid_object_key(const String& key) const {
    // S3 object key rules:
    // - 1-1024 characters
    // - UTF-8 characters
    // - Cannot be empty
    // - No path traversal sequences
    // - No control characters

    if (key.empty() || key.length() > 1024) {
        return false;
    }

    // Check for null bytes
    if (key.find('\0') != String::npos) {
        return false;
    }

    // SECURITY: Block path traversal attempts
    if (key.find("..") != String::npos) {
        CONSOLE_LOG_WARN("Security: Blocked path traversal attempt in object key: {}", key.substr(0, 50));
        return false;
    }

    // Block absolute paths
    if (!key.empty() && (key[0] == '/' || key[0] == '\\')) {
        CONSOLE_LOG_WARN("Security: Blocked absolute path in object key");
        return false;
    }

    // Check for control characters (0x00-0x1F except tab, newline, carriage return)
    for (unsigned char c : key) {
        if (c < 0x20 && c != '\t' && c != '\n' && c != '\r') {
            CONSOLE_LOG_WARN("Security: Blocked control character in object key");
            return false;
        }
    }

    return true;
}

// ============================================================================
// Helper methods
// ============================================================================

String
PathManager::sanitize_key(const String& key) const {
    // SECURITY: Comprehensive path sanitization to prevent path traversal attacks

    if (key.empty()) {
        return key;
    }

    String sanitized = key;

    // 1. Remove null bytes
    sanitized.erase(std::remove(sanitized.begin(), sanitized.end(), '\0'), sanitized.end());

    // 2. Normalize path separators to forward slash
    std::replace(sanitized.begin(), sanitized.end(), '\\', '/');

    // 3. Remove leading slashes (prevent absolute paths)
    while (!sanitized.empty() && sanitized[0] == '/') {
        sanitized.erase(0, 1);
    }

    // 4. Remove path traversal sequences (..)
    // Replace all ".." with empty string
    size_t pos;
    while ((pos = sanitized.find("..")) != String::npos) {
        sanitized.erase(pos, 2);
        // Also remove any resulting double slashes
        while (pos < sanitized.size() && sanitized[pos] == '/') {
            sanitized.erase(pos, 1);
        }
    }

    // 5. Collapse multiple consecutive slashes
    String result;
    result.reserve(sanitized.size());
    bool last_was_slash = false;
    for (char c : sanitized) {
        if (c == '/') {
            if (!last_was_slash) {
                result += c;
                last_was_slash = true;
            }
        } else {
            result += c;
            last_was_slash = false;
        }
    }
    sanitized = std::move(result);

    // 6. Remove trailing slashes
    while (!sanitized.empty() && sanitized.back() == '/') {
        sanitized.pop_back();
    }

    // 7. If empty after sanitization, use a safe default
    if (sanitized.empty()) {
        sanitized = "_invalid_key_";
    }

    return sanitized;
}

} // namespace console::storage
