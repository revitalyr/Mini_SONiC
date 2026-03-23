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
