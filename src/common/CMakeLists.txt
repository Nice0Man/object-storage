# Common utilities and base classes

file(GLOB COMMON_SOURCES *.cpp)

add_library(console_common STATIC ${COMMON_SOURCES})

target_include_directories(console_common
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(console_common
    PUBLIC
        Drogon::Drogon
        spdlog::spdlog
)

# Add jwt-cpp if available
if(TARGET jwt-cpp::jwt-cpp)
    target_link_libraries(console_common PUBLIC jwt-cpp::jwt-cpp)
elseif(TARGET jwt-cpp)
    target_link_libraries(console_common PUBLIC jwt-cpp)
endif()
