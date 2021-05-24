
add_library(xtp_gateway STATIC
    xtp_gateway.cpp
    _xtp_quote_api.cpp
    xtp_trade_api.cpp)
add_library(ft::xtp_gateway ALIAS xtp_gateway)

target_include_directories(xtp_gateway PUBLIC
    "${PROJECT_SOURCE_DIR}/src"
    "${PROJECT_SOURCE_DIR}/third_party/trade_api/xtp/include")
target_link_directories(xtp_gateway PUBLIC "${PROJECT_SOURCE_DIR}/third_party/trade_api/xtp/lib")
target_link_libraries(xtp_gateway PUBLIC
    ft::ft_header
    ft::base
    ft::component
    ft::utils
    xtptraderapi
    xtpquoteapi)
