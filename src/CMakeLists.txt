find_package(Threads REQUIRED)

# The engine "library". INTERFACE because it's header-only today; change to STATIC
# the day you add a .cpp to it, and nothing that links it needs to change.
add_library(exchange_lib INTERFACE)
target_include_directories(exchange_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(exchange_lib INTERFACE exchange_options asio::asio Threads::Threads)

add_executable(exchange server.cpp)
target_link_libraries(exchange PRIVATE exchange_lib)

add_executable(parse parser.cpp)
target_link_libraries(parse PRIVATE exchange_lib)
