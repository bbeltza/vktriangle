project(vkhelloplatform)
add_library(vkhelloplatform INTERFACE)

if (WIN32)
    target_sources(vkhelloplatform INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/win32.c")
    return()
endif()

find_package(X11)

if (X11_FOUND)
    cmake_minimum_required(VERSION 3.14)

    target_sources(vkhelloplatform INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/xlib.c")
    target_compile_definitions(vkhelloplatform INTERFACE _XLIB=1)
    target_link_libraries(vkhelloplatform INTERFACE X11::X11)
    return()
endif()

message(ERROR_FATAL "No available platform layers!")
