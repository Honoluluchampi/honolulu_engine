cmake_minimum_required(VERSION 3.16)
project(fdtd_compute VERSION 1.0)

# specify the c++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-g3 -O0")

# 1D
set(SOURCES fdtd_1d/fdtd_1d.cpp)
# configure this variable
set(MCUT_DIR /home/honolulu/programs/mcut)
add_executable(fdtd_1d ${SOURCES})
target_include_directories(fdtd_1d PUBLIC
        $ENV{HNLL_ENGN}/include
        ${MCUT_DIR}/include)
target_link_libraries(fdtd_1d PUBLIC
        hnll_engine
        ${MCUT_DIR}/build/bin/libmcut.so
        )

# 2D
set(SOURCES
        fdtd_2d/fdtd_2d.cpp
        fdtd_2d/fdtd2_shading_system.cpp
        fdtd_2d/fdtd2_compute_shader.cpp
        fdtd_2d/fdtd2_field.cpp)
add_executable(fdtd_2d ${SOURCES})
target_include_directories(fdtd_2d PUBLIC $ENV{HNLL_ENGN}/include)
target_link_libraries(fdtd_2d PUBLIC hnll_engine)

# cylindrical
set(SOURCES
        fdtd_cylindrical/fdtd_cylindrical.cpp
        fdtd_cylindrical/fdtd_cylindrical_compute_shader.cpp
        fdtd_cylindrical/fdtd_cylindrical_field.cpp
        fdtd_cylindrical/fdtd_cylindrical_shading_system.cpp)
add_executable(fdtd_cylindrical ${SOURCES})
target_include_directories(fdtd_cylindrical PUBLIC $ENV{HNLL_ENGN}/include)
target_link_libraries(fdtd_cylindrical PUBLIC hnll_engine)