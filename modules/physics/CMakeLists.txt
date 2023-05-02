set(SOURCES
        # cloth
        shading_system/cloth_compute_shading_system.cpp
        compute_shader/cloth_compute_shader.cpp
        mass_spring_cloth.cpp

        # fdtd
        fdtd2_field.cpp
        compute_shader/fdtd2_compute_shader.cpp
        shading_system/fdtd2_shading_system.cpp

        resource_manager.cpp
        )

add_library(hnll_physics STATIC ${SOURCES})

# config include directories
include($ENV{HNLL_ENGN}/cmake_utils/include.cmake)
include_common_dependencies(hnll_physics)
if (APPLE)
    include_mac_dependencies(hnll_physics)
endif(APPLE)