# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_executable(contract-collector ContractCollector.cpp)
target_link_libraries(contract-collector common gateway ${COMMON_LIB})

add_executable(send-order SendOrder.cpp)
target_link_libraries(send-order common ${COMMON_LIB})
