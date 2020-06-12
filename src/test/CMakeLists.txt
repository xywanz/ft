# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_library(grid-strategy SHARED grid_strategy.cpp)
target_link_libraries(grid-strategy strategy ${COMMON_LIB})

add_executable(etf-arbitrage etf_arbitrage.cpp)
target_link_libraries(etf-arbitrage ${COMMON_LIB})
