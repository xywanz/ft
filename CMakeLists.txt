# Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

cmake_minimum_required(VERSION 3.13)

project(ft VERSION 1.1.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-stringop-truncation")
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

option(FT_MEASURE_TICK_TO_TRADE "Measure tick-to-trade cost" OFF)

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

include(fmt)
include(spdlog)
include(hiredis)
include(yaml-cpp)
include(libuv)
include(nlohmann_json)

add_library(cereal INTERFACE)
target_include_directories(cereal INTERFACE "third_party/cereal/include")

add_library(ft_header INTERFACE)
add_library(ft::ft_header ALIAS ft_header)
target_include_directories(ft_header INTERFACE "include")

add_subdirectory(src)
add_subdirectory(demo)
add_subdirectory(tools)

option(FT_BUILD_BENCHMARK "Build the benchmark" ON)
if(FT_BUILD_BENCHMARK)
    include(benchmark)
    add_subdirectory(benchmark)
endif()

option(FT_BUILD_TESTS "Build the tests" ON)
if(FT_BUILD_TESTS)
    enable_testing()
    include(GoogleTest)
    add_subdirectory(test)
endif()
