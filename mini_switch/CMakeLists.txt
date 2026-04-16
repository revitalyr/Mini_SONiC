# =============================================================================
# SONiC Mini Switch - Simple CMake Configuration
# =============================================================================
# This is a simplified CMakeLists.txt for the mini_switch subdirectory
# that uses Catch2 v3 instead of googletest

cmake_minimum_required(VERSION 3.16)
project(SONiC_Mini_Switch
    VERSION 1.0.0
    DESCRIPTION "SONiC Mini Switch - Software for Open Networking in Cloud"
    LANGUAGES C CXX
)

# =============================================================================
# BUILD OPTIONS
# =============================================================================

option(SONIC_BUILD_DEMO "Build demo version for web interface" OFF)
option(SONIC_BUILD_TESTS "Build test suite" ON)
option(SONIC_ENABLE_DEBUG "Enable debug features" OFF)

# =============================================================================
# COMPILER FLAGS
# =============================================================================

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
    set(CMAKE_C_FLAGS_DEBUG "/Od /Zi /DDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "/O2 /DNDEBUG")
endif()

if(SONIC_ENABLE_DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDEBUG -g3")
endif()

# =============================================================================
# FIND PACKAGES
# =============================================================================

find_package(Threads REQUIRED)

# =============================================================================
# INCLUDE DIRECTORIES
# =============================================================================

include_directories(src)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# =============================================================================
# SOURCE FILES
# =============================================================================

# Core sources
set(SONIC_CORE_SOURCES
    src/sonic_types.c
    src/utils.c
    src/logging.c
    src/memory.c
    src/hash_table.c
    src/list.c
    src/timer.c
)

# Test sources
set(SONIC_TEST_SOURCES
    tests/test_simple.c
)

# =============================================================================
# LIBRARIES
# =============================================================================

# Create static library for core functionality
add_library(sonic_core STATIC
    ${SONIC_CORE_SOURCES}
)

target_include_directories(sonic_core PUBLIC src)
target_link_libraries(sonic_core PUBLIC Threads::Threads)

# Ensure DataPlane dependencies are correctly resolved for module compilation
target_link_libraries(MiniSonic.DataPlane 
    PUBLIC 
        MiniSonic.Networking
        MiniSonic.Utils
)

# Ensure the DataPlane module is built before L2L3
target_link_libraries(MiniSonic.L2L3 
    PUBLIC 
        MiniSonic.DataPlane
        MiniSonic.SAI
        MiniSonic.Utils
)

# =============================================================================
# EXECUTABLES
# =============================================================================

# Test executable
if(SONIC_BUILD_TESTS)
    enable_testing()
    
    add_executable(test_runner
        ${SONIC_TEST_SOURCES}
    )
    
    target_link_libraries(test_runner
        PRIVATE
            sonic_core
    )
    
    # Add tests
    add_test(NAME AllTests COMMAND test_runner)
    
    # Custom test target
    add_custom_target(run_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --verbose
        DEPENDS test_runner
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()

# =============================================================================
# INSTALLATION
# =============================================================================

# Install targets
install(TARGETS sonic_core
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

if(SONIC_BUILD_TESTS)
    install(TARGETS test_runner
        RUNTIME DESTINATION bin
    )
endif()

# =============================================================================
# INFORMATION
# =============================================================================

message(STATUS "")
message(STATUS "SONiC Mini Switch Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler: ${CMAKE_C_COMPILER_ID}")
message(STATUS "")
message(STATUS "Build Options:")
message(STATUS "  SONIC_BUILD_TESTS: ${SONIC_BUILD_TESTS}")
message(STATUS "  SONIC_ENABLE_DEBUG: ${SONIC_ENABLE_DEBUG}")
message(STATUS "")
message(STATUS "Optional Features:")
message(STATUS "  Catch2: ${Catch2_FOUND}")
message(STATUS "")
