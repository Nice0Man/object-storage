#include "console/api/DocsController.hpp"

#include "console/common/Logger.hpp"

#include <fstream>
#include <sstream>
#include <vector>

namespace console::api {

void
DocsController::swagger_ui(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
    // Swagger UI HTML with CDN
    const std::string html = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Object Storage Console - API Documentation</title>
    <link rel="stylesheet" type="text/css" href="https://unpkg.com/swagger-ui-dist@5.10.0/swagger-ui.css" />
    <style>
        html {
            box-sizing: border-box;
            overflow: -moz-scrollbars-vertical;
            overflow-y: scroll;
        }
        *, *:before, *:after {
            box-sizing: inherit;
        }
        body {
            margin: 0;
            padding: 0;
        }
        .topbar {
            background-color: #1f2937 !important;
        }
        .topbar-wrapper img {
            content: url('data:image/svg+xml;utf8,<svg xmlns="http://www.w3.org/2000/svg" width="100" height="30"><text x="0" y="20" font-family="Arial" font-size="18" fill="white">Object Storage</text></svg>');
        }
    </style>
</head>
<body>
    <div id="swagger-ui"></div>
    <script src="https://unpkg.com/swagger-ui-dist@5.10.0/swagger-ui-bundle.js"></script>
    <script src="https://unpkg.com/swagger-ui-dist@5.10.0/swagger-ui-standalone-preset.js"></script>
    <script>
    window.onload = function() {
        const ui = SwaggerUIBundle({
            url: "/docs/swagger.json",
            dom_id: '#swagger-ui',
            deepLinking: true,
            presets: [
                SwaggerUIBundle.presets.apis,
                SwaggerUIStandalonePreset
            ],
            plugins: [
                SwaggerUIBundle.plugins.DownloadUrl
            ],
            layout: "StandaloneLayout",
            validatorUrl: null,
            supportedSubmitMethods: ['get', 'post', 'put', 'delete', 'patch'],
            defaultModelsExpandDepth: 1,
            defaultModelExpandDepth: 1,
            docExpansion: 'list',
            filter: true,
            persistAuthorization: true,
            displayRequestDuration: true
        });

        window.ui = ui;
    };
    </script>
</body>
</html>
)";

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k200OK);
    resp->setContentTypeCode(drogon::CT_TEXT_HTML);
    resp->setBody(html);
    callback(resp);

    CONSOLE_LOG_INFO("Served Swagger UI documentation");
}

void
DocsController::swagger_spec(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
    // Try to read swagger.json from multiple possible locations
    std::vector<std::string> possible_paths = {
        "swagger.json",                          // Current directory
        "../swagger.json",                       // Parent directory
        "../../swagger.json",                    // Build directory case
        "./swagger.json",                        // Explicit current
        "/mnt/c/Cpp/object-storage/swagger.json" // Absolute path (fallback)
    };

    std::ifstream file;
    std::string found_path;

    for (const auto& path : possible_paths) {
        file.open(path);
        if (file.is_open()) {
            found_path = path;
            CONSOLE_LOG_DEBUG("Found swagger.json at: {}", path);
            break;
        }
    }

    if (!file.is_open()) {
        CONSOLE_LOG_ERROR("swagger.json not found in any of the expected locations");
        Json::Value error;
        error["error"] = "swagger.json not found";
        error["message"] = "API documentation file is missing. Please ensure swagger.json exists in the project root.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k404NotFound);
        callback(resp);
        return;
    }

    // Read file content
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    // Parse JSON to validate
    Json::Value json;
    Json::CharReaderBuilder builder;
    std::istringstream stream(content);
    std::string errors;

    if (!Json::parseFromStream(builder, stream, &json, &errors)) {
        CONSOLE_LOG_ERROR("Invalid swagger.json: {}", errors);
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value());
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    // Return JSON response
    auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
    resp->setStatusCode(drogon::k200OK);
    resp->addHeader("Access-Control-Allow-Origin", "*");
    resp->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    resp->addHeader("Access-Control-Allow-Headers",
                    "Content-Type, Authorization, X-Requested-With, Accept, Origin, "
                    "x-amz-server-side-encryption-customer-key, "
                    "x-amz-server-side-encryption-customer-algorithm, "
                    "x-amz-server-side-encryption-customer-key-md5, "
                    "x-amz-content-sha256, x-amz-date, x-amz-meta-*");
    callback(resp);

    CONSOLE_LOG_DEBUG("Served OpenAPI specification");
}

} // namespace console::api
