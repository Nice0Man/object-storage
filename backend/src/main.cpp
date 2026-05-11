#include "console/cli/CommandLine.hpp"
#include "console/common/Config.hpp"
#include "console/common/Logger.hpp"
#include "console/common/ServerBootstrap.hpp"
#include "console/common/ServiceLocator.hpp"
#include "console/common/Types.hpp"

// Register controllers by including their headers
#include "console/api/AuthController.hpp"
#include "console/api/BucketsController.hpp"
#include "console/api/DashboardController.hpp"
#include "console/api/DocsController.hpp"
#include "console/api/HealthController.hpp"
#include "console/api/InfrastructureController.hpp"
#include "console/api/ObjectsController.hpp"
#include "console/api/PresignedController.hpp"
#include "console/api/StatsController.hpp"
#include "console/api/UsersController.hpp"

// Register filters and middleware by including their headers
#include "console/filters/AuthFilter.hpp"
#include "console/filters/CorsFilter.hpp"

// Services
#include "console/services/AuthService.hpp"
#include "console/services/BucketService.hpp"
#include "console/services/EncryptionService.hpp"
#include "console/services/InfrastructureService.hpp"
#include "console/services/LifecycleService.hpp"
#include "console/services/ObjectService.hpp"
#include "console/services/StatsCollector.hpp"
#include "console/services/UserService.hpp"

// Storage clients
#include "console/clients/LocalAdminClient.hpp"
#include "console/clients/LocalStorageClient.hpp"

// Storage managers
#include "console/storage/DatabaseManager.hpp"

// Websocket
#include "console/websocket/EventsController.hpp"

#include <drogon/drogon.h>

#include <atomic>
#include <csignal>
#include <filesystem>
#include <iostream>

using namespace console;
using namespace drogon;

namespace {

std::atomic<bool> shutdown_requested{false};

void
signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cerr << "\n[INFO] Received shutdown signal (" << signal << ")" << std::endl;
        shutdown_requested = true;

        try {
            app().quit();
        } catch (...) {}
    }
}

void
setup_signal_handlers() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
}

void
init_services() {
    CONSOLE_LOG_INFO("════════════════════════════════════════════════════════");
    CONSOLE_LOG_INFO("Initializing Services...");
    CONSOLE_LOG_INFO("════════════════════════════════════════════════════════");

    auto& config = Config::instance();
    auto storage_root = config.get<String>("storage.root_path").value_or("./storage");
    auto db_path = config.get<String>("database.path").value_or("./data/rocksdb");
    auto db_encryption_key = config.get<String>("database.encryption_key").value_or("");

    CONSOLE_LOG_INFO("  Storage root:  {}", storage_root);
    CONSOLE_LOG_INFO("  Database path: {}", db_path);
    CONSOLE_LOG_INFO("");

    std::filesystem::create_directories(storage_root);
    std::filesystem::create_directories(db_path);

    auto database = std::make_shared<storage::DatabaseManager>(db_path, db_encryption_key);
    auto init_result = database->initialize();
    if (!init_result) {
        CONSOLE_LOG_ERROR("Failed to initialize database: {}", init_result.error());
        std::exit(1);
    }
    ServiceLocator::set_database(database);
    CONSOLE_LOG_INFO("  ✓ DatabaseManager (RocksDB) initialized");

    auto storage_client = std::make_shared<clients::LocalStorageClient>(storage_root);
    ServiceLocator::set_storage_client(storage_client);
    CONSOLE_LOG_INFO("  ✓ LocalStorageClient initialized");

    auto admin_client = std::make_shared<clients::LocalAdminClient>(database);
    ServiceLocator::set_admin_client(admin_client);
    CONSOLE_LOG_INFO("  ✓ LocalAdminClient initialized");

    auto object_service = std::make_shared<services::ObjectService>(storage_client);
    ServiceLocator::set_object_service(object_service);
    CONSOLE_LOG_INFO("  ✓ ObjectService initialized");

    auto bucket_service = std::make_shared<services::BucketService>(storage_client);
    ServiceLocator::set_bucket_service(bucket_service);
    CONSOLE_LOG_INFO("  ✓ BucketService initialized");

    auto user_service = std::make_shared<services::UserService>(admin_client);
    ServiceLocator::set_user_service(user_service);
    CONSOLE_LOG_INFO("  ✓ UserService initialized");

    std::shared_ptr<Config> config_ptr(&Config::instance(), [](Config*) {});
    auto auth_service = std::make_shared<services::AuthService>(admin_client, config_ptr, database);
    ServiceLocator::set_auth_service(auth_service);
    CONSOLE_LOG_INFO("  ✓ AuthService initialized");

    auto stats_collector = std::make_shared<services::StatsCollector>(database);
    ServiceLocator::set_stats_collector(stats_collector);
    stats_collector->initialize_sample_data();
    stats_collector->start();
    CONSOLE_LOG_INFO("  ✓ StatsCollector initialized and started");

    auto infrastructure_service = std::make_shared<services::InfrastructureService>(database);
    ServiceLocator::set_infrastructure_service(infrastructure_service);
    CONSOLE_LOG_INFO("  ✓ InfrastructureService initialized");

    auto encryption_service = std::make_shared<services::EncryptionService>();
    ServiceLocator::set_encryption_service(encryption_service);
    CONSOLE_LOG_INFO("  ✓ EncryptionService initialized");

    auto lifecycle_service = std::make_shared<services::LifecycleService>(storage_client);
    ServiceLocator::set_lifecycle_service(lifecycle_service);
    CONSOLE_LOG_INFO("  ✓ LifecycleService initialized");

    CONSOLE_LOG_INFO("");
    CONSOLE_LOG_INFO("All services initialized successfully!");
    CONSOLE_LOG_INFO("════════════════════════════════════════════════════════");
}

} // anonymous namespace

int
main(int argc, char* argv[]) {
    try {
        if (cli::CommandLine::is_cli_command(argc, argv)) {
            cli::CommandLine cli;
            int result = cli.run(argc, argv);
            std::cout.flush();
            std::cerr.flush();
            std::_Exit(result);
        }

        print_banner();
        setup_signal_handlers();

        Logger::instance().init(LogLevel::Info, "logs", 10 * 1024 * 1024, 5);

        CONSOLE_LOG_INFO("Starting Object Storage Console...");

        auto& config = Config::instance();
        std::filesystem::path config_path = "config.json";

        for (int i = 1; i < argc; i++) {
            String arg = argv[i];
            if (arg == "--config" && i + 1 < argc) {
                config_path = argv[++i];
            } else if (arg.find("--config=") == 0) {
                config_path = arg.substr(9);
            }
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

        if (!config.is_valid()) {
            CONSOLE_LOG_ERROR("Invalid configuration:");
            for (const auto& error : config.validation_errors()) {
                CONSOLE_LOG_ERROR("  - {}", error);
            }
            return 1;
        }

        if (!validate_security_config()) {
            CONSOLE_LOG_ERROR("Security validation failed - refusing to start");
            return 1;
        }

        init_services();

        const auto& server_config = config.server();
        configure_drogon(server_config);

        setup_cors();
        setup_api_stats_collection();
        setup_http_access_logging();
        setup_background_tasks();
        register_routes();

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

        app().run();

        std::cerr << "\n[INFO] Server stopped" << std::endl;

        try {
            ServiceLocator::clear();
        } catch (...) {}

        try {
            Logger::instance().flush();
        } catch (...) {}

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        try {
            ServiceLocator::clear();
            Logger::instance().flush();
        } catch (...) {}
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        Logger::instance().flush();
        return 1;
    }
}
