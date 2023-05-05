# build test
cmake_minimum_required(VERSION 3.16)
project(hnll_test)
file(GLOB_RECURSE TEST_SRC
        audio/engine_test.cpp audio/audio_data_test.cpp
        geometry/bounding_volume_ctor.cpp geometry/half_edge_test.cpp geometry/mesh_separation_test.cpp
        geometry/intersection_test.cpp
        geometry/perspective_frustum_test.cpp
        )

add_definitions(-std=c++2a)

add_executable(hnll_test ${TEST_SRC})
enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-g3 -O0")
# for OS X
if (APPLE)
    # search brew's root
    execute_process(
            COMMAND brew --prefix
            RESULT_VARIABLE BREW
            OUTPUT_VARIABLE BREW_PREFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
            COMMAND ls ${BREW_PREFIX}/Cellar/googletest
            RESULT_VARIABLE GOOGLE_TEST
            OUTPUT_VARIABLE GOOGLE_TEST_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(GOOGLE_TEST_DIRECTORY ${BREW_PREFIX}/Cellar/googletest/${GOOGLE_TEST_VERSION})
endif (APPLE)

if (APPLE)
    target_include_directories(hnll_test PUBLIC
            # googletest
            ${GOOGLE_TEST_DIRECTORY}/include)
    target_link_directories(hnll_test PUBLIC build
            # googletest
            ${GOOGLE_TEST_DIRECTORY}/lib)
elseif (UNIX)
    target_include_directories(hnll_test PUBLIC $ENV{HNLL_ENGN}/include)
    target_link_directories(hnll_test PUBLIC build)
endif (APPLE)
add_subdirectory($ENV{HNLL_ENGN}/ $ENV{HNLL_ENGN}/build)
target_link_libraries(hnll_test hnll_engine gtest gtest_main pthread)