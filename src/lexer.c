/* Lexer / token handling */

#include "lexer.h"
#include "globals.h"

#include "common.h"

/* Returns 1 is considered whitespace */
static int iswhitespace(char c) {
	switch (c) {
	case ' ':
	case '\t':
	case '\n':
		return 1;
	default:
		return 0;
	}
}

/* If character is part of a punctuator */
int isofpunctuator(char c) {
	switch (c) {
	case '[':
	case ']':
	case '(':
	case ')':
	case '{':
	case '}':
	case '.':
	case '-':
	case '+':
	case '&':
	case '*':
	case '~':
	case '!':
	case '/':
	case '%':
	case '<':
	case '>':
	case '=':
	case '^':
	case '|':
	case '?':
	case ':':
	case ';':
	case ',':
	case '#':
		return 1;
	default:
		return 0;
	}
}

/* C keyword handling */

/* Returns 1 if string is considered as a keyword, 0 otherwise */
int tok_iskeyword(const char* token) {
	const char* token_keyword[] = {
		"_Bool",  "_Complex", "_Imaginary", "auto",		"break",  "case",	  "char",	"const",  "continue", "default",
		"do",	  "double",	  "else",		"enum",		"extern", "float",	  "for",	"goto",	  "if",		  "inline",
		"int",	  "long",	  "register",	"restrict", "return", "short",	  "signed", "sizeof", "static",	  "struct",
		"switch", "typedef",  "union",		"unsigned", "void",	  "volatile", "while"};
	return strbinfind(token, strlength(token), token_keyword, ARRAY_SIZE(token_keyword)) >= 0;
}

/* Returns 1 if string is considered a unary operator */
int tok_isunaryop(const char* str) {
	if (strlength(str) != 1) {
		return 0;
	}

	switch (str[0]) {
	case '&':
	case '*':
	case '+':
	case '-':
	case '~':
	case '!':
		return 1;
	default:
		return 0;
	}
}

/* Returns 1 if token is considered an assignment operator */
int tok_isassignmentop(const char* token) {
	const char* token_assignment_operator[] = {/* See strbinfind for ordering requirements */
											   "%=",
											   "&=",
											   "*=",
											   "+=",
											   "-=",
											   "/=",
											   "<<=",
											   "=",
											   ">>=",
											   "^=",
											   "|="};
	return strbinfind(token, strlength(token), token_assignment_operator, ARRAY_SIZE(token_assignment_operator)) >= 0;
}

int tok_isstoreclass(const char* str) {
	const char* token_storage_class_keyword[] = {/* See strbinfind for ordering requirements */
												 "auto",
												 "extern",
												 "register",
												 "static",
												 "typedef"};
	return strbinfind(str, strlength(str), token_storage_class_keyword, ARRAY_SIZE(token_storage_class_keyword)) >= 0;
}

int tok_istypespec(const char* str) {
	const char* token_type_keyword[] = {
		/* See strbinfind for ordering requirements */
		"char",
		"double",
		"float",
		"int",
		"long",
		"short",
		"signed",
		"unsigned",
		"void",
	};
	return strbinfind(str, strlength(str), token_type_keyword, ARRAY_SIZE(token_type_keyword)) >= 0;
}

int tok_istypequal(const char* str) {
	const char* token_type_qualifier_keyword[] = {/* See strbinfind for ordering requirements */
												  "const",
												  "restrict",
												  "volatile"};
	return strbinfind(str, strlength(str), token_type_qualifier_keyword, ARRAY_SIZE(token_type_qualifier_keyword)) >= 0;
}

int tok_isfuncspec(const char* str) {
	return strequ(str, "inline");
}

int tok_isidentifier(const char* str) {
	if (tok_iskeyword(str)) {
		return 0;
	}

	char c = str[0];
	if (c == '\0') {
		return 0;
	}
	int is_nondigit = 0;
	if ('a' <= c && c <= 'z') {
		is_nondigit = 1;
	}
	else if ('A' <= c && c <= 'Z') {
		is_nondigit = 1;
	}
	else if (c == '_') {
		is_nondigit = 1;
	}
	if (!is_nondigit) {
		return 0;
	}

	int i = 1;
	while ((c = str[i]) != '\0') {
		int is_id = 0;
		if ('a' <= c && c <= 'z') {
			is_id = 1;
		}
		else if ('A' <= c && c <= 'Z') {
			is_id = 1;
		}
		else if (c == '_') {
			is_id = 1;
		}
		else if ('0' <= c && c <= '9') {
			is_id = 1;
		}
		if (!is_id) {
			return 0;
		}
		++i;
	}
	return 1;
}

/* Reading characters from input */

static void consume_char(Lexer* lex);

/* Reads in a character from input
   Does not advance read position
   Returns EOF if end of input */
static char read_char(Lexer* lex) {
	/* Perhaps this can be a buffer in the future to reduce system calls */
	char c = lex->input[lex->input_idx];
	if (c == '\0') return EOF;

	/* Handle line marker from preprocessor, of form
	   # 12345 xxxxxxxx
	   Assuming the line markers are always of the same form, no checks for end of buffer */
	if (lex->char_num == 1) {
		while (c == '#') {
			++lex->input_idx; /* Consume space */

			/* Line number */
			int line_num = 0;
			while ((c = lex->input[lex->input_idx++]) != ' ') {
				line_num *= 10;
				line_num += c - '0';
			}
			lex->line_num = line_num;

			/* Consume linemarker */
			while ((c = lex->input[lex->input_idx++]) != '\n')
				;
			c = lex->input[lex->input_idx]; /* First char of next line */
		}
	}

	return c;
}

/* Reads in a character from input 1 ahead
   of character from read_char
   Does not advance read position */
static char read_char_next(Lexer* lex) {
	/* Save current index, read 1 ahead, return to current index */
	int old_input_idx = lex->input_idx;
	consume_char(lex);
	char c = read_char(lex);
	lex->input_idx = old_input_idx;

	return c;
}

/* Advances read position to next char */
static void consume_char(Lexer* lex) {
	/* Move the file position forwards */
	char c = lex->input[lex->input_idx++];

	if (c == '\n') {
		++lex->line_num;
		lex->char_num = 1;
	}
	else {
		++lex->char_num;
	}

	/* Overwrite the front contents if buffer is full */
	if ((lex->line_buf_end + 1) % LEXER_LINE_BUF_SIZE == lex->line_buf_front) {
		lex->line_buf_front = (lex->line_buf_front + 1) % LEXER_LINE_BUF_SIZE;
	}
	lex->line_buf[lex->line_buf_end] = c;
	lex->line_buf_end = (lex->line_buf_end + 1) % LEXER_LINE_BUF_SIZE;
}

/* Lexer */

ErrorCode lexer_construct(Lexer* lex, const char* input) {
	lex->input = input;
	lex->input_idx = 0;

	lex->line_num = 1;
	lex->char_num = 1;

	lex->line_buf_front = 0;
	lex->line_buf_end = 0;

	lex->primary_line_num = 0;
	lex->primary_char_num = 0;
	lex->primary_length = 0;

	lex->secondary_line_num = 0;
	lex->secondary_char_num = 0;
	lex->secondary_length = 0;

	lex->get_buf[0][0] = '\0';
	lex->get_buf[1][0] = '\0';
	lex->primary = 0;
	lex->secondary = 1;

	return ec_noerr;
}

void lexer_destruct(Lexer* lex) {
	(void)lex;
}

/* Fetches token into specified buffer index */
static ErrorCode load_buf(Lexer* lex, int idx) {
	ErrorCode ecode = ec_noerr;

	char* buf = lex->get_buf[idx];

	char c = read_char(lex);

	/* Skip characters which do not form tokens */
	int skips_chars = 1;
	while (skips_chars) {
		/* Skip leading whitespace */
		if (iswhitespace(c)) {
			while (iswhitespace(c = read_char(lex))) {
				consume_char(lex);
			}
		}
		/* Skip /* comments * / */
		else if (c == '/' && read_char_next(lex) == '*') {
			while (1) {
				consume_char(lex);
				c = read_char(lex);

				if (c == EOF) break;
				if (c == '*' && read_char_next(lex) == '/') {
					consume_char(lex); // Consume *

					c = read_char(lex); // Consume /
					consume_char(lex);

					c = read_char(lex); // Next character to continue processing
					break;
				}
			}
		}
		/* Skip // comments */
		else if (c == '/' && read_char_next(lex) == '/') {
			while (1) {
				consume_char(lex);
				c = read_char(lex);

				if (c == EOF) break;
				/* Consume the newline to continue processing */
				if (c == '\n') {
					consume_char(lex);
					c = read_char(lex);
					break;
				}
			}
		}
		else {
			skips_chars = 0;
		}
	}

	/* Handle punctuators
	   sufficient space guaranteed to hold all punctuators*/
	ASSERT(MAX_TOKEN_LEN >= 4, "Max token length must be at least 4 to hold C operators");
	if (isofpunctuator(c)) {
		char cn = read_char_next(lex);
		/* <= >= == != *= /= %= += -= &= ^= |= */
		if (cn == '=') {
			buf[0] = c;
			buf[1] = '=';
			buf[2] = '\0';
			consume_char(lex);
			consume_char(lex);
			goto exit;
		}

		/* Handle double char punctuators
		   + ++ - -- & && | || */
		char chars[] = {'+', '-', '&', '|'};
		for (int i = 0; i < ARRAY_SIZE(chars); ++i) {
			if (c == chars[i]) {
				consume_char(lex);
				buf[0] = chars[i];
				if (cn == chars[i]) {
					consume_char(lex);
					buf[1] = chars[i];
					buf[2] = '\0';
				}
				else {
					buf[1] = '\0';
				}
				goto exit;
			}
		}

		/* TODO For now, treat everything else as token */
		buf[0] = c;
		buf[1] = '\0';
		consume_char(lex);
		goto exit;
	}

	/* Handle others */
	int i = 0;
	while (c != EOF) {
		if (iswhitespace(c) || isofpunctuator(c)) {
			break;
		}

		if (i >= MAX_TOKEN_LEN) {
			ecode = ec_lexer_tokbufexceed;
			break; /* Since MAX_TOKEN_LEN excludes null terminator, we can break and add null terminator */
		}
		buf[i] = c;
		consume_char(lex);
		c = read_char(lex);
		++i;
	}
	buf[i] = '\0';

exit:
	return ecode;
}

/* Loads buffers if their tokens have been consumed */
static ErrorCode load_all_buf(Lexer* lex) {
	ErrorCode ecode;
	if (lex->get_buf[lex->primary][0] == '\0') {
		if ((ecode = load_buf(lex, lex->primary)) != ec_noerr) return ecode;
		lex->primary_length = strlength(lex->get_buf[lex->primary]);
		lex->primary_line_num = lex->line_num;
		lex->primary_char_num = lex->char_num - lex->primary_length;
	}
	if (lex->get_buf[lex->secondary][0] == '\0') {
		if ((ecode = load_buf(lex, lex->secondary)) != ec_noerr) return ecode;
		lex->secondary_length = strlength(lex->get_buf[lex->secondary]);
		lex->secondary_line_num = lex->line_num;
		lex->secondary_char_num = lex->char_num - lex->secondary_length;
	}
	return ec_noerr;
}

ErrorCode lexer_getc(Lexer* lex, const char** tok_ptr) {
	ErrorCode ecode;

	if ((ecode = load_all_buf(lex)) != ec_noerr) return ecode;

	if (g_debug_print_parse_recursion) {
		LOGF("%s\n", lex->get_buf[lex->primary]);
	}

	*tok_ptr = lex->get_buf[lex->primary];
	return ec_noerr;
}

ErrorCode lexer_getc2(Lexer* lex, const char** tok_ptr) {
	ErrorCode ecode;

	if ((ecode = load_all_buf(lex)) != ec_noerr) return ecode;

	if (g_debug_print_parse_recursion) {
		LOGF("%s (Lookahead)\n", lex->get_buf[lex->secondary]);
	}

	*tok_ptr = lex->get_buf[lex->secondary];
	return ec_noerr;
}

/* Indicates the pointed to token is no longer in use */
void lexer_consume(Lexer* lex) {
	lex->get_buf[lex->primary][0] = '\0';

	/* Secondary buffer now primary
	   as its token is the next one */
	int tmp = lex->secondary;
	lex->secondary = lex->primary;
	lex->primary = tmp;

	lex->primary_line_num = lex->secondary_line_num;
	lex->primary_char_num = lex->secondary_char_num;
	lex->primary_length = lex->secondary_length;

	if (g_debug_print_parse_recursion) {
		LOG("^Consumed\n");
	}
}

void lexer_print_location(Lexer* lex) {
	char line_num_buf[10];
	itostr(lex->primary_line_num, line_num_buf);
	int line_num_len = strlength(line_num_buf);

	char char_num_buf[10];
	itostr(lex->primary_char_num, char_num_buf);
	int char_num_len = strlength(char_num_buf);

	LOGF(" %s:%s | ", line_num_buf, char_num_buf);

	/* Find start, end of line with primary token in line buffer */
	int line_start = lex->line_buf_end;
	int line_end = lex->line_buf_end; /* One past last character */
	{
		int line_num = lex->line_num;
		while (1) {
			if (line_start == lex->line_buf_front) {
				LOG("Line too long to be displayed\n");
				return;
			}

			--line_start;
			if (line_start < 0) line_start = LEXER_LINE_BUF_SIZE - 1;

			if (lex->line_buf[line_start] == '\n') {
				if (line_num == lex->primary_line_num) break;
				--line_num;
				line_end = line_start;
			}
		}
		/* Loop finishes on newline, move it to next char */
		line_start = (line_start + 1) % LEXER_LINE_BUF_SIZE;
	}

	/* Print out line */
	for (int i = line_start; i != line_end; i = (i + 1) % LEXER_LINE_BUF_SIZE) {
		LOGF("%c", lex->line_buf[i]);
	}
	LOG("\n");

	/* Underline the token */

	/* Align the '|' to the previous one */
	for (int i = 0; i < line_num_len + char_num_len + 3; ++i) {
		LOGF(" ");
	}
	LOG("| ");
	/* -1 as char_num starts at 1, not zero */
	for (int i = 0; i < lex->primary_char_num - 1; ++i) {
		LOG(" ");
	}
	for (int i = 0; i < lex->primary_length; ++i) {
		LOG("~");
	}
	LOG("\n");
}
