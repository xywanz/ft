# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

aux_source_directory(. RISKMGR_SRC)

add_library(RiskManagement STATIC ${RISKMGR_SRC})
target_link_libraries(RiskManagement ${COMMON_LIB})
