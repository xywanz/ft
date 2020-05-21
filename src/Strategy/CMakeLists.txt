# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_executable(StrategyLoader StrategyLoader.cpp)
target_link_libraries(StrategyLoader Strategy dl pthread yaml-cpp hiredis)

add_library(Strategy SHARED Strategy.cpp)
target_link_libraries(Strategy hiredis)
