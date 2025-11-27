# Система сбора статистики

## Обзор

Система сбора статистики использует SQLite БД для хранения метрик в реальном времени. Mock данные полностью удалены из API контроллеров.

## Таблицы БД

### 1. `api_request_stats`
Хранит информацию о каждом API запросе для анализа ошибок.

```sql
CREATE TABLE api_request_stats (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp INTEGER NOT NULL,
    endpoint TEXT NOT NULL,
    method TEXT NOT NULL,
    status_code INTEGER NOT NULL,
    response_time_ms INTEGER,
    user_access_key TEXT,
    ip_address TEXT,
    user_agent TEXT
);
```

**Индексы:**
- `idx_api_stats_timestamp` на `timestamp`
- `idx_api_stats_status` на `status_code`

### 2. `data_throughput_stats`
Хранит данные о пропускной способности.

```sql
CREATE TABLE data_throughput_stats (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp INTEGER NOT NULL,
    read_bytes INTEGER DEFAULT 0,
    write_bytes INTEGER DEFAULT 0,
    total_bytes INTEGER DEFAULT 0
);
```

**Индексы:**
- `idx_throughput_timestamp` на `timestamp`

### 3. `servers`
Информация о серверах в кластере.

```sql
CREATE TABLE servers (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    endpoint TEXT NOT NULL,
    status TEXT DEFAULT 'offline',
    uptime INTEGER DEFAULT 0,
    last_heartbeat INTEGER,
    metadata TEXT
);
```

### 4. `drives`
Информация о дисках.

```sql
CREATE TABLE drives (
    id TEXT PRIMARY KEY,
    server_id TEXT NOT NULL,
    path TEXT NOT NULL,
    status TEXT DEFAULT 'offline',
    capacity INTEGER DEFAULT 0,
    used INTEGER DEFAULT 0,
    available INTEGER DEFAULT 0,
    last_check INTEGER,
    metadata TEXT,
    FOREIGN KEY (server_id) REFERENCES servers(id) ON DELETE CASCADE
);
```

### 5. `storage_pools`
Информация о пулах хранения.

```sql
CREATE TABLE storage_pools (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    capacity INTEGER DEFAULT 0,
    used INTEGER DEFAULT 0,
    available INTEGER DEFAULT 0,
    drives_count INTEGER DEFAULT 0,
    online_drives INTEGER DEFAULT 0,
    offline_drives INTEGER DEFAULT 0,
    last_update INTEGER,
    metadata TEXT
);
```

## Сбор данных

### 1. API Request Stats (через middleware)

Добавьте middleware для логирования каждого API запроса:

```cpp
// В main.cpp или специальном middleware
app().registerPostHandlingAdvice([](const drogon::HttpRequestPtr& req,
                                     const drogon::HttpResponsePtr& resp) {
    auto db_manager = ServiceLocator::database_manager();
    if (!db_manager) return;

    storage::ApiRequestStats stats;
    stats.timestamp = std::time(nullptr);
    stats.endpoint = req->path();
    stats.method = drogon::to_string(req->method());
    stats.status_code = resp->statusCode();
    stats.ip_address = req->peerAddr().toIp();

    // Асинхронно сохраняем
    db_manager->add_api_request_stat(stats);
});
```

### 2. Data Throughput Stats

Собирайте данные при каждом чтении/записи объекта:

```cpp
// В ObjectsController::upload или download
auto db_manager = ServiceLocator::database_manager();
if (db_manager) {
    storage::DataThroughputStats stats;
    stats.timestamp = std::time(nullptr);
    stats.write_bytes = file_size;  // или read_bytes
    stats.total_bytes = file_size;
    db_manager->add_throughput_stat(stats);
}
```

### 3. Server Stats

Периодически обновляйте информацию о серверах (например, через heartbeat):

```cpp
// Фоновая задача каждые 30 секунд
void update_server_stats() {
    auto db_manager = ServiceLocator::database_manager();
    if (!db_manager) return;

    // Получить информацию о серверах из Object Storage Admin API
    // или из конфигурации
    storage::DbServer server;
    server.id = "server-1";
    server.name = "Server 1";
    server.endpoint = "http://localhost:9000";
    server.status = "online";
    server.uptime = get_system_uptime();
    server.last_heartbeat = std::time(nullptr);

    db_manager->upsert_server(server);
}
```

### 4. Drive Stats

Обновляйте информацию о дисках из Object Storage Admin API:

```cpp
// Используйте Object Storage Admin API для получения информации о дисках
void update_drive_stats() {
    auto admin_client = ServiceLocator::admin_client();
    auto db_manager = ServiceLocator::database_manager();

    if (!admin_client || !db_manager) return;

    // Получить информацию о серверах/дисках
    auto server_info = admin_client->get_server_info();
    if (!server_info) return;

    for (const auto& disk : server_info.value().disks) {
        storage::DbDrive drive;
        drive.id = disk.path;
        drive.server_id = disk.endpoint;
        drive.path = disk.path;
        drive.status = disk.state == "ok" ? "online" : "offline";
        drive.capacity = disk.totalSpace;
        drive.used = disk.usedSpace;
        drive.available = disk.availSpace;
        drive.last_check = std::time(nullptr);

        db_manager->upsert_drive(drive);
    }
}
```

### 5. Storage Pool Stats

Агрегируйте данные о пулах из дисков:

```cpp
void update_pool_stats() {
    auto db_manager = ServiceLocator::database_manager();
    if (!db_manager) return;

    // Получить все диски
    auto drives_result = db_manager->list_drives();
    if (!drives_result) return;

    // Группировать по пулам (логика зависит от вашей конфигурации)
    std::map<String, storage::DbStoragePool> pools;

    for (const auto& drive : drives_result.value()) {
        // Определить пул по server_id или другой логике
        String pool_id = "pool-1"; // ваша логика

        auto& pool = pools[pool_id];
        if (pool.id.empty()) {
            pool.id = pool_id;
            pool.name = "Pool " + pool_id;
        }

        pool.capacity += drive.capacity;
        pool.used += drive.used;
        pool.available += drive.available;
        pool.drives_count++;

        if (drive.status == "online") {
            pool.online_drives++;
        } else {
            pool.offline_drives++;
        }
    }

    // Сохранить пулы
    for (const auto& [id, pool] : pools) {
        auto pool_to_save = pool;
        pool_to_save.last_update = std::time(nullptr);
        db_manager->upsert_storage_pool(pool_to_save);
    }
}
```

## Автоматическая очистка старых данных

Настройте периодическую очистку данных старше 7 дней:

```cpp
// Фоновая задача раз в сутки
void cleanup_old_stats() {
    auto db_manager = ServiceLocator::database_manager();
    if (!db_manager) return;

    auto now = std::time(nullptr);
    auto week_ago = now - (7 * 24 * 3600);

    // Удалить старые API stats
    db_manager->cleanup_old_api_stats(week_ago);

    // Удалить старые throughput stats
    db_manager->cleanup_old_throughput_stats(week_ago);
}
```

## Запуск фоновых задач

В `main.cpp` добавьте периодические задачи:

```cpp
// В функции main() после инициализации
drogon::app().getLoop()->runEvery(30.0, []() {
    update_server_stats();
    update_drive_stats();
});

drogon::app().getLoop()->runEvery(60.0, []() {
    update_pool_stats();
});

drogon::app().getLoop()->runEvery(86400.0, []() {  // раз в сутки
    cleanup_old_stats();
});
```

## Миграция

Миграция v2 будет применена автоматически при запуске приложения. Убедитесь, что вызвана `migrations.run_all()` в коде инициализации БД.

## TODO для реальной интеграции

1. **Middleware для API requests** - добавить `registerPostHandlingAdvice` в main.cpp
2. **Throughput tracking** - добавить запись в методы upload/download объектов
3. **Server heartbeat** - реализовать периодическую проверку серверов
4. **Object Storage Admin API integration** - получать реальные данные о дисках/серверах
5. **Pool configuration** - определить логику группировки дисков в пулы
6. **Background tasks** - настроить все периодические задачи в main.cpp
7. **Performance monitoring** - следить за размером БД и производительностью запросов

## Производительность

- API request stats может генерировать много записей. Используйте batch inserts или async writes.
- Индексы на timestamp критичны для быстрых запросов за 24 часа.
- Регулярная очистка старых данных обязательна.
- Рассмотрите агрегацию данных старше суток (по часам вместо минут).
