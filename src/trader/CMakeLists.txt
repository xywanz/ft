# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_subdirectory(gateway)

add_executable(ft_trader
    oms.cpp
    risk/common/fund_risk.cpp
    risk/common/self_trade_risk.cpp
    risk/common/position_risk.cpp
    risk/common/throttle_rate_risk.cpp
    risk/risk_rule.cpp
    risk/rms.cpp
    main.cpp)
target_link_libraries(ft_trader PRIVATE
    ft::ft_header ft::base ft::component ft::utils spdlog fmt
    yijinjing gateway pthread)
if(FT_MEASURE_TICK_TO_TRADE)
    target_compile_definitions(ft_trader PRIVATE FT_MEASURE_TICK_TO_TRADE)
endif(FT_MEASURE_TICK_TO_TRADE)
