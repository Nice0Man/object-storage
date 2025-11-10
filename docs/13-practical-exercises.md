# 13. Практические упражнения (C++)

## 🎯 Setup локального окружения

### 1. Установка зависимостей

#### Установка vcpkg

```bash
# Clone vcpkg
cd ~
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh

# Добавить в PATH
echo 'export VCPKG_ROOT="$HOME/vcpkg"' >> ~/.bashrc
echo 'export PATH="$VCPKG_ROOT:$PATH"' >> ~/.bashrc
source ~/.bashrc

# Установка библиотек
vcpkg install drogon
vcpkg install nlohmann-json
vcpkg install jwt-cpp
vcpkg install spdlog
vcpkg install gtest
vcpkg install openssl
```

#### Установка компилятора

```bash
# GCC 11+ (Ubuntu/Debian)
sudo apt update
sudo apt install build-essential g++-11 cmake

# Clang 14+ (альтернатива)
sudo apt install clang-14

# Проверка версии
g++ --version  # Должен быть 11+
cmake --version  # Должен быть 3.20+
```

### 2. Создание проекта

```bash
mkdir object-storage-console
cd object-storage-console

# Структура проекта
mkdir -p src/{api,services,models,utils}
mkdir -p include/{api,services,models,utils}
mkdir -p tests
mkdir -p web-app
```

### 3. CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(ObjectStorageConsole VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# vcpkg toolchain
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

# Find packages
find_package(Drogon CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(jwt-cpp CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(GTest CONFIG REQUIRED)

# Source files
file(GLOB_RECURSE SOURCES
    src/*.cpp
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Drogon::Drogon
    nlohmann_json::nlohmann_json
    jwt-cpp::jwt-cpp
    spdlog::spdlog
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Enable testing
enable_testing()
add_subdirectory(tests)
```

## 🔨 Упражнение 1: Базовый HTTP Server

### main.cpp

```cpp
#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

int main() {
    // Configure logging
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("Starting Object Storage Console");
    
    // Load configuration
    drogon::app().loadConfigFile("config.json");
    
    // Configure server
    drogon::app()
        .setLogPath("./logs")
        .setLogLevel(trantor::Logger::kInfo)
        .addListener("0.0.0.0", 9090)
        .setThreadNum(4)
        .enableRunAsDaemon()
        .run();
    
    return 0;
}
```

### config.json

```json
{
  "listeners": [
    {
      "address": "0.0.0.0",
      "port": 9090,
      "https": false
    }
  ],
  "app": {
    "threads_num": 4,
    "enable_session": true,
    "session_timeout": 3600,
    "document_root": "./web-app/build",
    "upload_path": "./uploads"
  },
  "log": {
    "log_path": "./logs",
    "logfile_base_name": "console",
    "log_size_limit": 100000000,
    "log_level": "DEBUG"
  }
}
```

### Сборка и запуск

```bash
# Сборка
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)

# Запуск
./ObjectStorageConsole

# Тест
curl http://localhost:9090/
```

## 🔨 Упражнение 2: Создание REST API Controller

### include/api/HealthController.hpp

```cpp
#pragma once
#include <drogon/HttpController.h>
#include <nlohmann/json.hpp>

using namespace drogon;
using json = nlohmann::json;

namespace api {

class HealthController : public HttpController<HealthController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(HealthController::health, "/api/v1/health", Get);
    ADD_METHOD_TO(HealthController::version, "/api/v1/version", Get);
    METHOD_LIST_END
    
    void health(const HttpRequestPtr& req,
                std::function<void(const HttpResponsePtr&)>&& callback);
    
    void version(const HttpRequestPtr& req,
                 std::function<void(const HttpResponsePtr&)>&& callback);
};

}
```

### src/api/HealthController.cpp

```cpp
#include "api/HealthController.hpp"
#include <spdlog/spdlog.h>

namespace api {

void HealthController::health(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    spdlog::debug("Health check request from {}", req->getPeerAddr().toIp());
    
    json response = {
        {"status", "healthy"},
        {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
    };
    
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k200OK);
    callback(resp);
}

void HealthController::version(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    json response = {
        {"version", "1.0.0"},
        {"build", __DATE__ " " __TIME__},
        {"compiler", "GCC " __VERSION__}
    };
    
    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

}
```

### Тестирование

```bash
# Rebuild
cd build
cmake --build .

# Запуск
./ObjectStorageConsole

# Тесты
curl http://localhost:9090/api/v1/health
curl http://localhost:9090/api/v1/version
```

## 🔨 Упражнение 3: Аутентификация с JWT

### include/auth/JWTService.hpp

```cpp
#pragma once
#include <jwt-cpp/jwt.h>
#include <string>
#include <optional>

namespace auth {

struct Principal {
    std::string userId;
    std::string username;
    std::vector<std::string> roles;
};

class JWTService {
    std::string secret_;
    std::string issuer_;
    int expirationHours_;
    
public:
    JWTService(const std::string& secret, 
               const std::string& issuer = "console",
               int expirationHours = 12);
    
    std::string generateToken(const Principal& principal);
    std::optional<Principal> validateToken(const std::string& token);
};

}
```

### src/auth/JWTService.cpp

```cpp
#include "auth/JWTService.hpp"
#include <spdlog/spdlog.h>

namespace auth {

JWTService::JWTService(const std::string& secret, 
                       const std::string& issuer,
                       int expirationHours)
    : secret_(secret), issuer_(issuer), expirationHours_(expirationHours) {}

std::string JWTService::generateToken(const Principal& principal) {
    auto now = std::chrono::system_clock::now();
    auto exp = now + std::chrono::hours{expirationHours_};
    
    auto token = jwt::create()
        .set_issuer(issuer_)
        .set_type("JWT")
        .set_issued_at(now)
        .set_expires_at(exp)
        .set_payload_claim("userId", jwt::claim(principal.userId))
        .set_payload_claim("username", jwt::claim(principal.username))
        .set_payload_claim("roles", jwt::claim(principal.roles))
        .sign(jwt::algorithm::hs256{secret_});
    
    spdlog::debug("Generated JWT for user: {}", principal.username);
    return token;
}

std::optional<Principal> JWTService::validateToken(const std::string& token) {
    try {
        auto decoded = jwt::decode(token);
        
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{secret_})
            .with_issuer(issuer_);
        
        verifier.verify(decoded);
        
        Principal principal;
        principal.userId = decoded.get_payload_claim("userId").as_string();
        principal.username = decoded.get_payload_claim("username").as_string();
        
        auto roles = decoded.get_payload_claim("roles");
        for (const auto& role : roles.as_array()) {
            principal.roles.push_back(role.as_string());
        }
        
        return principal;
        
    } catch (const std::exception& e) {
        spdlog::error("JWT validation failed: {}", e.what());
        return std::nullopt;
    }
}

}
```

### include/api/AuthController.hpp

```cpp
#pragma once
#include <drogon/HttpController.h>
#include "auth/JWTService.hpp"

namespace api {

class AuthController : public HttpController<AuthController> {
    std::shared_ptr<auth::JWTService> jwtService_;
    
public:
    AuthController();
    
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthController::login, "/api/v1/login", Post);
    ADD_METHOD_TO(AuthController::logout, "/api/v1/logout", Post);
    METHOD_LIST_END
    
    void login(const HttpRequestPtr& req,
               std::function<void(const HttpResponsePtr&)>&& callback);
    
    void logout(const HttpRequestPtr& req,
                std::function<void(const HttpResponsePtr&)>&& callback);
};

}
```

### src/api/AuthController.cpp

```cpp
#include "api/AuthController.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace api {

AuthController::AuthController() {
    jwtService_ = std::make_shared<auth::JWTService>("your-secret-key");
}

void AuthController::login(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    auto jsonBody = req->getJsonObject();
    if (!jsonBody) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    
    std::string username = (*jsonBody)["username"].asString();
    std::string password = (*jsonBody)["password"].asString();
    
    // TODO: Validate credentials against database
    // For demo, accept any non-empty credentials
    
    if (username.empty() || password.empty()) {
        json error = {{"message", "Invalid credentials"}};
        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k401Unauthorized);
        callback(resp);
        return;
    }
    
    // Generate JWT
    auth::Principal principal{
        .userId = "user123",
        .username = username,
        .roles = {"user"}
    };
    
    std::string token = jwtService_->generateToken(principal);
    
    // Return token
    json response = {
        {"token", token},
        {"expiresIn", 43200}  // 12 hours in seconds
    };
    
    auto resp = HttpResponse::newHttpJsonResponse(response);
    
    // Also set as cookie
    Cookie cookie("token", token);
    cookie.setPath("/");
    cookie.setHttpOnly(true);
    cookie.setMaxAge(43200);
    resp->addCookie(cookie);
    
    callback(resp);
}

void AuthController::logout(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    // Clear cookie
    auto resp = HttpResponse::newHttpResponse();
    
    Cookie cookie("token", "");
    cookie.setPath("/");
    cookie.setMaxAge(0);
    resp->addCookie(cookie);
    
    resp->setStatusCode(k200OK);
    callback(resp);
}

}
```

### Тестирование

```bash
# Login
curl -X POST http://localhost:9090/api/v1/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"password"}'

# Сохранить токен
TOKEN="<полученный токен>"

# Использовать токен
curl http://localhost:9090/api/v1/protected \
  -H "Authorization: Bearer $TOKEN"
```

## 🔨 Упражнение 4: WebSocket для Real-time Logs

### include/api/LogStreamController.hpp

```cpp
#pragma once
#include <drogon/WebSocketController.h>
#include <memory>
#include <set>

namespace api {

class LogStreamController : public drogon::WebSocketController<LogStreamController> {
    std::set<drogon::WebSocketConnectionPtr> connections_;
    std::mutex connectionsMutex_;
    
public:
    void handleNewMessage(const drogon::WebSocketConnectionPtr& conn,
                         std::string&& message,
                         const drogon::WebSocketMessageType& type) override;
    
    void handleConnectionClosed(const drogon::WebSocketConnectionPtr& conn) override;
    
    void handleNewConnection(const HttpRequestPtr& req,
                            const drogon::WebSocketConnectionPtr& conn) override;
    
    void broadcastLog(const std::string& logMessage);
    
    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/ws/logs", Get);
    WS_PATH_LIST_END
};

}
```

### src/api/LogStreamController.cpp

```cpp
#include "api/LogStreamController.hpp"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace api {

void LogStreamController::handleNewConnection(
    const HttpRequestPtr& req,
    const drogon::WebSocketConnectionPtr& conn
) {
    spdlog::info("New WebSocket connection from {}", req->getPeerAddr().toIp());
    
    std::lock_guard lock(connectionsMutex_);
    connections_.insert(conn);
    
    // Send welcome message
    json welcome = {
        {"type", "connected"},
        {"message", "Connected to log stream"}
    };
    conn->send(welcome.dump());
}

void LogStreamController::handleNewMessage(
    const drogon::WebSocketConnectionPtr& conn,
    std::string&& message,
    const drogon::WebSocketMessageType& type
) {
    spdlog::debug("Received WebSocket message: {}", message);
    
    // Echo back
    conn->send(message);
}

void LogStreamController::handleConnectionClosed(
    const drogon::WebSocketConnectionPtr& conn
) {
    spdlog::info("WebSocket connection closed");
    
    std::lock_guard lock(connectionsMutex_);
    connections_.erase(conn);
}

void LogStreamController::broadcastLog(const std::string& logMessage) {
    json message = {
        {"type", "log"},
        {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()},
        {"message", logMessage}
    };
    
    std::lock_guard lock(connectionsMutex_);
    for (const auto& conn : connections_) {
        conn->send(message.dump());
    }
}

}
```

### Frontend WebSocket Client (JavaScript)

```html
<!DOCTYPE html>
<html>
<head>
    <title>Log Viewer</title>
</head>
<body>
    <h1>Real-time Logs</h1>
    <div id="logs" style="font-family: monospace; white-space: pre;"></div>
    
    <script>
        const ws = new WebSocket('ws://localhost:9090/ws/logs');
        const logsDiv = document.getElementById('logs');
        
        ws.onopen = () => {
            console.log('Connected to log stream');
        };
        
        ws.onmessage = (event) => {
            const data = JSON.parse(event.data);
            const logLine = `[${new Date(data.timestamp).toISOString()}] ${data.message}\n`;
            logsDiv.textContent += logLine;
            logsDiv.scrollTop = logsDiv.scrollHeight;
        };
        
        ws.onerror = (error) => {
            console.error('WebSocket error:', error);
        };
        
        ws.onclose = () => {
            console.log('Disconnected from log stream');
        };
    </script>
</body>
</html>
```

## 🔨 Упражнение 5: Unit Testing с Google Test

### tests/CMakeLists.txt

```cmake
# Test executable
add_executable(console_tests
    auth_tests.cpp
    utils_tests.cpp
)

target_link_libraries(console_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    jwt-cpp::jwt-cpp
    nlohmann_json::nlohmann_json
)

# Discover tests
include(GoogleTest)
gtest_discover_tests(console_tests)
```

### tests/auth_tests.cpp

```cpp
#include <gtest/gtest.h>
#include "auth/JWTService.hpp"

using namespace auth;

class JWTServiceTest : public ::testing::Test {
protected:
    std::unique_ptr<JWTService> service;
    
    void SetUp() override {
        service = std::make_unique<JWTService>("test-secret");
    }
};

TEST_F(JWTServiceTest, GenerateAndValidateToken) {
    Principal principal{
        .userId = "user123",
        .username = "testuser",
        .roles = {"admin", "user"}
    };
    
    std::string token = service->generateToken(principal);
    
    ASSERT_FALSE(token.empty());
    
    auto validated = service->validateToken(token);
    
    ASSERT_TRUE(validated.has_value());
    EXPECT_EQ(validated->userId, "user123");
    EXPECT_EQ(validated->username, "testuser");
    EXPECT_EQ(validated->roles.size(), 2);
}

TEST_F(JWTServiceTest, InvalidTokenReturnsNullopt) {
    auto result = service->validateToken("invalid.token.here");
    
    EXPECT_FALSE(result.has_value());
}

TEST_F(JWTServiceTest, TokenExpirationCheck) {
    // Create service with 0-hour expiration
    JWTService shortService("test-secret", "console", 0);
    
    Principal principal{
        .userId = "user123",
        .username = "testuser",
        .roles = {}
    };
    
    std::string token = shortService.generateToken(principal);
    
    // Wait for expiration
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    auto result = shortService.validateToken(token);
    EXPECT_FALSE(result.has_value());
}
```

### Запуск тестов

```bash
cd build
cmake --build .
ctest --output-on-failure
```

## 🎯 Дополнительные упражнения

### 1. Добавить Middleware для аутентификации
### 2. Реализовать CRUD для Buckets
### 3. Добавить rate limiting
### 4. Реализовать file upload/download
### 5. Добавить интеграционные тесты

## 🚀 Следующие шаги

- **[14-learning-roadmap.md](14-learning-roadmap.md)** - Дорожная карта обучения
- **[12-advanced-topics.md](12-advanced-topics.md)** - Продвинутые темы

