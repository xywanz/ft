# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

aux_source_directory(Ctp CTP_SRC)
add_library(ctp-gateway STATIC ${CTP_SRC})
target_link_libraries(ctp-gateway ${COMMON_LIB} ${GATEWAY_LIB})

aux_source_directory(Xtp XTP_SRC)
add_library(xtp-gateway STATIC ${XTP_SRC})
target_link_libraries(xtp-gateway ${COMMON_LIB} ${GATEWAY_LIB})

aux_source_directory(Virtual VIRTUAL_SRC)
add_library(virtual-gateway STATIC ${VIRTUAL_SRC})
target_link_libraries(virtual-gateway ${COMMON_LIB})

add_library(gateway STATIC Gateway.cpp)
target_link_libraries(gateway ctp-gateway xtp-gateway virtual-gateway)
