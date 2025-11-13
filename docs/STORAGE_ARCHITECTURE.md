# Local Storage Architecture

## Общая архитектура

Реализация собственного хранилища объектов на основе файловой системы, совместимого с S3 API.

## Структура файловой системы

```
storage_root/
├── buckets/
│   ├── bucket-name-1/
│   │   ├── .metadata.json              # Метаданные бакета
│   │   ├── .policy.json                # Политики доступа
│   │   ├── .versioning.json            # Настройки версионирования
│   │   ├── .tags.json                  # Теги бакета
│   │   └── objects/
│   │       ├── path/
│   │       │   └── to/
│   │       │       ├── object.bin      # Содержимое объекта
│   │       │       ├── object.meta     # Метаданные объекта
│   │       │       └── object.tags     # Теги объекта
│   │       └── versions/
│   │           └── object.bin.v1       # Версии объектов
│   └── bucket-name-2/
│       └── ...
├── storage.db                          # SQLite БД для users, groups, policies
└── logs/
    ├── access.log                      # Лог доступа
    └── audit.log                       # Аудит операций
```

**Примечание:** Users, groups, policies теперь хранятся в SQLite БД (`storage.db`) вместо отдельных JSON файлов для лучшей производительности и структурированного доступа.

## Компоненты системы

### 1. LocalStorageClient (IMinioClient)

**Ответственность:**
- Реализация всех S3-совместимых операций
- Управление файлами и директориями
- Валидация данных
- Атомарные операции с файлами

**Основные методы:**
- Bucket Operations: `list_buckets`, `create_bucket`, `delete_bucket`, `get_bucket`
- Object Operations: `put_object`, `get_object`, `delete_object`, `list_objects`
- Metadata Operations: `get/set_object_metadata`, `get/set_object_tags`
- Presigned URLs: `generate_presigned_url`

### 2. LocalAdminClient (IMinioAdminClient)

**Ответственность:**
- Управление пользователями
- Управление группами
- Управление политиками
- Сервисные аккаунты

**Особенности:**
- Использует DatabaseManager для хранения данных
- Конвертирует между models и DB structures
- Обработка ошибок и валидация
- Поддержка async операций

### 3. DatabaseManager

**Ответственность:**
- SQLite backend для users, groups, policies
- Async операции с thread pool
- Transaction management
- Audit logging
- Config key-value store

**Основные методы:**
- User Operations: `create_user`, `get_user`, `list_users`, `update_user`, `delete_user`
- Group Operations: `create_group`, `add_user_to_group`, `get_user_groups`
- Policy Operations: `create_policy`, `attach_policy_to_user`, `get_user_policies`
- Service Accounts: `create_service_account`, `list_service_accounts`
- Audit: `add_audit_log`, `get_audit_logs`
- Transactions: `begin_transaction`, `commit_transaction`, `rollback_transaction`

**Таблицы БД:**
- `users` - Пользователи системы
- `groups` - Группы пользователей
- `user_groups` - Связь M:N users ↔ groups
- `policies` - IAM-style политики доступа
- `user_policies` - Связь M:N users ↔ policies
- `group_policies` - Связь M:N groups ↔ policies
- `service_accounts` - Сервисные аккаунты
- `audit_log` - Журнал всех операций
- `config` - Конфигурация (key-value)

**См. также:**
- [DATABASE_SCHEMA.md](./DATABASE_SCHEMA.md) - Полная схема БД
- [SQLITE_ASYNC_USAGE.md](./SQLITE_ASYNC_USAGE.md) - Руководство по использованию
- [SQLITE_IMPLEMENTATION_SUMMARY.md](./SQLITE_IMPLEMENTATION_SUMMARY.md) - Статус реализации

### 4. PathManager

**Ответственность:**
- Управление путями файловой системы
- Валидация имен buckets и object keys
- Создание необходимых директорий
- S3-compatible path mapping

**Основные методы:**
- `get_bucket_path`, `get_object_path`
- `validate_bucket_name`, `validate_object_key`
- `create_bucket_dirs`

### 5. MetadataManager

**Ответственность:**
- Сериализация/десериализация метаданных (bucket, object)
- Кэширование метаданных
- Управление тегами объектов
- JSON operations для .meta, .tags файлов

**Основные методы:**
- `read/write_bucket_metadata`
- `read/write_object_metadata`
- `read/write_object_tags`

### 6. AccessControlManager

**Ответственность:**
- Проверка прав доступа на основе политик
- Валидация IAM policies
- Policy evaluation engine
- Аудит доступа

**Особенности:**
- Использует DatabaseManager для получения политик
- Поддержка AWS IAM policy syntax
- Group-based permissions
- Resource-based policies

### 7. PresignedURLManager

**Ответственность:**
- Генерация подписанных URL
- Валидация подписей (HMAC-SHA256)
- Управление временем жизни URL
- Контроль использования

**Особенности:**
- Совместимость с AWS S3 presigned URLs
- Настраиваемое время жизни
- Support GET/PUT methods

## Форматы данных

### Bucket Metadata (.metadata.json)

```json
{
  "name": "my-bucket",
  "creation_date": "2025-01-01T00:00:00Z",
  "region": "us-east-1",
  "versioning_enabled": false,
  "object_locking": false,
  "owner": "user-access-key",
  "statistics": {
    "object_count": 100,
    "total_size_bytes": 1048576
  }
}
```

### Object Metadata (.meta)

```json
{
  "key": "path/to/object.bin",
  "bucket": "my-bucket",
  "size": 1024,
  "etag": "md5-hash-of-content",
  "content_type": "application/octet-stream",
  "last_modified": "2025-01-01T00:00:00Z",
  "version_id": "v1",
  "metadata": {
    "custom-header-1": "value1",
    "custom-header-2": "value2"
  },
  "encryption": {
    "algorithm": "AES256",
    "key_id": "..."
  }
}
```

### Object Tags (.tags)

```json
{
  "Environment": "production",
  "Department": "Engineering",
  "Cost-Center": "12345"
}
```

### Policy Document (.policy.json)

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Principal": "*",
      "Action": ["s3:GetObject"],
      "Resource": ["arn:aws:s3:::my-bucket/*"]
    }
  ]
}
```

## Особенности реализации

### Атомарность операций

1. **Запись объектов:**
   - Запись во временный файл `.tmp`
   - Запись метаданных
   - Атомарное переименование через `rename()`

2. **Удаление объектов:**
   - Переименование в `.deleted`
   - Фоновая очистка удаленных файлов

3. **Обновление метаданных:**
   - Copy-on-write для метаданных
   - Версионирование изменений

### Версионирование

1. **Структура версий:**
   - Оригинал: `object.bin`
   - Версия 1: `versions/object.bin.v1`
   - Версия 2: `versions/object.bin.v2`

2. **Метаданные версий:**
   - Каждая версия имеет свой `.meta` файл
   - Version ID генерируется как UUID

3. **Управление версиями:**
   - Автоматическое создание версий при перезаписи
   - Удаление старых версий по политике
   - Восстановление из версий

### Производительность

1. **Кэширование:**
   - Метаданные бакетов в памяти
   - LRU кэш для часто используемых объектов
   - Кэш списка объектов

2. **Индексирование:**
   - SQLite БД (`storage.db`) для users, groups, policies
   - 15+ индексов для быстрых JOIN и поисков
   - Быстрый поиск по префиксам и тегам
   - Foreign key constraints для целостности данных

3. **Оптимизации:**
   - Memory-mapped files для больших объектов
   - Асинхронные операции I/O
   - Пул потоков для параллельных операций

### Безопасность

1. **Шифрование:**
   - Server-side encryption (SSE)
   - Client-provided keys (SSE-C)
   - KMS integration (опционально)

2. **Контроль доступа:**
   - IAM-style policies
   - Bucket policies
   - ACLs (Access Control Lists)

3. **Аудит:**
   - Логирование всех операций
   - Цепочка изменений
   - Compliance отчеты

## Преимущества подхода

1. **Независимость:** Не требует внешних зависимостей (MinIO, S3)
2. **Простота:** Легко отлаживать и понимать
3. **Контроль:** Полный контроль над данными
4. **Производительность:** Оптимизация под конкретные use cases
5. **Портативность:** Работает на любой FS (ext4, NTFS, etc.)

## Ограничения

1. **Масштабируемость:** Ограничена одной машиной
2. **Репликация:** Требует дополнительной реализации
3. **Распределенность:** Не поддерживается из коробки
4. **Совместимость:** Не 100% совместим с AWS S3

## Дорожная карта

### Phase 1: Core Implementation (v1.0)
- [x] Базовая архитектура
- [ ] Bucket operations
- [ ] Object operations (CRUD)
- [ ] Metadata management
- [ ] Basic testing

### Phase 2: Advanced Features (v1.1)
- [ ] Versioning
- [ ] Presigned URLs
- [ ] Tags management
- [ ] Multipart uploads

### Phase 3: Security & Performance (v1.2)
- [ ] Server-side encryption
- [ ] Access control policies
- [ ] Caching layer
- [ ] Index optimization

### Phase 4: Enterprise Features (v2.0)
- [ ] Audit logging
- [ ] Quota management
- [ ] Replication support
- [ ] Backup/Restore tools

