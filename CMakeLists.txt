# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

cmake_minimum_required(VERSION 3.0)

project(ft)

include(../Common.cmake)

include_directories(
    include
    ../thirdparty/v6.3.15_20190220_api_tradeapi_se_linux64
    ../thirdparty/spdlog/include
    ../thirdparty/yaml-cpp/include
)

link_directories(
    ../thirdparty/v6.3.15_20190220_api_tradeapi_se_linux64
    ../thirdparty/spdlog
    ../thirdparty/yaml-cpp
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpic")
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")

add_subdirectory(test)

aux_source_directory(src FT_SRC_FILES)

add_library(ft STATIC ${FT_SRC_FILES})
target_link_libraries(ft thosttraderapi_se thostmduserapi_se spdlog fmt)
