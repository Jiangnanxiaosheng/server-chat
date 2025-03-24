cmake_minimum_required(VERSION 3.15)
project(StatusServer)

set(CMAKE_CXX_STANDARD 20)

set(SOURCES
    src/main.cpp
    src/redis_connection_pool.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE include ${CMAKE_CURRENT_SOURCE_DIR} ${GENERATED_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE common coro proto)
target_link_libraries(${PROJECT_NAME} PRIVATE redis++ hiredis /usr/lib/libmysqlclient.so)

