# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_library(ctp-gateway STATIC
    Ctp/CtpGateway.cpp
    Ctp/CtpTradeApi.cpp
    Ctp/CtpMdApi.cpp
)
target_link_libraries(ctp-gateway ${COMMON_LIB} ${GATEWAY_LIB})

add_library(xtp-gateway STATIC
    Xtp/XtpGateway.cpp
    Xtp/XtpTradeApi.cpp
    Xtp/XtpMdApi.cpp
)
target_link_libraries(xtp-gateway ${COMMON_LIB} ${GATEWAY_LIB})

add_library(virtual-gateway STATIC
    Virtual/VirtualGateway.cpp
    Virtual/VirtualTradeApi.cpp
)
target_link_libraries(virtual-gateway ${COMMON_LIB})

add_library(gateway STATIC Gateway.cpp)
target_link_libraries(gateway ctp-gateway xtp-gateway virtual-gateway)
