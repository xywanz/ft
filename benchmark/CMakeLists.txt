
add_executable(BM_yijinjing BM_yijinjing.cpp)
target_link_libraries(BM_yijinjing PRIVATE ft_header yijinjing)

add_executable(BM_ipc BM_ipc.cpp)
target_link_libraries(BM_ipc yijinjing benchmark pthread)
