# Object Storage Console - Source Modules

# Add subdirectories for modular library structure
add_subdirectory(common)
add_subdirectory(utils)
add_subdirectory(models)
add_subdirectory(storage)
add_subdirectory(clients)
add_subdirectory(services)
add_subdirectory(api)
add_subdirectory(middleware)
add_subdirectory(filters)
add_subdirectory(websocket)

# Main console library combining all modules
add_library(console_lib INTERFACE)

target_link_libraries(console_lib INTERFACE
    console_common
    console_utils
    console_models
    console_storage
    console_clients
    console_services
    console_api
    console_middleware
    console_filters
    console_websocket
)

target_include_directories(console_lib INTERFACE
    ${CMAKE_SOURCE_DIR}/include
)
