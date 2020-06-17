# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

aux_source_directory(common RM_COMMON_SRC)
aux_source_directory(stock RM_STOCK_SRC)
aux_source_directory(futures RM_FUTURES_SRC)
aux_source_directory(etf ETF_RM_SRC)
aux_source_directory(. RM)

add_library(risk-management STATIC
  ${RM_COMMON_SRC}
  ${RM_STOCK_SRC}
  ${RM_FUTURES_SRC}
  ${ETF_RM_SRC}
  ${RM})
target_link_libraries(risk-management common ${COMMON_LIB})
