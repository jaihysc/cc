# Targets
# cclib : Library of compiled files for cc, excluding main.c
# cc    : Executable for compiler
add_library(cclib
	cfg.c
	errorcode.c
	globals.c
	il2gen.c
	il2statement.c
	lexer.c
	parser.c
	set.c
	symbol.c
	symtab.c
	tree.c
	type.c
	vec.c
)
target_include_directories(cclib PUBLIC ./)

add_executable(cc main.c)
target_link_libraries(cc PRIVATE cclib)