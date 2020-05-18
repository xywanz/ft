# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_executable(ContractCollector ContractCollector.cpp)
target_link_libraries(ContractCollector yaml-cpp Gateway)

add_executable(SendOrder SendOrder.cpp)
target_link_libraries(SendOrder hiredis fmt)
