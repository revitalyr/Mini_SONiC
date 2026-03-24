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

# Find Boost (optional for basic functionality)
find_package(Boost 1.81 QUIET COMPONENTS system thread)
if(Boost_FOUND)
    message(STATUS "Found Boost ${Boost_VERSION}")
    message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost libraries: ${Boost_LIBRARIES}")
else()
    message(STATUS "Boost not found - using basic benchmarks only")
    set(Boost_INCLUDE_DIR "")
    set(Boost_LIBRARIES "")
endif()

# Include directories
include_directories(src)
if(Boost_FOUND)
    include_directories(src ${Boost_INCLUDE_DIRS})
endif()

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
    src/core/app_basic.cpp
    src/core/event_loop.cpp
    src/dataplane/pipeline.cpp
    src/dataplane/pipeline_thread.cpp
    src/l2/l2_service.cpp
    src/l3/l3_service.cpp
    src/l3/lpm_trie.cpp
    src/sai/simulated_sai.cpp
)

# Add Boost-dependent components only if Boost is found
if(Boost_FOUND)
    target_sources(mini_sonic PRIVATE
        src/core/app.cpp
        src/link/tcp_link_async.cpp
    )
    target_compile_definitions(mini_sonic PRIVATE BOOST_FOUND)
endif()

# Find and link threads
find_package(Threads REQUIRED)
target_link_libraries(mini_sonic
    Threads::Threads
)
if(Boost_FOUND)
    target_link_libraries(mini_sonic ${Boost_LIBRARIES})
endif()

# Create static library for core components
add_library(mini_sonic_lib STATIC
    src/core/app_basic.cpp
    src/core/event_loop.cpp
    src/dataplane/pipeline.cpp
    src/dataplane/pipeline_thread.cpp
    src/l2/l2_service.cpp
    src/l3/l3_service.cpp
    src/l3/lpm_trie.cpp
    src/sai/simulated_sai.cpp
)

# Add Boost-dependent components only if Boost is found
if(Boost_FOUND)
    target_sources(mini_sonic_lib PRIVATE
        src/core/app.cpp
        src/link/tcp_link_async.cpp
    )
    target_compile_definitions(mini_sonic_lib PRIVATE BOOST_FOUND)
endif()

target_link_libraries(mini_sonic_lib Threads::Threads)
if(Boost_FOUND)
    target_link_libraries(mini_sonic_lib ${Boost_LIBRARIES})
endif()

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
if(Boost_FOUND)
    target_link_libraries(mini_sonic_tests ${Boost_LIBRARIES})
endif()

# Benchmark executable
add_executable(mini_sonic_benchmarks
    benchmarks/main_benchmark.cpp
    benchmarks/benchmark_basic.cpp
)

# Add Boost-dependent benchmarks only if Boost is found
if(Boost_FOUND)
    target_sources(mini_sonic_benchmarks PRIVATE
        benchmarks/benchmark_packet_processing.cpp
        benchmarks/benchmark_latency.cpp
        benchmarks/benchmark_throughput.cpp
        benchmarks/benchmark_memory.cpp
        benchmarks/benchmark_multithreading.cpp
    )
endif()

target_link_libraries(mini_sonic_benchmarks
    benchmark
    benchmark_main
    mini_sonic_lib
    Threads::Threads
)
if(Boost_FOUND)
    target_link_libraries(mini_sonic_benchmarks ${Boost_LIBRARIES})
    target_compile_definitions(mini_sonic_benchmarks PRIVATE BOOST_FOUND)
endif()

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
