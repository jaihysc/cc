# CC - C Compiler in C

The main components are under the following files

* Entry point: `main.c`
* Lexer: `lexer.c`
* Parser: `parser.c`
* Intermediate Language 2 (IL2) generator: `il2gen.c`
* Intermediate Language 2 (IL2) analysis algorithms: `il2analysis.c`

Data structures used by the main components

* Abstract Syntax Tree (AST): `tree.c`
* Control Flow Graph (CFG): `cfg.c`
* Symbol table: `symtab.c`

* Error codes: `errorcode.c`
* Symbols (source code variable names, constants, labels, etc): `symbol.c`
* C data types: `type.c`

* Generic set of pointers: `set.c`
* Generic vector: `vec.c`
