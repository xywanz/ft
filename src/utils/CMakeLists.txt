aux_source_directory(lockfree-queue LFQ_SRC)
aux_source_directory(. UTILS_SRC)
add_library(utils STATIC ${LFQ_SRC} ${UTILS_SRC})
target_link_libraries(utils ft_component yaml-cpp hiredis)
