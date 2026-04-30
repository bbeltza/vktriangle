project(vkhelloplatform)
add_library(vkhelloplatform INTERFACE)

if (WIN32)
    target_sources(vkhelloplatform INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/win32.c")
else()
    message(FATAL_ERROR "Incompatible platform buddy!!")
endif()