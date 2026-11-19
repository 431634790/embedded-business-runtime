cmake_minimum_required(VERSION 3.16)
project(embedded-business-runtime VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Require POSIX pthreads, rather than a native Windows thread backend.
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
if(NOT CMAKE_USE_PTHREADS_INIT)
    message(FATAL_ERROR "A POSIX pthread environment is required (e.g. Linux or WSL).")
endif()

# Stage 1 bootstrap. Add core/platform/simulator sources as implemented.
add_executable(embedded-business-runtime simulator/main.c)
target_include_directories(embedded-business-runtime PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_definitions(embedded-business-runtime PRIVATE
    _POSIX_C_SOURCE=200809L
)
target_link_libraries(embedded-business-runtime PRIVATE Threads::Threads)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(embedded-business-runtime PRIVATE -Wall -Wextra -Wpedantic)
endif()
