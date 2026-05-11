# Business logic services

set(SERVICES_SOURCES
    AuthService.cpp
    BucketService.cpp
    EncryptionService.cpp
    InfrastructureService.cpp
    LifecycleService.cpp
    ObjectService.cpp
    PolicyEvaluator.cpp
    StatsCollector.cpp
    UserService.cpp
)

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
