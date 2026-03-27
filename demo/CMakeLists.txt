# CMakeLists.txt for Mini SONiC Real Demo
cmake_minimum_required(VERSION 3.20)

project(MiniSonic_RealDemo VERSION 1.0.0 LANGUAGES CXX)

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

# Find parent project's build directory
if(NOT DEFINED MINI_SONIC_BUILD_DIR)
    set(MINI_SONIC_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../build)
endif()

# Include directories from parent project
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${MINI_SONIC_BUILD_DIR}
)

# Find and link threads
find_package(Threads REQUIRED)

# Create the real demo executable
add_executable(mini_sonic_real_demo
    real_network_demo.cpp
)

# Create the clean visual demo executable
add_executable(mini_sonic_clean_demo
    clean_visual_demo.cpp
)

# Create the animated real demo executable
add_executable(mini_sonic_animated_demo
    animated_real_demo.cpp
)

# Create silent animated demo executable
add_executable(mini_sonic_silent_demo
    silent_animated_demo.cpp
)

# Create clean animated demo executable
add_executable(mini_sonic_clean_anim_demo
    clean_animated_demo.cpp
)

# Link with Mini SONiC library
target_link_libraries(mini_sonic_real_demo
    PRIVATE
    Threads::Threads
    ${MINI_SONIC_BUILD_DIR}/mini_sonic_lib
)

# Link with Mini SONiC library for clean demo
target_link_libraries(mini_sonic_clean_demo
    PRIVATE
    Threads::Threads
    ${MINI_SONIC_BUILD_DIR}/mini_sonic_lib
)

# Link with Mini SONiC library for animated demo
target_link_libraries(mini_sonic_animated_demo
    PRIVATE
    Threads::Threads
    ${MINI_SONIC_BUILD_DIR}/mini_sonic_lib
)

# Link with Mini SONiC library for silent demo
target_link_libraries(mini_sonic_silent_demo
    PRIVATE
    Threads::Threads
    ${MINI_SONIC_BUILD_DIR}/mini_sonic_lib
)

# Link with Mini SONiC library for clean animated demo
target_link_libraries(mini_sonic_clean_anim_demo
    PRIVATE
    Threads::Threads
    ${MINI_SONIC_BUILD_DIR}/mini_sonic_lib
)

# Set target properties
set_target_properties(mini_sonic_real_demo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

set_target_properties(mini_sonic_clean_demo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

set_target_properties(mini_sonic_animated_demo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

set_target_properties(mini_sonic_silent_demo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

set_target_properties(mini_sonic_clean_anim_demo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Installation
install(TARGETS mini_sonic_real_demo
    RUNTIME DESTINATION bin
)

# Custom commands for running demo
add_custom_target(run_demo
    COMMAND mini_sonic_real_demo
    DEPENDS mini_sonic_real_demo
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running Mini SONiC Real Demo"
)

# Add demo with different configurations
add_custom_target(run_demo_basic
    COMMAND mini_sonic_real_demo --switches 3 --rate 50 --duration 15
    DEPENDS mini_sonic_real_demo
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running Mini SONiC Demo (Basic Configuration)"
)

add_custom_target(run_demo_stress
    COMMAND mini_sonic_real_demo --switches 5 --rate 200 --duration 60
    DEPENDS mini_sonic_real_demo
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running Mini SONiC Demo (Stress Test)"
)

add_custom_target(run_demo_extended
    COMMAND mini_sonic_real_demo --switches 8 --rate 100 --duration 120
    DEPENDS mini_sonic_real_demo
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running Mini SONiC Demo (Extended Configuration)"
)

# Print configuration
message(STATUS "")
message(STATUS "=== Mini SONiC Real Demo Configuration ===")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Mini SONiC Build Dir: ${MINI_SONIC_BUILD_DIR}")
message(STATUS "==========================================")
message(STATUS "")
