cmake_minimum_required(VERSION 3.16)
project(tick_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PostgreSQL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_path(RABBITMQ_INCLUDE_DIR amqp.h REQUIRED)
find_library(RABBITMQ_LIBRARY rabbitmq REQUIRED)

add_executable(
    tick_server
        src/main.cpp
        src/postgres.cpp
        src/rabbitmq.cpp
)

target_compile_options(tick_server PRIVATE -Wall -Wextra -Wpedantic)

target_include_directories(
    tick_server
    PRIVATE
        include
        ${RABBITMQ_INCLUDE_DIR}
)
target_link_libraries(
    tick_server
    PRIVATE
        PostgreSQL::PostgreSQL
        nlohmann_json::nlohmann_json
        ${RABBITMQ_LIBRARY}
)