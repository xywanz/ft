cmake_minimum_required(VERSION 3.0)
project(cppex)

set(CMAKE_CXX_STANDARD 17)

set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR})

include_directories(include)

set(SRC_FILES
    string/split.cpp)

add_library(cppex STATIC
    ${SRC_FILES}
)
target_link_libraries(cppex)

add_subdirectory(test)
