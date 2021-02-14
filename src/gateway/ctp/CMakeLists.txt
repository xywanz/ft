set(CTP_LIB_PATH ${CMAKE_SOURCE_DIR}/third_party/ctp)
include_directories(${CTP_LIB_PATH}/include)
link_directories(${CTP_LIB_PATH}/lib)

aux_source_directory(. CTP_GATEWAY_SRC)
add_library(ctp-gateway SHARED ${CTP_GATEWAY_SRC})
target_link_libraries(ctp-gateway utils thostmduserapi_se thosttraderapi_se)
