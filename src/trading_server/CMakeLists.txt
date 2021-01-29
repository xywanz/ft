# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

aux_source_directory(order_book ORDER_BOOK_SRC)
aux_source_directory(order_management ORDER_MANAGEMENT_SRC)

aux_source_directory(risk_management RISK_MANAGEMENT_MAIN_SRC)
aux_source_directory(risk_management/common RISK_MANAGEMENT_COMMON_SRC)
aux_source_directory(risk_management/etf RISK_MANAGEMENT_ETF_SRC)
aux_source_directory(risk_management/futures RISK_MANAGEMENT_FUTURES_SRC)
aux_source_directory(risk_management/stock RISK_MANAGEMENT_STOCK_SRC)

set(RISK_MANAGEMENT_SRC ${RISK_MANAGEMENT_MAIN_SRC}
                        ${RISK_MANAGEMENT_COMMON_SRC}
                        ${RISK_MANAGEMENT_ETF_SRC}
                        ${RISK_MANAGEMENT_FUTURES_SRC}
                        ${RISK_MANAGEMENT_STOCK_SRC})

add_executable(trading_server main.cpp
                              ${ORDER_BOOK_SRC}
                              ${ORDER_MANAGEMENT_SRC}
                              ${RISK_MANAGEMENT_SRC})
target_link_libraries(trading_server ${COMMON_LIB} utils dl pthread)
