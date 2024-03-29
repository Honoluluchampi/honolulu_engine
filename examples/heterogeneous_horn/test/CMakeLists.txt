project(fdtd_horn_test)
set(TEST_SRC
        ../fdtd12_horn.cpp
        fdtd12_horn_test.cpp
        )

add_executable(fdtd12_horn_test ${TEST_SRC})
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

    target_include_directories(fdtd12_horn_test PUBLIC ${GOOGLE_TEST_DIRECTORY}/include)
    target_link_directories(fdtd12_horn_test PUBLIC build ${GOOGLE_TEST_DIRECTORY}/lib)
elseif (UNIX)
    target_include_directories(fdtd12_horn_test PUBLIC $ENV{HNLL_ENGN}/include)
    target_link_directories(fdtd12_horn_test PUBLIC build)
endif (APPLE)
target_link_libraries(fdtd12_horn_test hnll_engine gtest gtest_main pthread)