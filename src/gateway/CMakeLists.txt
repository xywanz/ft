# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

include_directories(.)

aux_source_directory(ctp CTP_SRC)
add_library(ctp-gateway STATIC ${CTP_SRC})
target_link_libraries(ctp-gateway ${COMMON_LIB} ${GATEWAY_LIB})

aux_source_directory(xtp XTP_SRC)
add_library(xtp-gateway STATIC ${XTP_SRC})
target_link_libraries(xtp-gateway ${COMMON_LIB} ${GATEWAY_LIB})

aux_source_directory(virtual VIRTUAL_SRC)
add_library(virtual-gateway STATIC ${VIRTUAL_SRC})
target_link_libraries(virtual-gateway ${COMMON_LIB} oms)

include_directories(ocg_bss)
add_subdirectory(ocg_bss)

add_library(gateway STATIC gateway.cpp)
target_link_libraries(gateway
  ctp-gateway
  xtp-gateway
  virtual-gateway
  bss-broker)
