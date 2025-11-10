# Дорожная карта сборки проекта

**Дата:** 2025-11-10  
**Статус:** 🚧 В ПРОЦЕССЕ (40% готовности к сборке)

## ✅ Выполнено

### 1. Структура проекта (100%)
- ✅ Архитектура Clean Architecture
- ✅ CMake build system с оптимизациями
- ✅ Структура папок (include/, src/, tests/)
- ✅ vcpkg integration через FetchContent

### 2. Зависимости (100%)
- ✅ Drogon Framework
- ✅ nlohmann-json  
- ✅ spdlog
- ✅ jwt-cpp (FetchContent)
- ✅ OpenSSL, Boost
- ✅ GoogleTest

### 3. Модели данных (100%)
- ✅ Bucket, Object, User, Group, Policy
- ✅ Error, ApiError, ApiException  
- ✅ Response templates
- ✅ JSON serialization

### 4. Утилиты (70%)
- ✅ Types.hpp (Result<T,E>, базовые типы)
- ✅ JWT (PBKDF2, AES-256-GCM)
- ⚠️ Config (private constructor issues)
- ⚠️ Logger (spdlog format string issues)
- ✅ StringUtils

## ❌ Критические проблемы компиляции

### P0: Type System Issues

#### 1. Result<T, E> конфликт при T == E
**Файл:** `include/console/common/Types.hpp`  
**Проблема:**
```cpp
Result(T value) : value_(std::move(value)), has_value_(true) {}
Result(E error) : error_(std::move(error)), has_value_(false) {}
// ❌ Конфликт когда T==E (например Result<String, String>)
```

**Решение:**  
Использовать tagged constructors:
```cpp
struct OkTag {};
struct ErrTag {};
static constexpr OkTag ok_tag{};
static constexpr err_tag{};

Result(OkTag, T value) : value_(std::move(value)), has_value_(true) {}
Result(ErrTag, E error) : error_(std::move(error)), has_value_(false) {}

// Helper functions
template<typename T, typename E>
Result<T, E> Ok(T value) {
    return Result<T, E>(Result<T, E>::ok_tag, std::move(value));
}
```

#### 2. Result operator! отсутствует
**Проблема:** Controllers используют `if (!result)`, но operator! не определен.

**Решение:**
```cpp
bool operator!() const noexcept { return !has_value_; }
explicit operator bool() const noexcept { return has_value_; }
```

### P1: Logger Issues

#### 1. spdlog требует compile-time format strings
**Файл:** `include/console/common/Logger.hpp`  
**Проблема:**
```cpp
template<typename... Args>
void info(const String& fmt, Args&&... args) {
    console_logger_->info(fmt, std::forward<Args>(args)...);
    // ❌ spdlog требует fmt быть constexpr
}
```

**Решение A - использовать fmt::runtime():**
```cpp
void info(const String& fmt, Args&&... args) {
    console_logger_->info(fmt::runtime(fmt), std::forward<Args>(args)...);
}
```

**Решение B - использовать fmt::format():**
```cpp
void info(const String& fmt, Args&&... args) {
    auto msg = fmt::format(fmt, std::forward<Args>(args)...);
    console_logger_->info(msg);
}
```

#### 2. Неверные сигнатуры warn/error
**Проблема:** Методы ожидают `source_location` как последний параметр.

**Решение:** Упростить сигнатуры, удалить source_location из variadic methods.

### P2: API Models Issues

#### 1. ApiError::status() не существует
**Файл:** `include/console/models/Error.hpp`  
**Проблема:** Controllers вызывают `error.status()`, но метод называется `to_http_status()`.

**Решение:**
```cpp
HttpStatus status() const { return to_http_status(); }
```

#### 2. models::ServerInfo не определен
**Файл:** `include/console/clients/MinioClient.hpp`  
**Проблема:** `IMinioAdminClient::get_server_info()` возвращает несуществующий тип.

**Решение:** Создать `include/console/models/ServerInfo.hpp`.

### P3: Config Issues

#### 1. Config() конструктор private
**Файл:** `include/console/common/Config.hpp`  
**Проблема:** `std::make_shared<Config>()` не работает с private конструктором.

**Решение A - singleton:**
```cpp
public:
    static Config& instance() {
        static Config instance;
        return instance;
    }
```

**Решение B - public конструктор:**
```cpp
public:
    Config() = default;
```

### P4: Namespace Issues

#### 1. console::utils не существует
**Файлы:** `UsersController.cpp`, `ObjectService.cpp`, и др.  
**Проблема:** `using namespace console::utils;` - namespace не существует.

**Решение:** Удалить все `using namespace console::utils;`.

### P5: JWT Issues

#### 1. jwt::builder requires type parameter
**Файл:** `include/console/utils/JWT.hpp`  
**Проблема:**
```cpp
static jwt::builder create_token_builder(const JWTClaims& claims);
// ❌ jwt::builder требует json_traits параметр
```

**Решение:**
```cpp
using jwt_builder = jwt::builder<jwt::traits::kazuho_picojson>;
static jwt_builder create_token_builder(const JWTClaims& claims);
```

## 📋 План исправления (по приоритетам)

### Фаза 1: Type System (2-3 часа)
1. ✅ Добавить Result<void, E> specialization
2. ⏳ Исправить Result<T, E> конфликт (tagged constructors)
3. ⏳ Добавить operator! и operator bool
4. ⏳ Добавить ApiError::status() метод
5. ⏳ Создать models::ServerInfo

### Фаза 2: Logger & Utils (1-2 часа)
6. ⏳ Исправить Logger (fmt::runtime или fmt::format)
7. ⏳ Упростить сигнатуры warn/error
8. ⏳ Исправить Config constructor
9. ⏳ Удалить console::utils namespace

### Фаза 3: Services & Controllers (2-3 часа)
10. ⏳ Исправить ObjectService signatures
11. ⏳ Исправить BucketService
12. ⏳ Исправить UserService  
13. ⏳ Исправить все Controllers
14. ⏳ Обновить ObjectsController (operator!)
15. ⏳ Обновить UsersController

### Фаза 4: JWT & Auth (1-2 часа)
16. ⏳ Исправить JWT::create_token_builder
17. ⏳ Тестировать JWT encryption/decryption
18. ⏳ Обновить AuthMiddleware

### Фаза 5: MinioClient (3-4 часа)
19. ✅ Stub implementation (done)
20. ⏳ Добавить HTTP client (libcurl или Boost.Beast)
21. ⏳ Реализовать AWS Signature V4
22. ⏳ Реализовать basic operations

### Фаза 6: Testing & Verification (2-3 часа)
23. ⏳ Собрать проект без ошибок
24. ⏳ Запустить unit tests
25. ⏳ Запустить integration tests
26. ⏳ Тестировать с MinIO server

## 📊 Оценка времени

| Фаза | Компонент | Время | Сложность |
|------|-----------|-------|-----------|
| 1 | Type System | 2-3 ч | Средняя |
| 2 | Logger & Utils | 1-2 ч | Низкая |
| 3 | Services | 2-3 ч | Средняя |
| 4 | JWT | 1-2 ч | Низкая |
| 5 | MinioClient | 3-4 ч | Высокая |
| 6 | Testing | 2-3 ч | Средняя |
| **ИТОГО** | **Полная реализация** | **12-17 ч** | **Высокая** |

## 🎯 Milestone targets

### M1: Компиляция без ошибок (Фазы 1-2)
**Цель:** Собрать проект, все зависимости корректны  
**Время:** 3-5 часов  
**Статус:** 🚧 В ПРОЦЕССЕ

### M2: Services работают (Фазы 3-4)
**Цель:** Все сервисы и контроллеры функциональны  
**Время:** +3-5 часов  
**Статус:** ⏳ ОЖИДАЕТ

### M3: MinIO интеграция (Фаза 5)
**Цель:** Реальное взаимодействие с MinIO  
**Время:** +3-4 часа  
**Статус:** ⏳ ОЖИДАЕТ

### M4: Production Ready (Фаза 6)
**Цель:** Все тесты проходят, готов к деплою  
**Время:** +2-3 часа  
**Статус:** ⏳ ОЖИДАЕТ

## 💡 Рекомендации

### Краткосрочные (сейчас)
1. Отключить `-Werror` временно для прогресса
2. Сосредоточиться на Type System  
3. Использовать fmt::runtime() для spdlog

### Среднесрочные (1 неделя)
1. Реализовать все fixes из Фаз 1-2
2. Добавить comprehensive unit tests
3. Настроить CI/CD pipeline

### Долгосрочные (1 месяц)
1. Реальная MinIO интеграция
2. Frontend React интеграция
3. Docker deployment
4. Performance optimization

## 📚 Ссылки

- [Drogon Documentation](https://drogon.org/)
- [spdlog Format](https://github.com/gabime/spdlog#user-defined-types)
- [jwt-cpp Examples](https://github.com/Thalhammer/jwt-cpp#examples)
- [MinIO S3 API](https://min.io/docs/minio/linux/developers/minio-drivers.html)

---

**Последнее обновление:** 2025-11-10 23:45 UTC

