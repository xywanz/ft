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
)
target_link_libraries(XtpGateway ${DEPENDENCIES})

add_library(Gateway STATIC
    Gateway.cpp
)
target_link_libraries(Gateway CtpGateway XtpGateway)
