# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

aux_source_directory(. PHASE_1_SRC)
add_executable(phase_1 ${PHASE_1_SRC})
target_link_libraries(phase_1 proto-codec)
