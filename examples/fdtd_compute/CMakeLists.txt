cmake_minimum_required(VERSION 3.16)
project(fdtd_compute VERSION 1.0)

# specify the c++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-g3 -O0")

set(SOURCES serial_test.cpp)
add_executable(fdtd_compute ${SOURCES})
#target_include_directories(fdtd_compute PUBLIC
#        $ENV{HNLL_ENGN}/include
#        )
#target_link_libraries(fdtd_compute PUBLIC hnll_engine)