# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_executable(ContractCollector ContractCollector.cpp)
target_link_libraries(ContractCollector Gateway ${COMMON_LIB})

add_executable(SendOrder SendOrder.cpp)
target_link_libraries(SendOrder ${COMMON_LIB})
