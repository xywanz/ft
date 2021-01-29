aux_source_directory(. UTILS_SRC)
add_library(utils STATIC ${UTILS_SRC})
target_link_libraries(utils yaml-cpp)
