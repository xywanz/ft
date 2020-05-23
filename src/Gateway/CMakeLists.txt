# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_library(CtpGateway STATIC
    Ctp/CtpGateway.cpp
    Ctp/CtpTradeApi.cpp
    Ctp/CtpMdApi.cpp
)
target_link_libraries(CtpGateway ${DEPENDENCIES})

add_library(XtpGateway STATIC
    Xtp/XtpGateway.cpp
    Xtp/XtpTradeApi.cpp
    Xtp/XtpMdApi.cpp
)
target_link_libraries(XtpGateway ${DEPENDENCIES})

add_library(VirtualGateway STATIC
    Virtual/VirtualGateway.cpp
    Virtual/VirtualTradeApi.cpp
)
target_link_libraries(VirtualGateway ${DEPENDENCIES} pthread)

add_library(Gateway STATIC
    Gateway.cpp
)
target_link_libraries(Gateway CtpGateway XtpGateway VirtualGateway)
