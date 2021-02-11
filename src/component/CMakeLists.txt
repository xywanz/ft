aux_source_directory(contract_table CONTRACT_TABLE_SRC)
aux_source_directory(order_book ORDER_BOOK_SRC)

add_library(ft_component STATIC ${CONTRACT_TABLE_SRC} ${ORDER_BOOK_SRC})
