include(FetchContent)
FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(unit_tests)
target_link_libraries(unit_tests PRIVATE exchange_lib GTest::gtest_main)
add_subdirectory(unit)
gtest_discover_tests(unit_tests)
