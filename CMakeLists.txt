# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

cmake_minimum_required(VERSION 3.0)

project(ft)

set(CTP_PATH ${CMAKE_SOURCE_DIR}/dependencies/v6.3.15_20190220_api_tradeapi_se_linux64)
set(XTP_PATH ${CMAKE_SOURCE_DIR}/dependencies/XTP_API_1.1.19.2_20190627/bin)
set(DEPENDENCIES spdlog hiredis thostmduserapi_se thosttraderapi_se xtptraderapi xtpquoteapi)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CTP_PATH}
    ${XTP_PATH}/include
    ${CMAKE_SOURCE_DIR}/dependencies/spdlog/include
    ${CMAKE_SOURCE_DIR}/dependencies/yaml-cpp/include
    ${CMAKE_SOURCE_DIR}/dependencies/fmt/include
    ${CMAKE_SOURCE_DIR}/dependencies/getopt
    ${CMAKE_SOURCE_DIR}/dependencies/hiredis
)

link_directories(
    ${CTP_PATH}
    ${XTP_PATH}/linux_centos7
    ${CMAKE_SOURCE_DIR}/dependencies/spdlog/lib
    ${CMAKE_SOURCE_DIR}/dependencies/yaml-cpp/lib
    ${CMAKE_SOURCE_DIR}/dependencies/fmt/lib
    ${CMAKE_SOURCE_DIR}/dependencies/hiredis/lib
)

set(CMAKE_CXX_STANDARD 17)
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")

add_subdirectory(src/Gateway)
add_subdirectory(src/TradingSystem)
add_subdirectory(src/RiskManagement)
add_subdirectory(src/Strategy)
add_subdirectory(src/Tools)
add_subdirectory(src/Test)
