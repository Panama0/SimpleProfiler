cmake_minimum_required(VERSION 3.20)

project(Timer)
set(LANGUAGE CXX)
set(CMAKE_CXX_STANDARD 17)

enable_testing()

include(FetchContent)

# sfml
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.1
)
# option(SFML_BUILD_AUDIO "Build audio" OFF)
# option(SFML_BUILD_NETWORK "Build network" OFF)
FetchContent_MakeAvailable(SFML)

# main
set(SRC_FILES src/main.cpp)
add_executable(${PROJECT_NAME} ${SRC_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE SFML::Graphics)


# tests
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest
    GIT_TAG v1.17.0
)
FetchContent_MakeAvailable(googletest)


set(TEST_FILES test/timing.cpp)

add_executable(${PROJECT_NAME}_Test ${TEST_FILES})

target_link_libraries(${PROJECT_NAME}_Test PRIVATE GTest::gtest_main SFML::Graphics)

target_include_directories(${PROJECT_NAME}_Test PUBLIC .)

add_test(NAME ${PROJECT_NAME}_Test COMMAND ${PROJECT_NAME}_Test)

