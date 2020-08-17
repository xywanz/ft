# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_subdirectory(oms)
add_subdirectory(rms)
add_subdirectory(order_book)

add_executable(ft-cep main.cpp)
target_link_libraries(ft-cep
  gateway
  rms
  oms
  ipc
  ${COMMON_LIB})
