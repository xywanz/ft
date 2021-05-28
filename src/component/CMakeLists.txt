
add_library(component STATIC
    order_book/order_book.cpp
    order_book/price_level.cpp
    pubsub/publisher.cpp
    pubsub/subscriber.cpp
    position/calculator.cpp
    position/manager.cpp)
add_library(ft::component ALIAS component)

target_include_directories(component PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(component PUBLIC ft::ft_header ft::base cereal)

add_subdirectory(yijinjing)
target_link_libraries(yijinjing PUBLIC ft::ft_header spdlog)
