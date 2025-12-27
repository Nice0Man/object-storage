#pragma once

#include "console/services/IBucketService.hpp"
#include "console/services/IUserService.hpp"

#include <drogon/HttpController.h>

#include <memory>

namespace console::api {

/**
 * @brief Dashboard API Controller
 *
 * Provides CRUD operations for configurable dashboard boards and widgets
 */
class DashboardController : public drogon::HttpController<DashboardController> {
  public:
    METHOD_LIST_BEGIN

    // Dashboard boards
    ADD_METHOD_TO(DashboardController::list_boards, "/api/v1/dashboard/boards", drogon::Get, "AuthFilter");
    ADD_METHOD_TO(DashboardController::create_board, "/api/v1/dashboard/boards", drogon::Post, "AuthFilter");
    ADD_METHOD_TO(DashboardController::get_board, "/api/v1/dashboard/boards/{id}", drogon::Get, "AuthFilter");
    ADD_METHOD_TO(DashboardController::update_board, "/api/v1/dashboard/boards/{id}", drogon::Put, "AuthFilter");
    ADD_METHOD_TO(DashboardController::delete_board, "/api/v1/dashboard/boards/{id}", drogon::Delete, "AuthFilter");

    // Dashboard widgets
    ADD_METHOD_TO(DashboardController::list_widgets,
                  "/api/v1/dashboard/boards/{board_id}/widgets",
                  drogon::Get,
                  "AuthFilter");
    ADD_METHOD_TO(DashboardController::save_widgets,
                  "/api/v1/dashboard/boards/{board_id}/widgets",
                  drogon::Put,
                  "AuthFilter");
    ADD_METHOD_TO(DashboardController::update_widget, "/api/v1/dashboard/widgets/{id}", drogon::Put, "AuthFilter");

    METHOD_LIST_END

    // Board handlers
    void list_boards(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void create_board(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void get_board(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const std::string& id);

    void update_board(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                      const std::string& id);

    void delete_board(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                      const std::string& id);

    // Widget handlers
    void list_widgets(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                      const std::string& board_id);

    void save_widgets(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                      const std::string& board_id);

    void update_widget(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       const std::string& id);

  private:
    UserInfo get_user_from_request(const drogon::HttpRequestPtr& req);
    std::string generate_uuid();
};

} // namespace console::api
