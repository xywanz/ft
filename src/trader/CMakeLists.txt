# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_executable(ft_trader
    order_management/order_management.cpp
    risk_management/common/fund_manager.cpp
    risk_management/common/no_self_trade.cpp
    risk_management/common/position_manager.cpp
    risk_management/common/strategy_notifier.cpp
    risk_management/common/throttle_rate_limit.cpp
    risk_management/etf/arbitrage_manager.cpp
    risk_management/risk_management.cpp
    main.cpp)
target_link_libraries(ft_trader PRIVATE
    ft::ft_header
    ft::base
    ft::component
    ft::utils
    ft::ft_third_party
    dl
    pthread)
