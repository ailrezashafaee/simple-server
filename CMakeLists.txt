cmake_minimum_required(VERSION 3.0)
project(MyCProject VERSION 1.0)

find_package(Threads REQUIRED)
find_library(SQLITE3_LIBRARY sqlite3)

add_executable(server server.c ini.c)

target_include_directories(server PRIVATE /path/to/pthread/headers /path/to/sqlite3/headers)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

target_link_libraries(server PRIVATE Threads::Threads ${SQLITE3_LIBRARY})
