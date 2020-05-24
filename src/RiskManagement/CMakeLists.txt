# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

aux_source_directory(. RISKMGR_SRC)

add_library(risk-management STATIC ${RISKMGR_SRC})
target_link_libraries(risk-management ${COMMON_LIB})
