
add_library(backtest_gateway SHARED back_test_gateway.cpp)
add_library(ft::backtest_gateway ALIAS backtest_gateway)
target_link_libraries(backtest_gateway PRIVATE ft::utils pthread)
