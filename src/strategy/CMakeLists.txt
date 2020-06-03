# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_executable(strategy-loader strategy_loader.cpp)
target_link_libraries(strategy-loader strategy ${COMMON_LIB})

add_library(strategy STATIC strategy.cpp order_manager.cpp)
target_link_libraries(strategy ${COMMON_LIB})
