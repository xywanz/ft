# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_executable(contract-collector contract_collector.cpp)
target_link_libraries(contract-collector gateway trading_server_static)

add_executable(send-order send_order.cpp)
target_link_libraries(send-order trading_server_static)

add_executable(cancel-order cancel_order.cpp)
target_link_libraries(cancel-order trading_server_static)

add_executable(query-position query_position.cpp)
target_link_libraries(query-position trading_server_static)

add_executable(etf-tool etf_tool.cpp)
target_link_libraries(etf-tool trading_server_static)
