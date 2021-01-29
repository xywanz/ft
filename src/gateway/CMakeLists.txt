# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

include_directories(.)

aux_source_directory(ctp CTP_SRC)
add_library(ctp-gateway SHARED ${CTP_SRC})
target_link_libraries(ctp-gateway ${COMMON_LIB} ${GATEWAY_LIB})

aux_source_directory(xtp XTP_SRC)
add_library(xtp-gateway SHARED ${XTP_SRC})
target_link_libraries(xtp-gateway ${COMMON_LIB} ${GATEWAY_LIB})

# aux_source_directory(virtual VIRTUAL_SRC)
# add_library(virtual-gateway SHARED ${VIRTUAL_SRC})
# target_link_libraries(virtual-gateway ${COMMON_LIB} ${GATEWAY_LIB})

include_directories(ocg_bss)
add_subdirectory(ocg_bss)
