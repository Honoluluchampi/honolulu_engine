# c++20 is available in VERSION 3.16 or later
cmake_minimum_required(VERSION 3.16)

project(hnll_engine VERSION 1.0)

# specify the c++ standard and compile frag
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# currently debug build
set(CMAKE_CXX_FLAGS "-g3 -O0")

# build engine
file(GLOB_RECURSE GAME_SOURCES modules/game/*.cpp)
add_library(hnll_engine STATIC ${GAME_SOURCES})

# add modules
add_subdirectory(modules/graphics)
add_subdirectory(modules/gui)

include($ENV{HNLL_ENGN}/cmake_utils/include.cmake)
include_common_dependencies(hnll_engine)

if (APPLE)
    include_mac_dependencies(hnll_engine)
endif()

target_link_libraries(hnll_engine PUBLIC
    hnll_graphics
    hnll_gui
    libvulkan.so.1.3.224
    glfw
)