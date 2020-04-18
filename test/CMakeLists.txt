# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_executable(simnow
    simnow.cpp)
target_link_libraries(simnow ft cppex pthread)

add_executable(misc
misc.cpp)
target_link_libraries(misc ft cppex pthread)
