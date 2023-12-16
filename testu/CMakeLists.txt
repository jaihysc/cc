# Targets
# cctests : The executable for unit tests
add_executable(cctests
	testu.c
	CuTest.c
	cfg_test.c
	lexer_test.c
	parser_test.c
	set_test.c
	symbol_test.c
	symtab_test.c
	tree_test.c
	type_test.c
 )
target_link_libraries(cctests PRIVATE cclib)
