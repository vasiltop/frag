cmake_minimum_required(VERSION 3.28)
project(frag CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)

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

add_custom_command(TARGET frag POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:frag>/assets"
    COMMENT "Copy assets next to frag"
)
if(CMAKE_CONFIGURATION_TYPES)
    add_custom_command(TARGET frag POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_BINARY_DIR}/shaders"
            "$<TARGET_FILE_DIR:frag>/shaders"
        COMMENT "Copy shaders next to frag"
    )
endif()

add_executable(frag_tests
    tests/test_runner.cpp
    tests/arena_test.cpp
    tests/thing_test.cpp
    tests/map_test.cpp
    tests/physics_test.cpp
    tests/nav_test.cpp
    src/base/mem.cpp
    src/base/string.cpp
    src/map.cpp
    src/thing.cpp
    src/physics.cpp
    src/nav.cpp
    src/asset.cpp
    src/gpu.cpp)
target_include_directories(frag_tests PRIVATE src tests vendor)
target_compile_definitions(frag_tests PRIVATE FRAG_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_link_libraries(frag_tests PRIVATE SDL3::SDL3 glm::glm)
