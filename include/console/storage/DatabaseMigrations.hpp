#pragma once

#include "console/common/Types.hpp"

#include <functional>
#include <sqlite3.h>
#include <vector>

namespace console::storage {

/**
 * @brief Database migration entry
 */
struct Migration {
    int version;
    String description;
    std::function<Result<void, String>(sqlite3*)> up;
    std::function<Result<void, String>(sqlite3*)> down;
};

/**
 * @brief Database migration manager
 *
 * Manages schema versioning and migration execution.
 */
class DatabaseMigrations {
  public:
    explicit DatabaseMigrations(sqlite3* db);

    /**
     * @brief Get current schema version
     */
    Result<int, String> get_current_version();

    /**
     * @brief Run all pending migrations
     */
    Result<void, String> migrate_to_latest();

    /**
     * @brief Run migrations up to specific version
     */
    Result<void, String> migrate_to(int target_version);

    /**
     * @brief Rollback to specific version
     */
    Result<void, String> rollback_to(int target_version);

    /**
     * @brief Get list of all migrations
     */
    const Vector<Migration>& get_migrations() const { return migrations_; }

  private:
    /**
     * @brief Ensure migrations table exists
     */
    Result<void, String> ensure_migrations_table();

    /**
     * @brief Record migration execution
     */
    Result<void, String> record_migration(int version, const String& description);

    /**
     * @brief Remove migration record
     */
    Result<void, String> remove_migration(int version);

    /**
     * @brief Execute SQL statement
     */
    Result<void, String> execute_sql(const String& sql);

    /**
     * @brief Initialize migration list
     */
    void init_migrations();

    sqlite3* db_;
    Vector<Migration> migrations_;
};

} // namespace console::storage
