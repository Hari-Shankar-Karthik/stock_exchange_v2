cmake_minimum_required(VERSION 3.20)
project(stock_exchange LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(EXCHANGE_BUILD_TESTS "Build unit/integration tests" ON)
option(EXCHANGE_BUILD_BENCH "Build benchmarks" OFF)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(ProjectOptions) # defines exchange_options
include(Asio) # defines asio::asio

add_subdirectory(src)
add_subdirectory(tools)

if(EXCHANGE_BUILD_TESTS)
    enable_testing() # must be called in the top-level file so `ctest` works from build/
    add_subdirectory(tests)
endif()
if(EXCHANGE_BUILD_BENCH)
    add_subdirectory(bench)
endif()
