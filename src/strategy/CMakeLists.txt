# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_library(strategy STATIC strategy.cpp)
add_library(ft::strategy ALIAS strategy)
target_link_libraries(strategy PUBLIC ft::utils)

add_executable(strategy-loader strategy_loader.cpp)
target_link_libraries(strategy-loader strategy dl)
