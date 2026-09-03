cmake_minimum_required(VERSION 3.28)
project(frag CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG main
)
FetchContent_MakeAvailable(SDL3)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "vendor/*.cpp" "vendor/*.c")

add_executable(frag ${SOURCES})

target_include_directories(frag PRIVATE vendor)

target_link_libraries(frag PRIVATE SDL3::SDL3 glm::glm)

add_executable(frag_tests
    tests/test_runner.cpp
    tests/arena_test.cpp
    tests/thing_test.cpp
    src/base/mem.cpp
    src/thing.cpp)
target_include_directories(frag_tests PRIVATE src src/base tests)
target_link_libraries(frag_tests PRIVATE SDL3::SDL3 glm::glm)
