
add_library(utils STATIC
    lockfree-queue/queue.c
    datetime.cpp)
add_library(ft::utils ALIAS utils)

target_include_directories(utils PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(utils PUBLIC ft::component hiredis fmt)
