cmake_minimum_required(VERSION 3.20)

project(SimpleProfiler)
set(LANGUAGE CXX)
set(CMAKE_CXX_STANDARD 17)

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

