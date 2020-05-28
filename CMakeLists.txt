# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

cmake_minimum_required(VERSION 3.0)

project(ft)

set(CTP_PATH ${CMAKE_SOURCE_DIR}/dependencies/v6.3.15_20190220_api_tradeapi_se_linux64)
set(XTP_PATH ${CMAKE_SOURCE_DIR}/dependencies/XTP_API_1.1.19.2_20190627/bin)
set(COMMON_LIB yaml-cpp hiredis pthread dl)
set(GATEWAY_LIB thostmduserapi_se thosttraderapi_se xtptraderapi xtpquoteapi)

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
    ${CMAKE_SOURCE_DIR}/dependencies/hiredis/lib
)

set(CMAKE_CXX_STANDARD 17)
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_definitions(-DFMT_HEADER_ONLY)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

add_subdirectory(src/common)
add_subdirectory(src/gateway)
add_subdirectory(src/trading_system)
add_subdirectory(src/risk_management)
add_subdirectory(src/strategy)
add_subdirectory(src/tools)
add_subdirectory(src/test)
