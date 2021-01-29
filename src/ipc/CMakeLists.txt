
add_library(ipc STATIC queue.c)
target_link_libraries(ipc hiredis)