# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_executable(StrategyLoader StrategyLoader.cpp)
target_link_libraries(StrategyLoader Strategy ${COMMON_LIB})

add_library(Strategy STATIC Strategy.cpp)
target_link_libraries(Strategy ${COMMON_LIB})
