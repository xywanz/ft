
add_library(ctp_gateway SHARED
    ctp_gateway.cpp
    ctp_quote_api.cpp
    ctp_trade_api.cpp)
add_library(ft::ctp_gateway ALIAS ctp_gateway)

target_include_directories(ctp_gateway PRIVATE "${PROJECT_SOURCE_DIR}/third_party/ctp/include")
target_link_directories(ctp_gateway PRIVATE "${PROJECT_SOURCE_DIR}/third_party/ctp/lib")
target_link_libraries(ctp_gateway PRIVATE ft::utils thostmduserapi_se thosttraderapi_se pthread)
