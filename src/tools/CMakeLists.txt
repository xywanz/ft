# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_executable(contract-collector contract_collector.cpp)
target_link_libraries(contract-collector common gateway ${COMMON_LIB})

add_executable(send-order send_order.cpp)
target_link_libraries(send-order common ${COMMON_LIB})

add_executable(cancel-order cancel_order.cpp)
target_link_libraries(cancel-order common ${COMMON_LIB})

add_executable(query-position query_position.cpp)
target_link_libraries(query-position common ${COMMON_LIB})

add_executable(etf-tool etf_tool.cpp)
target_link_libraries(etf-tool common ${COMMON_LIB} ${GATEWAY_LIB})
