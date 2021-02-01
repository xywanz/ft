set(XTP_LIB_PATH ${CMAKE_SOURCE_DIR}/third_party/XTP_API_1.1.19.2_20190627/bin)
include_directories(${XTP_LIB_PATH}/include)
link_directories(${XTP_LIB_PATH}/linux_centos7)

aux_source_directory(. XTP_GATEWAY_SRC)
add_library(xtp-gateway SHARED ${XTP_GATEWAY_SRC})
target_link_libraries(xtp-gateway xtptraderapi xtpquoteapi)
