cmake_minimum_required(VERSION 3.12)

project(vkhello)
add_executable(vkhello "main.c")

add_subdirectory("platform")
add_subdirectory("graphics")
target_link_libraries(vkhello PRIVATE vkhelloplatform)
target_link_libraries(vkhello PRIVATE vkhellographics)

target_include_directories(vkhello PRIVATE include/)
if (MSVC)
    target_compile_options(vkhello PRIVATE "/WX")
else()
    target_compile_options(vkhello PRIVATE "-Werror")
endif()
