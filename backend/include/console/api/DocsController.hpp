#pragma once

#include <drogon/HttpController.h>

namespace console::api {

/**
 * @brief Documentation controller for Swagger UI
 */
class DocsController : public drogon::HttpController<DocsController> {
  public:
    METHOD_LIST_BEGIN
    // Swagger UI page
    ADD_METHOD_TO(DocsController::swagger_ui, "/docs", drogon::Get);
    // OpenAPI specification
    ADD_METHOD_TO(DocsController::swagger_spec, "/docs/swagger.json", drogon::Get);
    METHOD_LIST_END

    /**
     * @brief Serve Swagger UI HTML page
     */
    void swagger_ui(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback) const;

    /**
     * @brief Serve OpenAPI specification JSON
     */
    void swagger_spec(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback) const;
};

} // namespace console::api
