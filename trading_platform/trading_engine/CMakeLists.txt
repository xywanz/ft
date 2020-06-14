# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

aux_source_directory(. TS_SRC)
add_executable(trading-engine ${TS_SRC})
target_link_libraries(trading-engine common gateway risk-management ${COMMON_LIB})
