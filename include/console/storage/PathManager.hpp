#pragma once

#include "console/common/Types.hpp"

#include <filesystem>

namespace console::storage {

/**
 * @brief Manages file system paths for storage operations
 *
 * Provides utilities for constructing and validating paths
 * to buckets, objects, metadata files, etc.
 */
class PathManager {
  public:
    explicit PathManager(const std::filesystem::path& storage_root);

    // Bucket paths
    std::filesystem::path buckets_root() const;
    std::filesystem::path bucket_path(const String& bucket_name) const;
    std::filesystem::path bucket_metadata_path(const String& bucket_name) const;
    std::filesystem::path bucket_policy_path(const String& bucket_name) const;
    std::filesystem::path bucket_versioning_path(const String& bucket_name) const;
    std::filesystem::path bucket_tags_path(const String& bucket_name) const;

    // Object paths
    std::filesystem::path objects_root(const String& bucket_name) const;
    std::filesystem::path object_path(const String& bucket_name, const String& object_key) const;
    std::filesystem::path object_metadata_path(const String& bucket_name, const String& object_key) const;
    std::filesystem::path object_tags_path(const String& bucket_name, const String& object_key) const;

    // Versioning paths
    std::filesystem::path versions_root(const String& bucket_name) const;
    std::filesystem::path object_version_path(const String& bucket_name,
                                              const String& object_key,
                                              const String& version_id) const;

    // Multipart uploads paths
    std::filesystem::path multipart_root(const String& bucket_name) const;
    std::filesystem::path multipart_upload_path(const String& bucket_name, const String& upload_id) const;

    // Admin paths
    std::filesystem::path users_root() const;
    std::filesystem::path user_path(const String& access_key) const;
    std::filesystem::path groups_root() const;
    std::filesystem::path group_path(const String& group_name) const;
    std::filesystem::path policies_root() const;
    std::filesystem::path policy_path(const String& policy_name) const;

    // Config paths
    std::filesystem::path config_root() const;
    std::filesystem::path server_config_path() const;
    std::filesystem::path access_log_path() const;

    // Validation
    bool is_valid_bucket_name(const String& name) const;
    bool is_valid_object_key(const String& key) const;

    // Storage root
    const std::filesystem::path& storage_root() const { return storage_root_; }

  private:
    std::filesystem::path storage_root_;

    // Helper to sanitize object key for filesystem
    String sanitize_key(const String& key) const;
};

} // namespace console::storage
