# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

include_directories(
    ${CMAKE_SOURCE_DIR}/dependencies/googletest/include
)

link_directories(
  ${CMAKE_SOURCE_DIR}/dependencies/googletest/lib
)

aux_source_directory(. TEST_SRC)

add_executable(run_unit_tests ${TEST_SRC})
target_link_libraries(run_unit_tests order_book rms gtest gtest_main pthread)
