/*
   Lexer
   Reads the input file, provides interfaces to read out tokens
*/
#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

#include "constant.h"
#include "errorcode.h"

#define LEXER_LINE_BUF_SIZE 128 // Holds specified number-1 characters

/* Returns 1 if character is part of a punctuator */
int isofpunctuator(char c);

/* Returns 1 if string is considered as a keyword, 0 otherwise */
int tok_iskeyword(const char* token);

/* Returns 1 if string is considered a unary operator */
int tok_isunaryop(const char* str);

/* Returns if token is considered an assignment operator */
int tok_isassignmentop(const char* token);

/* Returns 1 if token c string is a storage class keyword, 0 otherwise */
int tok_isstoreclass(const char* str);

/* Returns 1 if token c string is a type specifier keyword, 0 otherwise */
int tok_istypespec(const char* str);

/* Returns 1 if token c string is a type qualifier keyword, 0 otherwise */
int tok_istypequal(const char* str);

/* Returns 1 if token c string is a function specifier keyword, 0 otherwise */
int tok_isfuncspec(const char* str);

/* Returns 1 if token c string is an identifier */
int tok_isidentifier(const char* str);

typedef struct
{
	/* Input file */
	FILE* rf;

	/* Tracks position within input file for error messages */
	int line_num;
	int char_num;

	/* Circular buffer for current line */
	char line_buf[LEXER_LINE_BUF_SIZE];
	int line_buf_front;
	int line_buf_end;

	/* Location of primary and secondary tokens */
	int primary_line_num;
	int primary_char_num;
	int primary_length;

	int secondary_line_num;
	int secondary_char_num;
	int secondary_length;

	/* The read token is kept here, subsequent calls to lexer_getc()
	   will return this.
	   When lexer_consume() is called, the next call to lexer_getc()
	   will fill this buffer with a new token
	   2 buffers for 2 token lookahead */
	char get_buf[2][MAX_TOKEN_LEN + 1];
	/* Index in get_buf for primary buffer (next token) */
	int primary;
	int secondary;
} Lexer;

/* Initializes lexer lexer object at memory
   Returns zero if success, non-zero if error */
ErrorCode lexer_construct(Lexer* lex, const char* filepath);

/* Destructs lexer object at memory */
void lexer_destruct(Lexer* lex);

/* Reads a token (c = char*)
   The pointer to token is stored the the provided pointer
   The token is blank (just a null terminator)
   if end of file is reached or error happened */
ErrorCode lexer_getc(Lexer* lex, const char** tok_ptr);

/* Reads token after next (c = char*)
   The pointer to token is stored the the provided pointer
   The token is blank (just a null terminator)
   if end of file is reached or error happened */
ErrorCode lexer_getc2(Lexer* lex, const char** tok_ptr);

/* Discard the last read token, next token will be next token in stream */
void lexer_consume(Lexer* lex);

/* Prints out the current location of the Lexer within the source file
   Example:
   73 | void lexer_consume(Lexer* lex);
	  |      ~~~~~~~~~~~~~ */
void lexer_print_location(Lexer* lex);

#endif
