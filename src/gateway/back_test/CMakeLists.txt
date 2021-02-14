aux_source_directory(. BACKTEST_GATEWAY_SRC)
add_library(backtest-gateway SHARED ${BACKTEST_GATEWAY_SRC})
target_link_libraries(backtest-gateway utils pthread)
