# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

aux_source_directory(. TS_SRC)
add_executable(MTE ${TS_SRC})
target_link_libraries(MTE yaml-cpp spdlog Gateway RiskManagement)
