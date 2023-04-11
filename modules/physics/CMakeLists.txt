set(SOURCES
        shading_system/cloth_compute.cpp
        )

add_library(hnll_physics STATIC ${SOURCES})

# config include directories
include($ENV{HNLL_ENGN}/cmake_utils/include.cmake)
include_common_dependencies(hnll_physics)
if (APPLE)
    include_mac_dependencies(hnll_physics)
endif(APPLE)