
add_library(xtp_gateway SHARED
    xtp_gateway.cpp
    _xtp_quote_api.cpp
    xtp_trade_api.cpp)
add_library(ft::xtp_gateway ALIAS xtp_gateway)

target_include_directories(xtp_gateway PRIVATE
    "${PROJECT_SOURCE_DIR}/src"
    "${PROJECT_SOURCE_DIR}/third_party/xtp/include")
target_link_directories(xtp_gateway PRIVATE "${PROJECT_SOURCE_DIR}/third_party/xtp/lib")
target_link_libraries(xtp_gateway PRIVATE
    ft::ft_header
    ft::base
    ft::component
    ft::utils
    ft::ft_third_party
    xtptraderapi
    xtpquoteapi)
