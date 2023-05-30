cmake_minimum_required(VERSION 3.16)

add_definitions(-std=c++2a)

# build honolulu_audio_engine
add_library(hnll_audio STATIC engine.cpp utils.cpp)
include($ENV{HNLL_ENGN}/cmake_utils/include.cmake)
include_common_dependencies(hnll_audio)
# for OS X
if (APPLE)
    # target link directories is executed in this function
    include_mac_dependencies(hnll_audio)
endif (APPLE)
target_link_libraries(hnll_audio PUBLIC openal)

# build example
project(sample)
set(SOURCE_FILES sample.cpp)
add_executable(sample ${SOURCE_FILES})
include_common_dependencies(sample)
if (APPLE)
    include_mac_dependencies(sample)
endif (APPLE)
target_link_libraries(sample hnll_audio)