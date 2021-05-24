# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_subdirectory(ctp)
add_subdirectory(xtp)
add_subdirectory(back_test)

add_library(gateway STATIC gateway.cpp)
target_link_libraries(gateway PUBLIC ft_header ctp_gateway xtp_gateway backtest_gateway)
target_include_directories(gateway PUBLIC "${PROJECT_SOURCE_DIR}/src")
