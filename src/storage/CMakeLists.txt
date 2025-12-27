# Storage backend implementations (RocksDB-based)

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

# RocksDB is required
if(TARGET rocksdb)
    # Fetched via FetchContent
    target_link_libraries(console_storage PUBLIC rocksdb)
    message(STATUS "RocksDB: linked (FetchContent)")
elseif(TARGET RocksDB::rocksdb)
    # Found via find_package (vcpkg)
    target_link_libraries(console_storage PUBLIC RocksDB::rocksdb)
    message(STATUS "RocksDB: linked (vcpkg)")
elseif(ROCKSDB_FOUND)
    # Found via pkg-config
    target_include_directories(console_storage PUBLIC ${ROCKSDB_INCLUDE_DIRS})
    target_link_libraries(console_storage PUBLIC ${ROCKSDB_LIBRARIES})
    message(STATUS "RocksDB: linked (pkg-config)")
else()
    message(FATAL_ERROR "RocksDB is required but not found")
endif()
