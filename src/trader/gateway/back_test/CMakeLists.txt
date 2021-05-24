
add_library(backtest_gateway STATIC back_test_gateway.cpp)
add_library(ft::backtest_gateway ALIAS backtest_gateway)
target_include_directories(backtest_gateway PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(backtest_gateway PUBLIC
    ft::ft_header
    ft::base
    ft::component
    ft::utils
    pthread)
