# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_executable(strategy_loader
    StrategyLoader.cpp)
target_link_libraries(strategy_loader dl pthread cppex yaml-cpp ft)

add_library(grid_strategy SHARED
    GridStrategy.cpp)
target_link_libraries(grid_strategy ft fmt)

add_executable(contract_collector ContractCollector.cpp)
target_link_libraries(contract_collector ft cppex yaml-cpp pthread)

add_executable(data_collector DataCollector.cpp)
target_link_libraries(data_collector ft cppex yaml-cpp pthread)

add_executable(redis_cli misc.cpp)
target_link_libraries(redis_cli hiredis fmt pthread)
