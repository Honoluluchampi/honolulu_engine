set(TARGETS
    imgui_build.cpp
    renderer.cpp
)

add_library(hnll_gui STATIC ${TARGETS})

include($ENV{HNLL_ENGN}/cmake_utils/include.cmake)
include_common_dependencies(hnll_gui)
if (APPLE)
    include_mac_dependencies(hnll_gui)
endif(APPLE)