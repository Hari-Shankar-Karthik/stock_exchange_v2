cmake_minimum_required(VERSION 3.20)
project(stock_exchange LANGUAGES CXX)

# C++20, no compiler-specific extensions (-std=c++20, not -std=gnu++20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Write compile_commands.json for clangd / VS Code IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to Release if no build type was given
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Standalone (header-only) Asio. Override with -DASIO_INCLUDE_DIR=/some/path
set(ASIO_INCLUDE_DIR "$ENV{HOME}/asio-1.38.2/include" CACHE PATH "Asio include dir")
if(NOT EXISTS "${ASIO_INCLUDE_DIR}/asio.hpp")
    message(FATAL_ERROR "asio.hpp not found in ${ASIO_INCLUDE_DIR}")
endif()

find_package(Threads REQUIRED)

# Shared settings: every executable links this to get the same flags, includes and defines
add_library(exchange_common INTERFACE)

# Lets you write #include "core/types.hpp" (from src/) or "src/core/types.hpp" (from the root)
target_include_directories(exchange_common INTERFACE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR})
# SYSTEM suppresses warnings coming from inside Asio's headers
target_include_directories(exchange_common SYSTEM INTERFACE ${ASIO_INCLUDE_DIR})
target_compile_definitions(exchange_common INTERFACE
    ASIO_STANDALONE
    $<$<CONFIG:Release>:ASIO_NO_EXCEPTIONS>   # Asio needs this when building with -fno-exceptions
)
target_link_libraries(exchange_common INTERFACE Threads::Threads)

target_compile_options(exchange_common INTERFACE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion
    $<$<CONFIG:Debug>:-O0 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer>
    # Release: no throw/try/catch, no landing pads or LSDA tables. Debug keeps exceptions so they're visible.
    $<$<CONFIG:Release>:-O3 -march=native -fno-exceptions>
)

target_link_options(exchange_common INTERFACE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)

# Server
add_executable(exchange src/server.cpp)
# add_executable(exchange src/demo.cpp)
target_link_libraries(exchange PRIVATE exchange_common)

# Tests: every .cpp under test/ becomes its own executable, named after the file (test/client.cpp -> client)
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/test/*.cpp)
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE exchange_common)
endforeach()
