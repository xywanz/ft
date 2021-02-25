
add_library(utils STATIC
    lockfree-queue/queue.c
    config_loader.cpp
    datetime.cpp
    portfolio.cpp
    redis.cpp)
add_library(ft::utils ALIAS utils)

target_include_directories(utils PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(utils PUBLIC ft::component ft::ft_third_party)
