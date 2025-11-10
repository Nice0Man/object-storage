#include "console/middleware/AuthMiddleware.hpp"
#include "console/utils/Logger.hpp"
#include <json/json.h>

namespace console::middleware {

using namespace console::utils;

AuthMiddleware::AuthMiddleware() {
    // Config will be injected or accessed via singleton
    config_ = std::make_shared<Config>();
    config_->load("config.json");
}

void AuthMiddleware::doFilter(
    const drogon::HttpRequestPtr& req,
    drogon::FilterCallback&& fcb,
    drogon::FilterChainCallback&& fccb
) {
    // Extract token from request
    auto token_opt = extract_token(req);

    if (!token_opt) {
        LOG_WARN("No authentication token provided for {}", 
                 req->getPath());
        send_unauthorized(std::move(fcb), "No token provided");
        return;
    }

    // Validate token
    auto jwt_secret = config_->get_string("auth.jwt_secret", "");
    auto user_info_opt = JWT::validate_token(*token_opt, jwt_secret);

    if (!user_info_opt) {
        LOG_WARN("Invalid or expired token for {}", req->getPath());
        send_unauthorized(std::move(fcb), "Invalid or expired token");
        return;
    }

    // Token is valid - inject user info into request attributes
    auto& user_info = *user_info_opt;
    req->attributes()->insert("user_access_key", user_info.access_key);
    req->attributes()->insert("user_is_admin", user_info.is_admin);
    req->attributes()->insert("user_account", user_info.account_name);

    // Also store full UserInfo for handlers
    req->attributes()->insert("user_info", user_info);

    LOG_DEBUG("Authenticated user: {} (admin: {})", 
              user_info.access_key, user_info.is_admin);

    // Continue to next filter/handler
    fccb();
}

Optional<String> AuthMiddleware::extract_token(
    const drogon::HttpRequestPtr& req
) const {
    // Check Authorization header
    auto auth_header = req->getHeader("Authorization");
    if (!auth_header.empty()) {
        // Expected format: "Bearer <token>"
        const String bearer_prefix = "Bearer ";
        if (auth_header.find(bearer_prefix) == 0) {
            return auth_header.substr(bearer_prefix.length());
        }
    }

    // Check query parameter as fallback (for downloads/websocket)
    auto token_param = req->getParameter("token");
    if (!token_param.empty()) {
        return String(token_param);
    }

    // Check cookie as another fallback
    auto token_cookie = req->getCookie("access_token");
    if (!token_cookie.empty()) {
        return String(token_cookie);
    }

    return std::nullopt;
}

void AuthMiddleware::send_unauthorized(
    drogon::FilterCallback&& fcb,
    const String& message
) const {
    Json::Value error;
    error["error"] = "Unauthorized";
    error["message"] = message;
    error["code"] = 401;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(drogon::k401Unauthorized);
    resp->addHeader("WWW-Authenticate", "Bearer realm=\"Console API\"");
    
    fcb(resp);
}

} // namespace console::middleware
