include(FetchContent)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE) # avoids clashing with the GoogleTest fetched under tests/
FetchContent_Declare(googlebenchmark
    URL https://github.com/google/benchmark/archive/refs/tags/v1.9.4.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(googlebenchmark)

# One executable per file, listed explicitly. benchmark_main supplies main().
set(BENCHES order_book_bench spsc_queue_bench slab_allocator_bench)
foreach(bench ${BENCHES})
    add_executable(${bench} ${bench}.cpp)
    # tests/ is on the include path so benchmarks can reuse support/order_generator.hpp
    target_include_directories(${bench} PRIVATE ${CMAKE_SOURCE_DIR}/tests)
    target_link_libraries(${bench} PRIVATE exchange_lib benchmark::benchmark_main)
endforeach()

# `cmake --build <dir> --target benchmarks` builds only the benchmarks, not the server and tools.
add_custom_target(benchmarks DEPENDS ${BENCHES})
