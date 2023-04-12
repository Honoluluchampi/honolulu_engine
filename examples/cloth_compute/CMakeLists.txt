# MESH SHADER SAMPLE
cmake_minimum_required(VERSION 3.16)
project(cloth_compute VERSION 1.0)

# specify the c++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-g3 -O0")

set(SOURCES cloth_simulation.cpp)
add_executable(cloth_compute ${SOURCES})
add_custom_target(setup_env COMMAND "bash" "setup-env.sh")
target_include_directories(cloth_compute PUBLIC
        $ENV{HNLL_ENGN}/include
        )
add_subdirectory($ENV{HNLL_ENGN}/ $ENV{HNLL_ENGN}/build)
target_link_libraries(cloth_compute PUBLIC hnll_engine)