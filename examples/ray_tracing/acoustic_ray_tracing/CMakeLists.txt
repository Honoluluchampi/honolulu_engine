cmake_minimum_required(VERSION 3.16)
project(acoustic_ray_tracing VERSION 1.0)

# specify the c++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-g3 -O0")

set(SOURCES main.cpp)
add_executable(acoustic_ray_tracing ${SOURCES})
target_include_directories(acoustic_ray_tracing PUBLIC
        $ENV{HNLL_ENGN}/include
        )
target_link_libraries(acoustic_ray_tracing PUBLIC hnll_engine)