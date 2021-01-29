# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_executable(contract-collector contract_collector.cpp)
target_link_libraries(contract-collector utils dl)

add_executable(send-order send_order.cpp)
target_link_libraries(send-order utils)

add_executable(cancel-order cancel_order.cpp)
target_link_libraries(cancel-order utils)

add_executable(query-position query_position.cpp)
target_link_libraries(query-position utils)

add_executable(etf-tool etf_tool.cpp)
target_link_libraries(etf-tool ${GATEWAY_LIB} utils)
