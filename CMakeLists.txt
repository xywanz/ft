# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

cmake_minimum_required(VERSION 3.0)
project(ft)

set(CMAKE_CXX_STANDARD 17)
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-unused")

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party/spdlog/include
    ${CMAKE_SOURCE_DIR}/third_party/yaml-cpp/include
    ${CMAKE_SOURCE_DIR}/third_party/fmt/include
    ${CMAKE_SOURCE_DIR}/third_party/getopt
    ${CMAKE_SOURCE_DIR}/third_party/hiredis
    ${CMAKE_SOURCE_DIR}/third_party/cereal/include
)

link_directories(
    ${CMAKE_SOURCE_DIR}/third_party/spdlog/lib
    ${CMAKE_SOURCE_DIR}/third_party/yaml-cpp/lib
    ${CMAKE_SOURCE_DIR}/third_party/hiredis/lib
)

add_definitions(-DFMT_HEADER_ONLY)

add_subdirectory(src)
add_subdirectory(demo)

# 如果不需要编译单元测试，注释下面一行
# add_subdirectory(test)
