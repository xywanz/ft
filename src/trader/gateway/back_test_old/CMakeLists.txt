
add_library(backtest_old_gateway STATIC back_test_gateway.cpp)
add_library(ft::backtest_old_gateway ALIAS backtest_old_gateway)
target_include_directories(backtest_old_gateway PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(backtest_old_gateway PUBLIC
    ft::ft_header
    ft::base
    ft::component
    ft::utils
    pthread)
