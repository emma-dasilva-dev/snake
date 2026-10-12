cmake_minimum_required(VERSION 3.20)
project(snake_desktop C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

add_executable(snake_desktop desktop/snake_raylib.c)
target_link_libraries(snake_desktop PRIVATE raylib)
if (WIN32)
    target_link_libraries(snake_desktop PRIVATE winmm)
endif()
