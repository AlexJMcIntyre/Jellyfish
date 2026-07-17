cmake_minimum_required(VERSION 3.13)

project(JellyOSTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

include(FetchContent)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
)

FetchContent_MakeAvailable(googletest)

add_executable(JellyOSTests
        jelly_test.cpp
)

target_link_libraries(JellyOSTests
        PRIVATE
        GTest::gtest_main
)

include(GoogleTest)
add_test(
        NAME JellyOSTests
        COMMAND JellyOSTests

)   