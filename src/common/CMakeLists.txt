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

# RocksDB headers are needed for DatabaseManager.hpp
if(TARGET rocksdb)
    target_include_directories(console_common PUBLIC
        ${CMAKE_BINARY_DIR}/_deps/rocksdb-src/include
    )
elseif(TARGET RocksDB::rocksdb)
    # vcpkg provides includes automatically
elseif(ROCKSDB_FOUND)
    target_include_directories(console_common PUBLIC ${ROCKSDB_INCLUDE_DIRS})
endif()

# Add jwt-cpp if available
if(TARGET jwt-cpp::jwt-cpp)
    target_link_libraries(console_common PUBLIC jwt-cpp::jwt-cpp)
elseif(TARGET jwt-cpp)
    target_link_libraries(console_common PUBLIC jwt-cpp)
endif()
