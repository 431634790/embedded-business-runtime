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

add_library(embedded_business_runtime STATIC src/core/module_mgr.c)
target_include_directories(embedded_business_runtime PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_definitions(embedded_business_runtime PUBLIC
    _POSIX_C_SOURCE=200809L
)
target_link_libraries(embedded_business_runtime PUBLIC Threads::Threads)

# main.c supplies a temporary event_loop_run until the queue loop is implemented.
add_executable(embedded-business-runtime simulator/main.c)
target_include_directories(embedded-business-runtime PRIVATE src/core)
target_link_libraries(embedded-business-runtime PRIVATE embedded_business_runtime)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(embedded_business_runtime PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(embedded-business-runtime PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(test_module_mgr tests/test_module_mgr.c)
    target_include_directories(test_module_mgr PRIVATE src/core)
    target_link_libraries(test_module_mgr PRIVATE embedded_business_runtime)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(test_module_mgr PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    # Separate processes give each case its own single startup attempt.
    foreach(case_name IN ITEMS order init_failure ready_failure empty invalid
                               capacity reentrant concurrent snapshot)
        add_test(NAME module_mgr_${case_name} COMMAND test_module_mgr ${case_name})
        set_tests_properties(module_mgr_${case_name} PROPERTIES TIMEOUT 10)
    endforeach()
endif()
