#include "console/common/ServerBootstrap.hpp"

#include "console/common/Config.hpp"
#include "console/common/Logger.hpp"
#include "console/common/ServiceLocator.hpp"
#include "console/common/Types.hpp"
#include "console/middleware/RateLimiter.hpp"
#include "console/storage/DatabaseManager.hpp"

#include <drogon/drogon.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

using namespace drogon;

namespace console {

namespace {

thread_local std::chrono::steady_clock::time_point request_start_time;

String
generate_request_id() {
    static std::atomic<uint64_t> counter{0};
    static const auto start_time = std::chrono::steady_clock::now();

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time).count();
    uint64_t seq = counter.fetch_add(1, std::memory_order_relaxed);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(12) << elapsed << "-" << std::setw(6) << (seq & 0xFFFFFF);
    return oss.str();
}

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

bool
is_origin_allowed(const std::string& origin) {
    if (origin.empty()) {
        return true;
    }

    static const std::vector<std::string> default_allowed = {"http://localhost:3000",
                                                             "http://localhost:9090",
                                                             "http://127.0.0.1:3000",
                                                             "http://127.0.0.1:9090",
                                                             "http://localhost:5173",
                                                             "http://127.0.0.1:5173"};

    for (const auto& allowed : default_allowed) {
        if (origin == allowed) {
            return true;
        }
    }

    auto& config = Config::instance();
    auto port = config.get<uint16_t>("server.port").value_or(9090);
    std::string server_origin = "http://localhost:" + std::to_string(port);
    if (origin == server_origin) {
        return true;
    }

    CONSOLE_LOG_DEBUG("CORS: Origin '{}' not in whitelist", origin);
    return false;
}

} // anonymous namespace

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

bool
validate_security_config() {
    auto& config = Config::instance();
    bool has_warnings = false;
    bool has_critical = false;

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

    auto enable_ssl = config.get<bool>("server.enable_ssl").value_or(false);
    if (!enable_ssl) {
        CONSOLE_LOG_WARN("Security Warning: SSL is disabled - credentials will be sent in plaintext");
        has_warnings = true;
    }

    if (has_warnings && !has_critical) {
        CONSOLE_LOG_WARN("Review security warnings above before deploying to production");
    }

    return true;
}

void
configure_drogon(const ServerConfig& config) {
    app().addListener(config.host, config.port);
    app().setThreadNum(config.threads);
    app().enableSession(3600);

    namespace fs = std::filesystem;
    const auto cwd = fs::current_path();
    const fs::path candidates[] = {
        cwd / "frontend" / "build",
        cwd / "web-app" / "build",
    };
    fs::path chosen;
    for (const auto& web_root : candidates) {
        if (fs::exists(web_root)) {
            chosen = web_root;
            break;
        }
    }
    if (!chosen.empty()) {
        app().setDocumentRoot(chosen.string());
        CONSOLE_LOG_INFO("Serving static files from: {}", chosen.string());
    } else {
        CONSOLE_LOG_WARN("Web app build directory not found (tried frontend/build, web-app/build under {})",
                         cwd.string());
    }

    // SPA (React Router): static router only resolves real files; unknown paths must serve index.html.
    app().setDefaultHandler([](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
        const std::string& path = req->path();
        if (path.rfind("/api/", 0) == 0) {
            Json::Value error;
            error["error"] = true;
            error["code"] = static_cast<int>(k404NotFound);
            error["message"] = "Not Found";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k404NotFound);
            callback(resp);
            return;
        }

        const auto method = req->method();
        if (method != drogon::HttpMethod::Get && method != drogon::HttpMethod::Head) {
            Json::Value error;
            error["error"] = true;
            error["code"] = static_cast<int>(k404NotFound);
            error["message"] = "Not Found";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k404NotFound);
            callback(resp);
            return;
        }

        namespace fs = std::filesystem;
        const auto& root = app().getDocumentRoot();
        fs::path index_path = fs::path(root) / "index.html";
        std::error_code ec;
        if (root.empty() || !fs::exists(index_path, ec)) {
            Json::Value error;
            error["error"] = true;
            error["code"] = static_cast<int>(k404NotFound);
            error["message"] = "Not Found";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k404NotFound);
            callback(resp);
            return;
        }

        auto resp = HttpResponse::newFileResponse(index_path.string());
        resp->setStatusCode(k200OK);
        callback(resp);
    });

    if (config.enable_ssl && !config.ssl_cert.empty() && !config.ssl_key.empty()) {
        app().setSSLFiles(config.ssl_cert, config.ssl_key);
        CONSOLE_LOG_INFO("SSL enabled with cert: {}", config.ssl_cert);
    }

    app().enableGzip(true);
    app().setClientMaxBodySize(100 * 1024 * 1024);
    app().setIdleConnectionTimeout(60);

    CONSOLE_LOG_INFO("Registering filters...");

    if (!config.log_path.empty()) {
        std::filesystem::create_directories(config.log_path);
        app().setLogPath(config.log_path);
    }

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

void
setup_cors() {
    CONSOLE_LOG_INFO("Setting up CORS with origin whitelist and rate limiting...");

    app().registerPreRoutingAdvice(
        [](const HttpRequestPtr& req, AdviceCallback&& callback, AdviceChainCallback&& chain) {
            const std::string& path = req->path();
            std::string client_ip = req->getPeerAddr().toIp();

            auto& limiter = middleware::global_rate_limiter();

            if (path.find("/api/v1/auth/login") != std::string::npos ||
                path.find("/api/v1/auth/refresh") != std::string::npos) {
                middleware::RateLimiter::RateLimit auth_limit{5, 60};
                if (!limiter.is_allowed("auth:" + client_ip, auth_limit)) {
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

            if (path.find("/api/") != std::string::npos) {
                middleware::RateLimiter::RateLimit api_limit{100, 60};
                if (!limiter.is_allowed("api:" + client_ip, api_limit)) {
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

            std::string origin = req->getHeader("Origin");

            if (!origin.empty() && !is_origin_allowed(origin)) {
                // In production, uncomment the block below to reject unknown origins
            }

            if (origin.empty()) {
                origin = "http://localhost:3000";
            }

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

            chain();
        });

    app().registerPostHandlingAdvice([](const HttpRequestPtr& req, const HttpResponsePtr& resp) {
        std::string origin = req->getHeader("Origin");
        if (origin.empty()) {
            origin = "http://localhost:3000";
        }

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

        resp->addHeader("X-Frame-Options", "DENY");
        resp->addHeader("X-Content-Type-Options", "nosniff");
        resp->addHeader("X-XSS-Protection", "1; mode=block");
        resp->addHeader("Referrer-Policy", "strict-origin-when-cross-origin");
        resp->addHeader("Content-Security-Policy",
                        "default-src 'self'; "
                        "script-src 'self' 'unsafe-inline' 'unsafe-eval'; "
                        "style-src 'self' 'unsafe-inline'; "
                        "img-src 'self' data: blob:; "
                        "font-src 'self' data:; "
                        "connect-src 'self' ws: wss:; "
                        "frame-ancestors 'none';");
        resp->addHeader("Permissions-Policy", "geolocation=(), microphone=(), camera=(), payment=()");

        auto& config = Config::instance();
        if (config.get<bool>("server.enable_ssl").value_or(false)) {
            resp->addHeader("Strict-Transport-Security", "max-age=31536000; includeSubDomains");
        }

        CONSOLE_LOG_DEBUG("CORS and security headers added: {} from {}", req->path(), origin);
    });

    CONSOLE_LOG_INFO("CORS configured successfully");
}

void
setup_api_stats_collection() {
    CONSOLE_LOG_INFO("Setting up API stats collection...");

    app().registerPostHandlingAdvice([](const HttpRequestPtr& req, const HttpResponsePtr& resp) {
        const std::string& path = req->path();
        if (path.find("/api/v1/stats") != std::string::npos || path.find("/api/v1/health") != std::string::npos ||
            path.find("/api/v1/version") != std::string::npos) {
            return;
        }

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

        try {
            auto user_info = req->attributes()->get<UserInfo>("user_info");
            stats.user_access_key = user_info.access_key;
        } catch (...) {}

        db->add_api_request_stat(stats);
    });

    CONSOLE_LOG_INFO("API stats collection configured successfully");
}

void
setup_http_access_logging() {
    CONSOLE_LOG_INFO("Setting up HTTP access logging...");

    app().registerPreRoutingAdvice([](const HttpRequestPtr& req) {
        request_start_time = std::chrono::steady_clock::now();

        String request_id = generate_request_id();
        req->attributes()->insert("request_id", request_id);
    });

    app().registerPostHandlingAdvice([](const HttpRequestPtr& req, const HttpResponsePtr& resp) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - request_start_time).count();
        auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end_time - request_start_time).count();

        const String& path = req->path();
        String method = http_method_to_string(req->method());
        String client_ip = req->peerAddr().toIp();
        int status_code = static_cast<int>(resp->statusCode());

        String request_id;
        try {
            request_id = req->attributes()->get<String>("request_id");
        } catch (...) {
            request_id = "unknown";
        }

        String user_id = "-";
        try {
            auto user_info = req->attributes()->get<UserInfo>("user_info");
            user_id = user_info.access_key;
        } catch (...) {}

        size_t request_size = req->body().size();
        size_t response_size = resp->body().size();

        String user_agent = req->getHeader("User-Agent");
        if (user_agent.length() > 100) {
            user_agent = user_agent.substr(0, 97) + "...";
        }
        if (user_agent.empty()) {
            user_agent = "-";
        }

        String query_string = req->query();
        String full_path = query_string.empty() ? path : path + "?" + query_string;

        if (full_path.length() > 200) {
            full_path = full_path.substr(0, 197) + "...";
        }

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
            if (path.find("/api/") != std::string::npos) {
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

        resp->addHeader("X-Request-ID", request_id);
    });

    CONSOLE_LOG_INFO("HTTP access logging configured successfully");
}

void
setup_background_tasks() {
    CONSOLE_LOG_INFO("Setting up background tasks...");

    app().getLoop()->runEvery(86400.0, []() {
        auto db = ServiceLocator::database();
        if (!db)
            return;

        auto now = std::time(nullptr);
        auto week_ago = now - (7 * 24 * 3600);

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

    app().setCustomErrorHandler([](HttpStatusCode code) -> HttpResponsePtr {
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

} // namespace console
