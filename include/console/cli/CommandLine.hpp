#pragma once

#include "console/common/Types.hpp"

#include <functional>
#include <map>
#include <vector>

namespace console::cli {

/**
 * @brief CLI command handler type
 */
using CommandHandler = std::function<int(const Vector<String>& args)>;

/**
 * @brief CLI command definition
 */
struct Command {
    String name;
    String description;
    String usage;
    CommandHandler handler;
};

/**
 * @brief Command line interface for Object Storage Console
 *
 * Provides CLI commands for:
 * - User management (create, delete, list users)
 * - Bucket management (create, delete, list buckets)
 * - Configuration (generate, validate)
 * - Database migration
 */
class CommandLine {
  public:
    CommandLine();

    /**
     * @brief Parse and execute command line arguments
     * @param argc Argument count
     * @param argv Argument values
     * @return Exit code (0 = success, non-zero = error)
     */
    int run(int argc, char* argv[]);

    /**
     * @brief Check if arguments contain a CLI command
     * @param argc Argument count
     * @param argv Argument values
     * @return true if CLI command detected
     */
    static bool is_cli_command(int argc, char* argv[]);

  private:
    void register_commands();
    void print_help();
    void print_command_help(const String& command);

    // User management commands
    int cmd_user_create(const Vector<String>& args);
    int cmd_user_delete(const Vector<String>& args);
    int cmd_user_list(const Vector<String>& args);
    int cmd_user_info(const Vector<String>& args);
    int cmd_user_password(const Vector<String>& args);

    // Bucket management commands
    int cmd_bucket_create(const Vector<String>& args);
    int cmd_bucket_delete(const Vector<String>& args);
    int cmd_bucket_list(const Vector<String>& args);
    int cmd_bucket_info(const Vector<String>& args);

    // Config commands
    int cmd_config_init(const Vector<String>& args);
    int cmd_config_validate(const Vector<String>& args);
    int cmd_config_show(const Vector<String>& args);

    // Database commands
    int cmd_db_migrate(const Vector<String>& args);
    int cmd_db_backup(const Vector<String>& args);

    // Helper methods
    bool init_services_for_cli();
    void print_table(const Vector<Vector<String>>& rows, const Vector<String>& headers);

    std::map<String, Command> commands_;
    bool services_initialized_{false};
};

} // namespace console::cli
