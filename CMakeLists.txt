cmake_minimum_required(VERSION 3.20)

project(Timer)
set(LANGUAGE CXX)
set(CMAKE_CXX_STANDARD 17)

enable_testing()

include(FetchContent)

# main
set(SRC_FILES src/main.cpp)
add_executable(${PROJECT_NAME} ${SRC_FILES})


# tests
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest
    GIT_TAG v1.17.0
)
FetchContent_MakeAvailable(googletest)

set(TEST_FILES test/timing.cpp)

add_executable(${PROJECT_NAME}_Test ${TEST_FILES})

target_link_libraries(${PROJECT_NAME}_Test PRIVATE GTest::gtest_main)

target_include_directories(${PROJECT_NAME}_Test PUBLIC .)

add_test(NAME ${PROJECT_NAME}_Test COMMAND ${PROJECT_NAME}_Test)

if(MSVC)
    target_compile_options(${PROJECT_NAME}_Test PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME}_Test PRIVATE -Wall -Wextra -Wpedantic)
endif()
