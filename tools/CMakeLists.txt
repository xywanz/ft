# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_executable(trade_info_collector TradeInfoCollector.cpp)
target_link_libraries(trade_info_collector ft cppex pthread)

add_executable(data_collector DataCollector.cpp)
target_link_libraries(data_collector ft cppex pthread)
