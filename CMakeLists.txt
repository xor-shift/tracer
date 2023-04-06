cmake_minimum_required(VERSION 3.25)
project(tracer)

set(CMAKE_CXX_STANDARD 23)

if (NOT TARGET gtest)
    add_subdirectory(thirdparty/stuff/thirdparty/googletest ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/googletest EXCLUDE_FROM_ALL)
endif ()

if (NOT TARGET benchmark)
    add_subdirectory(thirdparty/stuff/thirdparty/benchmark ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/benchmark EXCLUDE_FROM_ALL)
endif ()

if (NOT TARGET stuff_random)
    add_subdirectory(thirdparty/stuff/libs/random ${CMAKE_CURRENT_BINARY_DIR}/libs/random)
endif ()

if (NOT TARGET stuff_ranvec)
    add_subdirectory(thirdparty/stuff/libs/ranvec ${CMAKE_CURRENT_BINARY_DIR}/libs/ranvec)
endif ()

if (NOT TARGET stuff_blas)
    add_subdirectory(thirdparty/stuff/libs/blas ${CMAKE_CURRENT_BINARY_DIR}/libs/blas)
endif ()

if (NOT TARGET stuff_qoi)
    add_subdirectory(thirdparty/stuff/libs/qoi ${CMAKE_CURRENT_BINARY_DIR}/libs/qoi)
endif ()

if (NOT TARGET stuff_thread)
    add_subdirectory(thirdparty/stuff/libs/thread ${CMAKE_CURRENT_BINARY_DIR}/libs/thread)
endif ()

if (NOT TARGET stuff_trilang)
    add_subdirectory(thirdparty/stuff/libs/trilang ${CMAKE_CURRENT_BINARY_DIR}/libs/trilang)
endif ()

set(IMGUI_SFML_FIND_SFML OFF)
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/thirdparty/imgui")
set(SPDLOG_FMT_EXTERNAL ON)

add_subdirectory(thirdparty/fmt)
add_subdirectory(thirdparty/spdlog)
add_subdirectory(thirdparty/sfml)
add_subdirectory(thirdparty/imgui-sfml)

add_executable(tracer main.cpp)
target_link_libraries(tracer
        SFML::System SFML::Window SFML::Graphics
        ImGui-SFML::ImGui-SFML
        fmt::fmt spdlog::spdlog
        stuff_core stuff_blas stuff_random stuff_ranvec stuff_qoi stuff_thread stuff_trilang)
target_include_directories(tracer PRIVATE include)

if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_compile_options(${PROJECT_NAME} PUBLIC -fsanitize=address -fsanitize=undefined)
    target_link_options(${PROJECT_NAME} PUBLIC -fsanitize=address -fsanitize=undefined)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_compile_options(${PROJECT_NAME} PUBLIC -ffast-math -march=native -mtune=native -fopenmp)
    target_link_options(${PROJECT_NAME} PUBLIC -fopenmp)
endif()
