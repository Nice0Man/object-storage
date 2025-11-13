# Storage backend implementations

file(GLOB STORAGE_SOURCES *.cpp)

add_library(console_storage STATIC ${STORAGE_SOURCES})

target_include_directories(console_storage
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(console_storage
    PUBLIC
        console_common
        console_utils
        Drogon::Drogon
        spdlog::spdlog
)
