#pragma once

#include "Types.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <sstream>

namespace console {

/**
 * @brief Application configuration manager
 *
 * Singleton class that manages application configuration
 * loaded from JSON file and environment variables.
 */
class Config {
  public:
    /**
     * @brief Get singleton instance
     */
    static Config& instance();

    /**
     * @brief Load configuration from file
     * @param config_path Path to configuration file
     * @return true if loaded successfully
     */
    bool load_from_file(const std::filesystem::path& config_path);

    /**
     * @brief Load configuration from JSON string
     * @param json_str JSON configuration string
     * @return true if loaded successfully
     */
    bool load_from_string(const String& json_str);

    /**
     * @brief Save current configuration to file
     * @param config_path Path to save configuration
     * @return true if saved successfully
     */
    bool save_to_file(const std::filesystem::path& config_path) const;

    /**
     * @brief Get server configuration
     */
    const ServerConfig& server() const { return server_config_; }
    ServerConfig& server() { return server_config_; }

    /**
     * @brief Get S3 configuration
     */
    const S3Config& s3() const { return s3_config_; }
    S3Config& s3() { return s3_config_; }

    /**
     * @brief Get authentication configuration
     */
    const AuthConfig& auth() const { return auth_config_; }
    AuthConfig& auth() { return auth_config_; }

    /**
     * @brief Get default admin configuration
     */
    const DefaultAdminConfig& default_admin() const { return default_admin_config_; }
    DefaultAdminConfig& default_admin() { return default_admin_config_; }

    /**
     * @brief Get configuration value by key
     * @param key Dot-separated key path (e.g., "server.port")
     * @return Optional value
     */
    template <typename T>
    Optional<T> get(const String& key) const;

    /**
     * @brief Set configuration value by key
     * @param key Dot-separated key path
     * @param value Value to set
     */
    template <typename T>
    void set(const String& key, const T& value);

    /**
     * @brief Check if configuration is valid
     */
    bool is_valid() const;

    /**
     * @brief Get validation errors
     */
    Vector<String> validation_errors() const;

    // Prevent copying
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

  public:
    Config() = default;

    void apply_defaults();
    void apply_environment_variables();
    bool validate();

    ServerConfig server_config_;
    S3Config s3_config_;
    AuthConfig auth_config_;
    DefaultAdminConfig default_admin_config_;
    nlohmann::json raw_json_;
    Vector<String> validation_errors_;

    // Helper to split string by delimiter
    static Vector<String> split_string(const String& str, char delimiter) {
        Vector<String> result;
        std::stringstream ss(str);
        String item;
        while (std::getline(ss, item, delimiter)) {
            // Trim whitespace
            item.erase(0, item.find_first_not_of(" \t\n\r"));
            item.erase(item.find_last_not_of(" \t\n\r") + 1);
            if (!item.empty()) {
                result.push_back(item);
            }
        }
        return result;
    }
    bool is_valid_{false};
};

// ============================================================================
// Template Implementations
// ============================================================================

template <typename T>
Optional<T>
Config::get(const String& key) const {
    try {
        auto keys = split_string(key, '.');
        nlohmann::json current = raw_json_;

        for (const auto& k : keys) {
            if (!current.contains(k)) {
                return std::nullopt;
            }
            current = current[k];
        }

        return current.get<T>();
    } catch (...) {
        return std::nullopt;
    }
}

template <typename T>
void
Config::set(const String& key, const T& value) {
    auto keys = split_string(key, '.');
    nlohmann::json* current = &raw_json_;

    for (size_t i = 0; i < keys.size() - 1; ++i) {
        if (!current->contains(keys[i])) {
            (*current)[keys[i]] = nlohmann::json::object();
        }
        current = &(*current)[keys[i]];
    }

    (*current)[keys.back()] = value;
}

} // namespace console
