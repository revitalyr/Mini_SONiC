# =============================================================================
# SONiC Mini Switch - Full Modular Build System
# =============================================================================
# Complete build with C++20 modules and C components

cmake_minimum_required(VERSION 3.28)
project(SONiC_Mini_Switch
    VERSION 1.0.0
    DESCRIPTION "SONiC Mini Switch - Full Modular System with C++20 Modules and C"
    LANGUAGES C CXX
)

# =============================================================================
# OPTIONS
# =============================================================================

option(SONIC_ENABLE_DEBUG "Enable debug features" OFF)
option(SONIC_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(SONIC_STATIC_ANALYSIS "Enable static analysis tools" OFF)
option(SONIC_USE_NINJA "Use Ninja generator if available" ON)

# Configure Ninja generator if requested and available
if(SONIC_USE_NINJA AND NOT CMAKE_GENERATOR MATCHES "Ninja")
    find_program(NINJA_EXECUTABLE ninja)
    if(NINJA_EXECUTABLE)
        message(STATUS "Ninja found at ${NINJA_EXECUTABLE}")
        message(STATUS "To use Ninja, run: cmake -G Ninja -B build -S .")
    else()
        message(WARNING "Ninja not found. Install Ninja for faster builds.")
    endif()
endif()

# =============================================================================
# COMPILER FLAGS
# =============================================================================

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    
    # Enable AddressSanitizer if requested
    if(SONIC_ENABLE_ASAN)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
        message(STATUS "AddressSanitizer enabled")
    endif()
    
    # Enable debug features if requested
    if(SONIC_ENABLE_DEBUG)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG -g3")
        message(STATUS "Debug features enabled")
    endif()
    
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /wd4251 /Zc:__cplusplus /Zc:preprocessor")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
    
    # Enable AddressSanitizer if requested (MSVC version)
    if(SONIC_ENABLE_ASAN)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")
        message(STATUS "AddressSanitizer enabled")
    endif()
    
    # Enable debug features if requested
    if(SONIC_ENABLE_DEBUG)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DDEBUG /Zi")
        message(STATUS "Debug features enabled")
    endif()
    
    # Define Windows version
    add_definitions(-D_WIN32_WINNT=0x0601)

    # Force include portability header for MSVC to handle GCC-specific attributes and Linux constants
    add_compile_options("/FI" "${CMAKE_SOURCE_DIR}/examples/mini_switch/include/cross_platform.h")
endif()

# Add FreeRTOS include directory if it exists (assuming it's in examples/mini_switch/include/FreeRTOS/Source/include)
if(EXISTS "${CMAKE_SOURCE_DIR}/examples/mini_switch/include/FreeRTOS/Source/include")
    include_directories("${CMAKE_SOURCE_DIR}/examples/mini_switch/include/FreeRTOS/Source/include")
endif()

# Static analysis tools
if(SONIC_STATIC_ANALYSIS)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fanalyzer")
        message(STATUS "Static analysis enabled")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /analyze")
        message(STATUS "Static analysis enabled")
    endif()
endif()

# =============================================================================
# FIND PACKAGES
# =============================================================================

find_package(Threads REQUIRED)

# Find Boost libraries
find_package(Boost 1.85.0 REQUIRED COMPONENTS
    system
    asio
    serialization
    filesystem
)
if(Boost_FOUND)
    message(STATUS "Boost found: ${Boost_VERSION}")
    message(STATUS "Boost libraries: ${Boost_LIBRARIES}")
    message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
    add_compile_definitions(BOOST_FOUND)
else()
    message(FATAL_ERROR "Boost 1.85.0 or higher is required")
endif()

# Find Curses/ncurses for the dashboard components
find_package(Curses QUIET)
if(CURSES_FOUND)
    message(STATUS "Curses found: ${CURSES_LIBRARIES}")
else()
    message(WARNING "Curses/ncurses not found. Dashboard features will be limited.")
endif()

# Find Celero for benchmarking - temporarily disabled due to build issues
message(STATUS "Celero temporarily disabled")
# include(FetchContent)
# FetchContent_Declare(
#     Celero
#     GIT_REPOSITORY https://github.com/DigitalInBlue/Celero.git
#     GIT_TAG        master
# )
# FetchContent_MakeAvailable(Celero)
# message(STATUS "Celero downloaded via FetchContent")
# # Create alias for Celero
# add_library(Celero::Celero ALIAS celero)

include(FetchContent)

# Find Catch2 v3 for testing
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.4.0
)

find_package(Catch2 QUIET)

if(NOT Catch2_FOUND)
    message(STATUS "Catch2 not found - will download via FetchContent")
    FetchContent_MakeAvailable(Catch2)
    message(STATUS "Catch2 downloaded via FetchContent")

    if(TARGET Catch2::Catch2 AND NOT TARGET Catch2::Catch2WithMain)
        add_library(Catch2::Catch2WithMain ALIAS Catch2::Catch2WithMain)
        message(STATUS "Created Catch2::Catch2WithMain alias for downloaded Catch2")
    endif()
endif()

# Find nlohmann/json for event serialization
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)

find_package(nlohmann_json QUIET)

if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann/json not found - will download via FetchContent")
    FetchContent_MakeAvailable(nlohmann_json)
    message(STATUS "nlohmann/json downloaded via FetchContent")
endif()

# Find Qt6 for visualizer
find_package(Qt6 QUIET COMPONENTS Core Network Widgets)
if(Qt6_FOUND)
    message(STATUS "Qt6 found - visualizer components will be built")
else()
    message(STATUS "Qt6 not found - visualizer components will not be built")
endif()

# =============================================================================
# INCLUDE DIRECTORIES
# =============================================================================

include_directories(src)

# =============================================================================
# C++20 MODULES (MiniSonic Modular System)
# =============================================================================

# Module: MiniSonic.Constants
add_library(MiniSonic.Constants STATIC)
target_sources(MiniSonic.Constants
    PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES
        src/core/constants.ixx
)
target_include_directories(MiniSonic.Constants PUBLIC src)
set_target_properties(MiniSonic.Constants PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Module: MiniSonic.Events
add_library(MiniSonic.Events STATIC)
target_sources(MiniSonic.Events
    PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES
        src/core/events.ixx
)
target_include_directories(MiniSonic.Events PUBLIC src)
target_link_libraries(MiniSonic.Events nlohmann_json::nlohmann_json)
set_target_properties(MiniSonic.Events PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Module: MiniSonic.Utils
add_library(MiniSonic.Utils STATIC)
target_sources(MiniSonic.Utils
    PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES
        src/core/utils.ixx
    PRIVATE
        src/core/utils.cpp
)
target_include_directories(MiniSonic.Utils PUBLIC src)
target_include_directories(MiniSonic.Utils PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(MiniSonic.Utils ${Boost_LIBRARIES})
set_target_properties(MiniSonic.Utils PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Module: MiniSonic.Boost.Wrappers
add_library(MiniSonic.Boost.Wrappers STATIC)
target_sources(MiniSonic.Boost.Wrappers
    PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES
        src/core/boost_wrappers.ixx
)
target_include_directories(MiniSonic.Boost.Wrappers PUBLIC src)
target_include_directories(MiniSonic.Boost.Wrappers PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(MiniSonic.Boost.Wrappers ${Boost_LIBRARIES})
set_target_properties(MiniSonic.Boost.Wrappers PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Module: MiniSonic.SAI
add_library(MiniSonic.SAI STATIC)
target_sources(MiniSonic.SAI
    PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES
        src/switching/sai.ixx
    PRIVATE
        src/switching/sai.cpp
)
target_include_directories(MiniSonic.SAI PUBLIC src)
target_link_libraries(MiniSonic.SAI MiniSonic.Utils MiniSonic.Events)
set_target_properties(MiniSonic.SAI PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Module: MiniSonic.DataPlane
add_library(MiniSonic.DataPlane STATIC)
target_sources(MiniSonic.DataPlane
    PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES
        src/switching/dataplane.ixx
    PRIVATE
        src/switching/dataplane.cpp
)
target_include_directories(MiniSonic.DataPlane PUBLIC src)
target_link_libraries(MiniSonic.DataPlane MiniSonic.Utils MiniSonic.SAI MiniSonic.Networking MiniSonic.Events)
set_target_properties(MiniSonic.DataPlane PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Module: MiniSonic.DataPlane.PacketEnhanced
add_library(MiniSonic.DataPlane.PacketEnhanced STATIC)
target_sources(MiniSonic.DataPlane.PacketEnhanced
    PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES
        src/switching/dataplane/packet_enhanced.ixx
)
target_include_directories(MiniSonic.DataPlane.PacketEnhanced PUBLIC src)
target_link_libraries(MiniSonic.DataPlane.PacketEnhanced MiniSonic.Boost.Wrappers MiniSonic.Common.Types)
set_target_properties(MiniSonic.DataPlane.PacketEnhanced PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Module: MiniSonic.L3.ArpPendingQueue
add_library(MiniSonic.L3.ArpPendingQueue STATIC)
target_sources(MiniSonic.L3.ArpPendingQueue
    PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES
        src/switching/l3/arp_pending_queue.ixx
)
target_include_directories(MiniSonic.L3.ArpPendingQueue PUBLIC src)
target_link_libraries(MiniSonic.L3.ArpPendingQueue MiniSonic.DataPlane.PacketEnhanced MiniSonic.Boost.Wrappers)
set_target_properties(MiniSonic.L3.ArpPendingQueue PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Module: MiniSonic.DataPlane.PipelineTracing
add_library(MiniSonic.DataPlane.PipelineTracing STATIC)
target_sources(MiniSonic.DataPlane.PipelineTracing
    PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES
        src/switching/dataplane/pipeline_tracing.ixx
)
target_include_directories(MiniSonic.DataPlane.PipelineTracing PUBLIC src)
target_link_libraries(MiniSonic.DataPlane.PipelineTracing MiniSonic.DataPlane.PacketEnhanced MiniSonic.Boost.Wrappers MiniSonic.Events)
set_target_properties(MiniSonic.DataPlane.PipelineTracing PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Module: MiniSonic.L2L3
add_library(MiniSonic.L2L3 STATIC)
target_sources(MiniSonic.L2L3
    PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES
        src/switching/l2l3.ixx
    PRIVATE
        src/switching/l2l3.cpp
)
target_include_directories(MiniSonic.L2L3 PUBLIC src)
target_link_libraries(MiniSonic.L2L3 MiniSonic.DataPlane MiniSonic.Utils MiniSonic.Constants MiniSonic.Events)
set_target_properties(MiniSonic.L2L3 PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Module: MiniSonic.Networking
add_library(MiniSonic.Networking STATIC)
target_sources(MiniSonic.Networking
    PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES
        src/networking/networking.ixx
    PRIVATE
        src/networking/networking.cpp
)
target_include_directories(MiniSonic.Networking PUBLIC src)
target_include_directories(MiniSonic.Networking PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(MiniSonic.Networking MiniSonic.Utils ${Boost_LIBRARIES})
set_target_properties(MiniSonic.Networking PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Module: MiniSonic.Protocol
add_library(MiniSonic.Protocol STATIC)
target_sources(MiniSonic.Protocol
    PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES
        src/protocols/protocol.ixx
    PRIVATE
        src/protocols/protocol.cpp
)
target_include_directories(MiniSonic.Protocol PUBLIC src)
target_link_libraries(MiniSonic.Protocol MiniSonic.Utils MiniSonic.Constants)
set_target_properties(MiniSonic.Protocol PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Module: MiniSonic.Serialization
add_library(MiniSonic.Serialization STATIC)
target_sources(MiniSonic.Serialization
    PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES
        src/protocols/serialization.ixx
)
target_include_directories(MiniSonic.Serialization PUBLIC src)
target_include_directories(MiniSonic.Serialization PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(MiniSonic.Serialization MiniSonic.Utils ${Boost_LIBRARIES})
set_target_properties(MiniSonic.Serialization PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Module: MiniSonic.WebSocket
add_library(MiniSonic.WebSocket STATIC)
target_sources(MiniSonic.WebSocket
    PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES
        src/protocols/websocket.ixx
    PRIVATE
        src/protocols/websocket.cpp
)
target_include_directories(MiniSonic.WebSocket PUBLIC src)
target_link_libraries(MiniSonic.WebSocket MiniSonic.Protocol MiniSonic.Utils Threads::Threads)
if(WIN32)
    target_link_libraries(MiniSonic.WebSocket ws2_32)
endif()
set_target_properties(MiniSonic.WebSocket PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Module: MiniSonic.Gossip
add_library(MiniSonic.Gossip STATIC)
target_sources(MiniSonic.Gossip
    PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES
        src/protocols/gossip.ixx
    PRIVATE
        src/protocols/gossip.cpp
)
target_include_directories(MiniSonic.Gossip PUBLIC src)
target_include_directories(MiniSonic.Gossip PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(MiniSonic.Gossip MiniSonic.Protocol MiniSonic.Utils MiniSonic.Constants Threads::Threads ${Boost_LIBRARIES})
set_target_properties(MiniSonic.Gossip PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Module: MiniSonic.App
add_library(MiniSonic.App STATIC)
target_sources(MiniSonic.App
    PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES
        src/applications/app.ixx
    PRIVATE
        src/applications/app.cpp
)
target_include_directories(MiniSonic.App PUBLIC src)
target_link_libraries(MiniSonic.App 
    MiniSonic.Networking 
    MiniSonic.DataPlane 
    MiniSonic.SAI 
    MiniSonic.L2L3 
    MiniSonic.Utils
    MiniSonic.Protocol
    MiniSonic.Serialization
)
set_target_properties(MiniSonic.App PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# =============================================================================
# C CODE (mini_switch Core)
# =============================================================================

# Include directories for C code
include_directories(
    examples/mini_switch/include
    examples/mini_switch/src
)

# C Library for mini_switch core
set(MINISWITCH_C_SOURCES
    examples/mini_switch/src/sonic_types.c
    examples/mini_switch/src/switch_core.c
    examples/mini_switch/src/switch_hal.c
    examples/mini_switch/src/port.c
    examples/mini_switch/src/vlan.c
    examples/mini_switch/src/mac_table.c
    examples/mini_switch/src/fdb_orch.c
    examples/mini_switch/src/orchagent.c
    examples/mini_switch/src/ethernet.c
    examples/mini_switch/src/arp.c
    examples/mini_switch/src/ip.c
    examples/mini_switch/src/forwarding.c
    examples/mini_switch/src/hash_table.c
    examples/mini_switch/src/list.c
    examples/mini_switch/src/memory.c
    examples/mini_switch/src/ring_buffer.c
    examples/mini_switch/src/timer.c
    examples/mini_switch/src/utils.c
    examples/mini_switch/src/logging.c
    examples/mini_switch/src/tx_simple.c
    examples/mini_switch/src/net_utils.c
)

add_library(mini_switch_core STATIC ${MINISWITCH_C_SOURCES})
if(CURSES_FOUND)
    target_sources(mini_switch_core PRIVATE 
        examples/mini_switch/src/dashboard.c 
        examples/mini_switch/src/visual.c
    )
    target_link_libraries(mini_switch_core PRIVATE ${CURSES_LIBRARIES})
    target_include_directories(mini_switch_core PRIVATE ${CURSES_INCLUDE_DIRS})
    target_compile_definitions(mini_switch_core PRIVATE HAS_CURSES)
endif()
target_include_directories(mini_switch_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/mini_switch/include
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/mini_switch/src
)
set_target_properties(mini_switch_core PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

# =============================================================================
# EXECUTABLES
# =============================================================================

# 1. C++ Modular SONiC (Main Application)
add_executable(mini_sonic_modular)
target_sources(mini_sonic_modular PRIVATE src/main.cpp)
target_link_libraries(mini_sonic_modular 
    MiniSonic.App
    MiniSonic.Networking
    MiniSonic.DataPlane
    MiniSonic.SAI
    MiniSonic.L2L3
    MiniSonic.Utils
    Threads::Threads
)
set_target_properties(mini_sonic_modular PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# 2. C-based SONiC Core
add_executable(mini_sonic_c examples/mini_switch/src/main.c)
target_link_libraries(mini_sonic_c mini_switch_core Threads::Threads)
set_target_properties(mini_sonic_c PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

# 3. C Windows Demo
if(WIN32)
    add_executable(mini_sonic_windows examples/mini_switch/src/main_windows.c)
    target_link_libraries(mini_sonic_windows mini_switch_core Threads::Threads)
    set_target_properties(mini_sonic_windows PROPERTIES
        C_STANDARD 99
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF
    )
endif()

# 4. Benchmarks (Celero only)
set(BENCHMARK_SOURCES
    tools/benchmarks/benchmark_celero_simple_working.cpp
)

add_executable(mini_sonic_benchmarks ${BENCHMARK_SOURCES})
target_link_libraries(mini_sonic_benchmarks
    Threads::Threads
)

if(TARGET Celero::Celero)
    target_link_libraries(mini_sonic_benchmarks Celero::Celero)
    message(STATUS "Linking benchmarks with Celero::Celero")
endif()

set_target_properties(mini_sonic_benchmarks PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# 5. Real-time Demo (C)
add_executable(mini_sonic_demo src/realtime_demo.c)
target_link_libraries(mini_sonic_demo Threads::Threads)
set_target_properties(mini_sonic_demo PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

# 6. Protocol Stack Tests
enable_testing()

set(PROTOCOL_TEST_SOURCES
    tests/unit/test_protocol.cpp
    tests/unit/test_serialization.cpp
    tests/unit/test_websocket.cpp
    tests/unit/test_gossip.cpp
)

add_executable(mini_sonic_protocol_tests ${PROTOCOL_TEST_SOURCES})
target_link_libraries(mini_sonic_protocol_tests
    Threads::Threads
    Catch2::Catch2WithMain
)
set_target_properties(mini_sonic_protocol_tests PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_test(NAME ProtocolTests COMMAND mini_sonic_protocol_tests)

# 7. Protocol Stack Demo
add_executable(mini_sonic_protocol_demo examples/demo/protocol_stack_demo.cpp)
target_link_libraries(mini_sonic_protocol_demo Threads::Threads)
set_target_properties(mini_sonic_protocol_demo PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# =============================================================================
# VISUALIZATION COMPONENTS
# =============================================================================

# Qt6 Visualizer (only if Qt6 is found)
if(Qt6_FOUND)
    add_executable(mini_sonic_qt_visualizer src/visualization/qt_visualizer.cpp)
    target_link_libraries(mini_sonic_qt_visualizer Qt6::Core Qt6::Network Qt6::Widgets)
    set_target_properties(mini_sonic_qt_visualizer PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
    message(STATUS "  ✓ mini_sonic_qt_visualizer - Qt6 visualizer application")
endif()

# Event Server (standalone WebSocket server for event streaming)
add_executable(mini_sonic_event_server src/visualization/event_server.cpp)
target_link_libraries(mini_sonic_event_server
    MiniSonic.Events
    nlohmann_json::nlohmann_json
    Boost::system
    Boost::asio
    Boost::filesystem
)
if(WIN32)
    target_link_libraries(mini_sonic_event_server ws2_32)
    # Enable exception handling for boost.beast
    target_compile_options(mini_sonic_event_server PRIVATE /EHsc)
else()
    target_compile_options(mini_sonic_event_server PRIVATE -fexceptions)
endif()
set_target_properties(mini_sonic_event_server PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
message(STATUS "  ✓ mini_sonic_event_server - WebSocket event streaming server")

# =============================================================================
# INFORMATION
# =============================================================================

message(STATUS "")
message(STATUS "╔════════════════════════════════════════════════════════════════════╗")
message(STATUS "║      SONiC Mini Switch - Full Modular Build System               ║")
message(STATUS "╚════════════════════════════════════════════════════════════════════╝")
message(STATUS "")
message(STATUS "Configuration:")
message(STATUS "  Version:     ${PROJECT_VERSION}")
message(STATUS "  Build Type:  ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler:    ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  C Standard:  99")
message(STATUS "")
message(STATUS "Directory Structure:")
message(STATUS "  src/core/         - Core utilities and common code")
message(STATUS "  src/networking/   - Low-level networking and transport")
message(STATUS "  src/protocols/    - Protocol-oriented layer (protocol, websocket, gossip, serialization)")
message(STATUS "  src/switching/    - Traditional SONiC switching (SAI, L2/L3, dataplane)")
message(STATUS "  src/applications/ - Application orchestration")
message(STATUS "")
message(STATUS "C++20 Modules Built:")
message(STATUS "  ✓ MiniSonic.App         - Main application orchestration")
message(STATUS "  ✓ MiniSonic.Networking  - Network communication abstraction")
message(STATUS "  ✓ MiniSonic.DataPlane   - High-performance packet processing")
message(STATUS "  ✓ MiniSonic.SAI         - Switch Abstraction Interface")
message(STATUS "  ✓ MiniSonic.L2L3        - Layer 2/3 packet processing")
message(STATUS "  ✓ MiniSonic.Utils       - Utility functions and metrics")
message(STATUS "  ✓ MiniSonic.Protocol    - Protocol handler interface (pluggable protocols)")
message(STATUS "  ✓ MiniSonic.Serialization- FlatBuffers zero-copy serialization")
message(STATUS "  ✓ MiniSonic.WebSocket   - Native C++ WebSocket gateway")
message(STATUS "  ✓ MiniSonic.Gossip      - P2P gossip protocol for distributed systems")
message(STATUS "")
message(STATUS "C Core Library:")
message(STATUS "  ✓ mini_switch_core     - C-based SONiC core")
message(STATUS "")
message(STATUS "Executables:")
message(STATUS "  → mini_sonic_modular         - C++20 Modular SONiC")
message(STATUS "  → mini_sonic_c               - C-based SONiC Core")
if(WIN32)
message(STATUS "  → mini_sonic_windows         - Windows-specific C demo")
endif()
message(STATUS "  → mini_sonic_benchmarks      - Performance benchmarks")
message(STATUS "  → mini_sonic_demo            - Real-time data flow demo")
message(STATUS "  → mini_sonic_protocol_tests  - Protocol stack unit tests")
message(STATUS "  → mini_sonic_protocol_demo   - Protocol stack demonstration")
message(STATUS "")
message(STATUS "Build Options:")
message(STATUS "  Threads:     ${Threads_FOUND}")
message(STATUS "  Celero:      ${Celero_FOUND}")
message(STATUS "  Catch2:      ${Catch2_FOUND}")
message(STATUS "  Debug:       ${SONIC_ENABLE_DEBUG}")
message(STATUS "  ASan:        ${SONIC_ENABLE_ASAN}")
message(STATUS "  Static Analysis: ${SONIC_STATIC_ANALYSIS}")
message(STATUS "")
