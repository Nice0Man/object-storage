#include "console/cli/CommandLine.hpp"

#include "console/clients/LocalAdminClient.hpp"
#include "console/clients/LocalStorageClient.hpp"
#include "console/common/Config.hpp"
#include "console/common/Logger.hpp"
#include "console/common/ServiceLocator.hpp"
#include "console/services/BucketService.hpp"
#include "console/services/UserService.hpp"
#include "console/storage/DatabaseManager.hpp"
#include "console/utils/PasswordHash.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <termios.h>
#include <unistd.h>

namespace console::cli {

namespace {

// ANSI color codes
const char* COLOR_RESET = "\033[0m";
const char* COLOR_GREEN = "\033[32m";
const char* COLOR_RED = "\033[31m";
const char* COLOR_YELLOW = "\033[33m";
const char* COLOR_CYAN = "\033[36m";
const char* COLOR_BOLD = "\033[1m";

void
print_success(const String& msg) {
    std::cout << COLOR_GREEN << "✓ " << COLOR_RESET << msg << std::endl;
}

void
print_error(const String& msg) {
    std::cerr << COLOR_RED << "✗ " << COLOR_RESET << msg << std::endl;
}

void
print_warning(const String& msg) {
    std::cout << COLOR_YELLOW << "⚠ " << COLOR_RESET << msg << std::endl;
}

void
print_info(const String& msg) {
    std::cout << COLOR_CYAN << "ℹ " << COLOR_RESET << msg << std::endl;
}

String
read_password(const String& prompt) {
    std::cout << prompt;

    // Disable echo
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~static_cast<tcflag_t>(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    String password;
    std::getline(std::cin, password);

    // Restore echo
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    std::cout << std::endl;

    return password;
}

String
format_size(int64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_idx = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024 && unit_idx < 4) {
        size /= 1024;
        unit_idx++;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit_idx];
    return oss.str();
}

String
format_timestamp(int64_t ts) {
    if (ts == 0)
        return "N/A";

    time_t time = static_cast<time_t>(ts);
    struct tm* tm_info = localtime(&time);

    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    return buffer;
}

} // anonymous namespace

// ============================================================================
// Constructor
// ============================================================================

CommandLine::CommandLine() {
    register_commands();
}

void
CommandLine::register_commands() {
    // User management
    commands_["user:create"] = {"user:create",
                                "Create a new user",
                                "user:create <username> [--admin] [--password <password>]",
                                [this](const auto& args) { return cmd_user_create(args); }};

    commands_["user:delete"] = {"user:delete",
                                "Delete a user",
                                "user:delete <username> [--force]",
                                [this](const auto& args) { return cmd_user_delete(args); }};

    commands_["user:list"] = {"user:list",
                              "List all users",
                              "user:list [--status <active|disabled>]",
                              [this](const auto& args) { return cmd_user_list(args); }};

    commands_["user:info"] = {"user:info", "Show user details", "user:info <username>", [this](const auto& args) {
                                  return cmd_user_info(args);
                              }};

    commands_["user:password"] = {"user:password",
                                  "Change user password",
                                  "user:password <username>",
                                  [this](const auto& args) { return cmd_user_password(args); }};

    // Bucket management
    commands_["bucket:create"] = {"bucket:create",
                                  "Create a new bucket",
                                  "bucket:create <bucket-name> [--region <region>]",
                                  [this](const auto& args) { return cmd_bucket_create(args); }};

    commands_["bucket:delete"] = {"bucket:delete",
                                  "Delete a bucket",
                                  "bucket:delete <bucket-name> [--force]",
                                  [this](const auto& args) { return cmd_bucket_delete(args); }};

    commands_["bucket:list"] = {"bucket:list", "List all buckets", "bucket:list", [this](const auto& args) {
                                    return cmd_bucket_list(args);
                                }};

    commands_["bucket:info"] = {"bucket:info",
                                "Show bucket details",
                                "bucket:info <bucket-name>",
                                [this](const auto& args) { return cmd_bucket_info(args); }};

    // Config commands
    commands_["config:init"] = {"config:init",
                                "Generate default configuration file",
                                "config:init [--output <path>] [--force]",
                                [this](const auto& args) { return cmd_config_init(args); }};

    commands_["config:validate"] = {"config:validate",
                                    "Validate configuration file",
                                    "config:validate [<config-path>]",
                                    [this](const auto& args) { return cmd_config_validate(args); }};

    commands_["config:show"] = {"config:show",
                                "Show current configuration",
                                "config:show [--secrets]",
                                [this](const auto& args) { return cmd_config_show(args); }};

    // Database commands
    commands_["db:migrate"] = {"db:migrate",
                               "Migrate database (SQLite <-> RocksDB)",
                               "db:migrate [--to <sqlite|rocksdb>]",
                               [this](const auto& args) { return cmd_db_migrate(args); }};

    commands_["db:backup"] = {"db:backup", "Backup database", "db:backup [--output <path>]", [this](const auto& args) {
                                  return cmd_db_backup(args);
                              }};
}

// ============================================================================
// Public Methods
// ============================================================================

bool
CommandLine::is_cli_command(int argc, char* argv[]) {
    if (argc < 2)
        return false;

    String arg = argv[1];

    // Check for known CLI commands
    if (arg.find(':') != String::npos)
        return true;
    if (arg == "help" || arg == "--help" || arg == "-h")
        return true;
    if (arg == "version" || arg == "--version" || arg == "-v")
        return true;

    return false;
}

int
CommandLine::run(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        return 0;
    }

    String command = argv[1];

    // Help command
    if (command == "help" || command == "--help" || command == "-h") {
        if (argc > 2) {
            print_command_help(argv[2]);
        } else {
            print_help();
        }
        return 0;
    }

    // Version command
    if (command == "version" || command == "--version" || command == "-v") {
        std::cout << "Object Storage Console v1.0.0" << std::endl;
        return 0;
    }

    // Find and execute command
    auto it = commands_.find(command);
    if (it == commands_.end()) {
        print_error("Unknown command: " + command);
        std::cout << "Run 'console help' for available commands." << std::endl;
        return 1;
    }

    cli_db_read_only_ = is_read_only_cli_command(command);

    // Collect arguments
    Vector<String> args;
    for (int i = 2; i < argc; i++) {
        args.push_back(argv[i]);
    }

    return it->second.handler(args);
}

bool
CommandLine::is_read_only_cli_command(const String& command) {
    static const std::set<String> read_only_commands = {
        "user:list",
        "user:info",
        "bucket:list",
        "bucket:info",
    };
    return read_only_commands.count(command) > 0;
}

void
CommandLine::print_help() {
    std::cout << COLOR_BOLD << "\nObject Storage Console - CLI Commands\n" << COLOR_RESET;
    std::cout << "======================================\n\n";

    std::cout << COLOR_CYAN << "User Management:\n" << COLOR_RESET;
    std::cout << "  user:create    Create a new user\n";
    std::cout << "  user:delete    Delete a user\n";
    std::cout << "  user:list      List all users\n";
    std::cout << "  user:info      Show user details\n";
    std::cout << "  user:password  Change user password\n\n";

    std::cout << COLOR_CYAN << "Bucket Management:\n" << COLOR_RESET;
    std::cout << "  bucket:create  Create a new bucket\n";
    std::cout << "  bucket:delete  Delete a bucket\n";
    std::cout << "  bucket:list    List all buckets\n";
    std::cout << "  bucket:info    Show bucket details\n\n";

    std::cout << COLOR_CYAN << "Configuration:\n" << COLOR_RESET;
    std::cout << "  config:init     Generate default configuration\n";
    std::cout << "  config:validate Validate configuration file\n";
    std::cout << "  config:show     Show current configuration\n\n";

    std::cout << COLOR_CYAN << "Database:\n" << COLOR_RESET;
    std::cout << "  db:migrate   Migrate database\n";
    std::cout << "  db:backup    Backup database\n\n";

    std::cout << "Run 'console help <command>' for detailed usage.\n";
    std::cout << "Run 'console' without arguments to start the web server.\n\n";
}

void
CommandLine::print_command_help(const String& command) {
    auto it = commands_.find(command);
    if (it == commands_.end()) {
        print_error("Unknown command: " + command);
        return;
    }

    const auto& cmd = it->second;
    std::cout << "\n" << COLOR_BOLD << cmd.name << COLOR_RESET << " - " << cmd.description << "\n\n";
    std::cout << "Usage: console " << cmd.usage << "\n\n";
}

// ============================================================================
// Service Initialization
// ============================================================================

bool
CommandLine::init_services_for_cli() {
    if (services_initialized_)
        return true;

    // Load config
    auto& config = Config::instance();
    std::filesystem::path config_path = "config.json";

    if (std::filesystem::exists(config_path)) {
        if (!config.load_from_file(config_path)) {
            print_error("Failed to load configuration from " + config_path.string());
            return false;
        }
    }

    // Initialize database - use absolute path from config or relative to project root
    String db_path = config.get<String>("database.path").value_or("./data/rocksdb");
    String db_encryption_key = config.get<String>("database.encryption_key").value_or("");
    std::filesystem::path db_file_path(db_path);

    if (db_file_path.is_relative()) {
        auto cwd = std::filesystem::current_path();
        // Go up from build directory if we're running from there
        if (cwd.filename() == "build" || cwd.string().find("/build") != std::string::npos) {
            db_file_path = cwd.parent_path() / db_path;
        } else {
            db_file_path = cwd / db_path;
        }
    }

    std::filesystem::create_directories(db_file_path.parent_path());
    db_path = db_file_path.string();

    const size_t thread_pool_size = cli_db_read_only_ ? 0 : 4;
    auto database = std::make_shared<storage::DatabaseManager>(db_path, db_encryption_key, thread_pool_size);
    if (auto result = database->initialize(cli_db_read_only_); !result) {
        const auto& err = result.error();
        if (!cli_db_read_only_ &&
            (err.find("LOCK") != String::npos || err.find("Resource temporarily unavailable") != String::npos)) {
            print_error("Failed to initialize database: " + err);
            print_warning("The database is locked by a running server. Stop the server first, or use read-only "
                          "commands (user:list, user:info, bucket:list, bucket:info).");
        } else {
            print_error("Failed to initialize database: " + err);
        }
        return false;
    }
    if (cli_db_read_only_) {
        print_info("Opened database in read-only mode (server may be running)");
    }
    ServiceLocator::set_database(database);

    // Initialize storage client - use absolute path from config or relative to executable
    String storage_root = config.get<String>("storage.root_path").value_or("storage");

    // If relative path, make it relative to the config file location or current working directory
    std::filesystem::path storage_path(storage_root);
    if (storage_path.is_relative()) {
        // Check if config exists and use its directory as base
        std::filesystem::path config_dir = std::filesystem::path("config.json").parent_path();
        if (config_dir.empty()) {
            config_dir = std::filesystem::current_path();
        }
        // Go up from build directory if we're running from there
        auto cwd = std::filesystem::current_path();
        if (cwd.filename() == "build" || cwd.string().find("/build") != std::string::npos) {
            storage_path = cwd.parent_path() / storage_root;
        } else {
            storage_path = cwd / storage_root;
        }
    }

    auto storage_client = std::make_shared<clients::LocalStorageClient>(storage_path.string());
    ServiceLocator::set_storage_client(storage_client);

    // Initialize admin client (wraps database for user operations)
    auto admin_client = std::make_shared<clients::LocalAdminClient>(database);
    ServiceLocator::set_admin_client(admin_client);

    // Initialize user service with admin client
    auto user_service = std::make_shared<services::UserService>(admin_client);
    ServiceLocator::set_user_service(user_service);

    // Initialize bucket service
    auto bucket_service = std::make_shared<services::BucketService>(storage_client);
    ServiceLocator::set_bucket_service(bucket_service);

    services_initialized_ = true;
    return true;
}

// ============================================================================
// User Management Commands
// ============================================================================

int
CommandLine::cmd_user_create(const Vector<String>& args) {
    if (args.empty()) {
        print_error("Username required");
        std::cout << "Usage: console user:create <username> [--admin] [--password <password>]\n";
        return 1;
    }

    String username = args[0];
    String password;
    bool is_admin = false;

    // Parse options
    for (size_t i = 1; i < args.size(); i++) {
        if (args[i] == "--admin") {
            is_admin = true;
        } else if (args[i] == "--password" && i + 1 < args.size()) {
            password = args[++i];
        }
    }

    // Prompt for password if not provided
    if (password.empty()) {
        password = read_password("Enter password: ");
        String confirm = read_password("Confirm password: ");

        if (password != confirm) {
            print_error("Passwords do not match");
            return 1;
        }
    }

    // Validate password
    if (password.length() < 16) {
        print_error("Password must be at least 16 characters");
        return 1;
    }

    // Initialize services
    if (!init_services_for_cli())
        return 1;

    // Create user in database
    auto database = ServiceLocator::database();

    // Check if user exists
    auto exists = database->user_exists(username);
    if (exists.is_ok() && exists.value()) {
        print_error("User already exists: " + username);
        return 1;
    }

    // Create user
    storage::DbUser user;
    user.access_key = username;
    user.secret_key = utils::PasswordHash::hash(password);
    user.account_name = username;
    user.status = "active";
    user.is_admin = is_admin;
    user.created_at =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    user.updated_at = user.created_at;

    auto result = database->create_user(user);
    if (!result) {
        print_error("Failed to create user: " + result.error());
        return 1;
    }

    print_success("User created: " + username);
    if (is_admin) {
        print_info("User has admin privileges");
    }

    return 0;
}

int
CommandLine::cmd_user_delete(const Vector<String>& args) {
    if (args.empty()) {
        print_error("Username required");
        std::cout << "Usage: console user:delete <username> [--force]\n";
        return 1;
    }

    String username = args[0];
    bool force = false;

    for (size_t i = 1; i < args.size(); i++) {
        if (args[i] == "--force" || args[i] == "-f") {
            force = true;
        }
    }

    // Confirm deletion
    if (!force) {
        std::cout << "Delete user '" << username << "'? (y/N): ";
        String confirm;
        std::getline(std::cin, confirm);

        if (confirm != "y" && confirm != "Y") {
            print_info("Cancelled");
            return 0;
        }
    }

    // Initialize services
    if (!init_services_for_cli())
        return 1;

    auto database = ServiceLocator::database();

    auto result = database->delete_user(username);
    if (!result) {
        print_error("Failed to delete user: " + result.error());
        return 1;
    }

    print_success("User deleted: " + username);
    return 0;
}

int
CommandLine::cmd_user_list(const Vector<String>& args) {
    String status_filter;

    for (size_t i = 0; i < args.size(); i++) {
        if (args[i] == "--status" && i + 1 < args.size()) {
            status_filter = args[++i];
        }
    }

    // Initialize services
    if (!init_services_for_cli())
        return 1;

    auto database = ServiceLocator::database();
    auto result = database->list_users(status_filter);

    if (!result) {
        print_error("Failed to list users: " + result.error());
        return 1;
    }

    const auto& users = result.value();

    if (users.empty()) {
        print_info("No users found");
        return 0;
    }

    // Print table header
    std::cout << "\n";
    std::cout << std::left << std::setw(20) << "USERNAME" << std::setw(25) << "ACCOUNT NAME" << std::setw(10)
              << "STATUS" << std::setw(8) << "ADMIN" << std::setw(20) << "CREATED"
              << "\n";
    std::cout << std::string(83, '-') << "\n";

    for (const auto& user : users) {
        std::cout << std::left << std::setw(20) << user.access_key << std::setw(25) << user.account_name
                  << std::setw(10) << user.status << std::setw(8) << (user.is_admin ? "Yes" : "No") << std::setw(20)
                  << format_timestamp(user.created_at) << "\n";
    }

    std::cout << "\nTotal: " << users.size() << " user(s)\n";
    return 0;
}

int
CommandLine::cmd_user_info(const Vector<String>& args) {
    if (args.empty()) {
        print_error("Username required");
        std::cout << "Usage: console user:info <username>\n";
        return 1;
    }

    String username = args[0];

    // Initialize services
    if (!init_services_for_cli())
        return 1;

    auto database = ServiceLocator::database();
    auto result = database->get_user(username);

    if (!result) {
        print_error("User not found: " + username);
        return 1;
    }

    const auto& user = result.value();

    std::cout << "\n" << COLOR_BOLD << "User: " << user.access_key << COLOR_RESET << "\n\n";
    std::cout << "  Account Name: " << user.account_name << "\n";
    std::cout << "  Status:       " << user.status << "\n";
    std::cout << "  Admin:        " << (user.is_admin ? "Yes" : "No") << "\n";
    std::cout << "  Created:      " << format_timestamp(user.created_at) << "\n";
    std::cout << "  Updated:      " << format_timestamp(user.updated_at) << "\n";

    // Get user groups
    auto groups = database->get_user_groups(username);
    if (groups.is_ok() && !groups.value().empty()) {
        std::cout << "  Groups:       ";
        for (size_t i = 0; i < groups.value().size(); i++) {
            if (i > 0)
                std::cout << ", ";
            std::cout << groups.value()[i];
        }
        std::cout << "\n";
    }

    std::cout << "\n";
    return 0;
}

int
CommandLine::cmd_user_password(const Vector<String>& args) {
    if (args.empty()) {
        print_error("Username required");
        std::cout << "Usage: console user:password <username>\n";
        return 1;
    }

    String username = args[0];

    // Initialize services
    if (!init_services_for_cli())
        return 1;

    auto database = ServiceLocator::database();
    auto result = database->get_user(username);

    if (!result) {
        print_error("User not found: " + username);
        return 1;
    }

    auto user = result.value();

    // Read new password
    String password = read_password("Enter new password: ");
    String confirm = read_password("Confirm new password: ");

    if (password != confirm) {
        print_error("Passwords do not match");
        return 1;
    }

    if (password.length() < 16) {
        print_error("Password must be at least 16 characters");
        return 1;
    }

    // Update password
    user.secret_key = utils::PasswordHash::hash(password);
    user.updated_at =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    auto update_result = database->update_user(user);
    if (!update_result) {
        print_error("Failed to update password: " + update_result.error());
        return 1;
    }

    print_success("Password updated for: " + username);
    return 0;
}

// ============================================================================
// Bucket Management Commands
// ============================================================================

int
CommandLine::cmd_bucket_create(const Vector<String>& args) {
    if (args.empty()) {
        print_error("Bucket name required");
        std::cout << "Usage: console bucket:create <bucket-name> [--region <region>]\n";
        return 1;
    }

    String bucket_name = args[0];
    String region = "us-east-1";

    for (size_t i = 1; i < args.size(); i++) {
        if (args[i] == "--region" && i + 1 < args.size()) {
            region = args[++i];
        }
    }

    // Initialize services
    if (!init_services_for_cli())
        return 1;

    auto storage_client = ServiceLocator::storage_client();
    auto result = storage_client->create_bucket(bucket_name, region);

    if (!result) {
        print_error("Failed to create bucket: " + result.error());
        return 1;
    }

    print_success("Bucket created: " + bucket_name);
    print_info("Region: " + region);
    return 0;
}

int
CommandLine::cmd_bucket_delete(const Vector<String>& args) {
    if (args.empty()) {
        print_error("Bucket name required");
        std::cout << "Usage: console bucket:delete <bucket-name> [--force]\n";
        return 1;
    }

    String bucket_name = args[0];
    bool force = false;

    for (size_t i = 1; i < args.size(); i++) {
        if (args[i] == "--force" || args[i] == "-f") {
            force = true;
        }
    }

    // Confirm deletion
    if (!force) {
        std::cout << "Delete bucket '" << bucket_name << "'? This will delete all objects! (y/N): ";
        String confirm;
        std::getline(std::cin, confirm);

        if (confirm != "y" && confirm != "Y") {
            print_info("Cancelled");
            return 0;
        }
    }

    // Initialize services
    if (!init_services_for_cli())
        return 1;

    auto storage_client = ServiceLocator::storage_client();
    auto result = storage_client->delete_bucket(bucket_name);

    if (!result) {
        print_error("Failed to delete bucket: " + result.error());
        return 1;
    }

    print_success("Bucket deleted: " + bucket_name);
    return 0;
}

int
CommandLine::cmd_bucket_list(const Vector<String>& /* args */) {
    // Initialize services
    if (!init_services_for_cli())
        return 1;

    auto storage_client = ServiceLocator::storage_client();
    auto result = storage_client->list_buckets();

    if (!result) {
        print_error("Failed to list buckets: " + result.error());
        return 1;
    }

    const auto& buckets = result.value();

    if (buckets.empty()) {
        print_info("No buckets found");
        return 0;
    }

    // Print table
    std::cout << "\n";
    std::cout << std::left << std::setw(30) << "BUCKET NAME" << std::setw(15) << "REGION" << std::setw(12) << "OBJECTS"
              << std::setw(15) << "SIZE" << std::setw(12) << "VERSIONING"
              << "\n";
    std::cout << std::string(84, '-') << "\n";

    for (const auto& bucket : buckets) {
        std::cout << std::left << std::setw(30) << bucket.name() << std::setw(15) << bucket.region() << std::setw(12)
                  << bucket.object_count() << std::setw(15) << format_size(bucket.size_bytes()) << std::setw(12)
                  << (bucket.versioning_enabled() ? "Enabled" : "Disabled") << "\n";
    }

    std::cout << "\nTotal: " << buckets.size() << " bucket(s)\n";
    return 0;
}

int
CommandLine::cmd_bucket_info(const Vector<String>& args) {
    if (args.empty()) {
        print_error("Bucket name required");
        std::cout << "Usage: console bucket:info <bucket-name>\n";
        return 1;
    }

    String bucket_name = args[0];

    // Initialize services
    if (!init_services_for_cli())
        return 1;

    auto storage_client = ServiceLocator::storage_client();
    auto result = storage_client->get_bucket(bucket_name);

    if (!result) {
        print_error("Bucket not found: " + bucket_name);
        return 1;
    }

    const auto& bucket = result.value();

    std::cout << "\n" << COLOR_BOLD << "Bucket: " << bucket.name() << COLOR_RESET << "\n\n";
    std::cout << "  Region:       " << bucket.region() << "\n";
    std::cout << "  Objects:      " << bucket.object_count() << "\n";
    std::cout << "  Size:         " << format_size(bucket.size_bytes()) << "\n";
    std::cout << "  Versioning:   " << (bucket.versioning_enabled() ? "Enabled" : "Disabled") << "\n";
    std::cout << "  Encryption:   " << (bucket.encryption_enabled() ? bucket.encryption_type() : "None") << "\n";

    // Get tags
    auto tags_result = storage_client->get_bucket_tags(bucket_name);
    if (tags_result.is_ok() && !tags_result.value().empty()) {
        std::cout << "  Tags:\n";
        for (const auto& [key, value] : tags_result.value()) {
            std::cout << "    " << key << ": " << value << "\n";
        }
    }

    std::cout << "\n";
    return 0;
}

// ============================================================================
// Config Commands
// ============================================================================

int
CommandLine::cmd_config_init(const Vector<String>& args) {
    String output_path = "config.json";
    bool force = false;

    for (size_t i = 0; i < args.size(); i++) {
        if (args[i] == "--output" && i + 1 < args.size()) {
            output_path = args[++i];
        } else if (args[i] == "--force" || args[i] == "-f") {
            force = true;
        }
    }

    // Check if file exists
    if (std::filesystem::exists(output_path) && !force) {
        print_error("Configuration file already exists: " + output_path);
        std::cout << "Use --force to overwrite\n";
        return 1;
    }

    // Generate config
    String config_template = R"({
  "server": {
    "host": "0.0.0.0",
    "port": 9090,
    "threads": 4,
    "enable_ssl": false,
    "cert_path": "",
    "key_path": "",
    "log_level": "info",
    "log_path": "logs"
  },
  "storage": {
    "root_path": "storage",
    "type": "local"
  },
  "database": {
    "path": "data/console.db",
    "type": "sqlite"
  },
  "auth": {
    "jwt_secret": "CHANGE_THIS_TO_A_SECURE_RANDOM_STRING_AT_LEAST_32_CHARS",
    "token_expiry_hours": 24
  },
  "default_admin": {
    "enabled": true,
    "username": "admin",
    "password": "CHANGE_THIS_PASSWORD",
    "account_name": "Administrator"
  },
  "presigned_url": {
    "secret_key": "CHANGE_THIS_TO_A_SECURE_RANDOM_STRING_AT_LEAST_32_CHARS"
  },
  "cors": {
    "allowed_origins": ["http://localhost:3000"]
  }
}
)";

    std::ofstream file(output_path);
    if (!file) {
        print_error("Failed to create file: " + output_path);
        return 1;
    }

    file << config_template;
    file.close();

    print_success("Configuration file created: " + output_path);
    print_warning("IMPORTANT: Change the following values before use:");
    std::cout << "  - auth.jwt_secret\n";
    std::cout << "  - default_admin.password\n";
    std::cout << "  - presigned_url.secret_key\n";

    return 0;
}

int
CommandLine::cmd_config_validate(const Vector<String>& args) {
    String config_path = "config.json";

    if (!args.empty()) {
        config_path = args[0];
    }

    if (!std::filesystem::exists(config_path)) {
        print_error("Configuration file not found: " + config_path);
        return 1;
    }

    auto& config = Config::instance();

    if (!config.load_from_file(config_path)) {
        print_error("Failed to parse configuration");
        for (const auto& error : config.validation_errors()) {
            std::cout << "  - " << error << "\n";
        }
        return 1;
    }

    if (!config.is_valid()) {
        print_warning("Configuration has issues:");
        for (const auto& error : config.validation_errors()) {
            std::cout << "  - " << error << "\n";
        }
        return 1;
    }

    print_success("Configuration is valid: " + config_path);
    return 0;
}

int
CommandLine::cmd_config_show(const Vector<String>& args) {
    bool show_secrets = false;

    for (const auto& arg : args) {
        if (arg == "--secrets") {
            show_secrets = true;
        }
    }

    String config_path = "config.json";

    if (!std::filesystem::exists(config_path)) {
        print_error("Configuration file not found: " + config_path);
        return 1;
    }

    auto& config = Config::instance();
    config.load_from_file(config_path);

    const auto& server = config.server();

    std::cout << "\n" << COLOR_BOLD << "Current Configuration" << COLOR_RESET << "\n\n";

    std::cout << COLOR_CYAN << "Server:\n" << COLOR_RESET;
    std::cout << "  Host:     " << server.host << "\n";
    std::cout << "  Port:     " << server.port << "\n";
    std::cout << "  Threads:  " << server.threads << "\n";
    std::cout << "  SSL:      " << (server.enable_ssl ? "Enabled" : "Disabled") << "\n";
    std::cout << "  Log:      " << server.log_level << "\n\n";

    std::cout << COLOR_CYAN << "Storage:\n" << COLOR_RESET;
    std::cout << "  Root:     " << config.get<String>("storage.root_path").value_or("storage") << "\n\n";

    std::cout << COLOR_CYAN << "Database:\n" << COLOR_RESET;
    std::cout << "  Path:     " << config.get<String>("database.path").value_or("data/console.db") << "\n\n";

    std::cout << COLOR_CYAN << "Auth:\n" << COLOR_RESET;
    if (show_secrets) {
        std::cout << "  JWT Secret: " << config.auth().jwt_secret << "\n";
    } else {
        std::cout << "  JWT Secret: ******* (use --secrets to show)\n";
    }
    std::cout << "  Token Expiry: " << config.auth().token_expiry << " hours\n\n";

    return 0;
}

// ============================================================================
// Database Commands
// ============================================================================

int
CommandLine::cmd_db_migrate(const Vector<String>& /* args */) {
    print_info("Database migration");

#ifdef HAS_ROCKSDB
    print_info("RocksDB support is enabled");
    // TODO: Implement SQLite to RocksDB migration
    print_warning("Migration not yet implemented");
#else
    print_warning("RocksDB support not enabled. Build with -DENABLE_ROCKSDB=ON");
#endif

    return 0;
}

int
CommandLine::cmd_db_backup(const Vector<String>& args) {
    String output_path = "backup";

    for (size_t i = 0; i < args.size(); i++) {
        if (args[i] == "--output" && i + 1 < args.size()) {
            output_path = args[++i];
        }
    }

    // Initialize services
    if (!init_services_for_cli())
        return 1;

    String db_path = Config::instance().get<String>("database.path").value_or("data/console.db");

    // Create backup directory
    std::filesystem::create_directories(output_path);

    // Copy database file
    String backup_file = output_path + "/console_" +
                         std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".db";

    try {
        std::filesystem::copy_file(db_path, backup_file);
        print_success("Database backed up to: " + backup_file);
    } catch (const std::exception& e) {
        print_error("Backup failed: " + String(e.what()));
        return 1;
    }

    return 0;
}

// ============================================================================
// Helper Methods
// ============================================================================

void
CommandLine::print_table(const Vector<Vector<String>>& rows, const Vector<String>& headers) {
    if (rows.empty())
        return;

    // Calculate column widths
    Vector<size_t> widths(headers.size(), 0);
    for (size_t i = 0; i < headers.size(); i++) {
        widths[i] = headers[i].length();
    }

    for (const auto& row : rows) {
        for (size_t i = 0; i < row.size() && i < widths.size(); i++) {
            widths[i] = std::max(widths[i], row[i].length());
        }
    }

    // Print headers
    for (size_t i = 0; i < headers.size(); i++) {
        std::cout << std::left << std::setw(static_cast<int>(widths[i] + 2)) << headers[i];
    }
    std::cout << "\n";

    // Print separator
    for (size_t i = 0; i < headers.size(); i++) {
        std::cout << std::string(widths[i] + 1, '-') << " ";
    }
    std::cout << "\n";

    // Print rows
    for (const auto& row : rows) {
        for (size_t i = 0; i < row.size() && i < widths.size(); i++) {
            std::cout << std::left << std::setw(static_cast<int>(widths[i] + 2)) << row[i];
        }
        std::cout << "\n";
    }
}

} // namespace console::cli
