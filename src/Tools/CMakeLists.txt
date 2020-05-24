# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_executable(contract-collector ContractCollector.cpp)
target_link_libraries(contract-collector gateway ${COMMON_LIB})

add_executable(send-order SendOrder.cpp)
target_link_libraries(send-order ${COMMON_LIB})
