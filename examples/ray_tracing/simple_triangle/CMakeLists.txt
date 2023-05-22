cmake_minimum_required(VERSION 3.16)
project(simple_triangle VERSION 1.0)

# specify the c++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-g3 -O0")

set(SOURCES main.cpp)
add_executable(simple_triangle ${SOURCES})
target_include_directories(simple_triangle PUBLIC
        $ENV{HNLL_ENGN}/include
        )
target_link_libraries(simple_triangle PUBLIC hnll_engine)