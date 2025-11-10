# 14. Дорожная карта обучения (C++)

## 🎯 Обзор

Этот план предназначен для изучения современной C++ разработки веб-приложений на примере Object Storage Console.

## 📚 Предварительные требования

### Минимальные знания

- ✅ Базовый C++ (классы, наследование, указатели)
- ✅ STL (vector, map, string, iostream)
- ✅ Основы HTTP/REST
- ✅ Базовые знания Linux/Unix command line
- ✅ Git basics

### Желательно знать

- 🔸 Умные указатели (unique_ptr, shared_ptr)
- 🔸 Lambda functions
- 🔸 Templates basics
- 🔸 Многопоточность (threads, mutex)
- 🔸 Базовые паттерны проектирования

## 🗓️ План обучения (8 недель)

### Неделя 1: Основы Modern C++ и Setup

#### Цели
- Изучить C++11/14/17/20 features
- Настроить окружение разработки
- Понять структуру проекта

#### Задачи

**День 1-2: Установка окружения**
```bash
# Установить необходимые инструменты
- Компилятор (GCC 11+ или Clang 14+)
- CMake 3.20+
- vcpkg package manager
- Visual Studio Code / CLion IDE
```

**День 3-4: Modern C++ features**
- Smart pointers (unique_ptr, shared_ptr, weak_ptr)
- Move semantics и rvalue references
- Lambda expressions
- auto, decltype
- Range-based for loops
- nullptr

**Код для практики:**
```cpp
// Smart pointers
auto ptr = std::make_unique<MyClass>();
auto shared = std::make_shared<Data>();

// Lambda
auto add = [](int a, int b) { return a + b; };

// Range-based for
std::vector<int> vec{1, 2, 3, 4, 5};
for (const auto& val : vec) {
    std::cout << val << '\n';
}
```

**День 5-7: Структура проекта**
- Читать [00-overview.md](00-overview.md)
- Читать [01-architecture.md](01-architecture.md)
- Настроить [13-practical-exercises.md#setup](13-practical-exercises.md) - Упражнение 1

**Проект недели:**
Создать "Hello World" HTTP сервер с Drogon.

---

### Неделя 2: Drogon Framework Deep Dive

#### Цели
- Освоить Drogon framework
- Понять асинхронное программирование
- Создать первые REST endpoints

#### Задачи

**День 1-2: Drogon основы**
```cpp
// Простой контроллер
class HelloController : public HttpController<HelloController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(HelloController::hello, "/hello", Get);
    METHOD_LIST_END
    
    void hello(const HttpRequestPtr& req,
               std::function<void(const HttpResponsePtr&)>&& callback) {
        json response = {{"message", "Hello World"}};
        auto resp = HttpResponse::newHttpJsonResponse(response);
        callback(resp);
    }
};
```

**День 3-4: Middleware и фильтры**
- Создать authentication middleware
- Logging middleware
- CORS middleware

**День 5-7: Асинхронность с callbacks**
```cpp
void asyncOperation(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    // Асинхронная операция
    drogon::app().getIOLoop()->queueInLoop([callback]() {
        auto resp = HttpResponse::newHttpResponse();
        callback(resp);
    });
}
```

**Проект недели:**
[13-practical-exercises.md#упражнение-2](13-practical-exercises.md) - REST API Controller

---

### Неделя 3: C++20 Coroutines и Async Programming

#### Цели
- Освоить C++20 coroutines
- Понять async/await pattern
- Реализовать асинхронные операции

#### Задачи

**День 1-3: Coroutines теория**
```cpp
// Coroutine basics
Task<int> asyncCompute() {
    co_await someAsyncOp();
    int result = 42;
    co_return result;
}

// Использование
Task<void> caller() {
    int value = co_await asyncCompute();
    std::cout << value << '\n';
}
```

**День 4-5: Promise и Future**
```cpp
std::future<int> asyncCalculation() {
    return std::async(std::launch::async, []() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return 42;
    });
}

int result = asyncCalculation().get();
```

**День 6-7: Практика**
- Реализовать async HTTP client
- Параллельные запросы с coroutines
- Error handling в async code

**Проект недели:**
Создать сервис с несколькими async endpoints, использующими coroutines.

---

### Неделя 4: JWT Authentication и Security

#### Цели
- Реализовать JWT authentication
- Понять криптографию (OpenSSL)
- Secure coding practices

#### Задачи

**День 1-2: JWT библиотека**
```cpp
#include <jwt-cpp/jwt.h>

auto token = jwt::create()
    .set_issuer("console")
    .set_payload_claim("userId", jwt::claim("123"))
    .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours{24})
    .sign(jwt::algorithm::hs256{"secret"});
```

**День 3-4: OpenSSL криптография**
- AES encryption/decryption
- PBKDF2 key derivation
- TLS/SSL certificates

**День 5-7: Реализация auth системы**
- Login endpoint
- Token validation middleware
- Password hashing (bcrypt/scrypt)

**Проект недели:**
[13-practical-exercises.md#упражнение-3](13-practical-exercises.md) - JWT Authentication

**Читать:**
- [04-authentication-authorization.md](04-authentication-authorization.md)

---

### Неделя 5: WebSocket и Real-time Communication

#### Цели
- Освоить WebSocket protocol
- Реализовать real-time features
- Broadcasting messages

#### Задачи

**День 1-2: WebSocket basics**
```cpp
class WsController : public WebSocketController<WsController> {
public:
    void handleNewConnection(const HttpRequestPtr& req,
                            const WebSocketConnectionPtr& conn) override {
        conn->send("Welcome!");
    }
    
    void handleNewMessage(const WebSocketConnectionPtr& conn,
                         std::string&& message,
                         const WebSocketMessageType& type) override {
        conn->send(message);  // Echo
    }
    
    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/ws/echo", Get);
    WS_PATH_LIST_END
};
```

**День 3-5: Broadcasting и connection management**
- Connection pool
- Broadcast to all clients
- Room-based messaging

**День 6-7: Практика**
- Real-time log streaming
- Live metrics dashboard
- Chat application

**Проект недели:**
[13-practical-exercises.md#упражнение-4](13-practical-exercises.md) - WebSocket Logs

**Читать:**
- [06-websocket-architecture.md](06-websocket-architecture.md)

---

### Неделя 6: Testing и Quality Assurance

#### Цели
- Освоить Google Test
- Написать unit tests
- Integration testing

#### Задачи

**День 1-2: Google Test framework**
```cpp
#include <gtest/gtest.h>

TEST(MyTest, BasicAssertion) {
    EXPECT_EQ(2 + 2, 4);
    ASSERT_TRUE(true);
}

class MyFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code
    }
};

TEST_F(MyFixture, UsesFixture) {
    // Test code
}
```

**День 3-4: Mocking с GMock**
```cpp
class MockClient : public ObjectStorageClient {
public:
    MOCK_METHOD(Task<std::vector<Bucket>>, listBuckets, (), (override));
    MOCK_METHOD(Task<void>, createBucket, (const std::string&), (override));
};

TEST(ServiceTest, ListBuckets) {
    MockClient mockClient;
    EXPECT_CALL(mockClient, listBuckets())
        .WillOnce(Return(std::vector<Bucket>{}));
    
    Service service(&mockClient);
    // Test logic
}
```

**День 5-7: TDD практика**
- Написать тесты для auth service
- Тесты для API controllers
- Integration tests

**Проект недели:**
[13-practical-exercises.md#упражнение-5](13-practical-exercises.md) - Unit Testing

**Читать:**
- [08-testing.md](08-testing.md)

---

### Неделя 7: Advanced C++ Patterns

#### Цели
- Template metaprogramming
- Design patterns
- Performance optimization

#### Задачи

**День 1-2: Templates**
```cpp
// Template class
template<typename T>
class Container {
    std::vector<T> data_;
public:
    void add(T item) { data_.push_back(item); }
    T get(size_t index) { return data_[index]; }
};

// Template function
template<typename T>
T max(T a, T b) {
    return (a > b) ? a : b;
}

// Variadic templates
template<typename... Args>
void print(Args... args) {
    (std::cout << ... << args) << '\n';
}
```

**День 3-4: Design Patterns**
- Singleton
- Factory
- Observer
- Strategy
- Repository

**День 5-7: Performance**
- Profiling с perf
- Memory optimization
- Lock-free structures

**Читать:**
- [10-patterns-best-practices.md](10-patterns-best-practices.md)
- [12-advanced-topics.md](12-advanced-topics.md)

---

### Неделя 8: Финальный проект

#### Цель
Создать полноценное приложение, объединяющее все изученное.

#### Проект: Mini Object Storage Console

**Функции:**
1. ✅ JWT authentication
2. ✅ CRUD операции для buckets/objects
3. ✅ WebSocket для real-time updates
4. ✅ File upload/download
5. ✅ User management
6. ✅ Logging и monitoring
7. ✅ Unit tests (coverage >80%)
8. ✅ Integration tests
9. ✅ Docker deployment

**Архитектура:**
```
src/
├── main.cpp
├── api/
│   ├── AuthController.cpp
│   ├── BucketController.cpp
│   ├── ObjectController.cpp
│   └── UserController.cpp
├── services/
│   ├── AuthService.cpp
│   ├── StorageService.cpp
│   └── UserService.cpp
├── models/
│   ├── Bucket.hpp
│   ├── Object.hpp
│   └── User.hpp
└── utils/
    ├── JWTService.cpp
    └── Logger.cpp
```

**Checklist:**
- [ ] Настроен CMake проект
- [ ] Установлены зависимости через vcpkg
- [ ] Реализованы все контроллеры
- [ ] Написаны unit tests (>80% coverage)
- [ ] Добавлены integration tests
- [ ] Настроен Docker
- [ ] Написана документация
- [ ] Code review пройден

---

## 📖 Дополнительные ресурсы

### Книги

1. **"Effective Modern C++"** - Scott Meyers
   - C++11/14 best practices
   
2. **"C++ Concurrency in Action"** - Anthony Williams
   - Multithreading и async

3. **"C++20 - The Complete Guide"** - Nicolai Josuttis
   - Современный C++20

4. **"Design Patterns: Elements of Reusable OO Software"** - Gang of Four
   - Классические паттерны

### Online курсы

- **CppCon Talks** (YouTube)
- **C++ Weekly** - Jason Turner
- **Back to Basics** series (CppCon)
- **Udemy**: C++ Advanced Topics

### Документация

- [cppreference.com](https://en.cppreference.com/)
- [Drogon Framework Docs](https://drogon.org/)
- [Boost Documentation](https://www.boost.org/doc/)
- [CMake Documentation](https://cmake.org/documentation/)

### Практика

- **LeetCode** (C++ problems)
- **HackerRank** (C++ track)
- **GitHub** (contribute to open source C++ projects)

---

## 🎯 Критерии успеха

### После 8 недель вы должны уметь:

✅ Писать современный C++20 код  
✅ Создавать high-performance веб-приложения  
✅ Использовать async/await с coroutines  
✅ Реализовывать secure authentication  
✅ Работать с WebSocket  
✅ Писать comprehensive tests  
✅ Применять design patterns  
✅ Оптимизировать performance  
✅ Deploy приложения в production  

---

## 🚀 Следующий уровень

### Advanced Topics (после 8 недель)

1. **Distributed Systems**
   - Microservices architecture
   - gRPC communication
   - Service mesh

2. **Cloud Native**
   - Kubernetes deployment
   - Helm charts
   - Auto-scaling

3. **Performance Engineering**
   - SIMD optimizations
   - Cache-friendly code
   - Zero-copy techniques

4. **Real-world Projects**
   - Contribute to Drogon
   - Build production apps
   - Open source contributions

---

## 📞 Поддержка

- **Stack Overflow** - C++ tag
- **Reddit** - r/cpp
- **Discord** - C++ servers
- **CppLang Slack**

---

**Удачи в изучении современной C++ разработки! 🚀**

