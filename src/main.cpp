
//
#include "console/common/Config.hpp"
#include "console/common/Logger.hpp"
#include "console/common/Types.hpp"

// Register controllers by including their headers
#include "console/api/AuthController.hpp"
#include "console/api/BucketsController.hpp"
#include "console/api/DocsController.hpp"
#include "console/api/HealthController.hpp"
#include "console/api/ObjectsController.hpp"
#include "console/api/StatsController.hpp"
#include "console/api/UsersController.hpp"

// Register filters and middleware by including their headers
#include "console/filters/AuthFilter.hpp" // Global namespace filter for Drogon
#include "console/filters/CorsFilter.hpp"

// Services
#include "console/services/AuthService.hpp"
#include "console/services/BucketService.hpp"
#include "console/services/ObjectService.hpp"
#include "console/services/StatsCollector.hpp"
#include "console/services/UserService.hpp"

// Storage clients
#include "console/clients/LocalAdminClient.hpp"
#include "console/clients/LocalStorageClient.hpp"

// Storage managers
#include "console/storage/DatabaseManager.hpp"

// Service locator
#include "console/common/ServiceLocator.hpp"

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
        // Use std::cerr instead of logger to avoid use-after-free
        std::cerr << "\n[INFO] Received shutdown signal (" << signal << ")" << std::endl;
        shutdown_requested = true;

        // Gracefully quit drogon
        try {
            app().quit();
        } catch (...) {
            // Ignore exceptions during shutdown
        }
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

    // Set max body size (100MB for file uploads)
    app().setClientMaxBodySize(100 * 1024 * 1024);

    // Set client max body size
    app().setClientMaxBodySize(100 * 1024 * 1024);

    // Set idle connection timeout
    app().setIdleConnectionTimeout(60);

    // Register global filters
    CONSOLE_LOG_INFO("Registering filters...");
    // Filters are registered automatically through HttpFilter template inheritance

    // Enable running location
    // Daemon mode disabled by default

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
setup_cors() {
    CONSOLE_LOG_INFO("Setting up CORS...");

    // Handle preflight OPTIONS requests BEFORE routing
    app().registerPreRoutingAdvice(
        [](const HttpRequestPtr& req, AdviceCallback&& callback, AdviceChainCallback&& chain) {
            // Get origin from request
            std::string origin = req->getHeader("Origin");
            if (origin.empty()) {
                origin = "http://localhost:3000"; // Default for development
            }

            // Handle preflight OPTIONS
            if (req->method() == drogon::HttpMethod::Options) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k204NoContent);
                resp->addHeader("Access-Control-Allow-Origin", origin);
                resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS, PATCH");
                resp->addHeader("Access-Control-Allow-Headers",
                                "Content-Type, Authorization, X-Requested-With, Accept, Origin");
                resp->addHeader("Access-Control-Expose-Headers", "Content-Length, Content-Range, Content-Disposition");
                resp->addHeader("Access-Control-Max-Age", "86400");
                resp->addHeader("Access-Control-Allow-Credentials", "true");
                resp->addHeader("Vary", "Origin");

                CONSOLE_LOG_DEBUG("CORS preflight: {} from {}", req->path(), origin);
                callback(resp);
                return;
            }

            // Continue to next handler
            chain();
        });

    // Add CORS headers to all responses AFTER handling
    app().registerPostHandlingAdvice([](const HttpRequestPtr& req, const HttpResponsePtr& resp) {
        std::string origin = req->getHeader("Origin");
        if (origin.empty()) {
            origin = "http://localhost:3000";
        }

        resp->addHeader("Access-Control-Allow-Origin", origin);
        resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS, PATCH");
        resp->addHeader("Access-Control-Allow-Headers",
                        "Content-Type, Authorization, X-Requested-With, Accept, Origin");
        resp->addHeader("Access-Control-Expose-Headers", "Content-Length, Content-Range, Content-Disposition");
        resp->addHeader("Access-Control-Allow-Credentials", "true");
        resp->addHeader("Vary", "Origin");

        CONSOLE_LOG_DEBUG("CORS headers added: {} from {}", req->path(), origin);
    });

    CONSOLE_LOG_INFO("CORS configured successfully");
}

void
setup_api_stats_collection() {
    CONSOLE_LOG_INFO("Setting up API stats collection...");

    // Register post-handling advice to record API request statistics
    app().registerPostHandlingAdvice([](const HttpRequestPtr& req, const HttpResponsePtr& resp) {
        // Skip stats endpoints to avoid recursion and noise
        const std::string& path = req->path();
        if (path.find("/api/v1/stats") != std::string::npos || path.find("/api/v1/health") != std::string::npos ||
            path.find("/api/v1/version") != std::string::npos) {
            return;
        }

        // Only track API endpoints
        if (path.find("/api/") == std::string::npos) {
            return;
        }

        auto db = ServiceLocator::database();
        if (!db) {
            return;
        }

        storage::ApiRequestStats stats;
        stats.timestamp = std::time(nullptr);
        stats.endpoint = path;

        // Convert method to string
        switch (req->method()) {
            case drogon::HttpMethod::Get:
                stats.method = "GET";
                break;
            case drogon::HttpMethod::Post:
                stats.method = "POST";
                break;
            case drogon::HttpMethod::Put:
                stats.method = "PUT";
                break;
            case drogon::HttpMethod::Delete:
                stats.method = "DELETE";
                break;
            case drogon::HttpMethod::Patch:
                stats.method = "PATCH";
                break;
            case drogon::HttpMethod::Options:
                stats.method = "OPTIONS";
                break;
            case drogon::HttpMethod::Head:
                stats.method = "HEAD";
                break;
            default:
                stats.method = "UNKNOWN";
                break;
        }

        stats.status_code = static_cast<int>(resp->statusCode());
        stats.ip_address = req->peerAddr().toIp();
        stats.user_agent = req->getHeader("User-Agent");

        // Get user access key from request attributes if available
        try {
            auto user_info = req->attributes()->get<UserInfo>("user_info");
            stats.user_access_key = user_info.access_key;
        } catch (...) {
            // User not authenticated, leave empty
        }

        // Record stats asynchronously to not block response
        db->add_api_request_stat(stats);
    });

    CONSOLE_LOG_INFO("API stats collection configured successfully");
}

void
setup_background_tasks() {
    CONSOLE_LOG_INFO("Setting up background tasks...");

    // Task to cleanup old stats (runs daily)
    app().getLoop()->runEvery(86400.0, []() {
        auto db = ServiceLocator::database();
        if (!db)
            return;

        auto now = std::time(nullptr);
        auto week_ago = now - (7 * 24 * 3600); // 7 days ago

        CONSOLE_LOG_INFO("Running cleanup of old statistics data...");

        auto api_cleanup = db->cleanup_old_api_stats(week_ago);
        if (!api_cleanup) {
            CONSOLE_LOG_WARN("Failed to cleanup old API stats: {}", api_cleanup.error());
        }

        auto throughput_cleanup = db->cleanup_old_throughput_stats(week_ago);
        if (!throughput_cleanup) {
            CONSOLE_LOG_WARN("Failed to cleanup old throughput stats: {}", throughput_cleanup.error());
        }

        CONSOLE_LOG_INFO("Cleanup of old statistics completed");
    });

    // Run initial cleanup on startup (delayed by 60 seconds)
    app().getLoop()->runAfter(60.0, []() {
        auto db = ServiceLocator::database();
        if (!db)
            return;

        auto now = std::time(nullptr);
        auto week_ago = now - (7 * 24 * 3600);

        CONSOLE_LOG_DEBUG("Running initial cleanup of old statistics...");
        db->cleanup_old_api_stats(week_ago);
        db->cleanup_old_throughput_stats(week_ago);
    });

    CONSOLE_LOG_INFO("Background tasks configured successfully");
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
    app().setCustomErrorHandler([](HttpStatusCode code) -> HttpResponsePtr {
        // Default error response
        Json::Value error;
        error["error"] = true;
        error["code"] = static_cast<int>(code);
        error["message"] = "Not Found";

        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(code);
        return resp;
    });

    CONSOLE_LOG_INFO("Routes registered successfully");
}

void
init_services() {
    CONSOLE_LOG_INFO("════════════════════════════════════════════════════════");
    CONSOLE_LOG_INFO("Initializing Services...");
    CONSOLE_LOG_INFO("════════════════════════════════════════════════════════");

    // Initialize storage root path
    auto& config = Config::instance();
    auto storage_root = config.get<String>("storage.root_path").value_or("./storage");
    auto db_path = config.get<String>("database.path").value_or("console.db");

    CONSOLE_LOG_INFO("  Storage root:  {}", storage_root);
    CONSOLE_LOG_INFO("  Database path: {}", db_path);
    CONSOLE_LOG_INFO("");

    // Create storage directories
    std::filesystem::create_directories(storage_root);

    // Initialize database manager first (required by admin client)
    auto database = std::make_shared<storage::DatabaseManager>(db_path);
    auto init_result = database->initialize_schema();
    if (!init_result) {
        CONSOLE_LOG_ERROR("Failed to initialize database schema: {}", init_result.error());
        std::exit(1);
    }
    ServiceLocator::set_database(database);
    CONSOLE_LOG_INFO("  ✓ DatabaseManager initialized");

    // Initialize storage client
    auto storage_client = std::make_shared<clients::LocalStorageClient>(storage_root);
    ServiceLocator::set_storage_client(storage_client);
    CONSOLE_LOG_INFO("  ✓ LocalStorageClient initialized");

    // Initialize admin client
    auto admin_client = std::make_shared<clients::LocalAdminClient>(database);
    ServiceLocator::set_admin_client(admin_client);
    CONSOLE_LOG_INFO("  ✓ LocalAdminClient initialized");

    // Initialize services
    auto object_service = std::make_shared<services::ObjectService>(storage_client);
    ServiceLocator::set_object_service(object_service);
    CONSOLE_LOG_INFO("  ✓ ObjectService initialized");

    auto bucket_service = std::make_shared<services::BucketService>(storage_client);
    ServiceLocator::set_bucket_service(bucket_service);
    CONSOLE_LOG_INFO("  ✓ BucketService initialized");

    auto user_service = std::make_shared<services::UserService>(admin_client);
    ServiceLocator::set_user_service(user_service);
    CONSOLE_LOG_INFO("  ✓ UserService initialized");

    // Create config pointer for AuthService (with custom deleter to avoid deleting singleton)
    std::shared_ptr<Config> config_ptr(&Config::instance(), [](Config*) {});
    auto auth_service = std::make_shared<services::AuthService>(admin_client, config_ptr, database);
    ServiceLocator::set_auth_service(auth_service);
    CONSOLE_LOG_INFO("  ✓ AuthService initialized");

    // Initialize StatsCollector for background statistics collection
    auto stats_collector = std::make_shared<services::StatsCollector>(database);
    ServiceLocator::set_stats_collector(stats_collector);

    // Initialize sample data for dashboard (servers, drives, pools)
    stats_collector->initialize_sample_data();

    // Start background collection tasks
    stats_collector->start();
    CONSOLE_LOG_INFO("  ✓ StatsCollector initialized and started");

    CONSOLE_LOG_INFO("");
    CONSOLE_LOG_INFO("All services initialized successfully!");
    CONSOLE_LOG_INFO("════════════════════════════════════════════════════════");
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

        CONSOLE_LOG_INFO("Starting Object Storage Console...");

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

        // Initialize services (CRITICAL: must be before configure_drogon)
        init_services();

        // Configure Drogon
        const auto& server_config = config.server();
        configure_drogon(server_config);

        // Setup CORS (MUST be before routes)
        setup_cors();

        // Setup API stats collection
        setup_api_stats_collection();

        // Setup background tasks
        setup_background_tasks();

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

        // Cleanup - use std::cerr to avoid use-after-free with logger
        std::cerr << "\n[INFO] Server stopped" << std::endl;

        // Explicitly clear service locator before static destruction
        try {
            ServiceLocator::clear();
        } catch (...) {
            // Ignore cleanup errors
        }

        // Flush logger safely
        try {
            Logger::instance().flush();
        } catch (...) {
            // Ignore flush errors during shutdown
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        // Safe cleanup
        try {
            ServiceLocator::clear();
            Logger::instance().flush();
        } catch (...) {
            // Ignore cleanup errors
        }
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        Logger::instance().flush();
        return 1;
    }
}
