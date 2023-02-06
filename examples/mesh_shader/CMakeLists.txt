# MESH SHADER SAMPLE
cmake_minimum_required(VERSION 3.16)
project(introduction VERSION 1.0)

# specify the c++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-g3 -O0")

set(SOURCES mesh_shader.cpp)
add_executable(mesh_shader ${SOURCES})
target_include_directories(mesh_shader PUBLIC
    $ENV{HNLL_ENGN}/include
)
add_subdirectory($ENV{HNLL_ENGN}/ $ENV{HNLL_ENGN}/build)
target_link_libraries(mesh_shader PUBLIC hnll_engine)