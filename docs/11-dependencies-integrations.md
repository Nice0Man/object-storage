# 11. Зависимости и интеграции (C++)

## 🔧 Ключевые C++ библиотеки

### Web Framework & HTTP

#### 1. Drogon (Рекомендуется)

```cmake
find_package(Drogon CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Drogon::Drogon)
```

**Возможности:**

- ✅ Высокая производительность (асинхронный I/O)
- ✅ Встроенная поддержка WebSocket
- ✅ ORM для баз данных
- ✅ HTTP/1.1 и HTTP/2
- ✅ Middleware поддержка
- ✅ JSON автоматическая сериализация

**Альтернативы:**

- **Oat++** - быстрый и современный
- **Crow** - легковесный, похож на Flask
- **Pistache** - REST-ориентированный
- **CppCMS** - полнофункциональный

### 2. JSON Processing

```cmake
# nlohmann/json - популярная библиотека
find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)

# RapidJSON - быстрая альтернатива
find_package(RapidJSON CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE rapidjson)
```

**Пример использования:**

```cpp
#include <nlohmann/json.hpp>
using json = nlohmann::json;

json obj = {
    {"name", "bucket-name"},
    {"size", 1048576},
    {"created", "2025-11-10"}
};

std::string serialized = obj.dump();
json parsed = json::parse(serialized);
```

### 3. JWT Authentication

```cmake
# jwt-cpp
find_package(jwt-cpp CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE jwt-cpp::jwt-cpp)
```

**Пример:**

```cpp
#include <jwt-cpp/jwt.h>

auto token = jwt::create()
    .set_issuer("console")
    .set_type("JWT")
    .set_payload_claim("userId", jwt::claim(std::string("user123")))
    .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours{12})
    .sign(jwt::algorithm::hs256{"secret"});

auto decoded = jwt::decode(token);
auto verifier = jwt::verify()
    .allow_algorithm(jwt::algorithm::hs256{"secret"})
    .with_issuer("console");

verifier.verify(decoded);
```

### 4. Logging

```cmake
# spdlog - быстрая асинхронная библиотека логирования
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)

# Альтернатива: Boost.Log
find_package(Boost COMPONENTS log REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::log)
```

**Пример:**

```cpp
#include <spdlog/spdlog.h>

spdlog::info("User logged in: {}", username);
spdlog::error("Failed to create bucket: {}", error_msg);
spdlog::debug("Request details: method={}, path={}", method, path);
```

### 5. Async I/O & Coroutines

```cmake
# Boost.Asio - асинхронный I/O
find_package(Boost COMPONENTS system REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::system)

# C++20 Coroutines (встроены в стандарт)
# Требует C++20 компилятор
```

**Пример с coroutines (C++20):**

```cpp
#include <coroutine>
#include <drogon/HttpController.h>

Task<HttpResponsePtr> listBuckets(HttpRequestPtr req) {
    auto client = co_await createObjectStorageClient(req);
    auto buckets = co_await client->listBuckets();

    json response = {{"buckets", buckets}};
    co_return HttpResponse::newHttpJsonResponse(response);
}
```

### 6. Database ORM

```cmake
# Drogon ORM (встроен в Drogon)
# Поддерживает: PostgreSQL, MySQL, SQLite

# Альтернативы:
# - ODB ORM
# - SOCI
find_package(SOCI CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE SOCI::soci_core)
```

### 7. Криптография

```cmake
# OpenSSL
find_package(OpenSSL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)

# Crypto++
find_package(cryptopp CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE cryptopp::cryptopp)
```

**Пример шифрования:**

```cpp
#include <openssl/evp.h>
#include <openssl/rand.h>

std::string encrypt(const std::string& plaintext, const std::string& key) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    unsigned char iv[16];
    RAND_bytes(iv, sizeof(iv));

    EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr,
                       (unsigned char*)key.data(), iv);

    // Encryption logic...

    EVP_CIPHER_CTX_free(ctx);
    return encrypted;
}
```

### 8. HTTP Client (для интеграций)

```cmake
# CPR - wrapper над libcurl
find_package(cpr CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE cpr::cpr)

# Альтернатива: CURL напрямую
find_package(CURL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE CURL::libcurl)
```

**Пример:**

```cpp
#include <cpr/cpr.h>

auto response = cpr::Get(
    cpr::Url{"https://api.example.com/data"},
    cpr::Bearer{"jwt-token"}
);

if (response.status_code == 200) {
    json data = json::parse(response.text);
}
```

### 9. WebSocket

```cmake
# websocketpp - header-only библиотека
find_package(websocketpp CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE websocketpp::websocketpp)

# Или встроенная поддержка в Drogon
```

**Пример WebSocket сервера:**

```cpp
#include <drogon/WebSocketController.h>

class LogStreamController : public drogon::WebSocketController<LogStreamController> {
public:
    void handleNewMessage(const WebSocketConnectionPtr& conn,
                         std::string&& message,
                         const WebSocketMessageType& type) override {
        // Handle incoming messages
    }

    void handleConnectionClosed(const WebSocketConnectionPtr& conn) override {
        spdlog::info("WebSocket connection closed");
    }

    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/ws/logs", Get);
    WS_PATH_LIST_END
};
```

### 10. Testing

```cmake
# Google Test
find_package(GTest CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gtest GTest::gtest_main)

# Catch2 - альтернатива
find_package(Catch2 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Catch2::Catch2)
```

**Пример теста:**

```cpp
#include <gtest/gtest.h>

TEST(BucketValidator, ValidateBucketName) {
    EXPECT_TRUE(validateBucketName("valid-bucket"));
    EXPECT_FALSE(validateBucketName("Invalid-Bucket"));
    EXPECT_FALSE(validateBucketName("ab"));
}
```

### 11. Configuration Management

```cmake
# yaml-cpp - для конфигурационных файлов
find_package(yaml-cpp CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE yaml-cpp)

# toml11 - TOML parser
find_package(toml11 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE toml11::toml11)
```

**Пример:**

```cpp
#include <yaml-cpp/yaml.h>

YAML::Node config = YAML::LoadFile("config.yaml");
std::string endpoint = config["object_storage"]["endpoint"].as<std::string>();
int port = config["server"]["port"].as<int>();
```

### 12. Thread Pool

```cmake
# ThreadPool (header-only)
# Или использовать встроенные возможности C++
```

**Пример:**

```cpp
#include <BS_thread_pool.hpp>

BS::thread_pool pool(std::thread::hardware_concurrency());

std::vector<std::future<BucketStats>> futures;
for (const auto& bucket : buckets) {
    futures.push_back(pool.submit([&bucket]() {
        return getBucketStats(bucket);
    }));
}

for (auto& future : futures) {
    auto stats = future.get();
    // Process stats
}
```

## 🔗 Интеграции с внешними сервисами

### 1. Object Storage SDK

```cpp
// Абстрактный интерфейс для работы с Object Storage
class ObjectStorageClient {
public:
    virtual ~ObjectStorageClient() = default;

    virtual Task<std::vector<Bucket>> listBuckets() = 0;
    virtual Task<void> createBucket(const std::string& name) = 0;
    virtual Task<void> deleteBucket(const std::string& name) = 0;

    virtual Task<std::vector<Object>> listObjects(const std::string& bucket) = 0;
    virtual Task<void> uploadObject(const std::string& bucket,
                                    const std::string& key,
                                    std::istream& data) = 0;
    virtual Task<std::string> getPresignedUrl(const std::string& bucket,
                                              const std::string& key) = 0;
};

// Реализация для S3-совместимого storage
class S3CompatibleClient : public ObjectStorageClient {
    // Implementation using AWS SDK or custom HTTP client
};
```

### 2. LDAP Integration

```cmake
# OpenLDAP client library
find_package(LDAP REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE ${LDAP_LIBRARIES})
```

**Пример:**

```cpp
#include <ldap.h>

bool authenticateLDAP(const std::string& username, const std::string& password) {
    LDAP* ld = nullptr;

    int rc = ldap_initialize(&ld, "ldap://ldap.example.com:389");
    if (rc != LDAP_SUCCESS) {
        return false;
    }

    std::string dn = fmt::format("uid={},dc=example,dc=org", username);
    rc = ldap_simple_bind_s(ld, dn.c_str(), password.c_str());

    ldap_unbind_ext_s(ld, nullptr, nullptr);
    return rc == LDAP_SUCCESS;
}
```

### 3. OAuth2/OIDC

```cpp
// Используя CPR для HTTP requests
class OAuth2Provider {
public:
    std::string getAuthorizationUrl() {
        return fmt::format(
            "{}?client_id={}&redirect_uri={}&response_type=code&scope={}",
            auth_endpoint_, client_id_, redirect_uri_, scope_
        );
    }

    Task<TokenResponse> exchangeCodeForToken(const std::string& code) {
        auto response = co_await cpr::PostAsync(
            cpr::Url{token_endpoint_},
            cpr::Payload{
                {"code", code},
                {"client_id", client_id_},
                {"client_secret", client_secret_},
                {"grant_type", "authorization_code"}
            }
        );

        json data = json::parse(response.text);
        co_return TokenResponse{
            .access_token = data["access_token"],
            .id_token = data["id_token"],
            .expires_in = data["expires_in"]
        };
    }
};
```

### 4. Redis (для кэширования)

```cmake
# redis-plus-plus
find_package(redis++ CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE redis++::redis++)
```

**Пример:**

```cpp
#include <sw/redis++/redis++.h>

using namespace sw::redis;

class RedisCache {
    Redis redis_;

public:
    RedisCache(const std::string& uri) : redis_(uri) {}

    std::optional<std::string> get(const std::string& key) {
        auto val = redis_.get(key);
        if (val) {
            return *val;
        }
        return std::nullopt;
    }

    void set(const std::string& key, const std::string& value,
             std::chrono::seconds ttl) {
        redis_.set(key, value, ttl);
    }
};
```

### 5. Prometheus Metrics

```cmake
# prometheus-cpp
find_package(prometheus-cpp CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE prometheus-cpp::core)
```

**Пример:**

```cpp
#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include <prometheus/gauge.h>

auto& counter_family = prometheus::BuildCounter()
    .Name("http_requests_total")
    .Help("Total HTTP requests")
    .Register(registry);

auto& counter = counter_family.Add({{"method", "GET"}});
counter.Increment();
```

## 📦 Package Managers

### vcpkg (Рекомендуется)

```bash
# Установка
git clone https://github.com/Microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh

# Установка библиотек
./vcpkg install drogon
./vcpkg install nlohmann-json
./vcpkg install jwt-cpp
./vcpkg install spdlog
./vcpkg install gtest

# Integration с CMake
cmake -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake ..
```

**vcpkg.json:**

```json
{
  "name": "object-storage-console",
  "version": "1.0.0",
  "dependencies": [
    "drogon",
    "nlohmann-json",
    "jwt-cpp",
    "spdlog",
    "openssl",
    "gtest",
    "yaml-cpp",
    "cpr"
  ]
}
```

### Conan (Альтернатива)

```python
# conanfile.txt
[requires]
drogon/1.9.0
nlohmann_json/3.11.2
jwt-cpp/0.7.0
spdlog/1.12.0
openssl/3.1.0
gtest/1.14.0

[generators]
CMakeDeps
CMakeToolchain
```

```bash
# Установка
conan install . --output-folder=build --build=missing
cmake -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake ..
```

## 🏗️ CMake Project Structure

```cmake
cmake_minimum_required(VERSION 3.20)
project(ObjectStorageConsole CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(Drogon CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(jwt-cpp CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(GTest CONFIG REQUIRED)

# Sources
file(GLOB_RECURSE SOURCES
    src/*.cpp
    src/api/*.cpp
    src/services/*.cpp
    src/models/*.cpp
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Drogon::Drogon
    nlohmann_json::nlohmann_json
    jwt-cpp::jwt-cpp
    spdlog::spdlog
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Tests
enable_testing()
add_subdirectory(tests)
```

## 🎯 Следующие шаги

- **[12-advanced-topics.md](12-advanced-topics.md)** - Продвинутые темы на C++
- **[13-practical-exercises.md](13-practical-exercises.md)** - Практические упражнения
- **[14-learning-roadmap.md](14-learning-roadmap.md)** - План обучения
