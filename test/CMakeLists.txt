# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

include_directories(
    ${CMAKE_SOURCE_DIR}/third_party/googletest/include
)

link_directories(
  ${CMAKE_SOURCE_DIR}/third_party/googletest/lib
)

aux_source_directory(. TEST_SRC)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/component/order_book ORDER_BOOK_SRC)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/trading_server/risk_management/common RISK_RULE_SRC)

add_executable(run_unit_tests ${TEST_SRC} ${ORDER_BOOK_SRC} ${RISK_RULE_SRC})
target_link_libraries(run_unit_tests utils gtest gtest_main pthread)
