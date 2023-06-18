# c++20 is available in VERSION 3.16 or later
cmake_minimum_required(VERSION 3.16)

project(hnll_engine VERSION 1.0)

# specify the c++ standard and compile frag
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# currently debug build
set(CMAKE_CXX_FLAGS "-g3 -O0")

# build engine -----------------------------------------------
file(GLOB_RECURSE GAME_SOURCES modules/game/*.cpp)
add_library(hnll_engine STATIC ${GAME_SOURCES})

# add modules
add_subdirectory(modules/graphics)
add_subdirectory(modules/gui)
add_subdirectory(modules/geometry)
add_subdirectory(modules/physics)
add_subdirectory(modules/audio)
add_subdirectory(modules/utils)

include($ENV{HNLL_ENGN}/cmake_utils/include.cmake)
include_common_dependencies(hnll_engine)

if (APPLE)
    include_mac_dependencies(hnll_engine)
endif()

target_link_libraries(hnll_engine PUBLIC
    hnll_gui
    hnll_physics
    hnll_audio
    hnll_graphics
    hnll_geometry
    hnll_utils
    libvulkan.so.1.3.224
    glfw
)

# build test ---------------------------------------------------
add_subdirectory(test)

# build applications -------------------------------------------
# MESH SHADER SAMPLE
set(SOURCES examples/mesh_shader/mesh_shader.cpp)
add_executable(mesh_shader ${SOURCES})
add_custom_target(setup_env COMMAND "bash" "setup-env.sh")
target_include_directories(mesh_shader PUBLIC $ENV{HNLL_ENGN}/include)
target_link_libraries(mesh_shader PUBLIC hnll_engine)
add_subdirectory(examples/fdtd_compute)
add_subdirectory(examples/horn_sound)
add_subdirectory(examples/ray_tracing)
add_subdirectory(examples/cloth_compute)