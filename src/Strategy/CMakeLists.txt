# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

add_executable(StrategyLoader StrategyLoader.cpp)
target_link_libraries(StrategyLoader Strategy ${COMMON_LIB})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
add_library(Strategy STATIC Strategy.cpp)
target_link_libraries(Strategy ${COMMON_LIB})
