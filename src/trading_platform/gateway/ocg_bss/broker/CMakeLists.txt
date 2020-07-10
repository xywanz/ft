# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

aux_source_directory(. BROKER_SRC)
add_library(broker SHARED ${BROKER_SRC})
target_link_libraries(broker proto-codec pthread)
