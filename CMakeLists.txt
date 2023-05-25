cmake_minimum_required(VERSION 3.10)

project(arfhttpd)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

file (GLOB SRC
    "main.c"
)

add_executable(arfminesweeper ${SRC})

target_link_libraries(arfminesweeper Threads::Threads)
