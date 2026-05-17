cmake_minimum_required(VERSION 3.16)
project(log_parser VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG   "-g -O0 -fsanitize=address,undefined")

add_executable(log_parser
    src/main.cpp
    src/thread_pool.cpp
    src/log_parser.cpp
    src/progress_bar.cpp
)

target_include_directories(log_parser PRIVATE include)

find_package(Threads REQUIRED)
target_link_libraries(log_parser PRIVATE Threads::Threads)

if(UNIX)
    target_link_libraries(log_parser PRIVATE stdc++fs)
endif()

target_compile_options(log_parser PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:
        -Wall -Wextra -Wpedantic -Wshadow
        -Wno-unused-parameter
    >
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4 /permissive-
    >
)