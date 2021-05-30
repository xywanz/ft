# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_executable(contract_collector contract_collector.cpp)
target_link_libraries(contract_collector ft::utils gateway)

add_executable(send_order send_order.cpp)
target_link_libraries(send_order yijinjing ft::utils)

# add_executable(cancel_order cancel_order.cpp)
# target_link_libraries(cancel_order yijinjing ft::utils)

add_executable(query_position query_position.cpp)
target_link_libraries(query_position ft::utils)

# add_executable(etf_tool etf_tool.cpp)
# target_include_directories(etf_tool PRIVATE "${PROJECT_SOURCE_DIR}/third_party/xtp/include")
# target_link_directories(etf_tool PRIVATE "${PROJECT_SOURCE_DIR}/third_party/xtp/lib/linux_centos7")
# target_link_libraries(etf_tool ft::utils xtptraderapi)
