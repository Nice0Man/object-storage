# Local Storage Implementation Plan

## Цель
Реализовать собственное файловое хранилище объектов, совместимое с S3 API, не требующее внешних зависимостей (Object Storage, AWS SDK).

## Этапы реализации

### Этап 1: Подготовка инфраструктуры (1-2 дня)

#### 1.1 Создание базовых классов
- [ ] `StorageManager` - низкоуровневая работа с FS
- [ ] `MetadataManager` - управление метаданными
- [ ] `PathManager` - управление путями к файлам
- [ ] `LockManager` - управление блокировками

**Файлы:**
- `include/console/storage/StorageManager.hpp`
- `include/console/storage/MetadataManager.hpp`
- `include/console/storage/PathManager.hpp`
- `include/console/storage/LockManager.hpp`
- Соответствующие `.cpp` файлы

#### 1.2 Утилиты
- [ ] `FileUtils` - работа с файлами (atomic write, rename, etc.)
- [ ] `HashUtils` - вычисление MD5, SHA256 для ETag
- [ ] `JsonSerializer` - сериализация метаданных

**Файлы:**
- `include/console/storage/utils/FileUtils.hpp`
- `include/console/storage/utils/HashUtils.hpp`

### Этап 2: LocalStorageClient - Bucket Operations (2-3 дня)

#### 2.1 Основные операции с бакетами
- [ ] `list_buckets()` - список всех бакетов
- [ ] `create_bucket()` - создание бакета
- [ ] `delete_bucket()` - удаление бакета
- [ ] `get_bucket()` - информация о бакете
- [ ] `bucket_exists()` - проверка существования

**Файлы:**
- `include/console/clients/LocalStorageClient.hpp`
- `src/clients/LocalStorageClient.cpp`

**Детали реализации:**
```cpp
// Структура бакета на диске:
// storage_root/buckets/bucket-name/.metadata.json

Result<bool, String> LocalStorageClient::create_bucket(
    const String& name, 
    const String& region
) {
    // 1. Валидация имени бакета (S3 rules)
    // 2. Проверка существования
    // 3. Создание директории
    // 4. Создание .metadata.json
    // 5. Атомарная запись
}
```

#### 2.2 Валидация
- [ ] Валидация имени бакета (S3 naming rules)
- [ ] Проверка прав доступа
- [ ] Проверка квот (опционально)

### Этап 3: LocalStorageClient - Object Operations (3-4 дня)

#### 3.1 Базовые операции с объектами
- [ ] `put_object()` - загрузка объекта
- [ ] `get_object()` - скачивание объекта
- [ ] `delete_object()` - удаление объекта
- [ ] `stat_object()` - информация об объекте
- [ ] `list_objects()` - список объектов в бакете
- [ ] `copy_object()` - копирование объекта

**Детали реализации:**
```cpp
// Структура объекта на диске:
// storage_root/buckets/bucket-name/objects/path/to/object.bin
// storage_root/buckets/bucket-name/objects/path/to/object.meta

Result<models::Object, String> LocalStorageClient::put_object(
    const String& bucket_name,
    const String& object_key,
    const ByteArray& data,
    const String& content_type,
    const StringMap& metadata
) {
    // 1. Валидация bucket и key
    // 2. Вычисление ETag (MD5)
    // 3. Запись во временный файл
    // 4. Создание метаданных
    // 5. Атомарное переименование
    // 6. Обновление индекса
}
```

#### 3.2 Оптимизация больших файлов
- [ ] Streaming для больших файлов
- [ ] Chunked upload
- [ ] Resume support (опционально)

### Этап 4: Metadata & Tags (1-2 дня)

#### 4.1 Метаданные объектов
- [ ] `get_object_metadata()` - чтение метаданных
- [ ] `set_object_metadata()` - установка метаданных

#### 4.2 Теги объектов
- [ ] `get_object_tags()` - чтение тегов
- [ ] `set_object_tags()` - установка тегов

**Формат файлов:**
```json
// object.meta
{
  "key": "path/to/object.bin",
  "bucket": "my-bucket",
  "size": 1024,
  "etag": "md5-hash",
  "content_type": "application/json",
  "last_modified": "2025-01-01T00:00:00Z",
  "metadata": {
    "x-amz-meta-custom": "value"
  }
}

// object.tags
{
  "Environment": "production",
  "Owner": "team-alpha"
}
```

### Этап 5: Presigned URLs (1-2 дня)

#### 5.1 Генерация подписанных URL
- [ ] `generate_presigned_url()` - создание URL
- [ ] Подпись с использованием HMAC-SHA256
- [ ] Валидация времени жизни

**Формат:**
```
/api/v1/presigned/{bucket}/{object}?signature=...&expires=...
```

**Детали:**
```cpp
Result<String, String> LocalStorageClient::generate_presigned_url(
    const String& bucket_name,
    const String& object_key,
    int64_t expires_in_seconds,
    const String& method
) {
    // 1. Создание строки для подписи
    // 2. HMAC-SHA256 с secret key
    // 3. Base64 encoding
    // 4. Формирование URL с параметрами
}
```

### Этап 6: Versioning Support (2-3 дня)

#### 6.1 Версионирование объектов
- [ ] Включение/выключение версионирования для бакета
- [ ] Создание версий при перезаписи
- [ ] Список версий объекта
- [ ] Восстановление из версии
- [ ] Удаление версий

**Структура:**
```
buckets/my-bucket/
  .versioning.json              # {enabled: true}
  objects/
    file.txt                    # Текущая версия
    versions/
      file.txt.v1-uuid1         # Старая версия 1
      file.txt.v2-uuid2         # Старая версия 2
```

### Этап 7: Multipart Uploads (2-3 дня)

#### 7.1 Составная загрузка
- [ ] `initiate_multipart_upload()` - инициализация
- [ ] `upload_part()` - загрузка части
- [ ] `complete_multipart_upload()` - завершение
- [ ] `abort_multipart_upload()` - отмена
- [ ] `list_parts()` - список частей

**Структура:**
```
buckets/my-bucket/
  .multipart/
    upload-id-1/
      part-1.bin
      part-2.bin
      manifest.json
```

### Этап 8: LocalAdminClient (3-4 дня)

#### 8.1 User Management
- [ ] `list_users()` - список пользователей
- [ ] `get_user()` - информация о пользователе
- [ ] `create_user()` - создание пользователя
- [ ] `delete_user()` - удаление пользователя
- [ ] `set_user_policy()` - установка политики

#### 8.2 Group Management
- [ ] `list_groups()` - список групп
- [ ] `create_group()` - создание группы
- [ ] `delete_group()` - удаление группы
- [ ] `add_user_to_group()` - добавление пользователя
- [ ] `remove_user_from_group()` - удаление пользователя

#### 8.3 Policy Management
- [ ] `list_policies()` - список политик
- [ ] `create_policy()` - создание политики
- [ ] `delete_policy()` - удаление политики
- [ ] `attach_policy()` - привязка политики
- [ ] `detach_policy()` - отвязка политики

**Файлы хранения:**
```
users/
  user1.json              # User definition
groups/
  group1.json             # Group definition
policies/
  policy1.json            # Policy document
```

### Этап 9: Access Control & Security (2-3 дня)

#### 9.1 Контроль доступа
- [ ] Парсинг и валидация IAM политик
- [ ] Проверка прав доступа при операциях
- [ ] Bucket policies
- [ ] Resource-based policies

#### 9.2 Аудит
- [ ] Логирование всех операций
- [ ] Access logs
- [ ] Error logs

### Этап 10: Configuration & Integration (1-2 дня)

#### 10.1 Конфигурация
- [ ] Добавление параметров в Config
- [ ] Выбор storage backend (Object Storage или Local)
- [ ] Настройка storage root path

**config.json:**
```json
{
  "storage": {
    "type": "local",  // или "object storage"
    "local": {
      "root_path": "/var/lib/object-storage",
      "enable_versioning": true,
      "cache_size_mb": 1024
    },
    "object storage": {
      "endpoint": "localhost:9000",
      "access_key": "minioadmin",
      "secret_key": "minioadmin"
    }
  }
}
```

#### 10.2 Factory Pattern
- [ ] `StorageClientFactory` - создание клиента
- [ ] Переключение между Local и Object Storage

```cpp
std::shared_ptr<IMinioClient> create_storage_client(const Config& config) {
    if (config.storage_type() == "local") {
        return std::make_shared<LocalStorageClient>(config.storage_root());
    } else {
        return std::make_shared<MinioClient>(/*...*/);
    }
}
```

### Этап 11: Testing (3-4 дня)

#### 11.1 Unit Tests
- [ ] StorageManager tests
- [ ] MetadataManager tests
- [ ] LocalStorageClient bucket operations tests
- [ ] LocalStorageClient object operations tests
- [ ] LocalAdminClient tests

#### 11.2 Integration Tests
- [ ] End-to-end bucket workflow
- [ ] End-to-end object workflow
- [ ] Concurrent access tests
- [ ] Error handling tests
- [ ] Performance tests

#### 11.3 Test Files
```
tests/unit/storage/
  StorageManagerTest.cpp
  MetadataManagerTest.cpp
  LocalStorageClientTest.cpp
  LocalAdminClientTest.cpp

tests/integration/storage/
  BucketWorkflowTest.cpp
  ObjectWorkflowTest.cpp
  ConcurrencyTest.cpp
  PerformanceTest.cpp
```

### Этап 12: Documentation (1 день)

#### 12.1 API Documentation
- [ ] Swagger/OpenAPI spec update
- [ ] Code comments (Doxygen)
- [ ] README updates

#### 12.2 User Guides
- [ ] Configuration guide
- [ ] Migration guide (Object Storage -> Local)
- [ ] Performance tuning guide

## Timeline Summary

| Этап | Описание | Дни |
|------|----------|-----|
| 1 | Подготовка инфраструктуры | 1-2 |
| 2 | Bucket Operations | 2-3 |
| 3 | Object Operations | 3-4 |
| 4 | Metadata & Tags | 1-2 |
| 5 | Presigned URLs | 1-2 |
| 6 | Versioning | 2-3 |
| 7 | Multipart Uploads | 2-3 |
| 8 | Admin Client | 3-4 |
| 9 | Access Control | 2-3 |
| 10 | Configuration | 1-2 |
| 11 | Testing | 3-4 |
| 12 | Documentation | 1 |
| **TOTAL** | | **22-33 дня** |

## Приоритеты

### MVP (Minimum Viable Product) - 10-12 дней
1. Этап 1: Инфраструктура
2. Этап 2: Bucket Operations
3. Этап 3: Object Operations (базовые)
4. Этап 10: Configuration (базовая)
5. Этап 11: Testing (базовое)

### Production Ready - 20-25 дней
MVP + 
6. Этап 4: Metadata & Tags
7. Этап 5: Presigned URLs
8. Этап 9: Access Control (базовый)
9. Этап 11: Testing (полное)

### Enterprise - 30+ дней
Production Ready +
10. Этап 6: Versioning
11. Этап 7: Multipart Uploads
12. Этап 8: Admin Client
13. Этап 9: Access Control (полное)
14. Этап 12: Documentation

## Риски и митигация

| Риск | Вероятность | Влияние | Митигация |
|------|-------------|---------|-----------|
| Производительность файловой системы | Средняя | Высокое | Кэширование, асинхронные операции |
| Атомарность операций | Высокая | Высокое | Тщательное тестирование, WAL |
| Совместимость с S3 API | Средняя | Среднее | Юнит тесты, интеграционные тесты |
| Масштабируемость | Низкая | Среднее | Документирование ограничений |
| Потеря данных | Низкая | Критическое | Atomic operations, backups |

## Следующие шаги

1. ✅ Создать архитектурную документацию
2. ✅ Создать план реализации
3. ⏳ Начать с Этапа 1: Подготовка инфраструктуры
4. ⏳ Реализовать MVP (Этапы 1-3 + 10-11)
5. ⏳ Провести тестирование
6. ⏳ Расширить функциональность до Production Ready

