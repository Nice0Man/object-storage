# SQLite Async Usage Guide

## Обзор

Реализация асинхронного доступа к SQLite для хранения данных пользователей, групп и политик.

## Архитектура

```
┌─────────────────────────────────────┐
│   LocalAdminClient (IMinioAdminClient) │
└──────────────┬──────────────────────┘
               │
               ▼
┌──────────────────────────────────────┐
│       DatabaseManager                 │
│  • Sync operations (блокирующие)     │
│  • Async operations (thread pool)    │
└──────────────┬───────────────────────┘
               │
               ▼
┌──────────────────────────────────────┐
│       SQLite Database                 │
│  • WAL mode (concurrent r/w)         │
│  • PRAGMA optimizations              │
│  • Prepared statements               │
└──────────────────────────────────────┘
```

## Компоненты

### 1. DatabaseManager

**Ответственность:**
- Управление SQLite подключением
- Выполнение SQL запросов
- Thread pool для async операций
- Транзакции

**Особенности:**
- WAL mode для concurrent access
- Prepared statements для безопасности
- Connection pooling
- Query timeouts

### 2. Schema

**Таблицы:**
- `users` - Пользователи системы
- `groups` - Группы пользователей
- `policies` - Политики доступа
- `user_groups` - Связь users ↔ groups
- `user_policies` - Связь users ↔ policies
- `group_policies` - Связь groups ↔ policies
- `service_accounts` - Сервисные аккаунты
- `audit_log` - Журнал аудита
- `config` - Конфигурация системы

### 3. Async Operations

**Thread Pool:**
```cpp
class DatabaseManager {
    Vector<std::thread> worker_threads_;
    std::queue<std::function<void()>> task_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
};
```

**Async pattern:**
```cpp
template<typename Func>
void submit_async(Func&& func) {
    {
        std::lock_guard lock(queue_mutex_);
        task_queue_.push(std::forward<Func>(func));
    }
    queue_cv_.notify_one();
}
```

## Примеры использования

### Инициализация

```cpp
#include "console/storage/DatabaseManager.hpp"

// Create database manager
auto db_manager = std::make_unique<DatabaseManager>(
    "/var/lib/object-storage/storage.db",
    4  // Thread pool size
);

// Initialize schema
auto result = db_manager->initialize_schema();
if (!result) {
    CONSOLE_LOG_ERROR("Failed to initialize database: {}", result.error());
    return;
}
```

### Синхронные операции

```cpp
// Create user
DbUser user;
user.access_key = "user123";
user.secret_key = "$2b$12$hash...";  // bcrypt hash
user.account_name = "John Doe";
user.is_admin = false;
user.status = "active";
user.created_at = std::time(nullptr);
user.updated_at = user.created_at;

auto create_result = db_manager->create_user(user);
if (!create_result) {
    CONSOLE_LOG_ERROR("Failed to create user: {}", create_result.error());
}

// Get user
auto get_result = db_manager->get_user("user123");
if (get_result) {
    const auto& u = get_result.value();
    std::cout << "User: " << u.account_name << std::endl;
}

// List users
auto list_result = db_manager->list_users("active");
if (list_result) {
    for (const auto& u : list_result.value()) {
        std::cout << "  - " << u.access_key << std::endl;
    }
}

// Update user
user.account_name = "John Smith";
user.updated_at = std::time(nullptr);
db_manager->update_user(user);

// Delete user
db_manager->delete_user("user123");
```

### Асинхронные операции

```cpp
// Async create user
db_manager->create_user_async(user, [](Result<void, String> result) {
    if (result) {
        std::cout << "User created successfully" << std::endl;
    } else {
        std::cout << "Failed: " << result.error() << std::endl;
    }
});

// Async get user
db_manager->get_user_async("user123", [](Result<DbUser, String> result) {
    if (result) {
        std::cout << "User: " << result.value().account_name << std::endl;
    }
});

// Main thread continues without blocking...
```

### Группы

```cpp
// Create group
DbGroup group;
group.name = "developers";
group.description = "Development team";
group.status = "active";
group.created_at = std::time(nullptr);
group.updated_at = group.created_at;

db_manager->create_group(group);

// Add user to group
db_manager->add_user_to_group("user123", "developers");

// Get user's groups
auto groups_result = db_manager->get_user_groups("user123");
if (groups_result) {
    for (const auto& g : groups_result.value()) {
        std::cout << "Group: " << g << std::endl;
    }
}

// Get group members
auto members_result = db_manager->get_group_users("developers");
```

### Политики

```cpp
// Create policy
DbPolicy policy;
policy.name = "read-only";
policy.version = "2012-10-17";
policy.document = R"({
    "Version": "2012-10-17",
    "Statement": [{
        "Effect": "Allow",
        "Action": ["s3:GetObject", "s3:ListBucket"],
        "Resource": "*"
    }]
})";
policy.description = "Read-only access to all buckets";
policy.created_at = std::time(nullptr);
policy.updated_at = policy.created_at;

db_manager->create_policy(policy);

// Attach policy to user
db_manager->attach_policy_to_user("user123", "read-only");

// Attach policy to group
db_manager->attach_policy_to_group("developers", "read-only");

// Get all user policies (including from groups)
auto policies_result = db_manager->get_user_policies("user123", true);
if (policies_result) {
    for (const auto& p : policies_result.value()) {
        std::cout << "Policy: " << p << std::endl;
    }
}
```

### Транзакции

```cpp
// Begin transaction
auto begin_result = db_manager->begin_transaction();

try {
    // Multiple operations
    db_manager->create_user(user);
    db_manager->add_user_to_group(user.access_key, "developers");
    db_manager->attach_policy_to_user(user.access_key, "read-only");
    
    // Commit
    db_manager->commit_transaction();
    
} catch (const std::exception& e) {
    // Rollback on error
    db_manager->rollback_transaction();
    CONSOLE_LOG_ERROR("Transaction failed: {}", e.what());
}
```

### Audit Log

```cpp
// Add audit entry
AuditLogEntry entry;
entry.timestamp = std::time(nullptr);
entry.user_access_key = "admin";
entry.action = "CREATE_USER";
entry.resource_type = "user";
entry.resource_id = "user123";
entry.status = "success";
entry.details = R"({"account_name": "John Doe"})";
entry.ip_address = "192.168.1.100";
entry.user_agent = "ObjectStorageConsole/1.0";

db_manager->add_audit_log(entry);

// Query audit logs
auto logs_result = db_manager->get_audit_logs(
    "admin",        // user
    1234567890,     // from
    std::time(0),   // to
    100             // limit
);

if (logs_result) {
    for (const auto& log : logs_result.value()) {
        std::cout << "[" << log.timestamp << "] "
                  << log.action << " "
                  << log.resource_type << "/"
                  << log.resource_id << " "
                  << log.status << std::endl;
    }
}
```

### Service Accounts

```cpp
// Create service account
DbServiceAccount sa;
sa.access_key = "sa-" + generate_uuid();
sa.secret_key = "$2b$12$hash...";
sa.parent_user = "user123";
sa.description = "CI/CD service account";
sa.expiration = std::nullopt;  // No expiration
sa.status = "active";
sa.created_at = std::time(nullptr);

db_manager->create_service_account(sa);

// List service accounts for user
auto sa_list = db_manager->list_service_accounts("user123");
```

### Configuration

```cpp
// Set config value
db_manager->set_config("max_upload_size", "5368709120", "int");
db_manager->set_config("enable_versioning", "true", "bool");
db_manager->set_config("default_region", "us-east-1", "string");

// Get config value
auto max_size = db_manager->get_config("max_upload_size");
if (max_size) {
    int64_t size = std::stoll(max_size.value());
    std::cout << "Max upload size: " << size << std::endl;
}

// Get all config
auto all_config = db_manager->get_all_config();
```

## LocalAdminClient Integration

```cpp
class LocalAdminClient : public IMinioAdminClient {
public:
    explicit LocalAdminClient(std::shared_ptr<DatabaseManager> db)
        : db_manager_(db) {}

    Result<Vector<models::User>, String> list_users() override {
        auto db_users = db_manager_->list_users("active");
        if (!db_users) {
            return Err<Vector<models::User>, String>(db_users.error());
        }

        Vector<models::User> users;
        for (const auto& db_user : db_users.value()) {
            users.push_back(db_user_to_model(db_user));
        }

        return Ok<Vector<models::User>, String>(users);
    }

private:
    models::User db_user_to_model(const DbUser& db_user) {
        UserInfo info;
        info.access_key = db_user.access_key;
        info.account_name = db_user.account_name;
        info.is_admin = db_user.is_admin;
        // ... convert other fields
        return models::User(info);
    }

    std::shared_ptr<DatabaseManager> db_manager_;
};
```

## Performance Tips

### 1. Use Transactions

```cpp
// BAD: Multiple individual queries
for (const auto& user : users) {
    db_manager->create_user(user);
}

// GOOD: Single transaction
db_manager->begin_transaction();
for (const auto& user : users) {
    db_manager->create_user(user);
}
db_manager->commit_transaction();
```

### 2. Use Async for Batch Operations

```cpp
std::vector<std::future<void>> futures;

for (const auto& user : users) {
    auto future = std::async(std::launch::async, [&]() {
        db_manager->create_user_async(user, [](auto result) {
            // Handle result
        });
    });
    futures.push_back(std::move(future));
}

// Wait for all
for (auto& future : futures) {
    future.wait();
}
```

### 3. Use Prepared Statements

```cpp
// DatabaseManager уже использует prepared statements внутри
// Просто используйте методы как обычно
db_manager->create_user(user);
```

### 4. Regular Maintenance

```cpp
// Периодически запускайте VACUUM
std::thread vacuum_thread([&db_manager]() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::hours(24));
        db_manager->vacuum();
    }
});
```

## Error Handling

```cpp
auto result = db_manager->create_user(user);

if (!result) {
    // Handle error
    const auto& error = result.error();
    
    if (error.find("UNIQUE constraint") != String::npos) {
        // User already exists
        CONSOLE_LOG_WARN("User already exists: {}", user.access_key);
    } else if (error.find("FOREIGN KEY constraint") != String::npos) {
        // Foreign key violation
        CONSOLE_LOG_ERROR("Foreign key violation: {}", error);
    } else {
        // Other error
        CONSOLE_LOG_ERROR("Database error: {}", error);
    }
}
```

## Security Best Practices

### 1. Hash Passwords

```cpp
#include <openssl/evp.h>

String hash_password(const String& password) {
    // Use bcrypt or argon2
    // For example with bcrypt:
    char hash[61];
    bcrypt_hashpw(password.c_str(), bcrypt_gensalt(12), hash);
    return String(hash);
}

// When creating user
user.secret_key = hash_password(plain_password);
```

### 2. SQL Injection Protection

```cpp
// DatabaseManager уже использует prepared statements
// НО всегда валидируйте входные данные

bool is_valid_access_key(const String& key) {
    // Only alphanumeric and dashes
    return std::regex_match(key, std::regex("^[a-zA-Z0-9-]+$"));
}

if (!is_valid_access_key(access_key)) {
    return Err<DbUser, String>("Invalid access key format");
}
```

### 3. Database Encryption

```cpp
// TODO: Integrate SQLCipher for database encryption
// #include <sqlcipher/sqlite3.h>

// Set encryption key
sqlite3_key(db_, encryption_key.c_str(), encryption_key.length());
```

## Testing

```cpp
// Use in-memory database for tests
auto test_db = std::make_unique<DatabaseManager>(":memory:", 2);
test_db->initialize_schema();

// Run tests
test_db->create_user(test_user);
auto result = test_db->get_user(test_user.access_key);
ASSERT_TRUE(result);
ASSERT_EQ(result.value().account_name, test_user.account_name);
```

## См. также

- [DATABASE_SCHEMA.md](./DATABASE_SCHEMA.md) - Полная схема БД
- [STORAGE_ARCHITECTURE.md](./STORAGE_ARCHITECTURE.md) - Общая архитектура
- [SQLite Documentation](https://www.sqlite.org/docs.html)

