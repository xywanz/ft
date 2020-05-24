# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_executable(strategy-loader StrategyLoader.cpp)
target_link_libraries(strategy-loader strategy ${COMMON_LIB})

add_library(strategy STATIC Strategy.cpp)
target_link_libraries(strategy ${COMMON_LIB})
