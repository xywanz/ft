
add_library(backtest_gateway STATIC backtest_gateway.cpp
                                    fund_calculator.cpp
                                    data_feed/data_feed.cpp
                                    data_feed/csv_data_feed.cpp
                                    match_engine/match_engine.cpp
                                    match_engine/simple_match_engine.cpp
                                    match_engine/advanced_match_engine.cpp)
add_library(ft::backtest_gateway ALIAS backtest_gateway)
target_include_directories(backtest_gateway PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(backtest_gateway PUBLIC
    ft::ft_header ft::base ft::component ft::utils pthread)
