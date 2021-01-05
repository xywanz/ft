# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_subdirectory(order_management)
add_subdirectory(risk_management)
add_subdirectory(order_book)

add_executable(trading_server main.cpp)
target_link_libraries(trading_server gateway rms oms ipc ${COMMON_LIB})
