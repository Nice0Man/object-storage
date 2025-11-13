#include "console/storage/DatabaseMigrations.hpp"

#include "console/common/Logger.hpp"

namespace console::storage {

DatabaseMigrations::DatabaseMigrations(sqlite3* db) : db_(db) {
    init_migrations();
}

Result<void, String>
DatabaseMigrations::ensure_migrations_table() {
    CONSOLE_LOG_DEBUG("DatabaseMigrations: Creating migrations table...");
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS schema_migrations (
            version INTEGER PRIMARY KEY,
            description TEXT NOT NULL,
            applied_at INTEGER NOT NULL
        )
    )";

    auto result = execute_sql(sql);
    CONSOLE_LOG_DEBUG("DatabaseMigrations: Migrations table created: {}", static_cast<bool>(result));
    return result;
}

Result<int, String>
DatabaseMigrations::get_current_version() {
    CONSOLE_LOG_DEBUG("DatabaseMigrations: Getting current version...");
    auto result = ensure_migrations_table();
    if (!result) {
        CONSOLE_LOG_ERROR("DatabaseMigrations: Failed to ensure migrations table");
        return Err<int, String>(result.error());
    }

    sqlite3_stmt* stmt;
    const char* sql = "SELECT COALESCE(MAX(version), 0) FROM schema_migrations";

    CONSOLE_LOG_DEBUG("DatabaseMigrations: Preparing SQL...");
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        CONSOLE_LOG_ERROR("DatabaseMigrations: Prepare failed: {}", sqlite3_errmsg(db_));
        return Err<int, String>(sqlite3_errmsg(db_));
    }

    CONSOLE_LOG_DEBUG("DatabaseMigrations: Executing SQL...");
    rc = sqlite3_step(stmt);
    int version = 0;

    if (rc == SQLITE_ROW) {
        version = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
        CONSOLE_LOG_ERROR("DatabaseMigrations: Step failed: {}", sqlite3_errmsg(db_));
        return Err<int, String>(sqlite3_errmsg(db_));
    }

    CONSOLE_LOG_DEBUG("DatabaseMigrations: Current version: {}", version);
    return Ok<int, String>(version);
}

Result<void, String>
DatabaseMigrations::migrate_to_latest() {
    auto current_version_result = get_current_version();
    if (!current_version_result) {
        return Err<void, String>(current_version_result.error());
    }

    int current_version = current_version_result.value();

    CONSOLE_LOG_INFO("Current database version: {}", current_version);

    for (const auto& migration : migrations_) {
        if (migration.version > current_version) {
            CONSOLE_LOG_INFO("Running migration v{}: {}", migration.version, migration.description);

            auto up_result = migration.up(db_);
            if (!up_result) {
                CONSOLE_LOG_ERROR("Migration v{} failed: {}", migration.version, up_result.error());
                return Err<void, String>("Migration v" + std::to_string(migration.version) +
                                         " failed: " + up_result.error());
            }

            auto record_result = record_migration(migration.version, migration.description);
            if (!record_result) {
                return record_result;
            }

            CONSOLE_LOG_INFO("Migration v{} completed successfully", migration.version);
        }
    }

    return Ok<String>();
}

Result<void, String>
DatabaseMigrations::migrate_to(int target_version) {
    auto current_version_result = get_current_version();
    if (!current_version_result) {
        return Err<void, String>(current_version_result.error());
    }

    int current_version = current_version_result.value();

    if (target_version < current_version) {
        return rollback_to(target_version);
    }

    for (const auto& migration : migrations_) {
        if (migration.version > current_version && migration.version <= target_version) {
            CONSOLE_LOG_INFO("Running migration v{}: {}", migration.version, migration.description);

            auto up_result = migration.up(db_);
            if (!up_result) {
                return Err<void, String>("Migration v" + std::to_string(migration.version) +
                                         " failed: " + up_result.error());
            }

            auto record_result = record_migration(migration.version, migration.description);
            if (!record_result) {
                return record_result;
            }
        }
    }

    return Ok<String>();
}

Result<void, String>
DatabaseMigrations::rollback_to(int target_version) {
    auto current_version_result = get_current_version();
    if (!current_version_result) {
        return Err<void, String>(current_version_result.error());
    }

    int current_version = current_version_result.value();

    // Rollback in reverse order
    for (auto it = migrations_.rbegin(); it != migrations_.rend(); ++it) {
        if (it->version <= current_version && it->version > target_version) {
            CONSOLE_LOG_INFO("Rolling back migration v{}: {}", it->version, it->description);

            if (!it->down) {
                return Err<void, String>("Migration v" + std::to_string(it->version) + " has no rollback");
            }

            auto down_result = it->down(db_);
            if (!down_result) {
                return Err<void, String>("Rollback v" + std::to_string(it->version) +
                                         " failed: " + down_result.error());
            }

            auto remove_result = remove_migration(it->version);
            if (!remove_result) {
                return remove_result;
            }
        }
    }

    return Ok<String>();
}

Result<void, String>
DatabaseMigrations::record_migration(int version, const String& description) {
    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO schema_migrations (version, description, applied_at) VALUES (?, ?, ?)";

    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Err<void, String>(sqlite3_errmsg(db_));
    }

    sqlite3_bind_int(stmt, 1, version);
    sqlite3_bind_text(stmt, 2, description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, std::time(nullptr));

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return Err<void, String>(sqlite3_errmsg(db_));
    }

    return Ok<String>();
}

Result<void, String>
DatabaseMigrations::remove_migration(int version) {
    sqlite3_stmt* stmt;
    const char* sql = "DELETE FROM schema_migrations WHERE version = ?";

    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Err<void, String>(sqlite3_errmsg(db_));
    }

    sqlite3_bind_int(stmt, 1, version);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return Err<void, String>(sqlite3_errmsg(db_));
    }

    return Ok<String>();
}

Result<void, String>
DatabaseMigrations::execute_sql(const String& sql) {
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);

    if (rc != SQLITE_OK) {
        String error = err_msg ? err_msg : "Unknown error";
        sqlite3_free(err_msg);
        return Err<void, String>(error);
    }

    return Ok<String>();
}

void
DatabaseMigrations::init_migrations() {
    // ========================================================================
    // Migration v1: Initial schema
    // ========================================================================
    migrations_.push_back(
        {.version = 1,
         .description = "Initial database schema",
         .up = [](sqlite3* db) -> Result<void, String> {
             // Helper to execute SQL without creating new DatabaseMigrations
             auto exec_sql = [db](const String& sql) -> Result<void, String> {
                 char* err_msg = nullptr;
                 int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err_msg);
                 if (rc != SQLITE_OK) {
                     String error = err_msg ? err_msg : "Unknown error";
                     sqlite3_free(err_msg);
                     return Err<void, String>(error);
                 }
                 return Ok<String>();
             };

             // Users table
             auto result = exec_sql(R"(
                CREATE TABLE IF NOT EXISTS users (
                    access_key TEXT PRIMARY KEY,
                    secret_key TEXT NOT NULL,
                    account_name TEXT,
                    status TEXT DEFAULT 'active',
                    is_admin BOOLEAN DEFAULT 0,
                    created_at INTEGER NOT NULL,
                    updated_at INTEGER NOT NULL,
                    metadata TEXT
                )
            )");
             if (!result)
                 return result;

             // Groups table
             result = exec_sql(R"(
                CREATE TABLE IF NOT EXISTS groups (
                    name TEXT PRIMARY KEY,
                    description TEXT,
                    status TEXT DEFAULT 'active',
                    created_at INTEGER NOT NULL,
                    updated_at INTEGER NOT NULL,
                    metadata TEXT
                )
            )");
             if (!result)
                 return result;

             // User-Group relationship
             result = exec_sql(R"(
                CREATE TABLE IF NOT EXISTS user_groups (
                    user_access_key TEXT NOT NULL,
                    group_name TEXT NOT NULL,
                    added_at INTEGER NOT NULL,
                    PRIMARY KEY (user_access_key, group_name),
                    FOREIGN KEY (user_access_key) REFERENCES users(access_key) ON DELETE CASCADE,
                    FOREIGN KEY (group_name) REFERENCES groups(name) ON DELETE CASCADE
                )
            )");
             if (!result)
                 return result;

             // Policies table
             result = exec_sql(R"(
                CREATE TABLE IF NOT EXISTS policies (
                    name TEXT PRIMARY KEY,
                    version TEXT DEFAULT '2012-10-17',
                    document TEXT NOT NULL,
                    description TEXT,
                    created_at INTEGER NOT NULL,
                    updated_at INTEGER NOT NULL,
                    metadata TEXT
                )
            )");
             if (!result)
                 return result;

             // User-Policy relationship
             result = exec_sql(R"(
                CREATE TABLE IF NOT EXISTS user_policies (
                    user_access_key TEXT NOT NULL,
                    policy_name TEXT NOT NULL,
                    attached_at INTEGER NOT NULL,
                    PRIMARY KEY (user_access_key, policy_name),
                    FOREIGN KEY (user_access_key) REFERENCES users(access_key) ON DELETE CASCADE,
                    FOREIGN KEY (policy_name) REFERENCES policies(name) ON DELETE CASCADE
                )
            )");
             if (!result)
                 return result;

             // Group-Policy relationship
             result = exec_sql(R"(
                CREATE TABLE IF NOT EXISTS group_policies (
                    group_name TEXT NOT NULL,
                    policy_name TEXT NOT NULL,
                    attached_at INTEGER NOT NULL,
                    PRIMARY KEY (group_name, policy_name),
                    FOREIGN KEY (group_name) REFERENCES groups(name) ON DELETE CASCADE,
                    FOREIGN KEY (policy_name) REFERENCES policies(name) ON DELETE CASCADE
                )
            )");
             if (!result)
                 return result;

             // Service Accounts table
             result = exec_sql(R"(
                CREATE TABLE IF NOT EXISTS service_accounts (
                    access_key TEXT PRIMARY KEY,
                    secret_key TEXT NOT NULL,
                    parent_user TEXT NOT NULL,
                    description TEXT,
                    expiration INTEGER,
                    status TEXT DEFAULT 'active',
                    created_at INTEGER NOT NULL,
                    metadata TEXT,
                    FOREIGN KEY (parent_user) REFERENCES users(access_key) ON DELETE CASCADE
                )
            )");
             if (!result)
                 return result;

             // Audit Log table
             result = exec_sql(R"(
                CREATE TABLE IF NOT EXISTS audit_log (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    timestamp INTEGER NOT NULL,
                    user_access_key TEXT,
                    action TEXT NOT NULL,
                    resource_type TEXT NOT NULL,
                    resource_id TEXT NOT NULL,
                    status TEXT NOT NULL,
                    details TEXT,
                    ip_address TEXT,
                    user_agent TEXT
                )
            )");
             if (!result)
                 return result;

             // Config table
             result = exec_sql(R"(
                CREATE TABLE IF NOT EXISTS config (
                    key TEXT PRIMARY KEY,
                    value TEXT NOT NULL,
                    value_type TEXT DEFAULT 'string',
                    description TEXT,
                    updated_at INTEGER NOT NULL,
                    updated_by TEXT
                )
            )");
             if (!result)
                 return result;

             // Create indexes
             exec_sql("CREATE INDEX IF NOT EXISTS idx_users_status ON users(status)");
             exec_sql("CREATE INDEX IF NOT EXISTS idx_users_created ON users(created_at)");
             exec_sql("CREATE INDEX IF NOT EXISTS idx_groups_status ON groups(status)");
             exec_sql("CREATE INDEX IF NOT EXISTS idx_user_groups_user ON user_groups(user_access_key)");
             exec_sql("CREATE INDEX IF NOT EXISTS idx_user_groups_group ON user_groups(group_name)");
             exec_sql("CREATE INDEX IF NOT EXISTS idx_policies_created ON policies(created_at)");
             exec_sql("CREATE INDEX IF NOT EXISTS idx_user_policies_user ON user_policies(user_access_key)");
             exec_sql("CREATE INDEX IF NOT EXISTS idx_user_policies_policy ON user_policies(policy_name)");
             exec_sql("CREATE INDEX IF NOT EXISTS idx_group_policies_group ON group_policies(group_name)");
             exec_sql("CREATE INDEX IF NOT EXISTS idx_group_policies_policy ON group_policies(policy_name)");
             exec_sql("CREATE INDEX IF NOT EXISTS idx_service_accounts_parent ON service_accounts(parent_user)");
             exec_sql("CREATE INDEX IF NOT EXISTS idx_service_accounts_status ON service_accounts(status)");
             exec_sql("CREATE INDEX IF NOT EXISTS idx_service_accounts_expiration ON service_accounts(expiration)");
             exec_sql("CREATE INDEX IF NOT EXISTS idx_audit_timestamp ON audit_log(timestamp)");
             exec_sql("CREATE INDEX IF NOT EXISTS idx_audit_user ON audit_log(user_access_key)");
             exec_sql("CREATE INDEX IF NOT EXISTS idx_audit_action ON audit_log(action)");
             exec_sql("CREATE INDEX IF NOT EXISTS idx_audit_resource ON audit_log(resource_type, resource_id)");
             exec_sql("CREATE INDEX IF NOT EXISTS idx_config_type ON config(value_type)");

             return Ok<String>();
         },
         .down = [](sqlite3* db) -> Result<void, String> {
             // Helper to execute SQL
             auto exec_sql = [db](const String& sql) -> Result<void, String> {
                 char* err_msg = nullptr;
                 int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err_msg);
                 if (rc != SQLITE_OK) {
                     String error = err_msg ? err_msg : "Unknown error";
                     sqlite3_free(err_msg);
                     return Err<void, String>(error);
                 }
                 return Ok<String>();
             };

             // Drop all tables in reverse order
             exec_sql("DROP TABLE IF EXISTS config");
             exec_sql("DROP TABLE IF EXISTS audit_log");
             exec_sql("DROP TABLE IF EXISTS service_accounts");
             exec_sql("DROP TABLE IF EXISTS group_policies");
             exec_sql("DROP TABLE IF EXISTS user_policies");
             exec_sql("DROP TABLE IF EXISTS policies");
             exec_sql("DROP TABLE IF EXISTS user_groups");
             exec_sql("DROP TABLE IF EXISTS groups");
             exec_sql("DROP TABLE IF EXISTS users");
             return Ok<String>();
         }});

    // Future migrations can be added here
    // Example:
    // migrations_.push_back({
    //     .version = 2,
    //     .description = "Add email column to users",
    //     .up = [](sqlite3* db) -> Result<void, String> {
    //         return exec_sql("ALTER TABLE users ADD COLUMN email TEXT");
    //     },
    //     .down = [](sqlite3* db) -> Result<void, String> {
    //         // Note: SQLite doesn't support DROP COLUMN easily
    //         return Ok<String>();
    //     }
    // });
}

} // namespace console::storage
