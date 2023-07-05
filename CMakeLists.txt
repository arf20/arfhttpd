cmake_minimum_required(VERSION 3.10)

project(arfhttpd)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

file (GLOB SRC
    "main.c"
    "strutils.c"
    "log.c"
    "config.c"
    "socket_util.c"
    "socket.c"
    "tls_socket.c"
    "http.c"
    "cache.c"
    "hashmap.c"
)

add_executable(arfhttpd ${SRC})

target_link_libraries(arfhttpd Threads::Threads magic tls)
