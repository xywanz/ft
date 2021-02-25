
add_library(backtest_gateway SHARED back_test_gateway.cpp)
add_library(ft::backtest_gateway ALIAS backtest_gateway)
target_include_directories(backtest_gateway PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(backtest_gateway PRIVATE
    ft::ft_header
    ft::base
    ft::component
    ft::utils
    ft::ft_third_party
    pthread)
