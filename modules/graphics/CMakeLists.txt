set(SOURCES
    window.cpp
    device.cpp
)

add_library(hnll_graphics STATIC ${SOURCES})

# config include directories
include($ENV{HNLL_ENGN}/cmake_utils/include.cmake)
include_common_dependencies(hnll_graphics)
if (APPLE)
    include_mac_dependencies(hnll_graphics)
endif(APPLE)