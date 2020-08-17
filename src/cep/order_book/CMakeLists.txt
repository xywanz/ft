# Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

aux_source_directory(. ORDER_BOOK_SRC)
add_library(order_book STATIC ${ORDER_BOOK_SRC})
