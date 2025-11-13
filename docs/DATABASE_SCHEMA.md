# SQLite Database Schema

## Обзор

Используется SQLite для хранения структурированных данных: пользователей, групп, политик и конфигурации.

## Схема базы данных

### Таблица: users

Хранит информацию о пользователях системы.

```sql
CREATE TABLE IF NOT EXISTS users (
    access_key TEXT PRIMARY KEY,
    secret_key TEXT NOT NULL,
    account_name TEXT,
    status TEXT DEFAULT 'active',  -- active, disabled
    is_admin BOOLEAN DEFAULT 0,
    created_at INTEGER NOT NULL,   -- Unix timestamp
    updated_at INTEGER NOT NULL,
    metadata TEXT                   -- JSON для дополнительных данных
);

CREATE INDEX idx_users_status ON users(status);
CREATE INDEX idx_users_created ON users(created_at);
```

**Поля:**
- `access_key` - Уникальный ключ доступа (первичный ключ)
- `secret_key` - Секретный ключ (хэшированный)
- `account_name` - Имя аккаунта
- `status` - Статус пользователя (active/disabled)
- `is_admin` - Флаг администратора
- `created_at` - Время создания (Unix timestamp)
- `updated_at` - Время последнего обновления
- `metadata` - JSON для расширяемости

### Таблица: groups

Хранит информацию о группах пользователей.

```sql
CREATE TABLE IF NOT EXISTS groups (
    name TEXT PRIMARY KEY,
    description TEXT,
    status TEXT DEFAULT 'active',
    created_at INTEGER NOT NULL,
    updated_at INTEGER NOT NULL,
    metadata TEXT
);

CREATE INDEX idx_groups_status ON groups(status);
```

**Поля:**
- `name` - Имя группы (первичный ключ)
- `description` - Описание группы
- `status` - Статус группы
- `created_at` - Время создания
- `updated_at` - Время обновления
- `metadata` - JSON для расширяемости

### Таблица: user_groups

Связь многие-ко-многим между пользователями и группами.

```sql
CREATE TABLE IF NOT EXISTS user_groups (
    user_access_key TEXT NOT NULL,
    group_name TEXT NOT NULL,
    added_at INTEGER NOT NULL,
    PRIMARY KEY (user_access_key, group_name),
    FOREIGN KEY (user_access_key) REFERENCES users(access_key) ON DELETE CASCADE,
    FOREIGN KEY (group_name) REFERENCES groups(name) ON DELETE CASCADE
);

CREATE INDEX idx_user_groups_user ON user_groups(user_access_key);
CREATE INDEX idx_user_groups_group ON user_groups(group_name);
```

### Таблица: policies

Хранит политики доступа.

```sql
CREATE TABLE IF NOT EXISTS policies (
    name TEXT PRIMARY KEY,
    version TEXT DEFAULT '2012-10-17',
    document TEXT NOT NULL,        -- JSON policy document
    description TEXT,
    created_at INTEGER NOT NULL,
    updated_at INTEGER NOT NULL,
    metadata TEXT
);

CREATE INDEX idx_policies_created ON policies(created_at);
```

**Поля:**
- `name` - Имя политики (первичный ключ)
- `version` - Версия политики (AWS IAM format)
- `document` - JSON документ политики
- `description` - Описание политики
- `created_at` - Время создания
- `updated_at` - Время обновления
- `metadata` - JSON для расширяемости

### Таблица: user_policies

Связь между пользователями и политиками.

```sql
CREATE TABLE IF NOT EXISTS user_policies (
    user_access_key TEXT NOT NULL,
    policy_name TEXT NOT NULL,
    attached_at INTEGER NOT NULL,
    PRIMARY KEY (user_access_key, policy_name),
    FOREIGN KEY (user_access_key) REFERENCES users(access_key) ON DELETE CASCADE,
    FOREIGN KEY (policy_name) REFERENCES policies(name) ON DELETE CASCADE
);

CREATE INDEX idx_user_policies_user ON user_policies(user_access_key);
CREATE INDEX idx_user_policies_policy ON user_policies(policy_name);
```

### Таблица: group_policies

Связь между группами и политиками.

```sql
CREATE TABLE IF NOT EXISTS group_policies (
    group_name TEXT NOT NULL,
    policy_name TEXT NOT NULL,
    attached_at INTEGER NOT NULL,
    PRIMARY KEY (group_name, policy_name),
    FOREIGN KEY (group_name) REFERENCES groups(name) ON DELETE CASCADE,
    FOREIGN KEY (policy_name) REFERENCES policies(name) ON DELETE CASCADE
);

CREATE INDEX idx_group_policies_group ON group_policies(group_name);
CREATE INDEX idx_group_policies_policy ON group_policies(policy_name);
```

### Таблица: service_accounts

Хранит сервисные аккаунты.

```sql
CREATE TABLE IF NOT EXISTS service_accounts (
    access_key TEXT PRIMARY KEY,
    secret_key TEXT NOT NULL,
    parent_user TEXT NOT NULL,
    description TEXT,
    expiration INTEGER,            -- Unix timestamp, NULL = no expiration
    status TEXT DEFAULT 'active',
    created_at INTEGER NOT NULL,
    metadata TEXT,
    FOREIGN KEY (parent_user) REFERENCES users(access_key) ON DELETE CASCADE
);

CREATE INDEX idx_service_accounts_parent ON service_accounts(parent_user);
CREATE INDEX idx_service_accounts_status ON service_accounts(status);
CREATE INDEX idx_service_accounts_expiration ON service_accounts(expiration);
```

### Таблица: audit_log

Журнал аудита всех операций.

```sql
CREATE TABLE IF NOT EXISTS audit_log (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp INTEGER NOT NULL,
    user_access_key TEXT,
    action TEXT NOT NULL,           -- CREATE_USER, DELETE_POLICY, etc.
    resource_type TEXT NOT NULL,    -- user, group, policy, etc.
    resource_id TEXT NOT NULL,
    status TEXT NOT NULL,           -- success, failure
    details TEXT,                   -- JSON с деталями
    ip_address TEXT,
    user_agent TEXT
);

CREATE INDEX idx_audit_timestamp ON audit_log(timestamp);
CREATE INDEX idx_audit_user ON audit_log(user_access_key);
CREATE INDEX idx_audit_action ON audit_log(action);
CREATE INDEX idx_audit_resource ON audit_log(resource_type, resource_id);
```

### Таблица: config

Хранит конфигурацию системы.

```sql
CREATE TABLE IF NOT EXISTS config (
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL,
    value_type TEXT DEFAULT 'string',  -- string, int, bool, json
    description TEXT,
    updated_at INTEGER NOT NULL,
    updated_by TEXT
);

CREATE INDEX idx_config_type ON config(value_type);
```

## Примеры запросов

### Создать пользователя

```sql
INSERT INTO users (access_key, secret_key, account_name, is_admin, created_at, updated_at)
VALUES ('user123', '$2b$12$hash...', 'John Doe', 0, 1234567890, 1234567890);
```

### Добавить пользователя в группу

```sql
INSERT INTO user_groups (user_access_key, group_name, added_at)
VALUES ('user123', 'developers', 1234567890);
```

### Прикрепить политику к пользователю

```sql
INSERT INTO user_policies (user_access_key, policy_name, attached_at)
VALUES ('user123', 'read-only', 1234567890);
```

### Получить все политики пользователя (включая через группы)

```sql
SELECT DISTINCT p.*
FROM policies p
WHERE p.name IN (
    -- Прямые политики
    SELECT policy_name FROM user_policies WHERE user_access_key = 'user123'
    UNION
    -- Политики через группы
    SELECT gp.policy_name
    FROM user_groups ug
    JOIN group_policies gp ON ug.group_name = gp.group_name
    WHERE ug.user_access_key = 'user123'
);
```

### Получить всех пользователей в группе

```sql
SELECT u.*
FROM users u
JOIN user_groups ug ON u.access_key = ug.user_access_key
WHERE ug.group_name = 'developers';
```

### Audit log query

```sql
SELECT * FROM audit_log
WHERE user_access_key = 'user123'
  AND timestamp >= 1234567890
ORDER BY timestamp DESC
LIMIT 100;
```

## Миграции

### Version 1 (Initial)

- Создание всех таблиц
- Создание индексов
- Создание дефолтного admin пользователя

### Version 2 (Planned)

- Добавление таблицы sessions
- Расширение audit_log
- Добавление триггеров для updated_at

## Производительность

### Индексы

Все foreign keys имеют индексы для ускорения:
- JOIN операций
- DELETE CASCADE
- Поиска по связям

### Query Optimization

1. **User policies** - Используйте prepared statements
2. **Audit log** - Партиционирование по времени (future)
3. **Bulk operations** - Используйте транзакции

### Рекомендации

- Используйте `PRAGMA journal_mode=WAL` для concurrent reads/writes
- Регулярно выполняйте `VACUUM` для оптимизации
- Настройте `PRAGMA cache_size` для больших БД

## Размеры данных

### Примерные оценки

- **1000 users** → ~500 KB
- **100 groups** → ~50 KB
- **500 policies** → ~2 MB (с policy documents)
- **10000 audit records** → ~5 MB

### Лимиты

- Max DB size: Не ограничено (практически до TB)
- Max row size: ~1 MB (достаточно для policy documents)
- Max connections: 1000+ (с WAL mode)

## Backup & Recovery

### Backup

```sql
-- Online backup
VACUUM INTO 'backup.db';

-- Or use .backup command
.backup backup.db
```

### Recovery

```bash
# Copy backup file
cp backup.db storage.db

# Verify integrity
sqlite3 storage.db "PRAGMA integrity_check;"
```

## Security

### Хранение паролей

- ❌ Не хранить secret_key в открытом виде
- ✅ Использовать bcrypt/argon2 для хэширования
- ✅ Salt для каждого пароля

### Защита БД

- Шифрование БД файла (SQLCipher)
- Ограничение прав доступа к файлу
- Регулярные backups

### SQL Injection

- ✅ Всегда используйте prepared statements
- ✅ Валидация входных данных
- ✅ Parameterized queries

## Мониторинг

### Полезные PRAGMA

```sql
-- Размер БД
PRAGMA page_count;
PRAGMA page_size;

-- Статистика
PRAGMA table_info(users);
PRAGMA index_list(users);

-- Performance
PRAGMA cache_size;
PRAGMA temp_store;
```

## Расширения (Future)

### Possible additions

1. **Full-text search** для audit_log
2. **Encryption** через SQLCipher
3. **Replication** для HA
4. **Sharding** для больших deployments

## Примеры использования

### C++ с sqlite_orm

```cpp
using namespace sqlite_orm;

auto storage = make_storage("storage.db",
    make_table("users",
        make_column("access_key", &User::access_key, primary_key()),
        make_column("secret_key", &User::secret_key),
        make_column("account_name", &User::account_name),
        make_column("is_admin", &User::is_admin),
        make_column("created_at", &User::created_at),
        make_column("updated_at", &User::updated_at)
    )
);

storage.sync_schema();

// Insert user
User user{"user123", "hash", "John Doe", false, time(0), time(0)};
storage.insert(user);

// Query
auto users = storage.get_all<User>(where(c(&User::is_admin) == true));
```

### Async operations with thread pool

```cpp
// Async insert
auto future = std::async(std::launch::async, [&storage, user]() {
    return storage.insert(user);
});

// Async query
auto users_future = std::async(std::launch::async, [&storage]() {
    return storage.get_all<User>();
});
```

## См. также

- [SQLite Documentation](https://www.sqlite.org/docs.html)
- [SQLite WAL Mode](https://www.sqlite.org/wal.html)
- [sqlite_orm](https://github.com/fnc12/sqlite_orm)
- [STORAGE_ARCHITECTURE.md](./STORAGE_ARCHITECTURE.md)

