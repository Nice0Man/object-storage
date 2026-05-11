#include "console/services/LifecycleService.hpp"

#include "console/common/Logger.hpp"

#include <ctime>
#include <filesystem>
#include <fstream>

namespace console::services {

LifecycleService::LifecycleService(std::shared_ptr<clients::LocalStorageClient> storage_client)
    : storage_client_(std::move(storage_client)) {
    CONSOLE_LOG_INFO("LifecycleService initialized");
}

Result<void, String>
LifecycleService::set_bucket_lifecycle(const String& bucket_name, const LifecycleConfiguration& config) {
    try {
        // Get bucket info to ensure it exists
        auto bucket_result = storage_client_->get_bucket(bucket_name);
        if (!bucket_result) {
            return Result<void, String>(err_tag, "Bucket not found: " + bucket_name);
        }

        // Write lifecycle configuration
        // For local storage, we write to a file in the bucket directory
        // The path_manager is private, so we use a known path pattern
        auto buckets_root = std::filesystem::path("data/buckets") / bucket_name;
        auto lifecycle_path = buckets_root / "lifecycle.json";

        std::filesystem::create_directories(buckets_root);

        Json::StreamWriterBuilder writer;
        std::ofstream file(lifecycle_path);
        file << Json::writeString(writer, config.to_json());
        file.close();

        CONSOLE_LOG_INFO("Set lifecycle configuration for bucket {} with {} rules", bucket_name, config.rules.size());
        return Ok<String>();

    } catch (const std::exception& e) {
        return Result<void, String>(err_tag, String("Failed to set lifecycle configuration: ") + e.what());
    }
}

Result<LifecycleConfiguration, String>
LifecycleService::get_bucket_lifecycle(const String& bucket_name) {
    try {
        // Check bucket exists
        auto bucket_result = storage_client_->get_bucket(bucket_name);
        if (!bucket_result) {
            return Err<LifecycleConfiguration, String>("Bucket not found: " + bucket_name);
        }

        auto buckets_root = std::filesystem::path("data/buckets") / bucket_name;
        auto lifecycle_path = buckets_root / "lifecycle.json";

        if (!std::filesystem::exists(lifecycle_path)) {
            // Return empty configuration
            return Ok<LifecycleConfiguration, String>(LifecycleConfiguration{});
        }

        std::ifstream file(lifecycle_path);
        Json::Value json;
        Json::CharReaderBuilder reader;
        std::string errors;
        if (!Json::parseFromStream(reader, file, &json, &errors)) {
            return Err<LifecycleConfiguration, String>("Failed to parse lifecycle configuration");
        }

        return Ok<LifecycleConfiguration, String>(LifecycleConfiguration::from_json(json));

    } catch (const std::exception& e) {
        return Err<LifecycleConfiguration, String>(String("Failed to get lifecycle configuration: ") + e.what());
    }
}

Result<void, String>
LifecycleService::delete_bucket_lifecycle(const String& bucket_name) {
    try {
        auto bucket_result = storage_client_->get_bucket(bucket_name);
        if (!bucket_result) {
            return Result<void, String>(err_tag, "Bucket not found: " + bucket_name);
        }

        auto buckets_root = std::filesystem::path("data/buckets") / bucket_name;
        auto lifecycle_path = buckets_root / "lifecycle.json";

        if (std::filesystem::exists(lifecycle_path)) {
            std::filesystem::remove(lifecycle_path);
        }

        CONSOLE_LOG_INFO("Deleted lifecycle configuration for bucket {}", bucket_name);
        return Ok<String>();

    } catch (const std::exception& e) {
        return Result<void, String>(err_tag, String("Failed to delete lifecycle configuration: ") + e.what());
    }
}

bool
LifecycleService::object_matches_filter(const models::Object& object, const LifecycleFilter& filter) {
    // Check prefix
    if (!filter.prefix.empty() && object.key().find(filter.prefix) != 0) {
        return false;
    }

    // Check size filters
    if (filter.object_size_greater_than >= 0 && object.size() <= filter.object_size_greater_than) {
        return false;
    }
    if (filter.object_size_less_than >= 0 && object.size() >= filter.object_size_less_than) {
        return false;
    }

    // Check tags (would need object tags here, simplified for now)
    // In a complete implementation, we'd fetch object tags and compare

    return true;
}

Result<bool, String>
LifecycleService::apply_action(const String& bucket_name, const models::Object& object, const LifecycleAction& action) {
    try {
        auto now = std::chrono::system_clock::now();
        auto object_age = std::chrono::duration_cast<std::chrono::hours>(now - object.last_modified()).count() /
                          24; // Age in days

        switch (action.type) {
            case LifecycleAction::Type::Expiration: {
                if (object_age >= action.days) {
                    auto delete_result = storage_client_->delete_object(bucket_name, object.key());
                    if (!delete_result) {
                        return Err<bool, String>("Failed to delete object: " + delete_result.error());
                    }
                    CONSOLE_LOG_INFO(
                        "Lifecycle: Deleted expired object {}/{} (age {} days)", bucket_name, object.key(), object_age);
                    return Ok<bool, String>(true);
                }
                break;
            }

            case LifecycleAction::Type::AbortIncompleteMultipartUpload: {
                // Clean up incomplete multipart uploads older than specified days
                auto uploads_result = storage_client_->list_multipart_uploads(bucket_name, "");
                if (uploads_result) {
                    for (const auto& upload : uploads_result.value()) {
                        auto upload_age = (std::time(nullptr) - upload.initiated) / (24 * 3600);
                        if (upload_age >= action.days) {
                            storage_client_->abort_multipart_upload(bucket_name, upload.key, upload.upload_id);
                            CONSOLE_LOG_INFO("Lifecycle: Aborted incomplete upload {} (age {} days)",
                                             upload.upload_id,
                                             upload_age);
                        }
                    }
                }
                break;
            }

            case LifecycleAction::Type::NoncurrentVersionExpiration: {
                // Delete old versions
                auto versions_result = storage_client_->list_object_versions(bucket_name, object.key());
                if (versions_result) {
                    for (const auto& version : versions_result.value()) {
                        if (!version.is_latest) {
                            auto version_age = (std::time(nullptr) - version.last_modified) / (24 * 3600);
                            if (version_age >= action.noncurrent_days) {
                                storage_client_->delete_object_version(bucket_name, object.key(), version.version_id);
                                CONSOLE_LOG_INFO("Lifecycle: Deleted old version {} of {}/{}",
                                                 version.version_id,
                                                 bucket_name,
                                                 object.key());
                            }
                        }
                    }
                }
                break;
            }

            case LifecycleAction::Type::Transition: {
                // Storage class transition (not fully implemented for local storage)
                CONSOLE_LOG_DEBUG("Lifecycle: Transition action not implemented for local storage");
                break;
            }
        }

        return Ok<bool, String>(false);

    } catch (const std::exception& e) {
        return Err<bool, String>(String("Failed to apply lifecycle action: ") + e.what());
    }
}

Result<int, String>
LifecycleService::execute_lifecycle_rules(const String& bucket_name) {
    try {
        // Get lifecycle configuration
        auto config_result = get_bucket_lifecycle(bucket_name);
        if (!config_result) {
            return Err<int, String>(config_result.error());
        }

        const auto& config = config_result.value();
        if (config.rules.empty()) {
            CONSOLE_LOG_DEBUG("No lifecycle rules configured for bucket {}", bucket_name);
            return Ok<int, String>(0);
        }

        int affected_count = 0;

        // Get all objects in bucket
        clients::ListObjectsOptions options;
        options.recursive = true;
        options.max_keys = 10000;

        auto objects_result = storage_client_->list_objects(bucket_name, options);
        if (!objects_result) {
            return Err<int, String>("Failed to list objects: " + objects_result.error());
        }

        // Apply rules to each object
        for (const auto& object : objects_result.value().objects) {
            for (const auto& rule : config.rules) {
                if (!rule.is_enabled())
                    continue;

                if (object_matches_filter(object, rule.filter)) {
                    for (const auto& action : rule.actions) {
                        auto result = apply_action(bucket_name, object, action);
                        if (result && result.value()) {
                            affected_count++;
                        }
                    }
                }
            }
        }

        CONSOLE_LOG_INFO("Lifecycle: Processed bucket {} - {} objects affected", bucket_name, affected_count);
        return Ok<int, String>(affected_count);

    } catch (const std::exception& e) {
        return Err<int, String>(String("Failed to execute lifecycle rules: ") + e.what());
    }
}

Result<int, String>
LifecycleService::execute_all_lifecycle_rules() {
    try {
        // Get all buckets
        auto buckets_result = storage_client_->list_buckets();
        if (!buckets_result) {
            return Err<int, String>("Failed to list buckets: " + buckets_result.error());
        }

        int total_affected = 0;

        for (const auto& bucket : buckets_result.value()) {
            auto result = execute_lifecycle_rules(bucket.name());
            if (result) {
                total_affected += result.value();
            } else {
                CONSOLE_LOG_WARN("Failed to process lifecycle for bucket {}: {}", bucket.name(), result.error());
            }
        }

        CONSOLE_LOG_INFO("Lifecycle: Processed all buckets - {} total objects affected", total_affected);
        return Ok<int, String>(total_affected);

    } catch (const std::exception& e) {
        return Err<int, String>(String("Failed to execute all lifecycle rules: ") + e.what());
    }
}

} // namespace console::services
