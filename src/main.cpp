
//
#include "console/common/Config.hpp"
#include "console/common/Logger.hpp"
#include "console/common/Types.hpp"

#include <drogon/drogon.h>

#include <csignal>
#include <filesystem>
#include <iostream>

using namespace console;
using namespace drogon;

namespace {

// Signal handler for graceful shutdown
std::atomic<bool> shutdown_requested{false};

void
signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        CONSOLE_LOG_INFO("Received shutdown signal ({})", signal);
        shutdown_requested = true;
        app().quit();
    }
}

void
setup_signal_handlers() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
}

void
print_banner() {
    std::cout << R"(
    ╔═══════════════════════════════════════════════════════════╗
    ║                                                           ║
    ║     Object Storage Console                                ║
    ║     S3-Compatible Web Console                             ║
    ║                                                           ║
    ║     Version: 1.0.0                                        ║
    ║     Built with C++20 & Drogon Framework                   ║
    ║                                                           ║
    ╚═══════════════════════════════════════════════════════════╝
)" << std::endl;
}

void
configure_drogon(const ServerConfig& config) {
    // Set listening address and port
    app().addListener(config.host, config.port);

    // Set thread number
    app().setThreadNum(config.threads);

    // Enable session
    app().enableSession(3600); // 1 hour

    // Set document root
    auto web_root = std::filesystem::current_path() / "web-app" / "build";
    if (std::filesystem::exists(web_root)) {
        app().setDocumentRoot(web_root.string());
        CONSOLE_LOG_INFO("Serving static files from: {}", web_root.string());
    } else {
        CONSOLE_LOG_WARN("Web app build directory not found: {}", web_root.string());
    }

    // Configure SSL if enabled
    if (config.enable_ssl && !config.ssl_cert.empty() && !config.ssl_key.empty()) {
        app().setSSLFiles(config.ssl_cert, config.ssl_key);
        CONSOLE_LOG_INFO("SSL enabled with cert: {}", config.ssl_cert);
    }

    // Enable compression
    app().enableGzip(true);

    // Enable CORS for development
    app().registerBeginningAdvice([]() {
        app().registerPostHandlingAdvice([](const HttpRequestPtr& req, const HttpResponsePtr& resp) {
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS, PATCH");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
            resp->addHeader("Access-Control-Max-Age", "3600");
        });
    });

    // Set max body size (100MB for file uploads)
    app().setMaxBodySize(100 * 1024 * 1024);

    // Set client max body size
    app().setClientMaxBodySize(100 * 1024 * 1024);

    // Set idle connection timeout
    app().setIdleConnectionTimeout(60);

    // Enable running location
    app().enableRunAsDaemon(false);

    // Set log path and level
    if (!config.log_path.empty()) {
        std::filesystem::create_directories(config.log_path);
        app().setLogPath(config.log_path);
    }

    // Map log level
    if (config.log_level == "trace") {
        app().setLogLevel(trantor::Logger::kTrace);
    } else if (config.log_level == "debug") {
        app().setLogLevel(trantor::Logger::kDebug);
    } else if (config.log_level == "info") {
        app().setLogLevel(trantor::Logger::kInfo);
    } else if (config.log_level == "warn") {
        app().setLogLevel(trantor::Logger::kWarn);
    } else {
        app().setLogLevel(trantor::Logger::kError);
    }
}

void
register_routes() {
    CONSOLE_LOG_INFO("Registering API routes...");

    // Health check endpoint
    app().registerHandler("/api/v1/health",
                          [](const HttpRequestPtr&, std::function<void(const HttpResponsePtr&)>&& callback) {
                              Json::Value response;
                              response["status"] = "ok";
                              response["service"] = "object-storage-console";
                              response["version"] = "1.0.0";
                              response["timestamp"] = static_cast<Json::Int64>(std::time(nullptr));

                              auto resp = HttpResponse::newHttpJsonResponse(response);
                              callback(resp);
                          },
                          {Get});

    // API version endpoint
    app().registerHandler("/api/v1/version",
                          [](const HttpRequestPtr&, std::function<void(const HttpResponsePtr&)>&& callback) {
                              Json::Value response;
                              response["version"] = "1.0.0";
                              response["api_version"] = "v1";
                              response["build_time"] = __DATE__ " " __TIME__;

                              auto resp = HttpResponse::newHttpJsonResponse(response);
                              callback(resp);
                          },
                          {Get});

    // Catch-all for SPA routing
    app().setCustomErrorHandler(
        [](HttpStatusCode code, const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
            // For 404s on non-API routes, serve index.html (SPA fallback)
            if (code == k404NotFound && req->path().find("/api") == std::string::npos) {
                auto web_root = std::filesystem::current_path() / "web-app" / "build";
                auto index_path = web_root / "index.html";

                if (std::filesystem::exists(index_path)) {
                    auto resp = HttpResponse::newFileResponse(index_path.string());
                    callback(resp);
                    return;
                }
            }

            // Default error response
            Json::Value error;
            error["error"] = true;
            error["code"] = static_cast<int>(code);
            error["message"] = "Not Found";

            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(code);
            callback(resp);
        });

    CONSOLE_LOG_INFO("Routes registered successfully");
}

} // anonymous namespace

int
main(int argc, char* argv[]) {
    try {
        print_banner();

        // Setup signal handlers
        setup_signal_handlers();

        // Initialize logger
        Logger::instance().init(LogLevel::Info,
                                "logs",
                                10 * 1024 * 1024, // 10MB
                                5);

        CONSOLE_LOG_INFO("Starting OpenMaxIO Object Browser...");

        // Load configuration
        auto& config = Config::instance();
        std::filesystem::path config_path = "config.json";

        if (argc > 1) {
            config_path = argv[1];
        }

        if (std::filesystem::exists(config_path)) {
            CONSOLE_LOG_INFO("Loading configuration from: {}", config_path.string());
            if (!config.load_from_file(config_path)) {
                CONSOLE_LOG_ERROR("Failed to load configuration");
                for (const auto& error : config.validation_errors()) {
                    CONSOLE_LOG_ERROR("  - {}", error);
                }
                return 1;
            }
        } else {
            CONSOLE_LOG_WARN("Configuration file not found, using defaults");
            CONSOLE_LOG_WARN("Create {} to customize settings", config_path.string());
        }

        // Validate configuration
        if (!config.is_valid()) {
            CONSOLE_LOG_ERROR("Invalid configuration:");
            for (const auto& error : config.validation_errors()) {
                CONSOLE_LOG_ERROR("  - {}", error);
            }
            return 1;
        }

        // Configure Drogon
        const auto& server_config = config.server();
        configure_drogon(server_config);

        // Register routes
        register_routes();

        // Print server info
        CONSOLE_LOG_INFO("════════════════════════════════════════════════════════");
        CONSOLE_LOG_INFO("Server Configuration:");
        CONSOLE_LOG_INFO("  Host:        {}", server_config.host);
        CONSOLE_LOG_INFO("  Port:        {}", server_config.port);
        CONSOLE_LOG_INFO("  Threads:     {}", server_config.threads);
        CONSOLE_LOG_INFO("  SSL:         {}", server_config.enable_ssl ? "enabled" : "disabled");
        CONSOLE_LOG_INFO("  Log Level:   {}", server_config.log_level);
        CONSOLE_LOG_INFO("════════════════════════════════════════════════════════");

        std::cout << "\n";
        CONSOLE_LOG_INFO("Server starting...");
        CONSOLE_LOG_INFO("Access the console at: http{}://{}:{}",
                 server_config.enable_ssl ? "s" : "",
                 server_config.host == "0.0.0.0" ? "localhost" : server_config.host,
                 server_config.port);
        CONSOLE_LOG_INFO("Press Ctrl+C to stop\n");

        // Run the application
        app().run();

        CONSOLE_LOG_INFO("Server stopped");
        Logger::instance().flush();

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        CONSOLE_LOG_CRITICAL("Fatal error: {}", e.what());
        Logger::instance().flush();
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        CONSOLE_LOG_CRITICAL("Unknown fatal error occurred");
        Logger::instance().flush();
        return 1;
    }
}
