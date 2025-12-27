#include "console/filters/CorsFilter.hpp"

#include "console/common/Logger.hpp"

using namespace drogon;

namespace console::filters {

void
CorsFilter::doFilter(const HttpRequestPtr& req, FilterCallback&& fcb, FilterChainCallback&& fccb) {
    // Get origin from request
    auto origin = req->getHeader("Origin");
    if (origin.empty()) {
        origin = "http://localhost:3000"; // Default for development
    }

    // Handle preflight OPTIONS request
    if (req->method() == drogon::HttpMethod::Options) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k204NoContent);
        resp->addHeader("Access-Control-Allow-Origin", origin);
        resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS, PATCH");
        resp->addHeader("Access-Control-Allow-Headers",
                        "Content-Type, Authorization, X-Requested-With, Accept, Origin, "
                        "x-amz-server-side-encryption-customer-key, "
                        "x-amz-server-side-encryption-customer-algorithm, "
                        "x-amz-server-side-encryption-customer-key-md5, "
                        "x-amz-content-sha256, x-amz-date, x-amz-meta-*");
        resp->addHeader("Access-Control-Allow-Credentials", "false");
        resp->addHeader("Access-Control-Max-Age", "86400");
        resp->addHeader("Vary", "Origin");

        CONSOLE_LOG_DEBUG("CORS preflight response for {} from {}", req->path(), origin);
        fcb(resp);
        return;
    }

    // For normal requests, add CORS headers in the callback
    fccb();

    // Add CORS headers to the response via callback
    auto callback_wrapper = [origin, fcb = std::move(fcb)](const HttpResponsePtr& resp) {
        resp->addHeader("Access-Control-Allow-Origin", origin);
        resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS, PATCH");
        resp->addHeader("Access-Control-Allow-Headers",
                        "Content-Type, Authorization, X-Requested-With, Accept, Origin, "
                        "x-amz-server-side-encryption-customer-key, "
                        "x-amz-server-side-encryption-customer-algorithm, "
                        "x-amz-server-side-encryption-customer-key-md5, "
                        "x-amz-content-sha256, x-amz-date, x-amz-meta-*");
        resp->addHeader("Access-Control-Allow-Credentials", "false");
        resp->addHeader("Vary", "Origin");

        CONSOLE_LOG_DEBUG("CORS headers added to response");
        fcb(resp);
    };
}

} // namespace console::filters
