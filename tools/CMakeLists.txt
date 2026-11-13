# One executable per .cpp in this directory, named after the file (tools/client.cpp -> client)
file(GLOB TOOL_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

foreach(tool_src ${TOOL_SOURCES})
    get_filename_component(tool_name ${tool_src} NAME_WE)
    add_executable(${tool_name} ${tool_src})
    target_link_libraries(${tool_name} PRIVATE exchange_lib)
endforeach()
