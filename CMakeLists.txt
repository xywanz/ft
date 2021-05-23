# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

cmake_minimum_required(VERSION 3.13)

project(ft VERSION 1.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--export-dynamic -Wall -Wno-unused -Wno-stringop-truncation")

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

add_subdirectory("third_party/fmt" "third_party/fmt")
add_subdirectory("third_party/hiredis" "third_party/hiredis")
add_subdirectory("third_party/spdlog" "third_party/spdlog")
add_subdirectory("third_party/yaml-cpp" "third_party/yaml-cpp")

add_library(cereal INTERFACE)
target_include_directories(cereal INTERFACE "third_party/cereal/include")

add_library(getopt INTERFACE)
target_include_directories(getopt INTERFACE "third_party/getopt")

add_library(ft_third_party INTERFACE)
add_library(ft::ft_third_party ALIAS ft_third_party)
target_link_libraries(ft_third_party INTERFACE fmt
                                               cereal
                                               spdlog
                                               getopt
                                               hiredis
                                               yaml-cpp)

add_library(ft_header INTERFACE)
add_library(ft::ft_header ALIAS ft_header)
target_include_directories(ft_header INTERFACE "include")

add_subdirectory(src)
add_subdirectory(demo)
add_subdirectory(tools)

option(FT_BUILD_TESTS "Build the tests" ON)
if(FT_BUILD_TESTS)
    enable_testing()
    include(GoogleTest)
    add_subdirectory(test)
endif()
