/* Parses tokens from preprocessed C file */
#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "symtab.h"
#include "tree.h"

typedef struct
{
	Lexer* lex;
	Symtab* symtab;
	Tree* tree;
} Parser;

ErrorCode parser_construct(Parser* p, Lexer* lex, Symtab* symtab, Tree* tree);

/* Parse a translation unit,
   reads from Lexer,
   stores into tree */
ErrorCode parse_translation_unit(Parser* p);

#endif
