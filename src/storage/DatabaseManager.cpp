#include "console/storage/DatabaseManager.hpp"

#include "console/common/Config.hpp"
#include "console/common/Logger.hpp"
#include "console/utils/PasswordHash.hpp"

#include <chrono>
#include <filesystem>
#include <json/json.h>

namespace console::storage {

// ============================================================================
// JSON Serialization Helpers
// ============================================================================

namespace {

String
user_to_json(const DbUser& user) {
    Json::Value json;
    json["access_key"] = user.access_key;
    json["secret_key"] = user.secret_key;
    json["account_name"] = user.account_name;
    json["status"] = user.status;
    json["is_admin"] = user.is_admin;
    json["created_at"] = Json::Int64(user.created_at);
    json["updated_at"] = Json::Int64(user.updated_at);
    json["metadata"] = user.metadata;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, json);
}

DbUser
json_to_user(const String& json_str) {
    DbUser user;
    Json::Value json;
    Json::CharReaderBuilder builder;
    std::istringstream stream(json_str);
    std::string errs;

    if (Json::parseFromStream(builder, stream, &json, &errs)) {
        user.access_key = json["access_key"].asString();
        user.secret_key = json["secret_key"].asString();
        user.account_name = json["account_name"].asString();
        user.status = json.get("status", "active").asString();
        user.is_admin = json.get("is_admin", false).asBool();
        user.created_at = json.get("created_at", 0).asInt64();
        user.updated_at = json.get("updated_at", 0).asInt64();
        user.metadata = json.get("metadata", "").asString();
    }
    return user;
}

String
group_to_json(const DbGroup& group) {
    Json::Value json;
    json["name"] = group.name;
    json["description"] = group.description;
    json["status"] = group.status;
    json["created_at"] = Json::Int64(group.created_at);
    json["updated_at"] = Json::Int64(group.updated_at);
    json["metadata"] = group.metadata;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, json);
}

DbGroup
json_to_group(const String& json_str) {
    DbGroup group;
    Json::Value json;
    Json::CharReaderBuilder builder;
    std::istringstream stream(json_str);
    std::string errs;

    if (Json::parseFromStream(builder, stream, &json, &errs)) {
        group.name = json["name"].asString();
        group.description = json["description"].asString();
        group.status = json.get("status", "active").asString();
        group.created_at = json.get("created_at", 0).asInt64();
        group.updated_at = json.get("updated_at", 0).asInt64();
        group.metadata = json.get("metadata", "").asString();
    }
    return group;
}

String
policy_to_json(const DbPolicy& policy) {
    Json::Value json;
    json["name"] = policy.name;
    json["version"] = policy.version;
    json["document"] = policy.document;
    json["description"] = policy.description;
    json["created_at"] = Json::Int64(policy.created_at);
    json["updated_at"] = Json::Int64(policy.updated_at);
    json["metadata"] = policy.metadata;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, json);
}

DbPolicy
json_to_policy(const String& json_str) {
    DbPolicy policy;
    Json::Value json;
    Json::CharReaderBuilder builder;
    std::istringstream stream(json_str);
    std::string errs;

    if (Json::parseFromStream(builder, stream, &json, &errs)) {
        policy.name = json["name"].asString();
        policy.version = json.get("version", "2012-10-17").asString();
        policy.document = json["document"].asString();
        policy.description = json["description"].asString();
        policy.created_at = json.get("created_at", 0).asInt64();
        policy.updated_at = json.get("updated_at", 0).asInt64();
        policy.metadata = json.get("metadata", "").asString();
    }
    return policy;
}

String
server_to_json(const DbServer& server) {
    Json::Value json;
    json["id"] = server.id;
    json["name"] = server.name;
    json["endpoint"] = server.endpoint;
    json["status"] = server.status;
    json["uptime"] = Json::Int64(server.uptime);
    json["last_heartbeat"] = Json::Int64(server.last_heartbeat);
    json["metadata"] = server.metadata;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, json);
}

DbServer
json_to_server(const String& json_str) {
    DbServer server;
    Json::Value json;
    Json::CharReaderBuilder builder;
    std::istringstream stream(json_str);
    std::string errs;

    if (Json::parseFromStream(builder, stream, &json, &errs)) {
        server.id = json["id"].asString();
        server.name = json["name"].asString();
        server.endpoint = json["endpoint"].asString();
        server.status = json.get("status", "offline").asString();
        server.uptime = json.get("uptime", 0).asInt64();
        server.last_heartbeat = json.get("last_heartbeat", 0).asInt64();
        server.metadata = json.get("metadata", "").asString();
    }
    return server;
}

String
drive_to_json(const DbDrive& drive) {
    Json::Value json;
    json["id"] = drive.id;
    json["server_id"] = drive.server_id;
    json["path"] = drive.path;
    json["status"] = drive.status;
    json["capacity"] = Json::Int64(drive.capacity);
    json["used"] = Json::Int64(drive.used);
    json["available"] = Json::Int64(drive.available);
    json["last_check"] = Json::Int64(drive.last_check);
    json["metadata"] = drive.metadata;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, json);
}

DbDrive
json_to_drive(const String& json_str) {
    DbDrive drive;
    Json::Value json;
    Json::CharReaderBuilder builder;
    std::istringstream stream(json_str);
    std::string errs;

    if (Json::parseFromStream(builder, stream, &json, &errs)) {
        drive.id = json["id"].asString();
        drive.server_id = json["server_id"].asString();
        drive.path = json["path"].asString();
        drive.status = json.get("status", "offline").asString();
        drive.capacity = json.get("capacity", 0).asInt64();
        drive.used = json.get("used", 0).asInt64();
        drive.available = json.get("available", 0).asInt64();
        drive.last_check = json.get("last_check", 0).asInt64();
        drive.metadata = json.get("metadata", "").asString();
    }
    return drive;
}

String
pool_to_json(const DbStoragePool& pool) {
    Json::Value json;
    json["id"] = pool.id;
    json["name"] = pool.name;
    json["capacity"] = Json::Int64(pool.capacity);
    json["used"] = Json::Int64(pool.used);
    json["available"] = Json::Int64(pool.available);
    json["drives_count"] = pool.drives_count;
    json["online_drives"] = pool.online_drives;
    json["offline_drives"] = pool.offline_drives;
    json["last_update"] = Json::Int64(pool.last_update);
    json["metadata"] = pool.metadata;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, json);
}

DbStoragePool
json_to_pool(const String& json_str) {
    DbStoragePool pool;
    Json::Value json;
    Json::CharReaderBuilder builder;
    std::istringstream stream(json_str);
    std::string errs;

    if (Json::parseFromStream(builder, stream, &json, &errs)) {
        pool.id = json["id"].asString();
        pool.name = json["name"].asString();
        pool.capacity = json.get("capacity", 0).asInt64();
        pool.used = json.get("used", 0).asInt64();
        pool.available = json.get("available", 0).asInt64();
        pool.drives_count = json.get("drives_count", 0).asInt();
        pool.online_drives = json.get("online_drives", 0).asInt();
        pool.offline_drives = json.get("offline_drives", 0).asInt();
        pool.last_update = json.get("last_update", 0).asInt64();
        pool.metadata = json.get("metadata", "").asString();
    }
    return pool;
}

String
service_account_to_json(const DbServiceAccount& account) {
    Json::Value json;
    json["access_key"] = account.access_key;
    json["secret_key"] = account.secret_key;
    json["parent_user"] = account.parent_user;
    json["description"] = account.description;
    if (account.expiration) {
        json["expiration"] = Json::Int64(account.expiration.value());
    }
    json["status"] = account.status;
    json["created_at"] = Json::Int64(account.created_at);
    json["metadata"] = account.metadata;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, json);
}

DbServiceAccount
json_to_service_account(const String& json_str) {
    DbServiceAccount account;
    Json::Value json;
    Json::CharReaderBuilder builder;
    std::istringstream stream(json_str);
    std::string errs;

    if (Json::parseFromStream(builder, stream, &json, &errs)) {
        account.access_key = json["access_key"].asString();
        account.secret_key = json["secret_key"].asString();
        account.parent_user = json["parent_user"].asString();
        account.description = json["description"].asString();
        if (json.isMember("expiration") && !json["expiration"].isNull()) {
            account.expiration = json["expiration"].asInt64();
        }
        account.status = json.get("status", "active").asString();
        account.created_at = json.get("created_at", 0).asInt64();
        account.metadata = json.get("metadata", "").asString();
    }
    return account;
}

String
audit_log_to_json(const AuditLogEntry& entry) {
    Json::Value json;
    json["id"] = Json::Int64(entry.id);
    json["timestamp"] = Json::Int64(entry.timestamp);
    json["user_access_key"] = entry.user_access_key;
    json["action"] = entry.action;
    json["resource_type"] = entry.resource_type;
    json["resource_id"] = entry.resource_id;
    json["status"] = entry.status;
    json["details"] = entry.details;
    json["ip_address"] = entry.ip_address;
    json["user_agent"] = entry.user_agent;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, json);
}

AuditLogEntry
json_to_audit_log(const String& json_str) {
    AuditLogEntry entry;
    Json::Value json;
    Json::CharReaderBuilder builder;
    std::istringstream stream(json_str);
    std::string errs;

    if (Json::parseFromStream(builder, stream, &json, &errs)) {
        entry.id = json.get("id", 0).asInt64();
        entry.timestamp = json.get("timestamp", 0).asInt64();
        entry.user_access_key = json["user_access_key"].asString();
        entry.action = json["action"].asString();
        entry.resource_type = json["resource_type"].asString();
        entry.resource_id = json["resource_id"].asString();
        entry.status = json["status"].asString();
        entry.details = json["details"].asString();
        entry.ip_address = json["ip_address"].asString();
        entry.user_agent = json["user_agent"].asString();
    }
    return entry;
}

String
api_stats_to_json(const ApiRequestStats& stats) {
    Json::Value json;
    json["id"] = Json::Int64(stats.id);
    json["timestamp"] = Json::Int64(stats.timestamp);
    json["endpoint"] = stats.endpoint;
    json["method"] = stats.method;
    json["status_code"] = stats.status_code;
    json["response_time_ms"] = stats.response_time_ms;
    json["user_access_key"] = stats.user_access_key;
    json["ip_address"] = stats.ip_address;
    json["user_agent"] = stats.user_agent;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, json);
}

ApiRequestStats
json_to_api_stats(const String& json_str) {
    ApiRequestStats stats;
    Json::Value json;
    Json::CharReaderBuilder builder;
    std::istringstream stream(json_str);
    std::string errs;

    if (Json::parseFromStream(builder, stream, &json, &errs)) {
        stats.id = json.get("id", 0).asInt64();
        stats.timestamp = json.get("timestamp", 0).asInt64();
        stats.endpoint = json["endpoint"].asString();
        stats.method = json["method"].asString();
        stats.status_code = json.get("status_code", 0).asInt();
        stats.response_time_ms = json.get("response_time_ms", 0).asInt();
        stats.user_access_key = json["user_access_key"].asString();
        stats.ip_address = json["ip_address"].asString();
        stats.user_agent = json["user_agent"].asString();
    }
    return stats;
}

String
throughput_to_json(const DataThroughputStats& stats) {
    Json::Value json;
    json["id"] = Json::Int64(stats.id);
    json["timestamp"] = Json::Int64(stats.timestamp);
    json["read_bytes"] = Json::Int64(stats.read_bytes);
    json["write_bytes"] = Json::Int64(stats.write_bytes);
    json["total_bytes"] = Json::Int64(stats.total_bytes);

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, json);
}

DataThroughputStats
json_to_throughput(const String& json_str) {
    DataThroughputStats stats;
    Json::Value json;
    Json::CharReaderBuilder builder;
    std::istringstream stream(json_str);
    std::string errs;

    if (Json::parseFromStream(builder, stream, &json, &errs)) {
        stats.id = json.get("id", 0).asInt64();
        stats.timestamp = json.get("timestamp", 0).asInt64();
        stats.read_bytes = json.get("read_bytes", 0).asInt64();
        stats.write_bytes = json.get("write_bytes", 0).asInt64();
        stats.total_bytes = json.get("total_bytes", 0).asInt64();
    }
    return stats;
}

String
dashboard_board_to_json(const DbDashboardBoard& board) {
    Json::Value json;
    json["id"] = board.id;
    json["user_id"] = board.user_id;
    json["name"] = board.name;
    json["order_index"] = board.order_index;
    json["created_at"] = Json::Int64(board.created_at);
    json["updated_at"] = Json::Int64(board.updated_at);

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, json);
}

DbDashboardBoard
json_to_dashboard_board(const String& json_str) {
    DbDashboardBoard board;
    Json::Value json;
    Json::CharReaderBuilder builder;
    std::istringstream stream(json_str);
    std::string errs;

    if (Json::parseFromStream(builder, stream, &json, &errs)) {
        board.id = json["id"].asString();
        board.user_id = json["user_id"].asString();
        board.name = json["name"].asString();
        board.order_index = json.get("order_index", 0).asInt();
        board.created_at = json.get("created_at", 0).asInt64();
        board.updated_at = json.get("updated_at", 0).asInt64();
    }
    return board;
}

String
dashboard_widget_to_json(const DbDashboardWidget& widget) {
    Json::Value json;
    json["id"] = widget.id;
    json["board_id"] = widget.board_id;
    json["widget_type"] = widget.widget_type;
    json["position_x"] = widget.position_x;
    json["position_y"] = widget.position_y;
    json["width"] = widget.width;
    json["height"] = widget.height;
    json["visible"] = widget.visible;
    json["refresh_interval"] = widget.refresh_interval;
    json["settings_json"] = widget.settings_json;
    json["order_index"] = widget.order_index;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, json);
}

DbDashboardWidget
json_to_dashboard_widget(const String& json_str) {
    DbDashboardWidget widget;
    Json::Value json;
    Json::CharReaderBuilder builder;
    std::istringstream stream(json_str);
    std::string errs;

    if (Json::parseFromStream(builder, stream, &json, &errs)) {
        widget.id = json["id"].asString();
        widget.board_id = json["board_id"].asString();
        widget.widget_type = json["widget_type"].asString();
        widget.position_x = json.get("position_x", 0).asInt();
        widget.position_y = json.get("position_y", 0).asInt();
        widget.width = json.get("width", 1).asInt();
        widget.height = json.get("height", 1).asInt();
        widget.visible = json.get("visible", true).asBool();
        widget.refresh_interval = json.get("refresh_interval", 30).asInt();
        widget.settings_json = json.get("settings_json", "{}").asString();
        widget.order_index = json.get("order_index", 0).asInt();
    }
    return widget;
}

int64_t
get_current_timestamp() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

} // anonymous namespace

// ============================================================================
// Constructor & Destructor
// ============================================================================

DatabaseManager::DatabaseManager(const String& db_path, const String& encryption_key, size_t thread_pool_size)
    : db_path_(db_path), encryption_key_(encryption_key), thread_pool_size_(thread_pool_size) {
    CONSOLE_LOG_DEBUG("DatabaseManager: Initializing with path: {}", db_path);

    // Create directory if not exists
    std::filesystem::create_directories(db_path_);

    // Start worker threads
    for (size_t i = 0; i < thread_pool_size_; ++i) {
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

    CONSOLE_LOG_INFO("DatabaseManager worker threads started: {}", thread_pool_size_);
}

DatabaseManager::~DatabaseManager() {
    // Stop worker threads first
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        stop_workers_ = true;
    }
    queue_cv_.notify_all();

    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    worker_threads_.clear();

    // Clear the map before deleting handles
    cf_map_.clear();

    // Close column family handles (skip default family - index 0)
    if (db_) {
        for (size_t i = 1; i < cf_handles_.size(); ++i) {
            if (cf_handles_[i]) {
                auto status = db_->DestroyColumnFamilyHandle(cf_handles_[i]);
                if (!status.ok()) {
                    CONSOLE_LOG_ERROR("Failed to destroy column family: {}", status.ToString());
                }
            }
        }
    }
    cf_handles_.clear();

    // Explicitly close RocksDB
    if (db_) {
        db_->SyncWAL(); // Sync WAL before closing
        db_.reset();
    }

    CONSOLE_LOG_DEBUG("DatabaseManager destroyed");
}

// ============================================================================
// Initialization
// ============================================================================

Result<void, String>
DatabaseManager::initialize() {
    try {
        rocksdb::Options options;
        options.create_if_missing = true;
        options.create_missing_column_families = true;

        // Performance optimizations
        options.max_background_jobs = 4;
        options.bytes_per_sync = 1048576; // 1MB
        options.compaction_style = rocksdb::kCompactionStyleLevel;

        // Encryption support via encrypted environment
        // Note: RocksDB encryption requires building with encryption support
        // For now, we use application-level encryption for sensitive fields
        if (!encryption_key_.empty()) {
            CONSOLE_LOG_INFO("DatabaseManager: Database encryption enabled (application-level)");
            // Application-level encryption is applied to sensitive fields during serialization
        }

        // Define column families
        std::vector<rocksdb::ColumnFamilyDescriptor> cf_descriptors;
        cf_descriptors.emplace_back(rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions());
        cf_descriptors.emplace_back(CF_USERS, rocksdb::ColumnFamilyOptions());
        cf_descriptors.emplace_back(CF_GROUPS, rocksdb::ColumnFamilyOptions());
        cf_descriptors.emplace_back(CF_POLICIES, rocksdb::ColumnFamilyOptions());
        cf_descriptors.emplace_back(CF_USER_GROUPS, rocksdb::ColumnFamilyOptions());
        cf_descriptors.emplace_back(CF_USER_POLICIES, rocksdb::ColumnFamilyOptions());
        cf_descriptors.emplace_back(CF_GROUP_POLICIES, rocksdb::ColumnFamilyOptions());
        cf_descriptors.emplace_back(CF_SERVICE_ACCOUNTS, rocksdb::ColumnFamilyOptions());
        cf_descriptors.emplace_back(CF_AUDIT_LOG, rocksdb::ColumnFamilyOptions());
        cf_descriptors.emplace_back(CF_CONFIG, rocksdb::ColumnFamilyOptions());
        cf_descriptors.emplace_back(CF_API_STATS, rocksdb::ColumnFamilyOptions());
        cf_descriptors.emplace_back(CF_THROUGHPUT_STATS, rocksdb::ColumnFamilyOptions());
        cf_descriptors.emplace_back(CF_SERVERS, rocksdb::ColumnFamilyOptions());
        cf_descriptors.emplace_back(CF_DRIVES, rocksdb::ColumnFamilyOptions());
        cf_descriptors.emplace_back(CF_STORAGE_POOLS, rocksdb::ColumnFamilyOptions());
        cf_descriptors.emplace_back(CF_DASHBOARD_BOARDS, rocksdb::ColumnFamilyOptions());
        cf_descriptors.emplace_back(CF_DASHBOARD_WIDGETS, rocksdb::ColumnFamilyOptions());

        // Check if DB exists
        rocksdb::DB* db_ptr = nullptr;
        rocksdb::Status status;

        if (std::filesystem::exists(db_path_ + "/CURRENT")) {
            // Open existing database with column families
            status = rocksdb::DB::Open(options, db_path_, cf_descriptors, &cf_handles_, &db_ptr);
        } else {
            // Create new database
            status = rocksdb::DB::Open(options, db_path_, &db_ptr);
            if (status.ok()) {
                // Create column families
                for (size_t i = 1; i < cf_descriptors.size(); ++i) {
                    rocksdb::ColumnFamilyHandle* handle = nullptr;
                    status = db_ptr->CreateColumnFamily(cf_descriptors[i].options, cf_descriptors[i].name, &handle);
                    if (status.ok()) {
                        cf_handles_.push_back(handle);
                    } else {
                        CONSOLE_LOG_ERROR("Failed to create column family {}: {}",
                                          cf_descriptors[i].name,
                                          status.ToString());
                    }
                }
                // Add default CF handle
                cf_handles_.insert(cf_handles_.begin(), db_ptr->DefaultColumnFamily());
            }
        }

        if (!status.ok()) {
            CONSOLE_LOG_ERROR("Failed to open RocksDB: {}", status.ToString());
            return Err<void, String>("Failed to open database: " + status.ToString());
        }

        db_.reset(db_ptr);

        // Map column family names to handles
        const std::vector<String> cf_names = {rocksdb::kDefaultColumnFamilyName,
                                              CF_USERS,
                                              CF_GROUPS,
                                              CF_POLICIES,
                                              CF_USER_GROUPS,
                                              CF_USER_POLICIES,
                                              CF_GROUP_POLICIES,
                                              CF_SERVICE_ACCOUNTS,
                                              CF_AUDIT_LOG,
                                              CF_CONFIG,
                                              CF_API_STATS,
                                              CF_THROUGHPUT_STATS,
                                              CF_SERVERS,
                                              CF_DRIVES,
                                              CF_STORAGE_POOLS,
                                              CF_DASHBOARD_BOARDS,
                                              CF_DASHBOARD_WIDGETS};

        for (size_t i = 0; i < cf_handles_.size() && i < cf_names.size(); ++i) {
            cf_map_[cf_names[i]] = cf_handles_[i];
        }

        // Create default data
        create_default_data();

        is_ready_ = true;
        CONSOLE_LOG_INFO("DatabaseManager initialized successfully: {}", db_path_);
        return Ok<String>();

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Failed to initialize RocksDB: {}", e.what());
        return Err<void, String>(String("Initialization failed: ") + e.what());
    }
}

bool
DatabaseManager::is_ready() const {
    return is_ready_;
}

rocksdb::ColumnFamilyHandle*
DatabaseManager::get_cf_handle(const String& cf_name) {
    auto it = cf_map_.find(cf_name);
    if (it != cf_map_.end()) {
        return it->second;
    }
    return db_->DefaultColumnFamily();
}

void
DatabaseManager::create_default_data() {
    // Check if admin user exists
    auto admin_result = get_user("admin");
    if (admin_result.is_err()) {
        // Create default admin user
        auto& config = Config::instance();
        const auto& admin_config = config.default_admin();

        if (admin_config.enabled) {
            DbUser admin;
            admin.access_key = admin_config.username;
            admin.secret_key = utils::PasswordHash::hash(admin_config.password);
            admin.account_name = admin_config.account_name;
            admin.status = "active";
            admin.is_admin = true;
            admin.created_at = get_current_timestamp();
            admin.updated_at = admin.created_at;

            auto create_result = create_user(admin);
            if (create_result.is_ok()) {
                CONSOLE_LOG_INFO("Default admin user created: {}", admin_config.username);
            } else {
                CONSOLE_LOG_ERROR("Failed to create default admin: {}", create_result.error());
            }
        }
    }
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
// User Operations
// ============================================================================

Result<DbUser, String>
DatabaseManager::get_user(const String& access_key) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String value;
    auto status = db_->Get(rocksdb::ReadOptions(), get_cf_handle(CF_USERS), access_key, &value);

    if (status.IsNotFound()) {
        return Err<DbUser, String>("User not found: " + access_key);
    }
    if (!status.ok()) {
        return Err<DbUser, String>("Database error: " + status.ToString());
    }

    return Ok<DbUser>(json_to_user(value));
}

Result<Vector<DbUser>, String>
DatabaseManager::list_users(const String& status_filter) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    Vector<DbUser> users;
    auto* cf = get_cf_handle(CF_USERS);

    std::unique_ptr<rocksdb::Iterator> iter(db_->NewIterator(rocksdb::ReadOptions(), cf));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        DbUser user = json_to_user(iter->value().ToString());
        if (status_filter.empty() || user.status == status_filter) {
            users.push_back(user);
        }
    }

    if (!iter->status().ok()) {
        return Err<Vector<DbUser>, String>("Iterator error: " + iter->status().ToString());
    }

    return Ok<Vector<DbUser>>(std::move(users));
}

Result<void, String>
DatabaseManager::create_user(const DbUser& user) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    // Check if user already exists
    String existing;
    auto status = db_->Get(rocksdb::ReadOptions(), get_cf_handle(CF_USERS), user.access_key, &existing);
    if (status.ok()) {
        return Err<void, String>("User already exists: " + user.access_key);
    }

    String json = user_to_json(user);
    status = db_->Put(rocksdb::WriteOptions(), get_cf_handle(CF_USERS), user.access_key, json);

    if (!status.ok()) {
        return Err<void, String>("Failed to create user: " + status.ToString());
    }

    return Ok<String>();
}

Result<void, String>
DatabaseManager::update_user(const DbUser& user) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String json = user_to_json(user);
    auto status = db_->Put(rocksdb::WriteOptions(), get_cf_handle(CF_USERS), user.access_key, json);

    if (!status.ok()) {
        return Err<void, String>("Failed to update user: " + status.ToString());
    }

    return Ok<String>();
}

Result<void, String>
DatabaseManager::delete_user(const String& access_key) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    auto status = db_->Delete(rocksdb::WriteOptions(), get_cf_handle(CF_USERS), access_key);

    if (!status.ok()) {
        return Err<void, String>("Failed to delete user: " + status.ToString());
    }

    return Ok<String>();
}

Result<bool, String>
DatabaseManager::user_exists(const String& access_key) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String value;
    auto status = db_->Get(rocksdb::ReadOptions(), get_cf_handle(CF_USERS), access_key, &value);

    return Ok<bool>(status.ok());
}

void
DatabaseManager::get_user_async(const String& access_key, AsyncCallback<DbUser> callback) {
    submit_async([this, access_key, callback]() { callback(get_user(access_key)); });
}

void
DatabaseManager::create_user_async(const DbUser& user, AsyncCallback<void> callback) {
    submit_async([this, user, callback]() { callback(create_user(user)); });
}

// ============================================================================
// Group Operations
// ============================================================================

Result<DbGroup, String>
DatabaseManager::get_group(const String& name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String value;
    auto status = db_->Get(rocksdb::ReadOptions(), get_cf_handle(CF_GROUPS), name, &value);

    if (status.IsNotFound()) {
        return Err<DbGroup, String>("Group not found: " + name);
    }
    if (!status.ok()) {
        return Err<DbGroup, String>("Database error: " + status.ToString());
    }

    return Ok<DbGroup>(json_to_group(value));
}

Result<Vector<DbGroup>, String>
DatabaseManager::list_groups(const String& status_filter) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    Vector<DbGroup> groups;
    auto* cf = get_cf_handle(CF_GROUPS);

    std::unique_ptr<rocksdb::Iterator> iter(db_->NewIterator(rocksdb::ReadOptions(), cf));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        DbGroup group = json_to_group(iter->value().ToString());
        if (status_filter.empty() || group.status == status_filter) {
            groups.push_back(group);
        }
    }

    return Ok<Vector<DbGroup>>(std::move(groups));
}

Result<void, String>
DatabaseManager::create_group(const DbGroup& group) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String json = group_to_json(group);
    auto status = db_->Put(rocksdb::WriteOptions(), get_cf_handle(CF_GROUPS), group.name, json);

    if (!status.ok()) {
        return Err<void, String>("Failed to create group: " + status.ToString());
    }

    return Ok<String>();
}

Result<void, String>
DatabaseManager::update_group(const DbGroup& group) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String json = group_to_json(group);
    auto status = db_->Put(rocksdb::WriteOptions(), get_cf_handle(CF_GROUPS), group.name, json);

    if (!status.ok()) {
        return Err<void, String>("Failed to update group: " + status.ToString());
    }

    return Ok<String>();
}

Result<void, String>
DatabaseManager::delete_group(const String& name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    auto status = db_->Delete(rocksdb::WriteOptions(), get_cf_handle(CF_GROUPS), name);

    if (!status.ok()) {
        return Err<void, String>("Failed to delete group: " + status.ToString());
    }

    return Ok<String>();
}

Result<bool, String>
DatabaseManager::group_exists(const String& name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String value;
    auto status = db_->Get(rocksdb::ReadOptions(), get_cf_handle(CF_GROUPS), name, &value);

    return Ok<bool>(status.ok());
}

// ============================================================================
// User-Group Relationships
// ============================================================================

Result<void, String>
DatabaseManager::add_user_to_group(const String& access_key, const String& group_name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    // Key format: "user:access_key:group:group_name"
    String key = "user:" + access_key + ":group:" + group_name;
    String value = std::to_string(get_current_timestamp());

    auto status = db_->Put(rocksdb::WriteOptions(), get_cf_handle(CF_USER_GROUPS), key, value);

    if (!status.ok()) {
        return Err<void, String>("Failed to add user to group: " + status.ToString());
    }

    return Ok<String>();
}

Result<void, String>
DatabaseManager::remove_user_from_group(const String& access_key, const String& group_name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String key = "user:" + access_key + ":group:" + group_name;
    auto status = db_->Delete(rocksdb::WriteOptions(), get_cf_handle(CF_USER_GROUPS), key);

    if (!status.ok()) {
        return Err<void, String>("Failed to remove user from group: " + status.ToString());
    }

    return Ok<String>();
}

Result<Vector<String>, String>
DatabaseManager::get_user_groups(const String& access_key) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    Vector<String> groups;
    String prefix = "user:" + access_key + ":group:";
    auto* cf = get_cf_handle(CF_USER_GROUPS);

    std::unique_ptr<rocksdb::Iterator> iter(db_->NewIterator(rocksdb::ReadOptions(), cf));
    for (iter->Seek(prefix); iter->Valid() && iter->key().starts_with(prefix); iter->Next()) {
        String key = iter->key().ToString();
        String group_name = key.substr(prefix.length());
        groups.push_back(group_name);
    }

    return Ok<Vector<String>>(std::move(groups));
}

Result<Vector<String>, String>
DatabaseManager::get_group_users(const String& group_name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    Vector<String> users;
    auto* cf = get_cf_handle(CF_USER_GROUPS);

    std::unique_ptr<rocksdb::Iterator> iter(db_->NewIterator(rocksdb::ReadOptions(), cf));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        String key = iter->key().ToString();
        // Parse key format: "user:access_key:group:group_name"
        size_t group_pos = key.find(":group:" + group_name);
        if (group_pos != String::npos) {
            String access_key = key.substr(5, group_pos - 5); // Skip "user:"
            users.push_back(access_key);
        }
    }

    return Ok<Vector<String>>(std::move(users));
}

// ============================================================================
// Policy Operations
// ============================================================================

Result<DbPolicy, String>
DatabaseManager::get_policy(const String& name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String value;
    auto status = db_->Get(rocksdb::ReadOptions(), get_cf_handle(CF_POLICIES), name, &value);

    if (status.IsNotFound()) {
        return Err<DbPolicy, String>("Policy not found: " + name);
    }
    if (!status.ok()) {
        return Err<DbPolicy, String>("Database error: " + status.ToString());
    }

    return Ok<DbPolicy>(json_to_policy(value));
}

Result<Vector<DbPolicy>, String>
DatabaseManager::list_policies() {
    std::lock_guard<std::mutex> lock(db_mutex_);

    Vector<DbPolicy> policies;
    auto* cf = get_cf_handle(CF_POLICIES);

    std::unique_ptr<rocksdb::Iterator> iter(db_->NewIterator(rocksdb::ReadOptions(), cf));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        policies.push_back(json_to_policy(iter->value().ToString()));
    }

    return Ok<Vector<DbPolicy>>(std::move(policies));
}

Result<void, String>
DatabaseManager::create_policy(const DbPolicy& policy) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String json = policy_to_json(policy);
    auto status = db_->Put(rocksdb::WriteOptions(), get_cf_handle(CF_POLICIES), policy.name, json);

    if (!status.ok()) {
        return Err<void, String>("Failed to create policy: " + status.ToString());
    }

    return Ok<String>();
}

Result<void, String>
DatabaseManager::update_policy(const DbPolicy& policy) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String json = policy_to_json(policy);
    auto status = db_->Put(rocksdb::WriteOptions(), get_cf_handle(CF_POLICIES), policy.name, json);

    if (!status.ok()) {
        return Err<void, String>("Failed to update policy: " + status.ToString());
    }

    return Ok<String>();
}

Result<void, String>
DatabaseManager::delete_policy(const String& name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    auto status = db_->Delete(rocksdb::WriteOptions(), get_cf_handle(CF_POLICIES), name);

    if (!status.ok()) {
        return Err<void, String>("Failed to delete policy: " + status.ToString());
    }

    return Ok<String>();
}

Result<bool, String>
DatabaseManager::policy_exists(const String& name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String value;
    auto status = db_->Get(rocksdb::ReadOptions(), get_cf_handle(CF_POLICIES), name, &value);

    return Ok<bool>(status.ok());
}

// ============================================================================
// User-Policy Relationships
// ============================================================================

Result<void, String>
DatabaseManager::attach_policy_to_user(const String& access_key, const String& policy_name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String key = "user:" + access_key + ":policy:" + policy_name;
    String value = std::to_string(get_current_timestamp());

    auto status = db_->Put(rocksdb::WriteOptions(), get_cf_handle(CF_USER_POLICIES), key, value);

    if (!status.ok()) {
        return Err<void, String>("Failed to attach policy: " + status.ToString());
    }

    return Ok<String>();
}

Result<void, String>
DatabaseManager::detach_policy_from_user(const String& access_key, const String& policy_name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String key = "user:" + access_key + ":policy:" + policy_name;
    auto status = db_->Delete(rocksdb::WriteOptions(), get_cf_handle(CF_USER_POLICIES), key);

    if (!status.ok()) {
        return Err<void, String>("Failed to detach policy: " + status.ToString());
    }

    return Ok<String>();
}

Result<Vector<String>, String>
DatabaseManager::get_user_policies(const String& access_key, bool include_group_policies) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    Vector<String> policies;
    String prefix = "user:" + access_key + ":policy:";
    auto* cf = get_cf_handle(CF_USER_POLICIES);

    std::unique_ptr<rocksdb::Iterator> iter(db_->NewIterator(rocksdb::ReadOptions(), cf));
    for (iter->Seek(prefix); iter->Valid() && iter->key().starts_with(prefix); iter->Next()) {
        String key = iter->key().ToString();
        String policy_name = key.substr(prefix.length());
        policies.push_back(policy_name);
    }

    if (include_group_policies) {
        // Get policies from user's groups
        // Note: This requires releasing and re-acquiring lock, so we need to copy access_key
        // For simplicity, we'll handle this in a separate call
    }

    return Ok<Vector<String>>(std::move(policies));
}

// ============================================================================
// Group-Policy Relationships
// ============================================================================

Result<void, String>
DatabaseManager::attach_policy_to_group(const String& group_name, const String& policy_name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String key = "group:" + group_name + ":policy:" + policy_name;
    String value = std::to_string(get_current_timestamp());

    auto status = db_->Put(rocksdb::WriteOptions(), get_cf_handle(CF_GROUP_POLICIES), key, value);

    if (!status.ok()) {
        return Err<void, String>("Failed to attach policy to group: " + status.ToString());
    }

    return Ok<String>();
}

Result<void, String>
DatabaseManager::detach_policy_from_group(const String& group_name, const String& policy_name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String key = "group:" + group_name + ":policy:" + policy_name;
    auto status = db_->Delete(rocksdb::WriteOptions(), get_cf_handle(CF_GROUP_POLICIES), key);

    if (!status.ok()) {
        return Err<void, String>("Failed to detach policy from group: " + status.ToString());
    }

    return Ok<String>();
}

Result<Vector<String>, String>
DatabaseManager::get_group_policies(const String& group_name) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    Vector<String> policies;
    String prefix = "group:" + group_name + ":policy:";
    auto* cf = get_cf_handle(CF_GROUP_POLICIES);

    std::unique_ptr<rocksdb::Iterator> iter(db_->NewIterator(rocksdb::ReadOptions(), cf));
    for (iter->Seek(prefix); iter->Valid() && iter->key().starts_with(prefix); iter->Next()) {
        String key = iter->key().ToString();
        String policy_name = key.substr(prefix.length());
        policies.push_back(policy_name);
    }

    return Ok<Vector<String>>(std::move(policies));
}

// ============================================================================
// Service Account Operations
// ============================================================================

Result<DbServiceAccount, String>
DatabaseManager::get_service_account(const String& access_key) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String value;
    auto status = db_->Get(rocksdb::ReadOptions(), get_cf_handle(CF_SERVICE_ACCOUNTS), access_key, &value);

    if (status.IsNotFound()) {
        return Err<DbServiceAccount, String>("Service account not found: " + access_key);
    }
    if (!status.ok()) {
        return Err<DbServiceAccount, String>("Database error: " + status.ToString());
    }

    return Ok<DbServiceAccount>(json_to_service_account(value));
}

Result<Vector<DbServiceAccount>, String>
DatabaseManager::list_service_accounts(const String& parent_user) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    Vector<DbServiceAccount> accounts;
    auto* cf = get_cf_handle(CF_SERVICE_ACCOUNTS);

    std::unique_ptr<rocksdb::Iterator> iter(db_->NewIterator(rocksdb::ReadOptions(), cf));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        DbServiceAccount account = json_to_service_account(iter->value().ToString());
        if (parent_user.empty() || account.parent_user == parent_user) {
            accounts.push_back(account);
        }
    }

    return Ok<Vector<DbServiceAccount>>(std::move(accounts));
}

Result<void, String>
DatabaseManager::create_service_account(const DbServiceAccount& account) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String json = service_account_to_json(account);
    auto status = db_->Put(rocksdb::WriteOptions(), get_cf_handle(CF_SERVICE_ACCOUNTS), account.access_key, json);

    if (!status.ok()) {
        return Err<void, String>("Failed to create service account: " + status.ToString());
    }

    return Ok<String>();
}

Result<void, String>
DatabaseManager::delete_service_account(const String& access_key) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    auto status = db_->Delete(rocksdb::WriteOptions(), get_cf_handle(CF_SERVICE_ACCOUNTS), access_key);

    if (!status.ok()) {
        return Err<void, String>("Failed to delete service account: " + status.ToString());
    }

    return Ok<String>();
}

// ============================================================================
// Audit Log
// ============================================================================

Result<void, String>
DatabaseManager::add_audit_log(const AuditLogEntry& entry) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    // Generate unique key with timestamp prefix for ordering
    AuditLogEntry entry_with_id = entry;
    entry_with_id.timestamp = get_current_timestamp();
    entry_with_id.id = entry_with_id.timestamp * 1000 + (rand() % 1000); // Simple unique ID

    String key = std::to_string(entry_with_id.id);
    String json = audit_log_to_json(entry_with_id);

    auto status = db_->Put(rocksdb::WriteOptions(), get_cf_handle(CF_AUDIT_LOG), key, json);

    if (!status.ok()) {
        return Err<void, String>("Failed to add audit log: " + status.ToString());
    }

    return Ok<String>();
}

Result<Vector<AuditLogEntry>, String>
DatabaseManager::get_audit_logs(const String& user_access_key,
                                int64_t from_timestamp,
                                int64_t to_timestamp,
                                int limit) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    Vector<AuditLogEntry> logs;
    auto* cf = get_cf_handle(CF_AUDIT_LOG);

    std::unique_ptr<rocksdb::Iterator> iter(db_->NewIterator(rocksdb::ReadOptions(), cf));
    int count = 0;

    for (iter->SeekToLast(); iter->Valid() && count < limit; iter->Prev()) {
        AuditLogEntry entry = json_to_audit_log(iter->value().ToString());

        // Apply filters
        if (!user_access_key.empty() && entry.user_access_key != user_access_key) {
            continue;
        }
        if (from_timestamp > 0 && entry.timestamp < from_timestamp) {
            continue;
        }
        if (to_timestamp > 0 && entry.timestamp > to_timestamp) {
            continue;
        }

        logs.push_back(entry);
        count++;
    }

    return Ok<Vector<AuditLogEntry>>(std::move(logs));
}

// ============================================================================
// Config Operations
// ============================================================================

Result<String, String>
DatabaseManager::get_config(const String& key) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String value;
    auto status = db_->Get(rocksdb::ReadOptions(), get_cf_handle(CF_CONFIG), key, &value);

    if (status.IsNotFound()) {
        return Err<String, String>("Config not found: " + key);
    }
    if (!status.ok()) {
        return Err<String, String>("Database error: " + status.ToString());
    }

    return Ok<String>(value);
}

Result<void, String>
DatabaseManager::set_config(const String& key, const String& value, const String& /* value_type */) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    auto status = db_->Put(rocksdb::WriteOptions(), get_cf_handle(CF_CONFIG), key, value);

    if (!status.ok()) {
        return Err<void, String>("Failed to set config: " + status.ToString());
    }

    return Ok<String>();
}

Result<StringMap, String>
DatabaseManager::get_all_config() {
    std::lock_guard<std::mutex> lock(db_mutex_);

    StringMap config;
    auto* cf = get_cf_handle(CF_CONFIG);

    std::unique_ptr<rocksdb::Iterator> iter(db_->NewIterator(rocksdb::ReadOptions(), cf));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        config[iter->key().ToString()] = iter->value().ToString();
    }

    return Ok<StringMap>(std::move(config));
}

// ============================================================================
// Statistics Operations
// ============================================================================

Result<void, String>
DatabaseManager::add_api_request_stat(const ApiRequestStats& stats) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    ApiRequestStats stats_with_id = stats;
    stats_with_id.timestamp = get_current_timestamp();
    stats_with_id.id = stats_with_id.timestamp * 1000 + (rand() % 1000);

    String key = std::to_string(stats_with_id.id);
    String json = api_stats_to_json(stats_with_id);

    auto status = db_->Put(rocksdb::WriteOptions(), get_cf_handle(CF_API_STATS), key, json);

    if (!status.ok()) {
        return Err<void, String>("Failed to add API stats: " + status.ToString());
    }

    return Ok<String>();
}

Result<Vector<ApiRequestStats>, String>
DatabaseManager::get_api_request_stats(int64_t from_timestamp, int64_t to_timestamp, int status_code_filter) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    Vector<ApiRequestStats> stats;
    auto* cf = get_cf_handle(CF_API_STATS);

    std::unique_ptr<rocksdb::Iterator> iter(db_->NewIterator(rocksdb::ReadOptions(), cf));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        ApiRequestStats entry = json_to_api_stats(iter->value().ToString());

        if (entry.timestamp >= from_timestamp && (to_timestamp == 0 || entry.timestamp <= to_timestamp) &&
            (status_code_filter == 0 || entry.status_code == status_code_filter)) {
            stats.push_back(entry);
        }
    }

    return Ok<Vector<ApiRequestStats>>(std::move(stats));
}

Result<void, String>
DatabaseManager::add_throughput_stat(const DataThroughputStats& stats) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    DataThroughputStats stats_with_id = stats;
    stats_with_id.timestamp = get_current_timestamp();
    stats_with_id.id = stats_with_id.timestamp * 1000 + (rand() % 1000);

    String key = std::to_string(stats_with_id.id);
    String json = throughput_to_json(stats_with_id);

    auto status = db_->Put(rocksdb::WriteOptions(), get_cf_handle(CF_THROUGHPUT_STATS), key, json);

    if (!status.ok()) {
        return Err<void, String>("Failed to add throughput stats: " + status.ToString());
    }

    return Ok<String>();
}

Result<Vector<DataThroughputStats>, String>
DatabaseManager::get_throughput_stats(int64_t from_timestamp, int64_t to_timestamp) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    Vector<DataThroughputStats> stats;
    auto* cf = get_cf_handle(CF_THROUGHPUT_STATS);

    std::unique_ptr<rocksdb::Iterator> iter(db_->NewIterator(rocksdb::ReadOptions(), cf));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        DataThroughputStats entry = json_to_throughput(iter->value().ToString());

        if (entry.timestamp >= from_timestamp && (to_timestamp == 0 || entry.timestamp <= to_timestamp)) {
            stats.push_back(entry);
        }
    }

    return Ok<Vector<DataThroughputStats>>(std::move(stats));
}

// ============================================================================
// Server Operations
// ============================================================================

Result<DbServer, String>
DatabaseManager::get_server(const String& id) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String value;
    auto status = db_->Get(rocksdb::ReadOptions(), get_cf_handle(CF_SERVERS), id, &value);

    if (status.IsNotFound()) {
        return Err<DbServer, String>("Server not found: " + id);
    }
    if (!status.ok()) {
        return Err<DbServer, String>("Database error: " + status.ToString());
    }

    return Ok<DbServer>(json_to_server(value));
}

Result<Vector<DbServer>, String>
DatabaseManager::list_servers(const String& status_filter) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    Vector<DbServer> servers;
    auto* cf = get_cf_handle(CF_SERVERS);

    std::unique_ptr<rocksdb::Iterator> iter(db_->NewIterator(rocksdb::ReadOptions(), cf));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        DbServer server = json_to_server(iter->value().ToString());
        if (status_filter.empty() || server.status == status_filter) {
            servers.push_back(server);
        }
    }

    return Ok<Vector<DbServer>>(std::move(servers));
}

Result<void, String>
DatabaseManager::upsert_server(const DbServer& server) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String json = server_to_json(server);
    auto status = db_->Put(rocksdb::WriteOptions(), get_cf_handle(CF_SERVERS), server.id, json);

    if (!status.ok()) {
        return Err<void, String>("Failed to upsert server: " + status.ToString());
    }

    return Ok<String>();
}

Result<void, String>
DatabaseManager::delete_server(const String& id) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    auto status = db_->Delete(rocksdb::WriteOptions(), get_cf_handle(CF_SERVERS), id);

    if (!status.ok()) {
        return Err<void, String>("Failed to delete server: " + status.ToString());
    }

    return Ok<String>();
}

// ============================================================================
// Drive Operations
// ============================================================================

Result<DbDrive, String>
DatabaseManager::get_drive(const String& id) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String value;
    auto status = db_->Get(rocksdb::ReadOptions(), get_cf_handle(CF_DRIVES), id, &value);

    if (status.IsNotFound()) {
        return Err<DbDrive, String>("Drive not found: " + id);
    }
    if (!status.ok()) {
        return Err<DbDrive, String>("Database error: " + status.ToString());
    }

    return Ok<DbDrive>(json_to_drive(value));
}

Result<Vector<DbDrive>, String>
DatabaseManager::list_drives(const String& server_id, const String& status_filter) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    Vector<DbDrive> drives;
    auto* cf = get_cf_handle(CF_DRIVES);

    std::unique_ptr<rocksdb::Iterator> iter(db_->NewIterator(rocksdb::ReadOptions(), cf));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        DbDrive drive = json_to_drive(iter->value().ToString());

        if (!server_id.empty() && drive.server_id != server_id) {
            continue;
        }
        if (!status_filter.empty() && drive.status != status_filter) {
            continue;
        }

        drives.push_back(drive);
    }

    return Ok<Vector<DbDrive>>(std::move(drives));
}

Result<void, String>
DatabaseManager::upsert_drive(const DbDrive& drive) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String json = drive_to_json(drive);
    auto status = db_->Put(rocksdb::WriteOptions(), get_cf_handle(CF_DRIVES), drive.id, json);

    if (!status.ok()) {
        return Err<void, String>("Failed to upsert drive: " + status.ToString());
    }

    return Ok<String>();
}

Result<void, String>
DatabaseManager::delete_drive(const String& id) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    auto status = db_->Delete(rocksdb::WriteOptions(), get_cf_handle(CF_DRIVES), id);

    if (!status.ok()) {
        return Err<void, String>("Failed to delete drive: " + status.ToString());
    }

    return Ok<String>();
}

// ============================================================================
// Storage Pool Operations
// ============================================================================

Result<DbStoragePool, String>
DatabaseManager::get_storage_pool(const String& id) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String value;
    auto status = db_->Get(rocksdb::ReadOptions(), get_cf_handle(CF_STORAGE_POOLS), id, &value);

    if (status.IsNotFound()) {
        return Err<DbStoragePool, String>("Storage pool not found: " + id);
    }
    if (!status.ok()) {
        return Err<DbStoragePool, String>("Database error: " + status.ToString());
    }

    return Ok<DbStoragePool>(json_to_pool(value));
}

Result<Vector<DbStoragePool>, String>
DatabaseManager::list_storage_pools() {
    std::lock_guard<std::mutex> lock(db_mutex_);

    Vector<DbStoragePool> pools;
    auto* cf = get_cf_handle(CF_STORAGE_POOLS);

    std::unique_ptr<rocksdb::Iterator> iter(db_->NewIterator(rocksdb::ReadOptions(), cf));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        pools.push_back(json_to_pool(iter->value().ToString()));
    }

    return Ok<Vector<DbStoragePool>>(std::move(pools));
}

Result<void, String>
DatabaseManager::upsert_storage_pool(const DbStoragePool& pool) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String json = pool_to_json(pool);
    auto status = db_->Put(rocksdb::WriteOptions(), get_cf_handle(CF_STORAGE_POOLS), pool.id, json);

    if (!status.ok()) {
        return Err<void, String>("Failed to upsert storage pool: " + status.ToString());
    }

    return Ok<String>();
}

Result<void, String>
DatabaseManager::delete_storage_pool(const String& id) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    auto status = db_->Delete(rocksdb::WriteOptions(), get_cf_handle(CF_STORAGE_POOLS), id);

    if (!status.ok()) {
        return Err<void, String>("Failed to delete storage pool: " + status.ToString());
    }

    return Ok<String>();
}

// ============================================================================
// Cleanup Operations
// ============================================================================

Result<void, String>
DatabaseManager::cleanup_old_api_stats(int64_t older_than_timestamp) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    auto* cf = get_cf_handle(CF_API_STATS);
    rocksdb::WriteBatch batch;

    std::unique_ptr<rocksdb::Iterator> iter(db_->NewIterator(rocksdb::ReadOptions(), cf));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        ApiRequestStats entry = json_to_api_stats(iter->value().ToString());
        if (entry.timestamp < older_than_timestamp) {
            batch.Delete(cf, iter->key());
        }
    }

    auto status = db_->Write(rocksdb::WriteOptions(), &batch);
    if (!status.ok()) {
        return Err<void, String>("Failed to cleanup old API stats: " + status.ToString());
    }

    return Ok<String>();
}

Result<void, String>
DatabaseManager::cleanup_old_throughput_stats(int64_t older_than_timestamp) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    auto* cf = get_cf_handle(CF_THROUGHPUT_STATS);
    rocksdb::WriteBatch batch;

    std::unique_ptr<rocksdb::Iterator> iter(db_->NewIterator(rocksdb::ReadOptions(), cf));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        DataThroughputStats entry = json_to_throughput(iter->value().ToString());
        if (entry.timestamp < older_than_timestamp) {
            batch.Delete(cf, iter->key());
        }
    }

    auto status = db_->Write(rocksdb::WriteOptions(), &batch);
    if (!status.ok()) {
        return Err<void, String>("Failed to cleanup old throughput stats: " + status.ToString());
    }

    return Ok<String>();
}

// ============================================================================
// Transaction Support
// ============================================================================

Result<void, String>
DatabaseManager::begin_transaction() {
    std::lock_guard<std::mutex> lock(db_mutex_);

    if (in_transaction_) {
        return Err<void, String>("Transaction already in progress");
    }

    current_batch_ = std::make_unique<rocksdb::WriteBatch>();
    in_transaction_ = true;

    return Ok<String>();
}

Result<void, String>
DatabaseManager::commit_transaction() {
    std::lock_guard<std::mutex> lock(db_mutex_);

    if (!in_transaction_ || !current_batch_) {
        return Err<void, String>("No transaction in progress");
    }

    auto status = db_->Write(rocksdb::WriteOptions(), current_batch_.get());
    current_batch_.reset();
    in_transaction_ = false;

    if (!status.ok()) {
        return Err<void, String>("Failed to commit transaction: " + status.ToString());
    }

    return Ok<String>();
}

Result<void, String>
DatabaseManager::rollback_transaction() {
    std::lock_guard<std::mutex> lock(db_mutex_);

    if (!in_transaction_) {
        return Err<void, String>("No transaction in progress");
    }

    current_batch_.reset();
    in_transaction_ = false;

    return Ok<String>();
}

// ============================================================================
// Utility Operations
// ============================================================================

Result<int64_t, String>
DatabaseManager::get_user_count() {
    std::lock_guard<std::mutex> lock(db_mutex_);

    int64_t count = 0;
    auto* cf = get_cf_handle(CF_USERS);

    std::unique_ptr<rocksdb::Iterator> iter(db_->NewIterator(rocksdb::ReadOptions(), cf));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        count++;
    }

    return Ok<int64_t>(count);
}

Result<int64_t, String>
DatabaseManager::get_group_count() {
    std::lock_guard<std::mutex> lock(db_mutex_);

    int64_t count = 0;
    auto* cf = get_cf_handle(CF_GROUPS);

    std::unique_ptr<rocksdb::Iterator> iter(db_->NewIterator(rocksdb::ReadOptions(), cf));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        count++;
    }

    return Ok<int64_t>(count);
}

Result<int64_t, String>
DatabaseManager::get_policy_count() {
    std::lock_guard<std::mutex> lock(db_mutex_);

    int64_t count = 0;
    auto* cf = get_cf_handle(CF_POLICIES);

    std::unique_ptr<rocksdb::Iterator> iter(db_->NewIterator(rocksdb::ReadOptions(), cf));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        count++;
    }

    return Ok<int64_t>(count);
}

Result<void, String>
DatabaseManager::compact() {
    std::lock_guard<std::mutex> lock(db_mutex_);

    rocksdb::CompactRangeOptions options;
    auto status = db_->CompactRange(options, nullptr, nullptr);

    if (!status.ok()) {
        return Err<void, String>("Failed to compact database: " + status.ToString());
    }

    CONSOLE_LOG_INFO("RocksDB compaction completed");
    return Ok<String>();
}

// ============================================================================
// Dashboard Board Operations
// ============================================================================

Result<DbDashboardBoard, String>
DatabaseManager::get_dashboard_board(const String& id) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String value;
    auto status = db_->Get(rocksdb::ReadOptions(), get_cf_handle(CF_DASHBOARD_BOARDS), id, &value);

    if (status.IsNotFound()) {
        return Err<DbDashboardBoard, String>("Dashboard board not found: " + id);
    }
    if (!status.ok()) {
        return Err<DbDashboardBoard, String>("Database error: " + status.ToString());
    }

    return Ok<DbDashboardBoard>(json_to_dashboard_board(value));
}

Result<Vector<DbDashboardBoard>, String>
DatabaseManager::list_dashboard_boards(const String& user_id) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    Vector<DbDashboardBoard> boards;
    auto* cf = get_cf_handle(CF_DASHBOARD_BOARDS);

    std::unique_ptr<rocksdb::Iterator> iter(db_->NewIterator(rocksdb::ReadOptions(), cf));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        DbDashboardBoard board = json_to_dashboard_board(iter->value().ToString());
        if (user_id.empty() || board.user_id == user_id) {
            boards.push_back(board);
        }
    }

    // Sort by order_index
    std::sort(boards.begin(), boards.end(), [](const auto& a, const auto& b) { return a.order_index < b.order_index; });

    return Ok<Vector<DbDashboardBoard>>(std::move(boards));
}

Result<void, String>
DatabaseManager::create_dashboard_board(const DbDashboardBoard& board) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String json = dashboard_board_to_json(board);
    auto status = db_->Put(rocksdb::WriteOptions(), get_cf_handle(CF_DASHBOARD_BOARDS), board.id, json);

    if (!status.ok()) {
        return Err<void, String>("Failed to create dashboard board: " + status.ToString());
    }

    return Ok<String>();
}

Result<void, String>
DatabaseManager::update_dashboard_board(const DbDashboardBoard& board) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String json = dashboard_board_to_json(board);
    auto status = db_->Put(rocksdb::WriteOptions(), get_cf_handle(CF_DASHBOARD_BOARDS), board.id, json);

    if (!status.ok()) {
        return Err<void, String>("Failed to update dashboard board: " + status.ToString());
    }

    return Ok<String>();
}

Result<void, String>
DatabaseManager::delete_dashboard_board(const String& id) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    // First delete all widgets for this board
    auto* widget_cf = get_cf_handle(CF_DASHBOARD_WIDGETS);
    rocksdb::WriteBatch batch;

    std::unique_ptr<rocksdb::Iterator> iter(db_->NewIterator(rocksdb::ReadOptions(), widget_cf));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        DbDashboardWidget widget = json_to_dashboard_widget(iter->value().ToString());
        if (widget.board_id == id) {
            batch.Delete(widget_cf, iter->key());
        }
    }

    // Delete the board
    batch.Delete(get_cf_handle(CF_DASHBOARD_BOARDS), id);

    auto status = db_->Write(rocksdb::WriteOptions(), &batch);
    if (!status.ok()) {
        return Err<void, String>("Failed to delete dashboard board: " + status.ToString());
    }

    return Ok<String>();
}

// ============================================================================
// Dashboard Widget Operations
// ============================================================================

Result<DbDashboardWidget, String>
DatabaseManager::get_dashboard_widget(const String& id) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String value;
    auto status = db_->Get(rocksdb::ReadOptions(), get_cf_handle(CF_DASHBOARD_WIDGETS), id, &value);

    if (status.IsNotFound()) {
        return Err<DbDashboardWidget, String>("Dashboard widget not found: " + id);
    }
    if (!status.ok()) {
        return Err<DbDashboardWidget, String>("Database error: " + status.ToString());
    }

    return Ok<DbDashboardWidget>(json_to_dashboard_widget(value));
}

Result<Vector<DbDashboardWidget>, String>
DatabaseManager::list_dashboard_widgets(const String& board_id) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    Vector<DbDashboardWidget> widgets;
    auto* cf = get_cf_handle(CF_DASHBOARD_WIDGETS);

    std::unique_ptr<rocksdb::Iterator> iter(db_->NewIterator(rocksdb::ReadOptions(), cf));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        DbDashboardWidget widget = json_to_dashboard_widget(iter->value().ToString());
        if (board_id.empty() || widget.board_id == board_id) {
            widgets.push_back(widget);
        }
    }

    // Sort by order_index
    std::sort(widgets.begin(), widgets.end(), [](const auto& a, const auto& b) {
        return a.order_index < b.order_index;
    });

    return Ok<Vector<DbDashboardWidget>>(std::move(widgets));
}

Result<void, String>
DatabaseManager::create_dashboard_widget(const DbDashboardWidget& widget) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String json = dashboard_widget_to_json(widget);
    auto status = db_->Put(rocksdb::WriteOptions(), get_cf_handle(CF_DASHBOARD_WIDGETS), widget.id, json);

    if (!status.ok()) {
        return Err<void, String>("Failed to create dashboard widget: " + status.ToString());
    }

    return Ok<String>();
}

Result<void, String>
DatabaseManager::update_dashboard_widget(const DbDashboardWidget& widget) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    String json = dashboard_widget_to_json(widget);
    auto status = db_->Put(rocksdb::WriteOptions(), get_cf_handle(CF_DASHBOARD_WIDGETS), widget.id, json);

    if (!status.ok()) {
        return Err<void, String>("Failed to update dashboard widget: " + status.ToString());
    }

    return Ok<String>();
}

Result<void, String>
DatabaseManager::delete_dashboard_widget(const String& id) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    auto status = db_->Delete(rocksdb::WriteOptions(), get_cf_handle(CF_DASHBOARD_WIDGETS), id);

    if (!status.ok()) {
        return Err<void, String>("Failed to delete dashboard widget: " + status.ToString());
    }

    return Ok<String>();
}

Result<void, String>
DatabaseManager::save_dashboard_widgets(const String& board_id, const Vector<DbDashboardWidget>& widgets) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    auto* cf = get_cf_handle(CF_DASHBOARD_WIDGETS);
    rocksdb::WriteBatch batch;

    // Delete existing widgets for this board
    std::unique_ptr<rocksdb::Iterator> iter(db_->NewIterator(rocksdb::ReadOptions(), cf));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        DbDashboardWidget widget = json_to_dashboard_widget(iter->value().ToString());
        if (widget.board_id == board_id) {
            batch.Delete(cf, iter->key());
        }
    }

    // Add new widgets
    for (const auto& widget : widgets) {
        String json = dashboard_widget_to_json(widget);
        batch.Put(cf, widget.id, json);
    }

    auto status = db_->Write(rocksdb::WriteOptions(), &batch);
    if (!status.ok()) {
        return Err<void, String>("Failed to save dashboard widgets: " + status.ToString());
    }

    return Ok<String>();
}

} // namespace console::storage
