# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_executable(trading_server
    order_management/order_management.cpp
    risk_management/common/fund_manager.cpp
    risk_management/common/no_self_trade.cpp
    risk_management/common/position_manager.cpp
    risk_management/common/strategy_notifier.cpp
    risk_management/common/throttle_rate_limit.cpp
    risk_management/etf/arbitrage_manager.cpp
    risk_management/risk_management.cpp
    main.cpp)
target_link_libraries(trading_server ft::utils dl pthread)
