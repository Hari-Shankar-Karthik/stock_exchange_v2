if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_library(exchange_options INTERFACE)
target_compile_options(exchange_options INTERFACE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion
    $<$<CONFIG:Debug>:-O0 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer>
    $<$<CONFIG:Release>:-O3 -march=native -fno-exceptions>)
target_link_options(exchange_options INTERFACE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>)
