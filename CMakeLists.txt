# c++20 is available in VERSION 3.16 or later
cmake_minimum_required(VERSION 3.16)

project(hnll_engine VERSION 1.0)

# specify the c++ standard and compile frag
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# currently debug build
set(CMAKE_CXX_FLAGS "-g3 -O0")

# build engine -----------------------------------------------
file(GLOB_RECURSE GAME_SOURCES modules/game/*.cpp)
add_library(hnll_engine STATIC ${GAME_SOURCES})

# add modules
add_subdirectory(modules/graphics)
add_subdirectory(modules/gui)
add_subdirectory(modules/geometry)
add_subdirectory(modules/physics)
add_subdirectory(modules/audio)
add_subdirectory(modules/utils)

include($ENV{HNLL_ENGN}/cmake_utils/include.cmake)
include_common_dependencies(hnll_engine)

if (APPLE)
    include_mac_dependencies(hnll_engine)
endif()

target_link_libraries(hnll_engine PUBLIC
    hnll_gui
    hnll_geometry
    hnll_physics
    hnll_audio
    hnll_utils
    hnll_graphics
    libvulkan.so.1.3.224
    glfw
)

# build test ---------------------------------------------------
project(hnll_test)
set(TEST_SRC
        test/geometry/half_edge_test.cpp
        )

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

    target_include_directories(hnll_test PUBLIC ${GOOGLE_TEST_DIRECTORY}/include)
    target_link_directories(hnll_test PUBLIC build ${GOOGLE_TEST_DIRECTORY}/lib)
elseif (UNIX)
    target_include_directories(hnll_test PUBLIC $ENV{HNLL_ENGN}/include)
    target_link_directories(hnll_test PUBLIC build)
endif (APPLE)
target_link_libraries(hnll_test hnll_engine gtest gtest_main pthread)

# build applications -------------------------------------------