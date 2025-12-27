//
#include "console/cli/CommandLine.hpp"
#include "console/common/Config.hpp"
#include "console/common/Logger.hpp"
#include "console/common/Types.hpp"

// Register controllers by including their headers
#include "console/api/AuthController.hpp"
#include "console/api/BucketsController.hpp"
#include "console/api/DocsController.hpp"
#include "console/api/HealthController.hpp"
#include "console/api/InfrastructureController.hpp"
#include "console/api/ObjectsController.hpp"
#include "console/api/StatsController.hpp"
#include "console/api/UsersController.hpp"

// Register filters and middleware by including their headers
#include "console/filters/AuthFilter.hpp" // Global namespace filter for Drogon
#include "console/filters/CorsFilter.hpp"

// Services
#include "console/services/AuthService.hpp"
#include "console/services/BucketService.hpp"
#include "console/services/EncryptionService.hpp"
#include "console/services/InfrastructureService.hpp"
#include "console/services/ObjectService.hpp"
#include "console/services/StatsCollector.hpp"
#include "console/services/UserService.hpp"

// Storage clients
#include "console/clients/LocalAdminClient.hpp"
#include "console/clients/LocalStorageClient.hpp"

// Storage managers
#include "console/storage/DatabaseManager.hpp"

// Service locator
#include "console/common/ServiceLocator.hpp"

#include <drogon/drogon.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <queue>
#include <random>
#include <sstream>

using namespace console;
using namespace drogon;

namespace {

// Signal handler for graceful shutdown
std::atomic<bool> shutdown_requested{false};

// Thread-local request timing for performance measurement
thread_local std::chrono::steady_clock::time_point request_start_time;

// Generate unique request ID for tracing
String
generate_request_id() {
    static std::atomic<uint64_t> counter{0};
    static const auto start_time = std::chrono::steady_clock::now();

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time).count();
    uint64_t seq = counter.fetch_add(1, std::memory_order_relaxed);

    // Format: timestamp-sequence (compact hex)
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(12) << elapsed << "-" << std::setw(6) << (seq & 0xFFFFFF);
    return oss.str();
}

// Convert HTTP method to string
String
http_method_to_string(drogon::HttpMethod method) {
    switch (method) {
        case drogon::HttpMethod::Get:
            return "GET";
        case drogon::HttpMethod::Post:
            return "POST";
        case drogon::HttpMethod::Put:
            return "PUT";
        case drogon::HttpMethod::Delete:
            return "DELETE";
        case drogon::HttpMethod::Patch:
            return "PATCH";
        case drogon::HttpMethod::Options:
            return "OPTIONS";
        case drogon::HttpMethod::Head:
            return "HEAD";
        default:
            return "UNKNOWN";
    }
}

// SECURITY: Rate limiting for authentication endpoints
class RateLimiter {
  public:
    struct RateLimit {
        int max_requests;   // Max requests allowed
        int window_seconds; // Time window in seconds
    };

    bool is_allowed(const std::string& key, const RateLimit& limit) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();

        // Clean up old entries periodically
        if (++cleanup_counter_ > 100) {
            cleanup_old_entries(now);
            cleanup_counter_ = 0;
        }

        auto& entry = entries_[key];
        auto window_start = now - std::chrono::seconds(limit.window_seconds);

        // Remove requests outside the window
        while (!entry.empty() && entry.front() < window_start) {
            entry.pop();
        }

        // Check if under limit
        if (static_cast<int>(entry.size()) >= limit.max_requests) {
            return false;
        }

        // Record this request
        entry.push(now);
        return true;
    }

  private:
    void cleanup_old_entries(std::chrono::steady_clock::time_point now) {
        auto expiry = now - std::chrono::seconds(300); // 5 minute cleanup window
        for (auto it = entries_.begin(); it != entries_.end();) {
            while (!it->second.empty() && it->second.front() < expiry) {
                it->second.pop();
            }
            if (it->second.empty()) {
                it = entries_.erase(it);
            } else {
                ++it;
            }
        }
    }

    std::mutex mutex_;
    std::map<std::string, std::queue<std::chrono::steady_clock::time_point>> entries_;
    int cleanup_counter_ = 0;
};

// Global rate limiter instance
RateLimiter g_rate_limiter;

void
signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        // Use std::cerr instead of logger to avoid use-after-free
        std::cerr << "\n[INFO] Received shutdown signal (" << signal << ")" << std::endl;
        shutdown_requested = true;

        // Gracefully quit drogon
        try {
            app().quit();
        } catch (...) {
            // Ignore exceptions during shutdown
        }
    }
}

void
setup_signal_handlers() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
}

void
print_banner() {
    std::cout << R"(
    ╔═══════════════════════════════════════════════════════════╗
    ║                                                           ║
    ║     Object Storage                                        ║
    ║     S3-Compatible Web Console                             ║
    ║                                                           ║
    ║     Version: 1.0.0                                        ║
    ║     Built with C++20 & Drogon Framework                   ║
    ║                                                           ║
    ╚═══════════════════════════════════════════════════════════╝
)" << std::endl;
}

// Security validation for configuration
bool
validate_security_config() {
    auto& config = Config::instance();
    bool has_warnings = false;
    bool has_critical = false;

    // Check JWT secret
    auto jwt_secret = config.get<String>("auth.jwt_secret").value_or("");
    const std::vector<String> weak_secrets = {
        "your-secret-key-change-this-in-production", "default-secret-change-me", "changeme", "secret", "password", ""};

    for (const auto& weak : weak_secrets) {
        if (jwt_secret == weak) {
            CONSOLE_LOG_ERROR("╔══════════════════════════════════════════════════════════╗");
            CONSOLE_LOG_ERROR("║  SECURITY CRITICAL: JWT secret is weak or default!       ║");
            CONSOLE_LOG_ERROR("║  Change 'auth.jwt_secret' in config.json immediately!    ║");
            CONSOLE_LOG_ERROR("╚══════════════════════════════════════════════════════════╝");
            has_critical = true;
            break;
        }
    }

    if (jwt_secret.length() < 32) {
        CONSOLE_LOG_WARN("Security Warning: JWT secret should be at least 32 characters");
        has_warnings = true;
    }

    // Check default admin credentials
    auto admin_enabled = config.get<bool>("default_admin.enabled").value_or(true);
    auto admin_password = config.get<String>("default_admin.password").value_or("");

    if (admin_enabled) {
        const std::vector<String> weak_passwords = {"changeme", "password", "admin", "123456", "admin123", ""};

        for (const auto& weak : weak_passwords) {
            if (admin_password == weak) {
                CONSOLE_LOG_ERROR("╔══════════════════════════════════════════════════════════╗");
                CONSOLE_LOG_ERROR("║  SECURITY CRITICAL: Default admin password is weak!      ║");
                CONSOLE_LOG_ERROR("║  Change 'default_admin.password' in config.json!         ║");
                CONSOLE_LOG_ERROR("╚══════════════════════════════════════════════════════════╝");
                has_critical = true;
                break;
            }
        }

        if (admin_password.length() < 12) {
            CONSOLE_LOG_WARN("Security Warning: Admin password should be at least 12 characters");
            has_warnings = true;
        }
    }

    // Check SSL configuration
    auto enable_ssl = config.get<bool>("server.enable_ssl").value_or(false);
    if (!enable_ssl) {
        CONSOLE_LOG_WARN("Security Warning: SSL is disabled - credentials will be sent in plaintext");
        has_warnings = true;
    }

    if (has_warnings && !has_critical) {
        CONSOLE_LOG_WARN("Review security warnings above before deploying to production");
    }

    // Return false only if critical issues exist and we want to block startup
    // For now, we warn but allow startup
    return true;
}

void
configure_drogon(const ServerConfig& config) {
    // Set listening address and port
    app().addListener(config.host, config.port);

    // Set thread number
    app().setThreadNum(config.threads);

    // Enable session
    app().enableSession(3600); // 1 hour

    // Set document root
    auto web_root = std::filesystem::current_path() / "web-app" / "build";
    if (std::filesystem::exists(web_root)) {
        app().setDocumentRoot(web_root.string());
        CONSOLE_LOG_INFO("Serving static files from: {}", web_root.string());
    } else {
        CONSOLE_LOG_WARN("Web app build directory not found: {}", web_root.string());
    }

    // Configure SSL if enabled
    if (config.enable_ssl && !config.ssl_cert.empty() && !config.ssl_key.empty()) {
        app().setSSLFiles(config.ssl_cert, config.ssl_key);
        CONSOLE_LOG_INFO("SSL enabled with cert: {}", config.ssl_cert);
    }

    // Enable compression
    app().enableGzip(true);

    // Set max body size (100MB for file uploads)
    app().setClientMaxBodySize(100 * 1024 * 1024);

    // Set client max body size
    app().setClientMaxBodySize(100 * 1024 * 1024);

    // Set idle connection timeout
    app().setIdleConnectionTimeout(60);

    // Register global filters
    CONSOLE_LOG_INFO("Registering filters...");
    // Filters are registered automatically through HttpFilter template inheritance

    // Enable running location
    // Daemon mode disabled by default

    // Set log path and level
    if (!config.log_path.empty()) {
        std::filesystem::create_directories(config.log_path);
        app().setLogPath(config.log_path);
    }

    // Map log level
    if (config.log_level == "trace") {
        app().setLogLevel(trantor::Logger::kTrace);
    } else if (config.log_level == "debug") {
        app().setLogLevel(trantor::Logger::kDebug);
    } else if (config.log_level == "info") {
        app().setLogLevel(trantor::Logger::kInfo);
    } else if (config.log_level == "warn") {
        app().setLogLevel(trantor::Logger::kWarn);
    } else {
        app().setLogLevel(trantor::Logger::kError);
    }
}

// SECURITY: Check if origin is in allowed list
bool
is_origin_allowed(const std::string& origin) {
    // Empty origin is allowed (same-origin requests, curl, etc.)
    if (origin.empty()) {
        return true;
    }

    // Default allowed origins for development/production
    // In production, configure cors.allowed_origins in config.json
    static const std::vector<std::string> default_allowed = {"http://localhost:3000",
                                                             "http://localhost:9090",
                                                             "http://127.0.0.1:3000",
                                                             "http://127.0.0.1:9090",
                                                             "http://localhost:5173", // Vite dev server
                                                             "http://127.0.0.1:5173"};

    // Check against allowed origins
    for (const auto& allowed : default_allowed) {
        if (origin == allowed) {
            return true;
        }
    }

    // Also allow same-origin (when origin matches server)
    auto& config = Config::instance();
    auto port = config.get<uint16_t>("server.port").value_or(9090);
    std::string server_origin = "http://localhost:" + std::to_string(port);
    if (origin == server_origin) {
        return true;
    }

    CONSOLE_LOG_DEBUG("CORS: Origin '{}' not in whitelist", origin);
    return false;
}

void
setup_cors() {
    CONSOLE_LOG_INFO("Setting up CORS with origin whitelist and rate limiting...");

    // Handle preflight OPTIONS requests and rate limiting BEFORE routing
    app().registerPreRoutingAdvice(
        [](const HttpRequestPtr& req, AdviceCallback&& callback, AdviceChainCallback&& chain) {
            const std::string& path = req->path();
            std::string client_ip = req->getPeerAddr().toIp();

            // SECURITY: Rate limiting for authentication endpoints
            if (path.find("/api/v1/auth/login") != std::string::npos ||
                path.find("/api/v1/auth/refresh") != std::string::npos) {
                // 5 login attempts per minute per IP
                RateLimiter::RateLimit auth_limit{5, 60};
                if (!g_rate_limiter.is_allowed("auth:" + client_ip, auth_limit)) {
                    CONSOLE_LOG_WARN("Security: Rate limit exceeded for auth from IP: {}", client_ip);
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k429TooManyRequests);
                    resp->addHeader("Retry-After", "60");
                    resp->setBody("{\"error\": \"Too many authentication attempts. Please try again later.\"}");
                    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                    callback(resp);
                    return;
                }
            }

            // SECURITY: General API rate limiting (100 requests per minute per IP)
            if (path.find("/api/") != std::string::npos) {
                RateLimiter::RateLimit api_limit{100, 60};
                if (!g_rate_limiter.is_allowed("api:" + client_ip, api_limit)) {
                    CONSOLE_LOG_WARN("Security: API rate limit exceeded from IP: {}", client_ip);
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k429TooManyRequests);
                    resp->addHeader("Retry-After", "60");
                    resp->setBody("{\"error\": \"Rate limit exceeded. Please slow down.\"}");
                    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                    callback(resp);
                    return;
                }
            }

            // Get origin from request
            std::string origin = req->getHeader("Origin");

            // SECURITY: Check origin against whitelist (log warning if not in list)
            if (!origin.empty() && !is_origin_allowed(origin)) {
                // Log warning but allow during development
                // In production, uncomment the block below to reject unknown origins
                /*
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k403Forbidden);
                resp->addHeader("Access-Control-Allow-Origin", origin);
                resp->setBody("{\"error\": \"Origin not allowed\"}");
                resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                callback(resp);
                return;
                */
            }

            // Use origin if provided, otherwise default
            if (origin.empty()) {
                origin = "http://localhost:3000";
            }

            // Handle preflight OPTIONS
            if (req->method() == drogon::HttpMethod::Options) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k204NoContent);
                resp->addHeader("Access-Control-Allow-Origin", origin);
                resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS, PATCH");
                resp->addHeader("Access-Control-Allow-Headers",
                                "Content-Type, Authorization, X-Requested-With, Accept, Origin, "
                                "x-amz-server-side-encryption-customer-key, "
                                "x-amz-server-side-encryption-customer-algorithm, "
                                "x-amz-server-side-encryption-customer-key-md5, "
                                "x-amz-content-sha256, x-amz-date, x-amz-meta-*");
                resp->addHeader("Access-Control-Expose-Headers", "Content-Length, Content-Range, Content-Disposition");
                resp->addHeader("Access-Control-Max-Age", "86400");
                resp->addHeader("Access-Control-Allow-Credentials", "true");
                resp->addHeader("Vary", "Origin");

                CONSOLE_LOG_DEBUG("CORS preflight: {} from {}", req->path(), origin);
                callback(resp);
                return;
            }

            // Continue to next handler
            chain();
        });

    // Add CORS and security headers to all responses AFTER handling
    app().registerPostHandlingAdvice([](const HttpRequestPtr& req, const HttpResponsePtr& resp) {
        std::string origin = req->getHeader("Origin");
        if (origin.empty()) {
            origin = "http://localhost:3000";
        }

        // CORS headers
        resp->addHeader("Access-Control-Allow-Origin", origin);
        resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS, PATCH");
        resp->addHeader("Access-Control-Allow-Headers",
                        "Content-Type, Authorization, X-Requested-With, Accept, Origin, "
                        "x-amz-server-side-encryption-customer-key, "
                        "x-amz-server-side-encryption-customer-algorithm, "
                        "x-amz-server-side-encryption-customer-key-md5, "
                        "x-amz-content-sha256, x-amz-date, x-amz-meta-*");
        resp->addHeader("Access-Control-Expose-Headers", "Content-Length, Content-Range, Content-Disposition");
        resp->addHeader("Access-Control-Allow-Credentials", "true");
        resp->addHeader("Vary", "Origin");

        // SECURITY HEADERS
        // Prevent clickjacking
        resp->addHeader("X-Frame-Options", "DENY");

        // Prevent MIME type sniffing
        resp->addHeader("X-Content-Type-Options", "nosniff");

        // XSS Protection (legacy but still useful for older browsers)
        resp->addHeader("X-XSS-Protection", "1; mode=block");

        // Referrer Policy
        resp->addHeader("Referrer-Policy", "strict-origin-when-cross-origin");

        // Content Security Policy (basic policy)
        resp->addHeader("Content-Security-Policy",
                        "default-src 'self'; "
                        "script-src 'self' 'unsafe-inline' 'unsafe-eval'; "
                        "style-src 'self' 'unsafe-inline'; "
                        "img-src 'self' data: blob:; "
                        "font-src 'self' data:; "
                        "connect-src 'self' ws: wss:; "
                        "frame-ancestors 'none';");

        // Permissions Policy
        resp->addHeader("Permissions-Policy", "geolocation=(), microphone=(), camera=(), payment=()");

        // Check if SSL is enabled for HSTS
        auto& config = Config::instance();
        if (config.get<bool>("server.enable_ssl").value_or(false)) {
            // HTTP Strict Transport Security (1 year)
            resp->addHeader("Strict-Transport-Security", "max-age=31536000; includeSubDomains");
        }

        CONSOLE_LOG_DEBUG("CORS and security headers added: {} from {}", req->path(), origin);
    });

    CONSOLE_LOG_INFO("CORS configured successfully");
}

void
setup_api_stats_collection() {
    CONSOLE_LOG_INFO("Setting up API stats collection...");

    // Register post-handling advice to record API request statistics
    app().registerPostHandlingAdvice([](const HttpRequestPtr& req, const HttpResponsePtr& resp) {
        // Skip stats endpoints to avoid recursion and noise
        const std::string& path = req->path();
        if (path.find("/api/v1/stats") != std::string::npos || path.find("/api/v1/health") != std::string::npos ||
            path.find("/api/v1/version") != std::string::npos) {
            return;
        }

        // Only track API endpoints
        if (path.find("/api/") == std::string::npos) {
            return;
        }

        auto db = ServiceLocator::database();
        if (!db) {
            return;
        }

        storage::ApiRequestStats stats;
        stats.timestamp = std::time(nullptr);
        stats.endpoint = path;
        stats.method = http_method_to_string(req->method());
        stats.status_code = static_cast<int>(resp->statusCode());
        stats.ip_address = req->peerAddr().toIp();
        stats.user_agent = req->getHeader("User-Agent");

        // Get user access key from request attributes if available
        try {
            auto user_info = req->attributes()->get<UserInfo>("user_info");
            stats.user_access_key = user_info.access_key;
        } catch (...) {
            // User not authenticated, leave empty
        }

        // Record stats asynchronously to not block response
        db->add_api_request_stat(stats);
    });

    CONSOLE_LOG_INFO("API stats collection configured successfully");
}

void
setup_http_access_logging() {
    CONSOLE_LOG_INFO("Setting up HTTP access logging...");

    // Pre-routing advice: capture request start time and assign request ID
    app().registerPreRoutingAdvice([](const HttpRequestPtr& req) {
        // Record start time for this request
        request_start_time = std::chrono::steady_clock::now();

        // Generate and store request ID for tracing
        String request_id = generate_request_id();
        req->attributes()->insert("request_id", request_id);
    });

    // Post-handling advice: log completed requests
    app().registerPostHandlingAdvice([](const HttpRequestPtr& req, const HttpResponsePtr& resp) {
        // Calculate request duration
        auto end_time = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - request_start_time).count();
        auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end_time - request_start_time).count();

        // Get request details
        const String& path = req->path();
        String method = http_method_to_string(req->method());
        String client_ip = req->peerAddr().toIp();
        int status_code = static_cast<int>(resp->statusCode());

        // Get request ID
        String request_id;
        try {
            request_id = req->attributes()->get<String>("request_id");
        } catch (...) {
            request_id = "unknown";
        }

        // Get authenticated user (if any)
        String user_id = "-";
        try {
            auto user_info = req->attributes()->get<UserInfo>("user_info");
            user_id = user_info.access_key;
        } catch (...) {
            // Not authenticated
        }

        // Get request/response sizes
        size_t request_size = req->body().size();
        size_t response_size = resp->body().size();

        // Get User-Agent (truncated for readability)
        String user_agent = req->getHeader("User-Agent");
        if (user_agent.length() > 100) {
            user_agent = user_agent.substr(0, 97) + "...";
        }
        if (user_agent.empty()) {
            user_agent = "-";
        }

        // Get query string for full URL reconstruction
        String query_string = req->query();
        String full_path = query_string.empty() ? path : path + "?" + query_string;

        // Truncate very long paths for logging
        if (full_path.length() > 200) {
            full_path = full_path.substr(0, 197) + "...";
        }

        // Determine log level based on status code
        // 2xx, 3xx -> INFO, 4xx -> WARN, 5xx -> ERROR
        if (status_code >= 500) {
            CONSOLE_LOG_ERROR("[{}] {} {} {} \"{}\" {} {}B/{}B {}ms \"{}\"",
                              request_id,
                              client_ip,
                              user_id,
                              method,
                              full_path,
                              status_code,
                              request_size,
                              response_size,
                              duration_ms,
                              user_agent);
        } else if (status_code >= 400) {
            CONSOLE_LOG_WARN("[{}] {} {} {} \"{}\" {} {}B/{}B {}ms \"{}\"",
                             request_id,
                             client_ip,
                             user_id,
                             method,
                             full_path,
                             status_code,
                             request_size,
                             response_size,
                             duration_ms,
                             user_agent);
        } else {
            // For successful requests, use DEBUG for static files, INFO for API
            if (path.find("/api/") != std::string::npos) {
                // Log slow requests (>1000ms) at WARN level
                if (duration_ms > 1000) {
                    CONSOLE_LOG_WARN("[{}] {} {} {} \"{}\" {} {}B/{}B {}ms (SLOW) \"{}\"",
                                     request_id,
                                     client_ip,
                                     user_id,
                                     method,
                                     full_path,
                                     status_code,
                                     request_size,
                                     response_size,
                                     duration_ms,
                                     user_agent);
                } else {
                    CONSOLE_LOG_INFO("[{}] {} {} {} \"{}\" {} {}B/{}B {}ms",
                                     request_id,
                                     client_ip,
                                     user_id,
                                     method,
                                     full_path,
                                     status_code,
                                     request_size,
                                     response_size,
                                     duration_ms);
                }
            } else {
                // Static files - DEBUG level to reduce noise
                CONSOLE_LOG_DEBUG("[{}] {} {} \"{}\" {} {}B {}us",
                                  request_id,
                                  client_ip,
                                  method,
                                  full_path,
                                  status_code,
                                  response_size,
                                  duration_us);
            }
        }

        // Add request ID to response headers for client-side tracing
        resp->addHeader("X-Request-ID", request_id);
    });

    CONSOLE_LOG_INFO("HTTP access logging configured successfully");
}

void
setup_background_tasks() {
    CONSOLE_LOG_INFO("Setting up background tasks...");

    // Task to cleanup old stats (runs daily)
    app().getLoop()->runEvery(86400.0, []() {
        auto db = ServiceLocator::database();
        if (!db)
            return;

        auto now = std::time(nullptr);
        auto week_ago = now - (7 * 24 * 3600); // 7 days ago

        CONSOLE_LOG_INFO("Running cleanup of old statistics data...");

        auto api_cleanup = db->cleanup_old_api_stats(week_ago);
        if (!api_cleanup) {
            CONSOLE_LOG_WARN("Failed to cleanup old API stats: {}", api_cleanup.error());
        }

        auto throughput_cleanup = db->cleanup_old_throughput_stats(week_ago);
        if (!throughput_cleanup) {
            CONSOLE_LOG_WARN("Failed to cleanup old throughput stats: {}", throughput_cleanup.error());
        }

        CONSOLE_LOG_INFO("Cleanup of old statistics completed");
    });

    // Run initial cleanup on startup (delayed by 60 seconds)
    app().getLoop()->runAfter(60.0, []() {
        auto db = ServiceLocator::database();
        if (!db)
            return;

        auto now = std::time(nullptr);
        auto week_ago = now - (7 * 24 * 3600);

        CONSOLE_LOG_DEBUG("Running initial cleanup of old statistics...");
        db->cleanup_old_api_stats(week_ago);
        db->cleanup_old_throughput_stats(week_ago);
    });

    CONSOLE_LOG_INFO("Background tasks configured successfully");
}

void
register_routes() {
    CONSOLE_LOG_INFO("Registering API routes...");

    // Health check endpoint
    app().registerHandler("/api/v1/health",
                          [](const HttpRequestPtr&, std::function<void(const HttpResponsePtr&)>&& callback) {
                              Json::Value response;
                              response["status"] = "ok";
                              response["service"] = "object-storage-console";
                              response["version"] = "1.0.0";
                              response["timestamp"] = static_cast<Json::Int64>(std::time(nullptr));

                              auto resp = HttpResponse::newHttpJsonResponse(response);
                              callback(resp);
                          },
                          {Get});

    // API version endpoint
    app().registerHandler("/api/v1/version",
                          [](const HttpRequestPtr&, std::function<void(const HttpResponsePtr&)>&& callback) {
                              Json::Value response;
                              response["version"] = "1.0.0";
                              response["api_version"] = "v1";
                              response["build_time"] = __DATE__ " " __TIME__;

                              auto resp = HttpResponse::newHttpJsonResponse(response);
                              callback(resp);
                          },
                          {Get});

    // Catch-all for SPA routing
    app().setCustomErrorHandler([](HttpStatusCode code) -> HttpResponsePtr {
        // Default error response
        Json::Value error;
        error["error"] = true;
        error["code"] = static_cast<int>(code);
        error["message"] = "Not Found";

        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(code);
        return resp;
    });

    CONSOLE_LOG_INFO("Routes registered successfully");
}

void
init_services() {
    CONSOLE_LOG_INFO("════════════════════════════════════════════════════════");
    CONSOLE_LOG_INFO("Initializing Services...");
    CONSOLE_LOG_INFO("════════════════════════════════════════════════════════");

    // Initialize storage root path
    auto& config = Config::instance();
    auto storage_root = config.get<String>("storage.root_path").value_or("./storage");
    auto db_path = config.get<String>("database.path").value_or("./data/rocksdb");
    auto db_encryption_key = config.get<String>("database.encryption_key").value_or("");

    CONSOLE_LOG_INFO("  Storage root:  {}", storage_root);
    CONSOLE_LOG_INFO("  Database path: {}", db_path);
    CONSOLE_LOG_INFO("");

    // Create storage directories
    std::filesystem::create_directories(storage_root);
    std::filesystem::create_directories(db_path);

    // Initialize RocksDB database manager (required by admin client)
    auto database = std::make_shared<storage::DatabaseManager>(db_path, db_encryption_key);
    auto init_result = database->initialize();
    if (!init_result) {
        CONSOLE_LOG_ERROR("Failed to initialize database: {}", init_result.error());
        std::exit(1);
    }
    ServiceLocator::set_database(database);
    CONSOLE_LOG_INFO("  ✓ DatabaseManager (RocksDB) initialized");

    // Initialize storage client
    auto storage_client = std::make_shared<clients::LocalStorageClient>(storage_root);
    ServiceLocator::set_storage_client(storage_client);
    CONSOLE_LOG_INFO("  ✓ LocalStorageClient initialized");

    // Initialize admin client
    auto admin_client = std::make_shared<clients::LocalAdminClient>(database);
    ServiceLocator::set_admin_client(admin_client);
    CONSOLE_LOG_INFO("  ✓ LocalAdminClient initialized");

    // Initialize services
    auto object_service = std::make_shared<services::ObjectService>(storage_client);
    ServiceLocator::set_object_service(object_service);
    CONSOLE_LOG_INFO("  ✓ ObjectService initialized");

    auto bucket_service = std::make_shared<services::BucketService>(storage_client);
    ServiceLocator::set_bucket_service(bucket_service);
    CONSOLE_LOG_INFO("  ✓ BucketService initialized");

    auto user_service = std::make_shared<services::UserService>(admin_client);
    ServiceLocator::set_user_service(user_service);
    CONSOLE_LOG_INFO("  ✓ UserService initialized");

    // Create config pointer for AuthService (with custom deleter to avoid deleting singleton)
    std::shared_ptr<Config> config_ptr(&Config::instance(), [](Config*) {});
    auto auth_service = std::make_shared<services::AuthService>(admin_client, config_ptr, database);
    ServiceLocator::set_auth_service(auth_service);
    CONSOLE_LOG_INFO("  ✓ AuthService initialized");

    // Initialize StatsCollector for background statistics collection
    auto stats_collector = std::make_shared<services::StatsCollector>(database);
    ServiceLocator::set_stats_collector(stats_collector);

    // Initialize sample data for dashboard (servers, drives, pools)
    stats_collector->initialize_sample_data();

    // Start background collection tasks
    stats_collector->start();
    CONSOLE_LOG_INFO("  ✓ StatsCollector initialized and started");

    // Initialize InfrastructureService for pools/drives/servers management
    auto infrastructure_service = std::make_shared<services::InfrastructureService>(database);
    ServiceLocator::set_infrastructure_service(infrastructure_service);
    CONSOLE_LOG_INFO("  ✓ InfrastructureService initialized");

    // Initialize EncryptionService for server-side encryption
    auto encryption_service = std::make_shared<services::EncryptionService>();
    ServiceLocator::set_encryption_service(encryption_service);
    CONSOLE_LOG_INFO("  ✓ EncryptionService initialized");

    CONSOLE_LOG_INFO("");
    CONSOLE_LOG_INFO("All services initialized successfully!");
    CONSOLE_LOG_INFO("════════════════════════════════════════════════════════");
}

} // anonymous namespace

int
main(int argc, char* argv[]) {
    try {
        // Check for CLI commands first
        if (cli::CommandLine::is_cli_command(argc, argv)) {
            cli::CommandLine cli;
            int result = cli.run(argc, argv);
            // Flush output buffers before quick exit
            std::cout.flush();
            std::cerr.flush();
            // Use _Exit() to avoid static destruction order issues with RocksDB
            std::_Exit(result);
        }

        print_banner();

        // Setup signal handlers
        setup_signal_handlers();

        // Initialize logger
        Logger::instance().init(LogLevel::Info,
                                "logs",
                                10 * 1024 * 1024, // 10MB
                                5);

        CONSOLE_LOG_INFO("Starting Object Storage Console...");

        // Load configuration
        auto& config = Config::instance();
        std::filesystem::path config_path = "config.json";

        // Check for --config argument
        for (int i = 1; i < argc; i++) {
            String arg = argv[i];
            if (arg == "--config" && i + 1 < argc) {
                config_path = argv[++i];
            } else if (arg.find("--config=") == 0) {
                config_path = arg.substr(9);
            }
        }

        if (std::filesystem::exists(config_path)) {
            CONSOLE_LOG_INFO("Loading configuration from: {}", config_path.string());
            if (!config.load_from_file(config_path)) {
                CONSOLE_LOG_ERROR("Failed to load configuration");
                for (const auto& error : config.validation_errors()) {
                    CONSOLE_LOG_ERROR("  - {}", error);
                }
                return 1;
            }
        } else {
            CONSOLE_LOG_WARN("Configuration file not found, using defaults");
            CONSOLE_LOG_WARN("Create {} to customize settings", config_path.string());
        }

        // Validate configuration
        if (!config.is_valid()) {
            CONSOLE_LOG_ERROR("Invalid configuration:");
            for (const auto& error : config.validation_errors()) {
                CONSOLE_LOG_ERROR("  - {}", error);
            }
            return 1;
        }

        // Validate security configuration
        if (!validate_security_config()) {
            CONSOLE_LOG_ERROR("Security validation failed - refusing to start");
            return 1;
        }

        // Initialize services (CRITICAL: must be before configure_drogon)
        init_services();

        // Configure Drogon
        const auto& server_config = config.server();
        configure_drogon(server_config);

        // Setup CORS (MUST be before routes)
        setup_cors();

        // Setup API stats collection
        setup_api_stats_collection();

        // Setup HTTP access logging (REST method, status, IP, timing)
        setup_http_access_logging();

        // Setup background tasks
        setup_background_tasks();

        // Register routes
        register_routes();

        // Print server info
        CONSOLE_LOG_INFO("════════════════════════════════════════════════════════");
        CONSOLE_LOG_INFO("Server Configuration:");
        CONSOLE_LOG_INFO("  Host:        {}", server_config.host);
        CONSOLE_LOG_INFO("  Port:        {}", server_config.port);
        CONSOLE_LOG_INFO("  Threads:     {}", server_config.threads);
        CONSOLE_LOG_INFO("  SSL:         {}", server_config.enable_ssl ? "enabled" : "disabled");
        CONSOLE_LOG_INFO("  Log Level:   {}", server_config.log_level);
        CONSOLE_LOG_INFO("════════════════════════════════════════════════════════");

        std::cout << "\n";
        CONSOLE_LOG_INFO("Server starting...");
        CONSOLE_LOG_INFO("Access the console at: http{}://{}:{}",
                         server_config.enable_ssl ? "s" : "",
                         server_config.host == "0.0.0.0" ? "localhost" : server_config.host,
                         server_config.port);
        CONSOLE_LOG_INFO("Press Ctrl+C to stop\n");

        // Run the application
        app().run();

        // Cleanup - use std::cerr to avoid use-after-free with logger
        std::cerr << "\n[INFO] Server stopped" << std::endl;

        // Explicitly clear service locator before static destruction
        try {
            ServiceLocator::clear();
        } catch (...) {
            // Ignore cleanup errors
        }

        // Flush logger safely
        try {
            Logger::instance().flush();
        } catch (...) {
            // Ignore flush errors during shutdown
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        // Safe cleanup
        try {
            ServiceLocator::clear();
            Logger::instance().flush();
        } catch (...) {
            // Ignore cleanup errors
        }
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        Logger::instance().flush();
        return 1;
    }
}
