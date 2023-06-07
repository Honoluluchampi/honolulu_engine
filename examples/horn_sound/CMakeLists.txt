cmake_minimum_required(VERSION 3.16)
project(horn_sound VERSION 1.0)

# specify the c++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-g3 -O0")

set(SOURCES main.cpp)
add_executable(horn_sound ${SOURCES})
target_include_directories(horn_sound PUBLIC
        $ENV{HNLL_ENGN}/include
        )
target_link_libraries(horn_sound PUBLIC hnll_engine)

set(SOURCES wav_loader.cpp)
add_executable(wav_loader ${SOURCES})
target_include_directories(wav_loader PUBLIC
        $ENV{HNLL_ENGN}/include
        )
target_link_libraries(wav_loader PUBLIC hnll_engine)