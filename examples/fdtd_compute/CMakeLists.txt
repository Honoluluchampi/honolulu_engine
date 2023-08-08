cmake_minimum_required(VERSION 3.16)
project(fdtd_compute VERSION 1.0)

# specify the c++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-g3 -O0")

# 2D
set(SOURCES
        fdtd_2d/fdtd_2d.cpp
        fdtd_2d/fdtd2_shading_system.cpp
        fdtd_2d/fdtd2_compute_shader.cpp
        fdtd_2d/fdtd2_field.cpp)
add_executable(fdtd_2d ${SOURCES})
target_include_directories(fdtd_2d PUBLIC $ENV{HNLL_ENGN}/include)
target_link_libraries(fdtd_2d PUBLIC hnll_engine)

# 3D
set(SOURCES fdtd_3d/fdtd_3d.cpp)
add_executable(fdtd_3d ${SOURCES})
target_include_directories(fdtd_3d PUBLIC
        $ENV{HNLL_ENGN}/include
        )
target_link_libraries(fdtd_3d PUBLIC hnll_engine)

# test
add_subdirectory(test)