set(SOURCES
    utils.cpp
    singleton.cpp
)

add_library(hnll_utils STATIC ${SOURCES})

# config include directories
include($ENV{HNLL_ENGN}/cmake_utils/include.cmake)
include_common_dependencies(hnll_utils)
if (APPLE)
    include_mac_dependencies(hnll_utils)
endif(APPLE)