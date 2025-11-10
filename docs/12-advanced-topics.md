# 12. Расширенные темы (C++)

## 🚀 Современный C++20

### Coroutines для асинхронных операций

```cpp
#include <coroutine>
#include <drogon/HttpController.h>

// Task type для coroutines
template<typename T>
struct Task {
    struct promise_type {
        T value;
        std::exception_ptr exception;
        
        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        
        void return_value(T val) { value = std::move(val); }
        
        void unhandled_exception() { exception = std::current_exception(); }
    };
    
    std::coroutine_handle<promise_type> handle;
    
    T get() {
        if (handle.promise().exception) {
            std::rethrow_exception(handle.promise().exception);
        }
        return std::move(handle.promise().value);
    }
};

// Использование coroutines в контроллере
Task<HttpResponsePtr> getBucketObjects(
    const std::string& bucket,
    const std::string& prefix
) {
    auto client = co_await getObjectStorageClient();
    
    // Асинхронный вызов
    auto objects = co_await client->listObjects(bucket, prefix);
    
    json response = {
        {"objects", objects},
        {"total", objects.size()}
    };
    
    co_return HttpResponse::newHttpJsonResponse(response);
}
```

### Concepts для type constraints

```cpp
#include <concepts>

// Concept для Object Storage клиента
template<typename T>
concept ObjectStorageClient = requires(T client, std::string bucket) {
    { client.listBuckets() } -> std::same_as<Task<std::vector<Bucket>>>;
    { client.createBucket(bucket) } -> std::same_as<Task<void>>;
    { client.deleteBucket(bucket) } -> std::same_as<Task<void>>;
};

// Функция с concept constraint
template<ObjectStorageClient Client>
Task<json> getBucketsInfo(Client& client) {
    auto buckets = co_await client.listBuckets();
    
    json result = json::array();
    for (const auto& bucket : buckets) {
        result.push_back({
            {"name", bucket.name},
            {"created", bucket.creationDate}
        });
    }
    
    co_return result;
}

// Concept для аутентификации
template<typename T>
concept AuthStrategy = requires(T auth, Credentials creds) {
    { auth.authenticate(creds) } -> std::same_as<Task<Principal>>;
    { auth.validate(std::string{}) } -> std::same_as<bool>;
};
```

### Ranges для обработки коллекций

```cpp
#include <ranges>
#include <algorithm>

// Фильтрация и трансформация объектов
auto filterLargeObjects(const std::vector<Object>& objects, size_t minSize) {
    return objects 
        | std::views::filter([minSize](const Object& obj) {
            return obj.size >= minSize;
          })
        | std::views::transform([](const Object& obj) {
            return json{
                {"name", obj.name},
                {"size", obj.size},
                {"modified", obj.lastModified}
            };
          });
}

// Pipeline обработки
auto processBuckets(const std::vector<Bucket>& buckets) {
    return buckets
        | std::views::filter([](const Bucket& b) { return !b.name.empty(); })
        | std::views::transform([](const Bucket& b) {
            return BucketInfo{
                .name = b.name,
                .stats = calculateStats(b)
            };
          })
        | std::views::take(100)  // Limit
        | std::ranges::to<std::vector>();
}
```

## ⚡ Высокопроизводительная архитектура

### Thread Pool для параллельной обработки

```cpp
#include <BS_thread_pool.hpp>

class BucketService {
    BS::thread_pool pool_;
    
public:
    BucketService() : pool_(std::thread::hardware_concurrency()) {}
    
    Task<std::vector<BucketWithStats>> getBucketsWithStats(
        ObjectStorageClient& client
    ) {
        auto buckets = co_await client.listBuckets();
        
        std::vector<std::future<BucketStats>> futures;
        futures.reserve(buckets.size());
        
        // Параллельное получение статистики
        for (const auto& bucket : buckets) {
            futures.push_back(pool_.submit([&client, &bucket]() -> BucketStats {
                auto objects = client.listObjects(bucket.name).get();
                
                size_t totalSize = 0;
                for (const auto& obj : objects) {
                    totalSize += obj.size;
                }
                
                return BucketStats{
                    .objectCount = objects.size(),
                    .totalSize = totalSize
                };
            }));
        }
        
        // Сбор результатов
        std::vector<BucketWithStats> results;
        for (size_t i = 0; i < buckets.size(); ++i) {
            auto stats = futures[i].get();
            results.push_back({buckets[i], stats});
        }
        
        co_return results;
    }
};
```

### Lock-free структуры данных

```cpp
#include <atomic>
#include <memory>

// Lock-free queue для логов
template<typename T>
class LockFreeQueue {
    struct Node {
        std::shared_ptr<T> data;
        std::atomic<Node*> next{nullptr};
    };
    
    std::atomic<Node*> head_{nullptr};
    std::atomic<Node*> tail_{nullptr};
    
public:
    void push(T value) {
        auto data = std::make_shared<T>(std::move(value));
        Node* newNode = new Node();
        newNode->data = data;
        
        Node* oldTail = tail_.exchange(newNode);
        if (oldTail) {
            oldTail->next.store(newNode);
        } else {
            head_.store(newNode);
        }
    }
    
    std::shared_ptr<T> pop() {
        Node* oldHead = head_.load();
        if (!oldHead) return nullptr;
        
        if (head_.compare_exchange_strong(oldHead, oldHead->next.load())) {
            return oldHead->data;
        }
        
        return nullptr;
    }
};

// Использование для асинхронного логирования
class AsyncLogger {
    LockFreeQueue<std::string> queue_;
    std::jthread worker_;
    std::atomic<bool> running_{true};
    
public:
    AsyncLogger() {
        worker_ = std::jthread([this] {
            while (running_) {
                if (auto msg = queue_.pop()) {
                    spdlog::info(*msg);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }
    
    void log(std::string message) {
        queue_.push(std::move(message));
    }
    
    ~AsyncLogger() {
        running_ = false;
    }
};
```

### Memory Pool для оптимизации аллокаций

```cpp
#include <memory_resource>

class RequestHandler {
    // Custom allocator для request processing
    std::pmr::synchronized_pool_resource pool_;
    
public:
    HttpResponsePtr handleRequest(HttpRequestPtr req) {
        // Используем pool для временных аллокаций
        std::pmr::polymorphic_allocator<char> alloc{&pool_};
        
        std::pmr::vector<uint8_t> buffer{alloc};
        buffer.reserve(1024);
        
        // Process request using buffer
        
        return response;
    }
};
```

## 🔐 Продвинутая безопасность

### PBKDF2 для ключа шифрования

```cpp
#include <openssl/evp.h>
#include <openssl/kdf.h>

class CryptoService {
public:
    static std::vector<uint8_t> deriveKey(
        const std::string& password,
        const std::vector<uint8_t>& salt,
        int iterations = 100000
    ) {
        std::vector<uint8_t> key(32);  // 256 bits
        
        PKCS5_PBKDF2_HMAC(
            password.data(), password.size(),
            salt.data(), salt.size(),
            iterations,
            EVP_sha256(),
            key.size(), key.data()
        );
        
        return key;
    }
    
    static std::string encryptAES256GCM(
        const std::string& plaintext,
        const std::vector<uint8_t>& key
    ) {
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        
        // Generate random IV
        std::vector<uint8_t> iv(12);
        RAND_bytes(iv.data(), iv.size());
        
        // Initialize encryption
        EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), iv.data());
        
        // Encrypt
        std::vector<uint8_t> ciphertext(plaintext.size() + 16);
        int len = 0;
        
        EVP_EncryptUpdate(ctx, ciphertext.data(), &len,
                         reinterpret_cast<const uint8_t*>(plaintext.data()),
                         plaintext.size());
        
        int finalLen = 0;
        EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &finalLen);
        
        // Get auth tag
        std::vector<uint8_t> tag(16);
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag.data());
        
        EVP_CIPHER_CTX_free(ctx);
        
        // Combine IV + ciphertext + tag
        std::vector<uint8_t> result;
        result.insert(result.end(), iv.begin(), iv.end());
        result.insert(result.end(), ciphertext.begin(), ciphertext.begin() + len);
        result.insert(result.end(), tag.begin(), tag.end());
        
        return base64Encode(result);
    }
};
```

### Rate Limiting с Token Bucket

```cpp
#include <chrono>
#include <mutex>

class TokenBucket {
    double tokens_;
    double maxTokens_;
    double refillRate_;  // tokens per second
    std::chrono::steady_clock::time_point lastRefill_;
    mutable std::mutex mutex_;
    
public:
    TokenBucket(double maxTokens, double refillRate)
        : tokens_(maxTokens),
          maxTokens_(maxTokens),
          refillRate_(refillRate),
          lastRefill_(std::chrono::steady_clock::now()) {}
    
    bool tryConsume(double tokens = 1.0) {
        std::lock_guard lock(mutex_);
        
        refill();
        
        if (tokens_ >= tokens) {
            tokens_ -= tokens;
            return true;
        }
        
        return false;
    }
    
private:
    void refill() {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration<double>(now - lastRefill_).count();
        
        tokens_ = std::min(maxTokens_, tokens_ + duration * refillRate_);
        lastRefill_ = now;
    }
};

// Middleware для rate limiting
class RateLimitMiddleware {
    std::unordered_map<std::string, std::unique_ptr<TokenBucket>> buckets_;
    std::mutex mapMutex_;
    
public:
    void doFilter(const HttpRequestPtr& req,
                  FilterCallback&& fcb,
                  FilterChainCallback&& fccb) override {
        std::string clientIp = req->getPeerAddr().toIp();
        
        TokenBucket* bucket = nullptr;
        {
            std::lock_guard lock(mapMutex_);
            auto it = buckets_.find(clientIp);
            if (it == buckets_.end()) {
                buckets_[clientIp] = std::make_unique<TokenBucket>(100, 10);  // 100 tokens, 10/sec refill
                bucket = buckets_[clientIp].get();
            } else {
                bucket = it->second.get();
            }
        }
        
        if (bucket->tryConsume()) {
            fccb();  // Continue to next filter
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k429TooManyRequests);
            fcb(resp);
        }
    }
};
```

## 📊 Observability & Monitoring

### Structured Logging с spdlog

```cpp
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

class Logger {
public:
    static void initialize() {
        // Console sink
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);
        
        // Rotating file sink
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "logs/console.log", 1024 * 1024 * 10, 3);  // 10MB, 3 files
        file_sink->set_level(spdlog::level::trace);
        
        // Combined logger
        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
        auto logger = std::make_shared<spdlog::logger>("main", sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::trace);
        
        spdlog::set_default_logger(logger);
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");
    }
    
    template<typename... Args>
    static void info(fmt::format_string<Args...> fmt, Args&&... args) {
        spdlog::info(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void error(fmt::format_string<Args...> fmt, Args&&... args) {
        spdlog::error(fmt, std::forward<Args>(args)...);
    }
};

// Usage
Logger::info("User {} logged in from IP {}", username, clientIp);
Logger::error("Failed to create bucket {}: {}", bucketName, error.what());
```

### Prometheus Metrics

```cpp
#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include <prometheus/histogram.h>
#include <prometheus/gauge.h>

class Metrics {
    std::shared_ptr<prometheus::Registry> registry_;
    
public:
    // HTTP request counter
    prometheus::Family<prometheus::Counter>& httpRequestsTotal;
    
    // Request duration histogram
    prometheus::Family<prometheus::Histogram>& httpDuration;
    
    // Active connections gauge
    prometheus::Family<prometheus::Gauge>& activeConnections;
    
    Metrics() 
        : registry_(std::make_shared<prometheus::Registry>()),
          httpRequestsTotal(prometheus::BuildCounter()
              .Name("http_requests_total")
              .Help("Total HTTP requests")
              .Register(*registry_)),
          httpDuration(prometheus::BuildHistogram()
              .Name("http_request_duration_seconds")
              .Help("HTTP request latency")
              .Register(*registry_)),
          activeConnections(prometheus::BuildGauge()
              .Name("active_connections")
              .Help("Active HTTP connections")
              .Register(*registry_)) {}
    
    void recordRequest(const std::string& method, int statusCode, 
                      std::chrono::duration<double> duration) {
        httpRequestsTotal.Add({
            {"method", method},
            {"status", std::to_string(statusCode)}
        }).Increment();
        
        httpDuration.Add({
            {"method", method}
        }, {0.001, 0.01, 0.1, 1.0, 10.0}).Observe(duration.count());
    }
};
```

### Health Checks

```cpp
class HealthCheckController : public HttpController<HealthCheckController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(HealthCheckController::liveness, "/health/live", Get);
    ADD_METHOD_TO(HealthCheckController::readiness, "/health/ready", Get);
    METHOD_LIST_END
    
    void liveness(const HttpRequestPtr& req,
                  std::function<void(const HttpResponsePtr&)>&& callback) {
        // Liveness - процесс жив
        json response = {
            {"status", "ok"},
            {"timestamp", std::chrono::system_clock::now()}
        };
        
        auto resp = HttpResponse::newHttpJsonResponse(response);
        callback(resp);
    }
    
    void readiness(const HttpRequestPtr& req,
                   std::function<void(const HttpResponsePtr&)>&& callback) {
        // Readiness - готов принимать трафик
        bool dbReady = checkDatabaseConnection();
        bool storageReady = checkObjectStorageConnection();
        
        if (dbReady && storageReady) {
            json response = {
                {"status", "ready"},
                {"checks", {
                    {"database", "ok"},
                    {"storage", "ok"}
                }}
            };
            
            auto resp = HttpResponse::newHttpJsonResponse(response);
            callback(resp);
        } else {
            json response = {
                {"status", "not_ready"},
                {"checks", {
                    {"database", dbReady ? "ok" : "failed"},
                    {"storage", storageReady ? "ok" : "failed"}
                }}
            };
            
            auto resp = HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(k503ServiceUnavailable);
            callback(resp);
        }
    }
};
```

## 🌐 Multi-threading Best Practices

### Thread-safe Singleton

```cpp
template<typename T>
class Singleton {
public:
    static T& getInstance() {
        static T instance;  // Thread-safe since C++11
        return instance;
    }
    
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    
protected:
    Singleton() = default;
    virtual ~Singleton() = default;
};

// Usage
class Config : public Singleton<Config> {
    friend class Singleton<Config>;
    
private:
    std::string endpoint_;
    int port_;
    
    Config() {
        loadFromFile("config.yaml");
    }
    
public:
    const std::string& getEndpoint() const { return endpoint_; }
    int getPort() const { return port_; }
};
```

## 🚀 Следующие шаги

- **[13-practical-exercises.md](13-practical-exercises.md)** - Практические упражнения
- **[14-learning-roadmap.md](14-learning-roadmap.md)** - Дорожная карта обучения
- **[02-backend-deep-dive.md](02-backend-deep-dive.md)** - Детальный разбор backend на C++

