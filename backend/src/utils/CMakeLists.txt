# Utility functions and helpers

set(UTILS_SOURCES
    Config.cpp
    JWT.cpp
    Logger.cpp
    PasswordHash.cpp
    StringUtils.cpp
    TokenBlacklist.cpp
)

add_library(console_utils STATIC ${UTILS_SOURCES})

target_include_directories(console_utils
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(console_utils
    PUBLIC
        Drogon::Drogon
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
)

# Add jwt-cpp if available
if(TARGET jwt-cpp::jwt-cpp)
    target_link_libraries(console_utils PUBLIC jwt-cpp::jwt-cpp)
elseif(TARGET jwt-cpp)
    target_link_libraries(console_utils PUBLIC jwt-cpp)
endif()
