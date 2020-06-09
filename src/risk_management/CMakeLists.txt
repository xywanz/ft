# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

aux_source_directory(. RISKMGR_SRC)
aux_source_directory(etf_arbitrage ETF_ARBITRAGE_SRC)

add_library(risk-management STATIC ${RISKMGR_SRC} ${ETF_ARBITRAGE_SRC})
target_link_libraries(risk-management common ${COMMON_LIB})
