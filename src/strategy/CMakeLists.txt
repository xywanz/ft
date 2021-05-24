# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_library(strategy STATIC strategy.cpp
                            bar_generator.cpp
                            algo_order/target_pos_engine.cpp)
add_library(ft::strategy ALIAS strategy)
target_link_libraries(strategy PUBLIC yijinjing ft::utils)

add_executable(strategy_engine strategy_engine.cpp)
target_link_libraries(strategy_engine strategy dl getopt)
