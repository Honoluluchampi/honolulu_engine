set(SOURCES
        mesh_separation.cpp
        he_mesh.cpp
        bounding_volume.cpp
        intersection.cpp
        primitives.cpp
        )

add_library(hnll_geometry STATIC ${SOURCES})

# config include directories
include($ENV{HNLL_ENGN}/cmake_utils/include.cmake)
include_common_dependencies(hnll_geometry)
if (APPLE)
    include_mac_dependencies(hnll_geometry)
endif(APPLE)