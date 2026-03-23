cmake_minimum_required(VERSION 3.20)
project(mini_sonic VERSION 1.0.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Find Boost
find_package(Boost 1.81 REQUIRED COMPONENTS system thread)
if(Boost_FOUND)
    message(STATUS "Found Boost ${Boost_VERSION}")
    message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost libraries: ${Boost_LIBRARIES}")
else()
    # Fetch Boost if not found
    message(STATUS "Boost not found, fetching...")
    include(FetchContent)
    FetchContent_Declare(
        boost
        URL https://github.com/boostorg/boost/releases/download/boost-1.81.0.tar.gz
        URL_HASH SHA256=71fe75000069223be3caca798b011e7be6e0c7177c6bd3be6f3d0a649
    )
    FetchContent_GetProperties(boost)
    FetchContent_MakeAvailable(boost)
    
    set(Boost_INCLUDE_DIR ${boost_SOURCE_DIR})
    set(Boost_LIBRARIES boost_system boost_thread)
    message(STATUS "Downloaded Boost to ${boost_SOURCE_DIR}")
endif()

# Include directories
include_directories(src ${Boost_INCLUDE_DIRS})

# Enable testing
enable_testing()

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

# Fetch Google Benchmark
FetchContent_Declare(
    googlebenchmark
    URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.zip
)
FetchContent_MakeAvailable(googlebenchmark)

# Main executable
add_executable(mini_sonic
    src/main.cpp
    src/core/app.cpp
    src/core/event_loop.cpp
    src/dataplane/pipeline.cpp
    src/dataplane/pipeline_thread.cpp
    src/l2/l2_service.cpp
    src/l3/l3_service.cpp
    src/l3/lpm_trie.cpp
    src/sai/simulated_sai.cpp
    src/link/tcp_link_async.cpp
)

# Find and link threads
find_package(Threads REQUIRED)
target_link_libraries(mini_sonic Threads::Threads ${Boost_LIBRARIES})

# Create static library for core components
add_library(mini_sonic_lib STATIC
    src/core/app.cpp
    src/core/event_loop.cpp
    src/dataplane/pipeline.cpp
    src/dataplane/pipeline_thread.cpp
    src/l2/l2_service.cpp
    src/l3/l3_service.cpp
    src/l3/lpm_trie.cpp
    src/sai/simulated_sai.cpp
    src/link/tcp_link_async.cpp
)

target_link_libraries(mini_sonic_lib Threads::Threads ${Boost_LIBRARIES})

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
    ${Boost_LIBRARIES}
)

# Benchmark executable
add_executable(mini_sonic_benchmarks
    benchmarks/main_benchmark.cpp
    benchmarks/benchmark_packet_processing.cpp
    benchmarks/benchmark_latency.cpp
    benchmarks/benchmark_throughput.cpp
    benchmarks/benchmark_memory.cpp
    benchmarks/benchmark_multithreading.cpp
)

target_link_libraries(mini_sonic_benchmarks
    benchmark
    benchmark_main
    mini_sonic_lib
    Threads::Threads
    ${Boost_LIBRARIES}
)

# Add include directories for benchmarks
target_include_directories(mini_sonic_benchmarks PRIVATE src)

# Add include directories for tests
target_include_directories(mini_sonic_tests PRIVATE src)

# Add tests
add_test(NAME AllTests COMMAND mini_sonic_tests)

# Set target properties
set_target_properties(mini_sonic PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

set_target_properties(mini_sonic_lib PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

set_target_properties(mini_sonic_tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

set_target_properties(mini_sonic_benchmarks PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
