#pragma once

#include "console/common/Types.hpp"
#include "console/models/Group.hpp"
#include "console/models/Policy.hpp"
#include "console/models/User.hpp"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/write_batch.h>
#include <thread>

namespace console::storage {

/**
 * @brief User data for database
 */
struct DbUser {
    String access_key;
    String secret_key; // Hashed
    String account_name;
    String status; // active, disabled
    bool is_admin{false};
    int64_t created_at{0};
    int64_t updated_at{0};
    String metadata; // JSON
};

/**
 * @brief Group data for database
 */
struct DbGroup {
    String name;
    String description;
    String status; // active, disabled
    int64_t created_at{0};
    int64_t updated_at{0};
    String metadata; // JSON
};

/**
 * @brief Policy data for database
 */
struct DbPolicy {
    String name;
    String version;  // Default: "2012-10-17"
    String document; // JSON policy document
    String description;
    int64_t created_at{0};
    int64_t updated_at{0};
    String metadata; // JSON
};

/**
 * @brief Service account data
 */
struct DbServiceAccount {
    String access_key;
    String secret_key; // Hashed
    String parent_user;
    String description;
    Optional<int64_t> expiration; // Unix timestamp
    String status;                // active, disabled
    int64_t created_at{0};
    String metadata; // JSON
};

/**
 * @brief Audit log entry
 */
struct AuditLogEntry {
    int64_t id{0};
    int64_t timestamp{0};
    String user_access_key;
    String action;        // CREATE_USER, DELETE_POLICY, etc.
    String resource_type; // user, group, policy
    String resource_id;
    String status;  // success, failure
    String details; // JSON
    String ip_address;
    String user_agent;
};

/**
 * @brief API request stats entry
 */
struct ApiRequestStats {
    int64_t id{0};
    int64_t timestamp{0};
    String endpoint;
    String method;
    int status_code{0};
    int response_time_ms{0};
    String user_access_key;
    String ip_address;
    String user_agent;
};

/**
 * @brief Data throughput stats entry
 */
struct DataThroughputStats {
    int64_t id{0};
    int64_t timestamp{0};
    int64_t read_bytes{0};
    int64_t write_bytes{0};
    int64_t total_bytes{0};
};

/**
 * @brief Server entry
 */
struct DbServer {
    String id;
    String name;
    String endpoint;
    String status; // online, offline
    int64_t uptime{0};
    int64_t last_heartbeat{0};
    String metadata; // JSON
};

/**
 * @brief Drive entry
 */
struct DbDrive {
    String id;
    String server_id;
    String path;
    String status; // online, offline
    int64_t capacity{0};
    int64_t used{0};
    int64_t available{0};
    int64_t last_check{0};
    String metadata; // JSON
};

/**
 * @brief Storage pool entry
 */
struct DbStoragePool {
    String id;
    String name;
    int64_t capacity{0};
    int64_t used{0};
    int64_t available{0};
    int drives_count{0};
    int online_drives{0};
    int offline_drives{0};
    int64_t last_update{0};
    String metadata; // JSON
};

/**
 * @brief Dashboard board entry
 */
struct DbDashboardBoard {
    String id;
    String user_id;
    String name;
    int order_index{0};
    int64_t created_at{0};
    int64_t updated_at{0};
};

/**
 * @brief Dashboard widget entry
 */
struct DbDashboardWidget {
    String id;
    String board_id;
    String widget_type; // capacity, servers, drives, buckets, api_errors, throughput, encryption, pools, quick_actions
    int position_x{0};
    int position_y{0};
    int width{1};
    int height{1};
    bool visible{true};
    int refresh_interval{30}; // seconds, 0 = disabled
    String settings_json;     // JSON for widget-specific settings
    int order_index{0};
};

/**
 * @brief Async callback type
 */
template <typename T>
using AsyncCallback = std::function<void(Result<T, String>)>;

/**
 * @brief RocksDB-based NoSQL database manager with encryption support
 *
 * High-performance key-value storage using RocksDB.
 * Uses column families for different data types (users, groups, policies, etc.)
 */
class DatabaseManager {
  public:
    /**
     * @brief Construct DatabaseManager with optional encryption
     *
     * @param db_path Path to RocksDB database directory
     * @param encryption_key Optional encryption key (32 bytes for AES-256)
     * @param thread_pool_size Number of worker threads for async operations
     */
    explicit DatabaseManager(const String& db_path, const String& encryption_key = "", size_t thread_pool_size = 4);

    ~DatabaseManager();

    // Prevent copying
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    // ========================================================================
    // Initialization
    // ========================================================================

    /**
     * @brief Initialize database and column families
     */
    Result<void, String> initialize();

    /**
     * @brief Check if database is ready
     */
    bool is_ready() const;

    // ========================================================================
    // User operations
    // ========================================================================

    Result<DbUser, String> get_user(const String& access_key);
    Result<Vector<DbUser>, String> list_users(const String& status = "");
    Result<void, String> create_user(const DbUser& user);
    Result<void, String> update_user(const DbUser& user);
    Result<void, String> delete_user(const String& access_key);
    Result<bool, String> user_exists(const String& access_key);

    // Async versions
    void get_user_async(const String& access_key, AsyncCallback<DbUser> callback);
    void create_user_async(const DbUser& user, AsyncCallback<void> callback);

    // ========================================================================
    // Group operations
    // ========================================================================

    Result<DbGroup, String> get_group(const String& name);
    Result<Vector<DbGroup>, String> list_groups(const String& status = "");
    Result<void, String> create_group(const DbGroup& group);
    Result<void, String> update_group(const DbGroup& group);
    Result<void, String> delete_group(const String& name);
    Result<bool, String> group_exists(const String& name);

    // User-Group relationships
    Result<void, String> add_user_to_group(const String& access_key, const String& group_name);
    Result<void, String> remove_user_from_group(const String& access_key, const String& group_name);
    Result<Vector<String>, String> get_user_groups(const String& access_key);
    Result<Vector<String>, String> get_group_users(const String& group_name);

    // ========================================================================
    // Policy operations
    // ========================================================================

    Result<DbPolicy, String> get_policy(const String& name);
    Result<Vector<DbPolicy>, String> list_policies();
    Result<void, String> create_policy(const DbPolicy& policy);
    Result<void, String> update_policy(const DbPolicy& policy);
    Result<void, String> delete_policy(const String& name);
    Result<bool, String> policy_exists(const String& name);

    // User-Policy relationships
    Result<void, String> attach_policy_to_user(const String& access_key, const String& policy_name);
    Result<void, String> detach_policy_from_user(const String& access_key, const String& policy_name);
    Result<Vector<String>, String> get_user_policies(const String& access_key, bool include_group_policies = true);

    // Group-Policy relationships
    Result<void, String> attach_policy_to_group(const String& group_name, const String& policy_name);
    Result<void, String> detach_policy_from_group(const String& group_name, const String& policy_name);
    Result<Vector<String>, String> get_group_policies(const String& group_name);

    // ========================================================================
    // Service Account operations
    // ========================================================================

    Result<DbServiceAccount, String> get_service_account(const String& access_key);
    Result<Vector<DbServiceAccount>, String> list_service_accounts(const String& parent_user = "");
    Result<void, String> create_service_account(const DbServiceAccount& account);
    Result<void, String> delete_service_account(const String& access_key);

    // ========================================================================
    // Audit log
    // ========================================================================

    Result<void, String> add_audit_log(const AuditLogEntry& entry);
    Result<Vector<AuditLogEntry>, String> get_audit_logs(const String& user_access_key = "",
                                                         int64_t from_timestamp = 0,
                                                         int64_t to_timestamp = 0,
                                                         int limit = 100);

    // ========================================================================
    // Config operations
    // ========================================================================

    Result<String, String> get_config(const String& key);
    Result<void, String> set_config(const String& key, const String& value, const String& value_type = "string");
    Result<StringMap, String> get_all_config();

    // ========================================================================
    // Statistics operations
    // ========================================================================

    // API request stats
    Result<void, String> add_api_request_stat(const ApiRequestStats& stats);
    Result<Vector<ApiRequestStats>, String> get_api_request_stats(int64_t from_timestamp,
                                                                  int64_t to_timestamp,
                                                                  int status_code_filter = 0);

    // Data throughput stats
    Result<void, String> add_throughput_stat(const DataThroughputStats& stats);
    Result<Vector<DataThroughputStats>, String> get_throughput_stats(int64_t from_timestamp, int64_t to_timestamp);

    // Server operations
    Result<DbServer, String> get_server(const String& id);
    Result<Vector<DbServer>, String> list_servers(const String& status = "");
    Result<void, String> upsert_server(const DbServer& server);
    Result<void, String> delete_server(const String& id);

    // Drive operations
    Result<DbDrive, String> get_drive(const String& id);
    Result<Vector<DbDrive>, String> list_drives(const String& server_id = "", const String& status = "");
    Result<void, String> upsert_drive(const DbDrive& drive);
    Result<void, String> delete_drive(const String& id);

    // Storage pool operations
    Result<DbStoragePool, String> get_storage_pool(const String& id);
    Result<Vector<DbStoragePool>, String> list_storage_pools();
    Result<void, String> upsert_storage_pool(const DbStoragePool& pool);
    Result<void, String> delete_storage_pool(const String& id);

    // Cleanup old stats
    Result<void, String> cleanup_old_api_stats(int64_t older_than_timestamp);
    Result<void, String> cleanup_old_throughput_stats(int64_t older_than_timestamp);

    // ========================================================================
    // Dashboard operations
    // ========================================================================

    // Dashboard board operations
    Result<DbDashboardBoard, String> get_dashboard_board(const String& id);
    Result<Vector<DbDashboardBoard>, String> list_dashboard_boards(const String& user_id);
    Result<void, String> create_dashboard_board(const DbDashboardBoard& board);
    Result<void, String> update_dashboard_board(const DbDashboardBoard& board);
    Result<void, String> delete_dashboard_board(const String& id);

    // Dashboard widget operations
    Result<DbDashboardWidget, String> get_dashboard_widget(const String& id);
    Result<Vector<DbDashboardWidget>, String> list_dashboard_widgets(const String& board_id);
    Result<void, String> create_dashboard_widget(const DbDashboardWidget& widget);
    Result<void, String> update_dashboard_widget(const DbDashboardWidget& widget);
    Result<void, String> delete_dashboard_widget(const String& id);
    Result<void, String> save_dashboard_widgets(const String& board_id, const Vector<DbDashboardWidget>& widgets);

    // ========================================================================
    // Utility operations
    // ========================================================================

    Result<void, String> begin_transaction();
    Result<void, String> commit_transaction();
    Result<void, String> rollback_transaction();

    Result<int64_t, String> get_user_count();
    Result<int64_t, String> get_group_count();
    Result<int64_t, String> get_policy_count();

    /**
     * @brief Compact database to reclaim space (equivalent to VACUUM in SQLite)
     */
    Result<void, String> compact();

  private:
    // ========================================================================
    // Column family names
    // ========================================================================
    static constexpr const char* CF_DEFAULT = "default";
    static constexpr const char* CF_USERS = "users";
    static constexpr const char* CF_GROUPS = "groups";
    static constexpr const char* CF_POLICIES = "policies";
    static constexpr const char* CF_USER_GROUPS = "user_groups";
    static constexpr const char* CF_USER_POLICIES = "user_policies";
    static constexpr const char* CF_GROUP_POLICIES = "group_policies";
    static constexpr const char* CF_SERVICE_ACCOUNTS = "service_accounts";
    static constexpr const char* CF_AUDIT_LOG = "audit_log";
    static constexpr const char* CF_CONFIG = "config";
    static constexpr const char* CF_API_STATS = "api_stats";
    static constexpr const char* CF_THROUGHPUT_STATS = "throughput_stats";
    static constexpr const char* CF_SERVERS = "servers";
    static constexpr const char* CF_DRIVES = "drives";
    static constexpr const char* CF_STORAGE_POOLS = "storage_pools";
    static constexpr const char* CF_DASHBOARD_BOARDS = "dashboard_boards";
    static constexpr const char* CF_DASHBOARD_WIDGETS = "dashboard_widgets";

    // ========================================================================
    // Helper methods
    // ========================================================================

    rocksdb::ColumnFamilyHandle* get_cf_handle(const String& cf_name);

    template <typename T>
    String serialize(const T& obj);

    template <typename T>
    T deserialize(const String& json);

    // Thread pool for async operations
    template <typename Func>
    void submit_async(Func&& func);

    void create_default_data();

    // ========================================================================
    // Member variables
    // ========================================================================

    std::unique_ptr<rocksdb::DB> db_;
    String db_path_;
    String encryption_key_;
    mutable std::mutex db_mutex_;
    bool is_ready_{false};

    // Column family handles
    std::vector<rocksdb::ColumnFamilyHandle*> cf_handles_;
    std::unordered_map<String, rocksdb::ColumnFamilyHandle*> cf_map_;

    // Transaction support (write batch)
    std::unique_ptr<rocksdb::WriteBatch> current_batch_;
    bool in_transaction_{false};

    // Thread pool for async operations
    size_t thread_pool_size_;
    Vector<std::thread> worker_threads_;
    std::queue<std::function<void()>> task_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::atomic<bool> stop_workers_{false};
};

} // namespace console::storage
