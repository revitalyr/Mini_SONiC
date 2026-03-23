cmake_minimum_required(VERSION 3.16)
project(mini_sonic)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /O2)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

include_directories(src)

# Enable testing
enable_testing()

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(mini_sonic
    src/main.cpp
    src/core/app.cpp
    src/core/event_loop.cpp
    src/dataplane/pipeline.cpp
    src/l2/l2_service.cpp
    src/l3/l3_service.cpp
    src/l3/lpm_trie.cpp
    src/sai/simulated_sai.cpp
)

# Threads
find_package(Threads REQUIRED)
target_link_libraries(mini_sonic Threads::Threads)

# Create static library for core components
add_library(mini_sonic_lib STATIC
    src/core/app.cpp
    src/core/event_loop.cpp
    src/dataplane/pipeline.cpp
    src/l2/l2_service.cpp
    src/l3/l3_service.cpp
    src/l3/lpm_trie.cpp
    src/sai/simulated_sai.cpp
)

target_link_libraries(mini_sonic_lib Threads::Threads)

# Test executable
add_executable(mini_sonic_tests
    tests/main_test.cpp
    tests/unit/test_lpm_trie.cpp
    tests/unit/test_l2_service.cpp
    tests/unit/test_l3_service.cpp
    tests/unit/test_sai.cpp
    tests/integration/test_pipeline.cpp
    tests/integration/test_end_to_end.cpp
)

target_link_libraries(mini_sonic_tests
    gtest_main
    gmock
    mini_sonic_lib
    Threads::Threads
)

# Add include directories for tests
target_include_directories(mini_sonic_tests PRIVATE src)

# Add tests
add_test(NAME AllTests COMMAND mini_sonic_tests)
