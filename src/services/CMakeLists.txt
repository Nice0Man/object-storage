# Business logic services

file(GLOB SERVICES_SOURCES *.cpp)

add_library(console_services STATIC ${SERVICES_SOURCES})

target_include_directories(console_services
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(console_services
    PUBLIC
        console_common
        console_utils
        console_models
        console_storage
        console_clients
        Drogon::Drogon
        spdlog::spdlog
        nlohmann_json::nlohmann_json
)
