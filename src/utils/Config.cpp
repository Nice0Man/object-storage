
//
#include "console/common/Config.hpp"

#include <cstdlib>
#include <fstream>

namespace console {

Config&
Config::instance() {
    static Config config;
    return config;
}

bool
Config::load_from_file(const std::filesystem::path& config_path) {
    try {
        std::ifstream file(config_path);
        if (!file.is_open()) {
            validation_errors_.push_back("Failed to open config file: " + config_path.string());
            return false;
        }

        file >> raw_json_;
        apply_defaults();
        apply_environment_variables();

        // Load structured configs
        if (raw_json_.contains("server")) {
            auto& server = raw_json_["server"];
            server_config_.host = server.value("host", server_config_.host);
            server_config_.port = server.value("port", server_config_.port);
            server_config_.threads = server.value("threads", server_config_.threads);
            server_config_.enable_ssl = server.value("enable_ssl", server_config_.enable_ssl);
            server_config_.ssl_cert = server.value("ssl_cert", server_config_.ssl_cert);
            server_config_.ssl_key = server.value("ssl_key", server_config_.ssl_key);
            server_config_.log_level = server.value("log_level", server_config_.log_level);
            server_config_.log_path = server.value("log_path", server_config_.log_path);
        }

        if (raw_json_.contains("s3")) {
            auto& s3 = raw_json_["s3"];
            s3_config_.endpoint = s3.value("endpoint", s3_config_.endpoint);
            s3_config_.access_key = s3.value("access_key", s3_config_.access_key);
            s3_config_.secret_key = s3.value("secret_key", s3_config_.secret_key);
            s3_config_.region = s3.value("region", s3_config_.region);
            s3_config_.use_ssl = s3.value("use_ssl", s3_config_.use_ssl);
            s3_config_.timeout_ms = s3.value("timeout_ms", s3_config_.timeout_ms);
        }

        if (raw_json_.contains("auth")) {
            auto& auth = raw_json_["auth"];
            auth_config_.jwt_secret = auth.value("jwt_secret", auth_config_.jwt_secret);
            auth_config_.enable_ldap = auth.value("enable_ldap", auth_config_.enable_ldap);
            auth_config_.ldap_server = auth.value("ldap_server", auth_config_.ldap_server);
            auth_config_.ldap_port = auth.value("ldap_port", auth_config_.ldap_port);
        }

        is_valid_ = validate();
        return is_valid_;

    } catch (const std::exception& e) {
        validation_errors_.push_back(String("JSON parsing error: ") + e.what());
        is_valid_ = false;
        return false;
    }
}

bool
Config::load_from_string(const String& json_str) {
    try {
        raw_json_ = nlohmann::json::parse(json_str);
        apply_defaults();
        apply_environment_variables();
        is_valid_ = validate();
        return is_valid_;
    } catch (const std::exception& e) {
        validation_errors_.push_back(String("JSON parsing error: ") + e.what());
        is_valid_ = false;
        return false;
    }
}

bool
Config::save_to_file(const std::filesystem::path& config_path) const {
    try {
        std::ofstream file(config_path);
        if (!file.is_open()) {
            return false;
        }

        file << raw_json_.dump(4);
        return true;
    } catch (...) {
        return false;
    }
}

void
Config::apply_defaults() {
    // Server defaults
    if (!raw_json_.contains("server")) {
        raw_json_["server"] = nlohmann::json::object();
    }

    auto& server = raw_json_["server"];
    server["host"] = server.value("host", "0.0.0.0");
    server["port"] = server.value("port", 9090);
    server["threads"] = server.value("threads", static_cast<int>(std::thread::hardware_concurrency()));
    server["log_level"] = server.value("log_level", "info");
    server["log_path"] = server.value("log_path", "logs/");
}

void
Config::apply_environment_variables() {
    // Server
    if (const char* host = std::getenv("CONSOLE_HOST")) {
        server_config_.host = host;
    }
    if (const char* port = std::getenv("CONSOLE_PORT")) {
        server_config_.port = static_cast<uint16_t>(std::atoi(port));
    }

    // S3
    if (const char* endpoint = std::getenv("S3_ENDPOINT")) {
        s3_config_.endpoint = endpoint;
    }
    if (const char* access_key = std::getenv("S3_ACCESS_KEY")) {
        s3_config_.access_key = access_key;
    }
    if (const char* secret_key = std::getenv("S3_SECRET_KEY")) {
        s3_config_.secret_key = secret_key;
    }

    // Auth
    if (const char* jwt_secret = std::getenv("JWT_SECRET")) {
        auth_config_.jwt_secret = jwt_secret;
    }
}

bool
Config::validate() {
    validation_errors_.clear();

    // Validate server config
    if (server_config_.port == 0) {
        validation_errors_.push_back("Invalid server port");
    }

    // Validate S3 config
    if (s3_config_.endpoint.empty()) {
        validation_errors_.push_back("S3 endpoint is required");
    }

    // Validate auth config
    if (auth_config_.jwt_secret.empty()) {
        validation_errors_.push_back("JWT secret is required");
    }

    return validation_errors_.empty();
}

bool
Config::is_valid() const {
    return is_valid_;
}

Vector<String>
Config::validation_errors() const {
    return validation_errors_;
}

} // namespace console
