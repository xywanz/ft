# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_library(grid_strategy SHARED grid_strategy.cpp)
target_link_libraries(grid_strategy strategy ft::utils)

add_library(target_pos_demo SHARED target_pos_demo.cpp)
target_link_libraries(target_pos_demo strategy ft::utils)

add_library(spread_arb SHARED spread_arb.cpp)
target_link_libraries(spread_arb strategy ft::utils)

add_library(bar_demo SHARED bar_demo.cpp)
target_link_libraries(bar_demo strategy ft::utils)

# add_executable(etf-arbitrage etf_arbitrage.cpp)
# target_link_libraries(etf-arbitrage ${COMMON_LIB} utils)

# add_library(etf-monitor SHARED etf_monitor.cpp)
# target_link_libraries(etf-monitor strategy ${COMMON_LIB} utils)
