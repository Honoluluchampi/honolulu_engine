cmake_minimum_required(VERSION 3.16)
project(fdtd_wg VERSION 1.0)

# specify the c++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-g3 -O0")

set(SOURCES fdtd1d.cpp)
add_executable(fdtd_wg ${SOURCES})
target_include_directories(fdtd_wg PUBLIC
        $ENV{HNLL_ENGN}/include
        )
target_link_libraries(fdtd_wg PUBLIC hnll_engine)

project(fdtd2d VERSION 1.0)
set(SOURCES fdtd2d.cpp fdtd_horn.cpp)
add_executable(fdtd2d ${SOURCES})
target_include_directories(fdtd2d PUBLIC
        $ENV{HNLL_ENGN}/include
        )
target_link_libraries(fdtd2d PUBLIC hnll_engine)

# add test
add_subdirectory(test)