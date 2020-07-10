# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

cmake_minimum_required(VERSION 3.0)
project(ocg-bss)

include_directories(
        ${CMAKE_SOURCE_DIR}/src/trading_platform/gateway/ocg_bss
)

add_subdirectory(broker)
add_subdirectory(phase_1)
add_subdirectory(protocol)
add_subdirectory(test)
add_subdirectory(virtual_ocg)
