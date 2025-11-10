#include "console/middleware/ErrorHandler.hpp"
#include "console/common/Logger.hpp"
#include <json/json.h>

namespace console::middleware {

using namespace console::utils;
using namespace console::models;

void ErrorHandler::doFilter(
    const drogon::HttpRequestPtr& req,
    drogon::FilterCallback&& fcb,
    drogon::FilterChainCallback&& fccb
) {
    try {
        // Continue processing request
        fccb();
    } catch (const ApiException& ex) {
        // Structured API error
        CONSOLE_LOG_ERROR("API error in {}: {} - {}", 
                  req->getPath(),
                  static_cast<int>(ex.error().status()),
                  ex.error().message());
        
        auto resp = create_error_response(ex.error());
        fcb(resp);
    } catch (const std::exception& ex) {
        // Generic C++ exception
        CONSOLE_LOG_ERROR("Unhandled exception in {}: {}", 
                  req->getPath(), ex.what());
        
        auto resp = create_error_response(ex);
        fcb(resp);
    } catch (...) {
        // Unknown error
        CONSOLE_LOG_ERROR("Unknown error in {}", req->getPath());
        
        auto resp = create_internal_error_response();
        fcb(resp);
    }
}

drogon::HttpResponsePtr ErrorHandler::create_error_response(
    const ApiError& error
) const {
    auto error_json = error.to_json();
    
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error_json);
    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(
        error.status()
    ));
    
    // Add CORS headers
    resp->addHeader("Access-Control-Allow-Origin", "*");
    resp->addHeader("Access-Control-Allow-Methods", 
                    "GET, POST, PUT, DELETE, OPTIONS");
    resp->addHeader("Access-Control-Allow-Headers", 
                    "Content-Type, Authorization");
    
    return resp;
}

drogon::HttpResponsePtr ErrorHandler::create_error_response(
    const std::exception& ex
) const {
    Json::Value error;
    error["code"] = 500;
    error["error"] = "Internal Server Error";
    error["message"] = ex.what();
    
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(drogon::k500InternalServerError);
    
    // Add CORS headers
    resp->addHeader("Access-Control-Allow-Origin", "*");
    resp->addHeader("Access-Control-Allow-Methods", 
                    "GET, POST, PUT, DELETE, OPTIONS");
    resp->addHeader("Access-Control-Allow-Headers", 
                    "Content-Type, Authorization");
    
    return resp;
}

drogon::HttpResponsePtr ErrorHandler::create_internal_error_response() const {
    Json::Value error;
    error["code"] = 500;
    error["error"] = "Internal Server Error";
    error["message"] = "An unexpected error occurred";
    
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(drogon::k500InternalServerError);
    
    // Add CORS headers
    resp->addHeader("Access-Control-Allow-Origin", "*");
    resp->addHeader("Access-Control-Allow-Methods", 
                    "GET, POST, PUT, DELETE, OPTIONS");
    resp->addHeader("Access-Control-Allow-Headers", 
                    "Content-Type, Authorization");
    
    return resp;
}

} // namespace console::middleware

