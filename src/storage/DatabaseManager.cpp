#include "console/storage/DatabaseManager.hpp"

#include "console/common/Config.hpp"
#include "console/common/Logger.hpp"
#include "console/storage/DatabaseMigrations.hpp"
#include "console/utils/PasswordHash.hpp"

#include <chrono>
#include <fstream>
#include <sstream>
#include <thread>

namespace console::storage {

// ============================================================================
// Constructor & Destructor
// ============================================================================

DatabaseManager::DatabaseManager(const String& db_path, size_t thread_pool_size)
    : db_path_(db_path), thread_pool_size_(thread_pool_size) {
    CONSOLE_LOG_DEBUG("DatabaseManager: Opening database: {}", db_path);
    // Open database
    int rc = sqlite3_open(db_path_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        CONSOLE_LOG_ERROR("Failed to open database: {}", sqlite3_errmsg(db_));
        return;
    }
    CONSOLE_LOG_DEBUG("DatabaseManager: Database opened successfully");

    // Enable WAL mode for concurrent access
    CONSOLE_LOG_DEBUG("DatabaseManager: Setting PRAGMA journal_mode=WAL");
    execute_sql("PRAGMA journal_mode=WAL");
    CONSOLE_LOG_DEBUG("DatabaseManager: Setting PRAGMA synchronous=NORMAL");
    execute_sql("PRAGMA synchronous=NORMAL");
    CONSOLE_LOG_DEBUG("DatabaseManager: Setting PRAGMA foreign_keys=ON");
    execute_sql("PRAGMA foreign_keys=ON");
    CONSOLE_LOG_DEBUG("DatabaseManager: Setting PRAGMA cache_size=10000");
    execute_sql("PRAGMA cache_size=10000");
    CONSOLE_LOG_DEBUG("DatabaseManager: Setting PRAGMA temp_store=MEMORY");
    execute_sql("PRAGMA temp_store=MEMORY");

    // Start worker threads
    CONSOLE_LOG_DEBUG("DatabaseManager: Starting {} worker threads", thread_pool_size_);
    for (size_t i = 0; i < thread_pool_size_; ++i) {
        CONSOLE_LOG_DEBUG("DatabaseManager: Starting worker thread {}", i);
        worker_threads_.emplace_back([this]() {
            while (!stop_workers_) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex_);
                    queue_cv_.wait(lock, [this] { return stop_workers_ || !task_queue_.empty(); });

                    if (stop_workers_ && task_queue_.empty()) {
                        return;
                    }

                    if (!task_queue_.empty()) {
                        task = std::move(task_queue_.front());
                        task_queue_.pop();
                    }
                }

                if (task) {
                    task();
                }
            }
        });
    }

    CONSOLE_LOG_INFO("DatabaseManager initialized: {} (threads: {})", db_path_, thread_pool_size_);
}

DatabaseManager::~DatabaseManager() {
    // Stop worker threads
    stop_workers_ = true;
    queue_cv_.notify_all();

    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    // Close database
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }

    CONSOLE_LOG_INFO("DatabaseManager destroyed");
}

// ============================================================================
// Initialization
// ============================================================================

Result<void, String>
DatabaseManager::initialize_schema() {
    try {
        {
            std::lock_guard<std::mutex> lock(db_mutex_);

            // Use migration system
            DatabaseMigrations migrations(db_);
            auto migrate_result = migrations.migrate_to_latest();
            if (!migrate_result) {
                CONSOLE_LOG_ERROR("Failed to run migrations: {}", migrate_result.error());
                return migrate_result;
            }

            auto version_result = migrations.get_current_version();
            if (version_result) {
                CONSOLE_LOG_INFO("Database schema at version: {}", version_result.value());
            }
        } // Release lock before create_default_data

        // Create default data (admin user, etc.) - needs to query/write to DB
        create_default_data();

        is_ready_ = true;
        CONSOLE_LOG_INFO("Database schema initialized successfully");
        return Ok<String>();
    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Failed to initialize schema: {}", e.what());
        return Err<void, String>(String("Schema initialization failed: ") + e.what());
    }
}

bool
DatabaseManager::is_ready() const {
    return is_ready_;
}

void
DatabaseManager::create_tables() {
    // Users table
    execute_sql(R"(
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

    // Groups table
    execute_sql(R"(
        CREATE TABLE IF NOT EXISTS groups (
            name TEXT PRIMARY KEY,
            description TEXT,
            status TEXT DEFAULT 'active',
            created_at INTEGER NOT NULL,
            updated_at INTEGER NOT NULL,
            metadata TEXT
        )
    )");

    // User-Group relationship
    execute_sql(R"(
        CREATE TABLE IF NOT EXISTS user_groups (
            user_access_key TEXT NOT NULL,
            group_name TEXT NOT NULL,
            added_at INTEGER NOT NULL,
            PRIMARY KEY (user_access_key, group_name),
            FOREIGN KEY (user_access_key) REFERENCES users(access_key) ON DELETE CASCADE,
            FOREIGN KEY (group_name) REFERENCES groups(name) ON DELETE CASCADE
        )
    )");

    // Policies table
    execute_sql(R"(
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

    // User-Policy relationship
    execute_sql(R"(
        CREATE TABLE IF NOT EXISTS user_policies (
            user_access_key TEXT NOT NULL,
            policy_name TEXT NOT NULL,
            attached_at INTEGER NOT NULL,
            PRIMARY KEY (user_access_key, policy_name),
            FOREIGN KEY (user_access_key) REFERENCES users(access_key) ON DELETE CASCADE,
            FOREIGN KEY (policy_name) REFERENCES policies(name) ON DELETE CASCADE
        )
    )");

    // Group-Policy relationship
    execute_sql(R"(
        CREATE TABLE IF NOT EXISTS group_policies (
            group_name TEXT NOT NULL,
            policy_name TEXT NOT NULL,
            attached_at INTEGER NOT NULL,
            PRIMARY KEY (group_name, policy_name),
            FOREIGN KEY (group_name) REFERENCES groups(name) ON DELETE CASCADE,
            FOREIGN KEY (policy_name) REFERENCES policies(name) ON DELETE CASCADE
        )
    )");

    // Service Accounts table
    execute_sql(R"(
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

    // Audit Log table
    execute_sql(R"(
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

    // Config table
    execute_sql(R"(
        CREATE TABLE IF NOT EXISTS config (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL,
            value_type TEXT DEFAULT 'string',
            description TEXT,
            updated_at INTEGER NOT NULL,
            updated_by TEXT
        )
    )");

    CONSOLE_LOG_DEBUG("Database tables created");
}

void
DatabaseManager::create_indexes() {
    // Users indexes
    execute_sql("CREATE INDEX IF NOT EXISTS idx_users_status ON users(status)");
    execute_sql("CREATE INDEX IF NOT EXISTS idx_users_created ON users(created_at)");

    // Groups indexes
    execute_sql("CREATE INDEX IF NOT EXISTS idx_groups_status ON groups(status)");

    // User-Group indexes
    execute_sql("CREATE INDEX IF NOT EXISTS idx_user_groups_user ON user_groups(user_access_key)");
    execute_sql("CREATE INDEX IF NOT EXISTS idx_user_groups_group ON user_groups(group_name)");

    // Policies indexes
    execute_sql("CREATE INDEX IF NOT EXISTS idx_policies_created ON policies(created_at)");

    // User-Policy indexes
    execute_sql("CREATE INDEX IF NOT EXISTS idx_user_policies_user ON user_policies(user_access_key)");
    execute_sql("CREATE INDEX IF NOT EXISTS idx_user_policies_policy ON user_policies(policy_name)");

    // Group-Policy indexes
    execute_sql("CREATE INDEX IF NOT EXISTS idx_group_policies_group ON group_policies(group_name)");
    execute_sql("CREATE INDEX IF NOT EXISTS idx_group_policies_policy ON group_policies(policy_name)");

    // Service Accounts indexes
    execute_sql("CREATE INDEX IF NOT EXISTS idx_service_accounts_parent ON service_accounts(parent_user)");
    execute_sql("CREATE INDEX IF NOT EXISTS idx_service_accounts_status ON service_accounts(status)");
    execute_sql("CREATE INDEX IF NOT EXISTS idx_service_accounts_expiration ON service_accounts(expiration)");

    // Audit Log indexes
    execute_sql("CREATE INDEX IF NOT EXISTS idx_audit_timestamp ON audit_log(timestamp)");
    execute_sql("CREATE INDEX IF NOT EXISTS idx_audit_user ON audit_log(user_access_key)");
    execute_sql("CREATE INDEX IF NOT EXISTS idx_audit_action ON audit_log(action)");
    execute_sql("CREATE INDEX IF NOT EXISTS idx_audit_resource ON audit_log(resource_type, resource_id)");

    // Config indexes
    execute_sql("CREATE INDEX IF NOT EXISTS idx_config_type ON config(value_type)");

    CONSOLE_LOG_DEBUG("Database indexes created");
}

void
DatabaseManager::create_default_data() {
    // Get default admin configuration
    const auto& admin_config = Config::instance().default_admin();

    // Check if default admin creation is enabled
    if (!admin_config.enabled) {
        CONSOLE_LOG_DEBUG("Default admin creation is disabled");
        return;
    }

    // Check if admin user already exists
    auto exists = user_exists(admin_config.username);
    if (exists && exists.value()) {
        CONSOLE_LOG_DEBUG("Default admin user already exists");
        return;
    }

    // Create default admin user with hashed password
    DbUser admin;
    admin.access_key = admin_config.username;
    admin.secret_key = utils::PasswordHash::hash(admin_config.password); // Hash password securely
    admin.account_name = admin_config.account_name;
    admin.status = "active";
    admin.is_admin = true;
    admin.created_at = std::time(nullptr);
    admin.updated_at = admin.created_at;
    admin.metadata = "{}";

    auto result = create_user(admin);
    if (result) {
        CONSOLE_LOG_INFO("Default admin user created: {}", admin.access_key);
    } else {
        CONSOLE_LOG_WARN("Failed to create default admin user: {}", result.error());
    }
}

// ============================================================================
// User Operations
// ============================================================================

Result<DbUser, String>
DatabaseManager::get_user(const String& access_key) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return query_single<DbUser>(
        "SELECT access_key, secret_key, account_name, status, is_admin, created_at, updated_at, metadata "
        "FROM users WHERE access_key = ?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_text(stmt, 1, access_key.c_str(), -1, SQLITE_TRANSIENT); },
        [](sqlite3_stmt* stmt) -> DbUser {
            DbUser user;
            user.access_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            user.secret_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

            if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
                user.account_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            }

            if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
                user.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            }

            user.is_admin = sqlite3_column_int(stmt, 4) != 0;
            user.created_at = sqlite3_column_int64(stmt, 5);
            user.updated_at = sqlite3_column_int64(stmt, 6);

            if (sqlite3_column_type(stmt, 7) != SQLITE_NULL) {
                user.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
            }

            return user;
        });
}

Result<Vector<DbUser>, String>
DatabaseManager::list_users(const String& status) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String sql =
        "SELECT access_key, secret_key, account_name, status, is_admin, created_at, updated_at, metadata FROM users";

    if (!status.empty()) {
        sql += " WHERE status = ?";
    }

    sql += " ORDER BY created_at DESC";

    return query_multiple<DbUser>(
        sql,
        [&](sqlite3_stmt* stmt) {
            if (!status.empty()) {
                sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_TRANSIENT);
            }
        },
        [](sqlite3_stmt* stmt) -> DbUser {
            DbUser user;
            user.access_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            user.secret_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

            if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
                user.account_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            }

            if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
                user.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            }

            user.is_admin = sqlite3_column_int(stmt, 4) != 0;
            user.created_at = sqlite3_column_int64(stmt, 5);
            user.updated_at = sqlite3_column_int64(stmt, 6);

            if (sqlite3_column_type(stmt, 7) != SQLITE_NULL) {
                user.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
            }

            return user;
        });
}

Result<void, String>
DatabaseManager::create_user(const DbUser& user) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return execute_sql_with_bind(
        "INSERT INTO users (access_key, secret_key, account_name, status, is_admin, created_at, updated_at, metadata) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
        [&](sqlite3_stmt* stmt) {
            sqlite3_bind_text(stmt, 1, user.access_key.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, user.secret_key.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, user.account_name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, user.status.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 5, user.is_admin ? 1 : 0);
            sqlite3_bind_int64(stmt, 6, user.created_at);
            sqlite3_bind_int64(stmt, 7, user.updated_at);
            sqlite3_bind_text(stmt, 8, user.metadata.c_str(), -1, SQLITE_TRANSIENT);
        });
}

Result<void, String>
DatabaseManager::update_user(const DbUser& user) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return execute_sql_with_bind(
        "UPDATE users SET secret_key = ?, account_name = ?, status = ?, is_admin = ?, updated_at = ?, metadata = ? "
        "WHERE access_key = ?",
        [&](sqlite3_stmt* stmt) {
            sqlite3_bind_text(stmt, 1, user.secret_key.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, user.account_name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, user.status.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 4, user.is_admin ? 1 : 0);
            sqlite3_bind_int64(stmt, 5, user.updated_at);
            sqlite3_bind_text(stmt, 6, user.metadata.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 7, user.access_key.c_str(), -1, SQLITE_TRANSIENT);
        });
}

Result<void, String>
DatabaseManager::delete_user(const String& access_key) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return execute_sql_with_bind("DELETE FROM users WHERE access_key = ?", [&](sqlite3_stmt* stmt) {
        sqlite3_bind_text(stmt, 1, access_key.c_str(), -1, SQLITE_TRANSIENT);
    });
}

Result<bool, String>
DatabaseManager::user_exists(const String& access_key) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    auto result = query_single<int>(
        "SELECT COUNT(*) FROM users WHERE access_key = ?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_text(stmt, 1, access_key.c_str(), -1, SQLITE_TRANSIENT); },
        [](sqlite3_stmt* stmt) -> int { return sqlite3_column_int(stmt, 0); });

    if (!result) {
        return Err<bool, String>(result.error());
    }

    return Ok<bool, String>(result.value() > 0);
}

// Async versions
void
DatabaseManager::get_user_async(const String& access_key, AsyncCallback<DbUser> callback) {
    submit_async([this, access_key, callback]() {
        auto result = get_user(access_key);
        callback(result);
    });
}

void
DatabaseManager::create_user_async(const DbUser& user, AsyncCallback<void> callback) {
    submit_async([this, user, callback]() {
        auto result = create_user(user);
        callback(result);
    });
}

// ============================================================================
// Helper Methods
// ============================================================================

Result<void, String>
DatabaseManager::execute_sql(const String& sql) {
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);

    if (rc != SQLITE_OK) {
        String error = err_msg ? err_msg : "Unknown error";
        sqlite3_free(err_msg);
        return Err<void, String>(error);
    }

    return Ok<String>();
}

Result<void, String>
DatabaseManager::execute_sql_with_bind(const String& sql, const std::function<void(sqlite3_stmt*)>& bind_func) {
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        String error = sqlite3_errmsg(db_);
        return Err<void, String>(error);
    }

    bind_func(stmt);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        String error = sqlite3_errmsg(db_);
        return Err<void, String>(error);
    }

    return Ok<String>();
}

template <typename T>
Result<T, String>
DatabaseManager::query_single(const String& sql,
                              const std::function<void(sqlite3_stmt*)>& bind_func,
                              const std::function<T(sqlite3_stmt*)>& extract_func) {
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        String error = sqlite3_errmsg(db_);
        return Err<T, String>(error);
    }

    bind_func(stmt);

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        T result = extract_func(stmt);
        sqlite3_finalize(stmt);
        return Ok<T, String>(result);
    } else if (rc == SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return Err<T, String>("No results found");
    } else {
        String error = sqlite3_errmsg(db_);
        sqlite3_finalize(stmt);
        return Err<T, String>(error);
    }
}

template <typename T>
Result<Vector<T>, String>
DatabaseManager::query_multiple(const String& sql,
                                const std::function<void(sqlite3_stmt*)>& bind_func,
                                const std::function<T(sqlite3_stmt*)>& extract_func) {
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        String error = sqlite3_errmsg(db_);
        return Err<Vector<T>, String>(error);
    }

    bind_func(stmt);

    Vector<T> results;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        results.push_back(extract_func(stmt));
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        String error = sqlite3_errmsg(db_);
        return Err<Vector<T>, String>(error);
    }

    return Ok<Vector<T>, String>(results);
}

template <typename Func>
void
DatabaseManager::submit_async(Func&& func) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        task_queue_.push(std::forward<Func>(func));
    }
    queue_cv_.notify_one();
}

// ============================================================================
// Group Operations
// ============================================================================

Result<DbGroup, String>
DatabaseManager::get_group(const String& name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return query_single<DbGroup>(
        "SELECT name, description, status, created_at, updated_at, metadata "
        "FROM groups WHERE name = ?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT); },
        [](sqlite3_stmt* stmt) -> DbGroup {
            DbGroup group;
            group.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));

            if (sqlite3_column_type(stmt, 1) != SQLITE_NULL) {
                group.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            }

            if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
                group.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            }

            group.created_at = sqlite3_column_int64(stmt, 3);
            group.updated_at = sqlite3_column_int64(stmt, 4);

            if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) {
                group.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            }

            return group;
        });
}

Result<Vector<DbGroup>, String>
DatabaseManager::list_groups(const String& status) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String sql = "SELECT name, description, status, created_at, updated_at, metadata FROM groups";

    if (!status.empty()) {
        sql += " WHERE status = ?";
    }

    sql += " ORDER BY created_at DESC";

    return query_multiple<DbGroup>(
        sql,
        [&](sqlite3_stmt* stmt) {
            if (!status.empty()) {
                sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_TRANSIENT);
            }
        },
        [](sqlite3_stmt* stmt) -> DbGroup {
            DbGroup group;
            group.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));

            if (sqlite3_column_type(stmt, 1) != SQLITE_NULL) {
                group.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            }

            if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
                group.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            }

            group.created_at = sqlite3_column_int64(stmt, 3);
            group.updated_at = sqlite3_column_int64(stmt, 4);

            if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) {
                group.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            }

            return group;
        });
}

Result<void, String>
DatabaseManager::create_group(const DbGroup& group) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return execute_sql_with_bind("INSERT INTO groups (name, description, status, created_at, updated_at, metadata) "
                                 "VALUES (?, ?, ?, ?, ?, ?)",
                                 [&](sqlite3_stmt* stmt) {
                                     sqlite3_bind_text(stmt, 1, group.name.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_text(stmt, 2, group.description.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_text(stmt, 3, group.status.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_int64(stmt, 4, group.created_at);
                                     sqlite3_bind_int64(stmt, 5, group.updated_at);
                                     sqlite3_bind_text(stmt, 6, group.metadata.c_str(), -1, SQLITE_TRANSIENT);
                                 });
}

Result<void, String>
DatabaseManager::update_group(const DbGroup& group) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return execute_sql_with_bind("UPDATE groups SET description = ?, status = ?, updated_at = ?, metadata = ? "
                                 "WHERE name = ?",
                                 [&](sqlite3_stmt* stmt) {
                                     sqlite3_bind_text(stmt, 1, group.description.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_text(stmt, 2, group.status.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_int64(stmt, 3, group.updated_at);
                                     sqlite3_bind_text(stmt, 4, group.metadata.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_text(stmt, 5, group.name.c_str(), -1, SQLITE_TRANSIENT);
                                 });
}

Result<void, String>
DatabaseManager::delete_group(const String& name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return execute_sql_with_bind("DELETE FROM groups WHERE name = ?", [&](sqlite3_stmt* stmt) {
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    });
}

Result<bool, String>
DatabaseManager::group_exists(const String& name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    auto result = query_single<int>(
        "SELECT COUNT(*) FROM groups WHERE name = ?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT); },
        [](sqlite3_stmt* stmt) -> int { return sqlite3_column_int(stmt, 0); });

    if (!result) {
        return Err<bool, String>(result.error());
    }

    return Ok<bool, String>(result.value() > 0);
}

Result<void, String>
DatabaseManager::add_user_to_group(const String& access_key, const String& group_name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    int64_t now = std::time(nullptr);

    return execute_sql_with_bind("INSERT INTO user_groups (user_access_key, group_name, added_at) VALUES (?, ?, ?)",
                                 [&](sqlite3_stmt* stmt) {
                                     sqlite3_bind_text(stmt, 1, access_key.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_text(stmt, 2, group_name.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_int64(stmt, 3, now);
                                 });
}

Result<void, String>
DatabaseManager::remove_user_from_group(const String& access_key, const String& group_name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return execute_sql_with_bind("DELETE FROM user_groups WHERE user_access_key = ? AND group_name = ?",
                                 [&](sqlite3_stmt* stmt) {
                                     sqlite3_bind_text(stmt, 1, access_key.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_text(stmt, 2, group_name.c_str(), -1, SQLITE_TRANSIENT);
                                 });
}

Result<Vector<String>, String>
DatabaseManager::get_user_groups(const String& access_key) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return query_multiple<String>(
        "SELECT group_name FROM user_groups WHERE user_access_key = ? ORDER BY added_at",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_text(stmt, 1, access_key.c_str(), -1, SQLITE_TRANSIENT); },
        [](sqlite3_stmt* stmt) -> String { return reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)); });
}

Result<Vector<String>, String>
DatabaseManager::get_group_users(const String& group_name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return query_multiple<String>(
        "SELECT user_access_key FROM user_groups WHERE group_name = ? ORDER BY added_at",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_text(stmt, 1, group_name.c_str(), -1, SQLITE_TRANSIENT); },
        [](sqlite3_stmt* stmt) -> String { return reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)); });
}

// ============================================================================
// Policy Operations
// ============================================================================

Result<DbPolicy, String>
DatabaseManager::get_policy(const String& name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return query_single<DbPolicy>(
        "SELECT name, version, document, description, created_at, updated_at, metadata "
        "FROM policies WHERE name = ?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT); },
        [](sqlite3_stmt* stmt) -> DbPolicy {
            DbPolicy policy;
            policy.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));

            if (sqlite3_column_type(stmt, 1) != SQLITE_NULL) {
                policy.version = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            }

            policy.document = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

            if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
                policy.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            }

            policy.created_at = sqlite3_column_int64(stmt, 4);
            policy.updated_at = sqlite3_column_int64(stmt, 5);

            if (sqlite3_column_type(stmt, 6) != SQLITE_NULL) {
                policy.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
            }

            return policy;
        });
}

Result<Vector<DbPolicy>, String>
DatabaseManager::list_policies() {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return query_multiple<DbPolicy>(
        "SELECT name, version, document, description, created_at, updated_at, metadata "
        "FROM policies ORDER BY created_at DESC",
        [](sqlite3_stmt*) {},
        [](sqlite3_stmt* stmt) -> DbPolicy {
            DbPolicy policy;
            policy.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));

            if (sqlite3_column_type(stmt, 1) != SQLITE_NULL) {
                policy.version = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            }

            policy.document = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

            if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
                policy.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            }

            policy.created_at = sqlite3_column_int64(stmt, 4);
            policy.updated_at = sqlite3_column_int64(stmt, 5);

            if (sqlite3_column_type(stmt, 6) != SQLITE_NULL) {
                policy.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
            }

            return policy;
        });
}

Result<void, String>
DatabaseManager::create_policy(const DbPolicy& policy) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return execute_sql_with_bind(
        "INSERT INTO policies (name, version, document, description, created_at, updated_at, metadata) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)",
        [&](sqlite3_stmt* stmt) {
            sqlite3_bind_text(stmt, 1, policy.name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, policy.version.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, policy.document.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, policy.description.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int64(stmt, 5, policy.created_at);
            sqlite3_bind_int64(stmt, 6, policy.updated_at);
            sqlite3_bind_text(stmt, 7, policy.metadata.c_str(), -1, SQLITE_TRANSIENT);
        });
}

Result<void, String>
DatabaseManager::update_policy(const DbPolicy& policy) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return execute_sql_with_bind(
        "UPDATE policies SET version = ?, document = ?, description = ?, updated_at = ?, metadata = ? "
        "WHERE name = ?",
        [&](sqlite3_stmt* stmt) {
            sqlite3_bind_text(stmt, 1, policy.version.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, policy.document.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, policy.description.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int64(stmt, 4, policy.updated_at);
            sqlite3_bind_text(stmt, 5, policy.metadata.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 6, policy.name.c_str(), -1, SQLITE_TRANSIENT);
        });
}

Result<void, String>
DatabaseManager::delete_policy(const String& name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return execute_sql_with_bind("DELETE FROM policies WHERE name = ?", [&](sqlite3_stmt* stmt) {
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    });
}

Result<bool, String>
DatabaseManager::policy_exists(const String& name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    auto result = query_single<int>(
        "SELECT COUNT(*) FROM policies WHERE name = ?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT); },
        [](sqlite3_stmt* stmt) -> int { return sqlite3_column_int(stmt, 0); });

    if (!result) {
        return Err<bool, String>(result.error());
    }

    return Ok<bool, String>(result.value() > 0);
}

Result<void, String>
DatabaseManager::attach_policy_to_user(const String& access_key, const String& policy_name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    int64_t now = std::time(nullptr);

    return execute_sql_with_bind(
        "INSERT INTO user_policies (user_access_key, policy_name, attached_at) VALUES (?, ?, ?)",
        [&](sqlite3_stmt* stmt) {
            sqlite3_bind_text(stmt, 1, access_key.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, policy_name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int64(stmt, 3, now);
        });
}

Result<void, String>
DatabaseManager::detach_policy_from_user(const String& access_key, const String& policy_name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return execute_sql_with_bind("DELETE FROM user_policies WHERE user_access_key = ? AND policy_name = ?",
                                 [&](sqlite3_stmt* stmt) {
                                     sqlite3_bind_text(stmt, 1, access_key.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_text(stmt, 2, policy_name.c_str(), -1, SQLITE_TRANSIENT);
                                 });
}

Result<Vector<String>, String>
DatabaseManager::get_user_policies(const String& access_key, bool include_group_policies) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    if (!include_group_policies) {
        // Only direct user policies
        return query_multiple<String>(
            "SELECT policy_name FROM user_policies WHERE user_access_key = ? ORDER BY attached_at",
            [&](sqlite3_stmt* stmt) { sqlite3_bind_text(stmt, 1, access_key.c_str(), -1, SQLITE_TRANSIENT); },
            [](sqlite3_stmt* stmt) -> String { return reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)); });
    } else {
        // Include policies from groups
        return query_multiple<String>(
            "SELECT DISTINCT policy_name FROM ("
            "  SELECT policy_name FROM user_policies WHERE user_access_key = ?"
            "  UNION"
            "  SELECT gp.policy_name FROM group_policies gp"
            "  JOIN user_groups ug ON gp.group_name = ug.group_name"
            "  WHERE ug.user_access_key = ?"
            ") ORDER BY policy_name",
            [&](sqlite3_stmt* stmt) {
                sqlite3_bind_text(stmt, 1, access_key.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 2, access_key.c_str(), -1, SQLITE_TRANSIENT);
            },
            [](sqlite3_stmt* stmt) -> String { return reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)); });
    }
}

Result<void, String>
DatabaseManager::attach_policy_to_group(const String& group_name, const String& policy_name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    int64_t now = std::time(nullptr);

    return execute_sql_with_bind("INSERT INTO group_policies (group_name, policy_name, attached_at) VALUES (?, ?, ?)",
                                 [&](sqlite3_stmt* stmt) {
                                     sqlite3_bind_text(stmt, 1, group_name.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_text(stmt, 2, policy_name.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_int64(stmt, 3, now);
                                 });
}

Result<void, String>
DatabaseManager::detach_policy_from_group(const String& group_name, const String& policy_name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return execute_sql_with_bind("DELETE FROM group_policies WHERE group_name = ? AND policy_name = ?",
                                 [&](sqlite3_stmt* stmt) {
                                     sqlite3_bind_text(stmt, 1, group_name.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_text(stmt, 2, policy_name.c_str(), -1, SQLITE_TRANSIENT);
                                 });
}

Result<Vector<String>, String>
DatabaseManager::get_group_policies(const String& group_name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return query_multiple<String>(
        "SELECT policy_name FROM group_policies WHERE group_name = ? ORDER BY attached_at",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_text(stmt, 1, group_name.c_str(), -1, SQLITE_TRANSIENT); },
        [](sqlite3_stmt* stmt) -> String { return reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)); });
}

// ============================================================================
// Service Account Operations
// ============================================================================

Result<DbServiceAccount, String>
DatabaseManager::get_service_account(const String& access_key) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return query_single<DbServiceAccount>(
        "SELECT access_key, secret_key, parent_user, description, expiration, status, created_at, metadata "
        "FROM service_accounts WHERE access_key = ?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_text(stmt, 1, access_key.c_str(), -1, SQLITE_TRANSIENT); },
        [](sqlite3_stmt* stmt) -> DbServiceAccount {
            DbServiceAccount account;
            account.access_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            account.secret_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            account.parent_user = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

            if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
                account.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            }

            if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) {
                account.expiration = sqlite3_column_int64(stmt, 4);
            }

            if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) {
                account.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            }

            account.created_at = sqlite3_column_int64(stmt, 6);

            if (sqlite3_column_type(stmt, 7) != SQLITE_NULL) {
                account.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
            }

            return account;
        });
}

Result<Vector<DbServiceAccount>, String>
DatabaseManager::list_service_accounts(const String& parent_user) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String sql = "SELECT access_key, secret_key, parent_user, description, expiration, status, created_at, metadata "
                 "FROM service_accounts";

    if (!parent_user.empty()) {
        sql += " WHERE parent_user = ?";
    }

    sql += " ORDER BY created_at DESC";

    return query_multiple<DbServiceAccount>(
        sql,
        [&](sqlite3_stmt* stmt) {
            if (!parent_user.empty()) {
                sqlite3_bind_text(stmt, 1, parent_user.c_str(), -1, SQLITE_TRANSIENT);
            }
        },
        [](sqlite3_stmt* stmt) -> DbServiceAccount {
            DbServiceAccount account;
            account.access_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            account.secret_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            account.parent_user = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

            if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
                account.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            }

            if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) {
                account.expiration = sqlite3_column_int64(stmt, 4);
            }

            if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) {
                account.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            }

            account.created_at = sqlite3_column_int64(stmt, 6);

            if (sqlite3_column_type(stmt, 7) != SQLITE_NULL) {
                account.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
            }

            return account;
        });
}

Result<void, String>
DatabaseManager::create_service_account(const DbServiceAccount& account) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return execute_sql_with_bind("INSERT INTO service_accounts (access_key, secret_key, parent_user, description, "
                                 "expiration, status, created_at, metadata) "
                                 "VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
                                 [&](sqlite3_stmt* stmt) {
                                     sqlite3_bind_text(stmt, 1, account.access_key.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_text(stmt, 2, account.secret_key.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_text(stmt, 3, account.parent_user.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_text(stmt, 4, account.description.c_str(), -1, SQLITE_TRANSIENT);

                                     if (account.expiration.has_value()) {
                                         sqlite3_bind_int64(stmt, 5, account.expiration.value());
                                     } else {
                                         sqlite3_bind_null(stmt, 5);
                                     }

                                     sqlite3_bind_text(stmt, 6, account.status.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_int64(stmt, 7, account.created_at);
                                     sqlite3_bind_text(stmt, 8, account.metadata.c_str(), -1, SQLITE_TRANSIENT);
                                 });
}

Result<void, String>
DatabaseManager::delete_service_account(const String& access_key) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return execute_sql_with_bind("DELETE FROM service_accounts WHERE access_key = ?", [&](sqlite3_stmt* stmt) {
        sqlite3_bind_text(stmt, 1, access_key.c_str(), -1, SQLITE_TRANSIENT);
    });
}

// ============================================================================
// Audit Log Operations
// ============================================================================

Result<void, String>
DatabaseManager::add_audit_log(const AuditLogEntry& entry) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return execute_sql_with_bind("INSERT INTO audit_log (timestamp, user_access_key, action, resource_type, "
                                 "resource_id, status, details, ip_address, user_agent) "
                                 "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
                                 [&](sqlite3_stmt* stmt) {
                                     sqlite3_bind_int64(stmt, 1, entry.timestamp);
                                     sqlite3_bind_text(stmt, 2, entry.user_access_key.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_text(stmt, 3, entry.action.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_text(stmt, 4, entry.resource_type.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_text(stmt, 5, entry.resource_id.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_text(stmt, 6, entry.status.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_text(stmt, 7, entry.details.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_text(stmt, 8, entry.ip_address.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_text(stmt, 9, entry.user_agent.c_str(), -1, SQLITE_TRANSIENT);
                                 });
}

Result<Vector<AuditLogEntry>, String>
DatabaseManager::get_audit_logs(const String& user_access_key,
                                int64_t from_timestamp,
                                int64_t to_timestamp,
                                int limit) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String sql = "SELECT id, timestamp, user_access_key, action, resource_type, resource_id, status, details, "
                 "ip_address, user_agent "
                 "FROM audit_log WHERE 1=1";

    if (!user_access_key.empty()) {
        sql += " AND user_access_key = ?";
    }

    if (from_timestamp > 0) {
        sql += " AND timestamp >= ?";
    }

    if (to_timestamp > 0) {
        sql += " AND timestamp <= ?";
    }

    sql += " ORDER BY timestamp DESC";

    if (limit > 0) {
        sql += " LIMIT ?";
    }

    return query_multiple<AuditLogEntry>(
        sql,
        [&](sqlite3_stmt* stmt) {
            int param_idx = 1;

            if (!user_access_key.empty()) {
                sqlite3_bind_text(stmt, param_idx++, user_access_key.c_str(), -1, SQLITE_TRANSIENT);
            }

            if (from_timestamp > 0) {
                sqlite3_bind_int64(stmt, param_idx++, from_timestamp);
            }

            if (to_timestamp > 0) {
                sqlite3_bind_int64(stmt, param_idx++, to_timestamp);
            }

            if (limit > 0) {
                sqlite3_bind_int(stmt, param_idx++, limit);
            }
        },
        [](sqlite3_stmt* stmt) -> AuditLogEntry {
            AuditLogEntry entry;
            entry.id = sqlite3_column_int64(stmt, 0);
            entry.timestamp = sqlite3_column_int64(stmt, 1);

            if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
                entry.user_access_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            }

            entry.action = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            entry.resource_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            entry.resource_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            entry.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));

            if (sqlite3_column_type(stmt, 7) != SQLITE_NULL) {
                entry.details = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
            }

            if (sqlite3_column_type(stmt, 8) != SQLITE_NULL) {
                entry.ip_address = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
            }

            if (sqlite3_column_type(stmt, 9) != SQLITE_NULL) {
                entry.user_agent = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
            }

            return entry;
        });
}

// ============================================================================
// Config Operations
// ============================================================================

Result<String, String>
DatabaseManager::get_config(const String& key) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return query_single<String>(
        "SELECT value FROM config WHERE key = ?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT); },
        [](sqlite3_stmt* stmt) -> String { return reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)); });
}

Result<void, String>
DatabaseManager::set_config(const String& key, const String& value, const String& value_type) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    int64_t now = std::time(nullptr);

    // Use REPLACE to insert or update
    return execute_sql_with_bind("REPLACE INTO config (key, value, value_type, updated_at) VALUES (?, ?, ?, ?)",
                                 [&](sqlite3_stmt* stmt) {
                                     sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_text(stmt, 2, value.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_text(stmt, 3, value_type.c_str(), -1, SQLITE_TRANSIENT);
                                     sqlite3_bind_int64(stmt, 4, now);
                                 });
}

Result<StringMap, String>
DatabaseManager::get_all_config() {
    std::lock_guard<std::mutex> lock(db_mutex_);

    auto results = query_multiple<std::pair<String, String>>(
        "SELECT key, value FROM config ORDER BY key",
        [](sqlite3_stmt*) {},
        [](sqlite3_stmt* stmt) -> std::pair<String, String> {
            String key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            String value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            return {key, value};
        });

    if (!results) {
        return Err<StringMap, String>(results.error());
    }

    StringMap config_map;
    for (const auto& [key, value] : results.value()) {
        config_map[key] = value;
    }

    return Ok<StringMap, String>(config_map);
}

// ============================================================================
// Transaction Operations (Stub - TODO: Implement)
// ============================================================================

Result<void, String>
DatabaseManager::begin_transaction() {
    std::lock_guard<std::mutex> lock(db_mutex_);
    return execute_sql("BEGIN TRANSACTION");
}

Result<void, String>
DatabaseManager::commit_transaction() {
    std::lock_guard<std::mutex> lock(db_mutex_);
    return execute_sql("COMMIT");
}

Result<void, String>
DatabaseManager::rollback_transaction() {
    std::lock_guard<std::mutex> lock(db_mutex_);
    return execute_sql("ROLLBACK");
}

// ============================================================================
// Utility Operations
// ============================================================================

Result<int64_t, String>
DatabaseManager::get_user_count() {
    std::lock_guard<std::mutex> lock(db_mutex_);

    auto result = query_single<int64_t>(
        "SELECT COUNT(*) FROM users",
        [](sqlite3_stmt*) {},
        [](sqlite3_stmt* stmt) -> int64_t { return sqlite3_column_int64(stmt, 0); });

    return result;
}

Result<int64_t, String>
DatabaseManager::get_group_count() {
    std::lock_guard<std::mutex> lock(db_mutex_);

    auto result = query_single<int64_t>(
        "SELECT COUNT(*) FROM groups",
        [](sqlite3_stmt*) {},
        [](sqlite3_stmt* stmt) -> int64_t { return sqlite3_column_int64(stmt, 0); });

    return result;
}

Result<int64_t, String>
DatabaseManager::get_policy_count() {
    std::lock_guard<std::mutex> lock(db_mutex_);

    auto result = query_single<int64_t>(
        "SELECT COUNT(*) FROM policies",
        [](sqlite3_stmt*) {},
        [](sqlite3_stmt* stmt) -> int64_t { return sqlite3_column_int64(stmt, 0); });

    return result;
}

Result<void, String>
DatabaseManager::vacuum() {
    std::lock_guard<std::mutex> lock(db_mutex_);
    CONSOLE_LOG_INFO("Running VACUUM on database");
    return execute_sql("VACUUM");
}

// ============================================================================
// Statistics Operations
// ============================================================================

Result<void, String>
DatabaseManager::add_api_request_stat(const ApiRequestStats& stats) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return execute_sql_with_bind(
        "INSERT INTO api_request_stats (timestamp, endpoint, method, status_code, response_time_ms, "
        "user_access_key, ip_address, user_agent) VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
        [&](sqlite3_stmt* stmt) {
            sqlite3_bind_int64(stmt, 1, stats.timestamp);
            sqlite3_bind_text(stmt, 2, stats.endpoint.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, stats.method.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 4, stats.status_code);
            sqlite3_bind_int(stmt, 5, stats.response_time_ms);
            sqlite3_bind_text(stmt, 6, stats.user_access_key.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 7, stats.ip_address.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 8, stats.user_agent.c_str(), -1, SQLITE_TRANSIENT);
        });
}

Result<Vector<ApiRequestStats>, String>
DatabaseManager::get_api_request_stats(int64_t from_timestamp, int64_t to_timestamp, int status_code_filter) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String query = "SELECT id, timestamp, endpoint, method, status_code, response_time_ms, "
                   "user_access_key, ip_address, user_agent FROM api_request_stats "
                   "WHERE timestamp >= ? AND timestamp <= ?";

    if (status_code_filter > 0) {
        query += " AND status_code = ?";
    }

    query += " ORDER BY timestamp DESC";

    return query_multiple<ApiRequestStats>(
        query,
        [&](sqlite3_stmt* stmt) {
            sqlite3_bind_int64(stmt, 1, from_timestamp);
            sqlite3_bind_int64(stmt, 2, to_timestamp);
            if (status_code_filter > 0) {
                sqlite3_bind_int(stmt, 3, status_code_filter);
            }
        },
        [](sqlite3_stmt* stmt) -> ApiRequestStats {
            ApiRequestStats stats;
            stats.id = sqlite3_column_int64(stmt, 0);
            stats.timestamp = sqlite3_column_int64(stmt, 1);
            stats.endpoint = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            stats.method = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            stats.status_code = sqlite3_column_int(stmt, 4);
            stats.response_time_ms = sqlite3_column_int(stmt, 5);
            if (sqlite3_column_type(stmt, 6) != SQLITE_NULL) {
                stats.user_access_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
            }
            if (sqlite3_column_type(stmt, 7) != SQLITE_NULL) {
                stats.ip_address = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
            }
            if (sqlite3_column_type(stmt, 8) != SQLITE_NULL) {
                stats.user_agent = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
            }
            return stats;
        });
}

Result<void, String>
DatabaseManager::add_throughput_stat(const DataThroughputStats& stats) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return execute_sql_with_bind(
        "INSERT INTO data_throughput_stats (timestamp, read_bytes, write_bytes, total_bytes) VALUES (?, ?, ?, ?)",
        [&](sqlite3_stmt* stmt) {
            sqlite3_bind_int64(stmt, 1, stats.timestamp);
            sqlite3_bind_int64(stmt, 2, stats.read_bytes);
            sqlite3_bind_int64(stmt, 3, stats.write_bytes);
            sqlite3_bind_int64(stmt, 4, stats.total_bytes);
        });
}

Result<Vector<DataThroughputStats>, String>
DatabaseManager::get_throughput_stats(int64_t from_timestamp, int64_t to_timestamp) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return query_multiple<DataThroughputStats>(
        "SELECT id, timestamp, read_bytes, write_bytes, total_bytes FROM data_throughput_stats "
        "WHERE timestamp >= ? AND timestamp <= ? ORDER BY timestamp ASC",
        [&](sqlite3_stmt* stmt) {
            sqlite3_bind_int64(stmt, 1, from_timestamp);
            sqlite3_bind_int64(stmt, 2, to_timestamp);
        },
        [](sqlite3_stmt* stmt) -> DataThroughputStats {
            DataThroughputStats stats;
            stats.id = sqlite3_column_int64(stmt, 0);
            stats.timestamp = sqlite3_column_int64(stmt, 1);
            stats.read_bytes = sqlite3_column_int64(stmt, 2);
            stats.write_bytes = sqlite3_column_int64(stmt, 3);
            stats.total_bytes = sqlite3_column_int64(stmt, 4);
            return stats;
        });
}

Result<DbServer, String>
DatabaseManager::get_server(const String& id) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return query_single<DbServer>(
        "SELECT id, name, endpoint, status, uptime, last_heartbeat, metadata FROM servers WHERE id = ?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT); },
        [](sqlite3_stmt* stmt) -> DbServer {
            DbServer server;
            server.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            server.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            server.endpoint = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            server.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            server.uptime = sqlite3_column_int64(stmt, 4);
            server.last_heartbeat = sqlite3_column_int64(stmt, 5);
            if (sqlite3_column_type(stmt, 6) != SQLITE_NULL) {
                server.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
            }
            return server;
        });
}

Result<Vector<DbServer>, String>
DatabaseManager::list_servers(const String& status) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String query = "SELECT id, name, endpoint, status, uptime, last_heartbeat, metadata FROM servers";
    if (!status.empty()) {
        query += " WHERE status = ?";
    }
    query += " ORDER BY name";

    return query_multiple<DbServer>(
        query,
        [&](sqlite3_stmt* stmt) {
            if (!status.empty()) {
                sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_TRANSIENT);
            }
        },
        [](sqlite3_stmt* stmt) -> DbServer {
            DbServer server;
            server.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            server.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            server.endpoint = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            server.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            server.uptime = sqlite3_column_int64(stmt, 4);
            server.last_heartbeat = sqlite3_column_int64(stmt, 5);
            if (sqlite3_column_type(stmt, 6) != SQLITE_NULL) {
                server.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
            }
            return server;
        });
}

Result<void, String>
DatabaseManager::upsert_server(const DbServer& server) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return execute_sql_with_bind(
        "INSERT INTO servers (id, name, endpoint, status, uptime, last_heartbeat, metadata) "
        "VALUES (?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(id) DO UPDATE SET "
        "name = excluded.name, endpoint = excluded.endpoint, status = excluded.status, "
        "uptime = excluded.uptime, last_heartbeat = excluded.last_heartbeat, metadata = excluded.metadata",
        [&](sqlite3_stmt* stmt) {
            sqlite3_bind_text(stmt, 1, server.id.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, server.name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, server.endpoint.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, server.status.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int64(stmt, 5, server.uptime);
            sqlite3_bind_int64(stmt, 6, server.last_heartbeat);
            sqlite3_bind_text(stmt, 7, server.metadata.c_str(), -1, SQLITE_TRANSIENT);
        });
}

Result<void, String>
DatabaseManager::delete_server(const String& id) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return execute_sql_with_bind("DELETE FROM servers WHERE id = ?", [&](sqlite3_stmt* stmt) {
        sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    });
}

Result<DbDrive, String>
DatabaseManager::get_drive(const String& id) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return query_single<DbDrive>(
        "SELECT id, server_id, path, status, capacity, used, available, last_check, metadata FROM drives WHERE id = ?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT); },
        [](sqlite3_stmt* stmt) -> DbDrive {
            DbDrive drive;
            drive.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            drive.server_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            drive.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            drive.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            drive.capacity = sqlite3_column_int64(stmt, 4);
            drive.used = sqlite3_column_int64(stmt, 5);
            drive.available = sqlite3_column_int64(stmt, 6);
            drive.last_check = sqlite3_column_int64(stmt, 7);
            if (sqlite3_column_type(stmt, 8) != SQLITE_NULL) {
                drive.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
            }
            return drive;
        });
}

Result<Vector<DbDrive>, String>
DatabaseManager::list_drives(const String& server_id, const String& status) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String query =
        "SELECT id, server_id, path, status, capacity, used, available, last_check, metadata FROM drives WHERE 1=1";
    if (!server_id.empty()) {
        query += " AND server_id = ?";
    }
    if (!status.empty()) {
        query += " AND status = ?";
    }
    query += " ORDER BY path";

    return query_multiple<DbDrive>(
        query,
        [&](sqlite3_stmt* stmt) {
            int param = 1;
            if (!server_id.empty()) {
                sqlite3_bind_text(stmt, param++, server_id.c_str(), -1, SQLITE_TRANSIENT);
            }
            if (!status.empty()) {
                sqlite3_bind_text(stmt, param++, status.c_str(), -1, SQLITE_TRANSIENT);
            }
        },
        [](sqlite3_stmt* stmt) -> DbDrive {
            DbDrive drive;
            drive.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            drive.server_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            drive.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            drive.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            drive.capacity = sqlite3_column_int64(stmt, 4);
            drive.used = sqlite3_column_int64(stmt, 5);
            drive.available = sqlite3_column_int64(stmt, 6);
            drive.last_check = sqlite3_column_int64(stmt, 7);
            if (sqlite3_column_type(stmt, 8) != SQLITE_NULL) {
                drive.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
            }
            return drive;
        });
}

Result<void, String>
DatabaseManager::upsert_drive(const DbDrive& drive) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return execute_sql_with_bind(
        "INSERT INTO drives (id, server_id, path, status, capacity, used, available, last_check, metadata) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(id) DO UPDATE SET "
        "server_id = excluded.server_id, path = excluded.path, status = excluded.status, "
        "capacity = excluded.capacity, used = excluded.used, available = excluded.available, "
        "last_check = excluded.last_check, metadata = excluded.metadata",
        [&](sqlite3_stmt* stmt) {
            sqlite3_bind_text(stmt, 1, drive.id.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, drive.server_id.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, drive.path.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, drive.status.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int64(stmt, 5, drive.capacity);
            sqlite3_bind_int64(stmt, 6, drive.used);
            sqlite3_bind_int64(stmt, 7, drive.available);
            sqlite3_bind_int64(stmt, 8, drive.last_check);
            sqlite3_bind_text(stmt, 9, drive.metadata.c_str(), -1, SQLITE_TRANSIENT);
        });
}

Result<void, String>
DatabaseManager::delete_drive(const String& id) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return execute_sql_with_bind("DELETE FROM drives WHERE id = ?", [&](sqlite3_stmt* stmt) {
        sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    });
}

Result<DbStoragePool, String>
DatabaseManager::get_storage_pool(const String& id) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return query_single<DbStoragePool>(
        "SELECT id, name, capacity, used, available, drives_count, online_drives, offline_drives, last_update, "
        "metadata "
        "FROM storage_pools WHERE id = ?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT); },
        [](sqlite3_stmt* stmt) -> DbStoragePool {
            DbStoragePool pool;
            pool.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            pool.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            pool.capacity = sqlite3_column_int64(stmt, 2);
            pool.used = sqlite3_column_int64(stmt, 3);
            pool.available = sqlite3_column_int64(stmt, 4);
            pool.drives_count = sqlite3_column_int(stmt, 5);
            pool.online_drives = sqlite3_column_int(stmt, 6);
            pool.offline_drives = sqlite3_column_int(stmt, 7);
            pool.last_update = sqlite3_column_int64(stmt, 8);
            if (sqlite3_column_type(stmt, 9) != SQLITE_NULL) {
                pool.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
            }
            return pool;
        });
}

Result<Vector<DbStoragePool>, String>
DatabaseManager::list_storage_pools() {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return query_multiple<DbStoragePool>(
        "SELECT id, name, capacity, used, available, drives_count, online_drives, offline_drives, last_update, "
        "metadata "
        "FROM storage_pools ORDER BY name",
        [](sqlite3_stmt*) {},
        [](sqlite3_stmt* stmt) -> DbStoragePool {
            DbStoragePool pool;
            pool.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            pool.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            pool.capacity = sqlite3_column_int64(stmt, 2);
            pool.used = sqlite3_column_int64(stmt, 3);
            pool.available = sqlite3_column_int64(stmt, 4);
            pool.drives_count = sqlite3_column_int(stmt, 5);
            pool.online_drives = sqlite3_column_int(stmt, 6);
            pool.offline_drives = sqlite3_column_int(stmt, 7);
            pool.last_update = sqlite3_column_int64(stmt, 8);
            if (sqlite3_column_type(stmt, 9) != SQLITE_NULL) {
                pool.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
            }
            return pool;
        });
}

Result<void, String>
DatabaseManager::upsert_storage_pool(const DbStoragePool& pool) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return execute_sql_with_bind(
        "INSERT INTO storage_pools (id, name, capacity, used, available, drives_count, online_drives, offline_drives, "
        "last_update, metadata) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(id) DO UPDATE SET "
        "name = excluded.name, capacity = excluded.capacity, used = excluded.used, available = excluded.available, "
        "drives_count = excluded.drives_count, online_drives = excluded.online_drives, "
        "offline_drives = excluded.offline_drives, last_update = excluded.last_update, metadata = excluded.metadata",
        [&](sqlite3_stmt* stmt) {
            sqlite3_bind_text(stmt, 1, pool.id.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, pool.name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int64(stmt, 3, pool.capacity);
            sqlite3_bind_int64(stmt, 4, pool.used);
            sqlite3_bind_int64(stmt, 5, pool.available);
            sqlite3_bind_int(stmt, 6, pool.drives_count);
            sqlite3_bind_int(stmt, 7, pool.online_drives);
            sqlite3_bind_int(stmt, 8, pool.offline_drives);
            sqlite3_bind_int64(stmt, 9, pool.last_update);
            sqlite3_bind_text(stmt, 10, pool.metadata.c_str(), -1, SQLITE_TRANSIENT);
        });
}

Result<void, String>
DatabaseManager::delete_storage_pool(const String& id) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return execute_sql_with_bind("DELETE FROM storage_pools WHERE id = ?", [&](sqlite3_stmt* stmt) {
        sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    });
}

Result<void, String>
DatabaseManager::cleanup_old_api_stats(int64_t older_than_timestamp) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return execute_sql_with_bind("DELETE FROM api_request_stats WHERE timestamp < ?",
                                 [&](sqlite3_stmt* stmt) { sqlite3_bind_int64(stmt, 1, older_than_timestamp); });
}

Result<void, String>
DatabaseManager::cleanup_old_throughput_stats(int64_t older_than_timestamp) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    return execute_sql_with_bind("DELETE FROM data_throughput_stats WHERE timestamp < ?",
                                 [&](sqlite3_stmt* stmt) { sqlite3_bind_int64(stmt, 1, older_than_timestamp); });
}

} // namespace console::storage
