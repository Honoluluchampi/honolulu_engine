set(SOURCES
    window.cpp
    device.cpp
    renderer.cpp
    $ENV{HNLL_ENGN}/submodules/extensions/extensions_vk.cpp
)

add_library(hnll_graphics STATIC ${SOURCES})

# config include directories
include($ENV{HNLL_ENGN}/cmake_utils/include.cmake)
include_common_dependencies(hnll_graphics)
if (APPLE)
    include_mac_dependencies(hnll_graphics)
endif(APPLE)