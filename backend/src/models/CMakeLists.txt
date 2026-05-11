# Data models and structures

set(MODELS_SOURCES
    Bucket.cpp
    Error.cpp
    Group.cpp
    Object.cpp
    Policy.cpp
    ServerInfo.cpp
    User.cpp
)

add_library(console_models STATIC ${MODELS_SOURCES})

target_include_directories(console_models
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(console_models
    PUBLIC
        nlohmann_json::nlohmann_json
        Drogon::Drogon
)
