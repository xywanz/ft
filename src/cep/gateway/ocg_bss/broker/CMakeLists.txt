# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

aux_source_directory(. BSS_BROKER_SRC)
add_library(bss-broker SHARED ${BSS_BROKER_SRC})
target_link_libraries(bss-broker proto-codec pthread)
