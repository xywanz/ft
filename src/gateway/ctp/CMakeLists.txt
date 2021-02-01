set(CTP_LIB_PATH ${CMAKE_SOURCE_DIR}/third_party/v6.3.15_20190220_api_tradeapi_se_linux64)
include_directories(${CTP_LIB_PATH})
link_directories(${CTP_LIB_PATH})

aux_source_directory(. CTP_GATEWAY_SRC)
add_library(ctp-gateway SHARED ${CTP_GATEWAY_SRC})
target_link_libraries(ctp-gateway thostmduserapi_se thosttraderapi_se)
