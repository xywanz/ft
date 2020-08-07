# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

aux_source_directory(common RMS_COMMON_SRC)
aux_source_directory(stock RMS_STOCK_SRC)
aux_source_directory(futures RMS_FUTURES_SRC)
aux_source_directory(etf RMS_ETF_SRC)
aux_source_directory(. RMS)

add_library(rms STATIC
  ${RMS_COMMON_SRC}
  ${RMS_STOCK_SRC}
  ${RMS_FUTURES_SRC}
  ${RMS_ETF_SRC}
  ${RMS})
target_link_libraries(rms ${COMMON_LIB})
