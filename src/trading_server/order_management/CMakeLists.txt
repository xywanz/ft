# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

aux_source_directory(. OMS_SRC)
add_library(oms STATIC ${OMS_SRC})
target_link_libraries(oms
  gateway
  rms
  ipc
  ${COMMON_LIB})
