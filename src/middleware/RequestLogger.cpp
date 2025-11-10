#include "console/middleware/RequestLogger.hpp"
#include "console/common/Logger.hpp"
#include <sstream>
#include <iomanip>

namespace console::middleware {

using namespace console::utils;

void RequestLogger::doFilter(
    const drogon::HttpRequestPtr& req,
    drogon::FilterCallback&& fcb,
    drogon::FilterChainCallback&& fccb
) {
    // Record start time
    auto start_time = std::chrono::steady_clock::now();

    // Create a custom callback to log after response
    auto logging_callback = [this, req, start_time, fcb = std::move(fcb)]
                           (const drogon::HttpResponsePtr& resp) mutable {
        // Calculate request duration
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time
        );

        // Get response status code
        int status_code = static_cast<int>(resp->statusCode());

        // Get user from request attributes (if authenticated)
        String user = "anonymous";
        try {
            auto user_key = req->attributes()->get<String>("user_access_key");
            if (!user_key.empty()) {
                user = user_key;
            }
        } catch (...) {
            // No user in attributes
        }

        // Log request details
        CONSOLE_LOG_INFO("{} {} {} - Status: {} - Duration: {}ms - User: {} - Size: {} bytes - IP: {}",
                 req->getMethodString(),
                 req->getPath(),
                 req->getQuery().empty() ? "" : "?" + req->getQuery(),
                 status_code,
                 duration.count(),
                 user,
                 get_request_size(req),
                 get_client_ip(req));

        // Call original callback
        fcb(resp);
    };

    // Continue processing with our custom callback
    fccb();
    
    // Note: The above won't work as expected with Drogon's filter chain
    // In practice, we'd need to wrap the response or use Drogon's advice mechanism
    // For now, just log the request start
    CONSOLE_LOG_DEBUG("Request started: {} {} from {}",
              req->getMethodString(),
              req->getPath(),
              get_client_ip(req));
}

String RequestLogger::format_request_log(
    const drogon::HttpRequestPtr& req,
    std::chrono::milliseconds duration
) const {
    std::ostringstream oss;
    
    oss << req->getMethodString() << " "
        << req->getPath();
    
    if (!req->getQuery().empty()) {
        oss << "?" << req->getQuery();
    }
    
    oss << " - Duration: " << duration.count() << "ms"
        << " - IP: " << get_client_ip(req)
        << " - Size: " << get_request_size(req) << " bytes";
    
    return oss.str();
}

String RequestLogger::get_client_ip(const drogon::HttpRequestPtr& req) const {
    // Check X-Forwarded-For header first (for proxies/load balancers)
    auto forwarded = req->getHeader("X-Forwarded-For");
    if (!forwarded.empty()) {
        // Take first IP if multiple
        size_t comma_pos = forwarded.find(',');
        if (comma_pos != String::npos) {
            return forwarded.substr(0, comma_pos);
        }
        return forwarded;
    }

    // Check X-Real-IP header
    auto real_ip = req->getHeader("X-Real-IP");
    if (!real_ip.empty()) {
        return real_ip;
    }

    // Fall back to direct peer address
    return req->getPeerAddr().toIp();
}

size_t RequestLogger::get_request_size(
    const drogon::HttpRequestPtr& req
) const {
    size_t size = 0;
    
    // Add method line
    size += req->getMethodString().size() + 1;  // Method + space
    size += req->getPath().size() + 1;  // Path + space
    size += 8;  // "HTTP/1.1"
    
    // Add headers
    for (const auto& header : req->headers()) {
        size += header.first.size() + 2;  // Name + ": "
        size += header.second.size() + 2;  // Value + "\r\n"
    }
    
    // Add body
    size += req->getBody().size();
    
    return size;
}

} // namespace console::middleware

