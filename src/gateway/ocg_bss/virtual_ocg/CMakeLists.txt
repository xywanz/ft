# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

aux_source_directory(. VIRTUAL_OCG_SRC)

add_executable(ocg-main ${VIRTUAL_OCG_SRC})
target_link_libraries(ocg-main proto-codec pthread)
