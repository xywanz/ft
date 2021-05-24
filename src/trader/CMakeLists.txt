# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_subdirectory(gateway)

add_executable(ft_trader
    oms.cpp
    risk/common/fund_manager.cpp
    risk/common/no_self_trade.cpp
    risk/common/position_manager.cpp
    risk/common/throttle_rate_limit.cpp
    risk/rms.cpp
    main.cpp)
target_link_libraries(ft_trader PRIVATE
    ft::ft_header
    ft::base
    ft::component
    ft::utils
    spdlog
    yijinjing
    gateway
    pthread)
