
add_library(stub_gateway STATIC stub_gateway.cpp)
target_include_directories(stub_gateway PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(stub_gateway PUBLIC
    ft::ft_header ft::base ft::component
    ft::utils pthread)
