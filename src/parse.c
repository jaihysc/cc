/* Preprocessed c file parser */
/* Generated output is imm2 */

#include <stdio.h>
#include <stdarg.h>
#include "common.h"

#define MAX_TOKEN_LEN 255 /* Excluding null terminator, Tokens is string with no whitespace */
#define MAX_PARSER_BUFFER_LEN 255 /* Excluding null terminator */

/* ============================================================ */
/* Parser data structure + functions */

/* Should always be initialized to ec_noerr */
/* Since functions will only sets if error occurred */
typedef enum {
    ec_noerr = 0,
    ec_tokbufexceed,
    ec_pbufexceed, /* Parser buffer exceeded */
    ec_syntaxerr,
    ec_writefailed,
} errcode;

/* Indicates what parser currently reading,
   helps parser select the right buffer to output to */
typedef enum {
    ps_fdecl = 0, /* Function declaration */
    ps_paramtypelist,
} pstate;

/* The text buffers of the parser */
/* Holds text until enough information is known to generate an il instruction */
typedef enum {
    /* Reuse buffers if they will never be both used at the same time*/
    pb_declspec_type = 0,
    pb_declarator_id = 1,
    pb_paramtypelist = 2,
    pb_op1 = 1, /* Expression first operand and result */
    pb_op2 = 2,
    pb_count /* Count of entries in enum */
} pbuffer;

typedef struct {
    FILE* rf; /* Input file */
    FILE* of; /* Output file */

    pstate state;
    /* Functions set error code only if an error occurred */
    /* By default this is set as ec_noerr */
    errcode ecode;

    /* Token read buffer, used by read_token to store token read */
    /* The read token is kept here, subsequent calls to read_token()
       will return this.
       When consume_token() is called, the next call to read_token()
       will fill this buffer with a new token
       */
    char tok_buf[MAX_TOKEN_LEN + 1];

    char buf[pb_count][MAX_PARSER_BUFFER_LEN + 1];
    int i_buf[pb_count]; /* Index 1 past end in buf */
} parser;

static void debug_parser_buf_dump(parser* p);

/* Sets error code in parser */
static void parser_set_error(parser* p, errcode ecode) {
    p->ecode = ecode;
}

/* Returns error code in parser */
static errcode parser_get_error(parser* p) {
    return p->ecode;
}

/* Returns 1 if error is set, 0 otherwise */
static int parser_has_error(parser* p) {
    return p->ecode != ec_noerr;
}

/* Reads contents of parser buffer specified by target */
/* Read is null terminated string */
static char* parser_buf_rd(parser* p, pbuffer target) {
    return p->buf[target];
}

/* Adds null terminated string into buffer specified by target */
/* The null terminator is not included */
/* Can be called again even if error occurred previously */
static void parser_buf_push(parser* p, pbuffer target, const char* str) {
    int i = 0;
    char c;
    while ((c = str[i]) != '\0') {
        if (p->i_buf[target] >= MAX_PARSER_BUFFER_LEN) {
            parser_set_error(p, ec_pbufexceed);
            break;
        }
        p->buf[target][p->i_buf[target]] = c;
        ++p->i_buf[target];
        ++i;
    }
    /* Always 1 extra slot to insert null terminator */
    p->buf[target][p->i_buf[target]] = '\0';
}

/* Clears all buffers of parser, leaving only null terminator */
static void parser_buf_clear(parser* p) {
    for (int i = 0; i < pb_count; ++i) {
        p->buf[i][0] = '\0';
        p->i_buf[i] = 0;
    }
}

/* Writes provided IL using format string and va_args into output */
static void parser_output_il(parser* p, const char* fmt, ...) {
    debug_parser_buf_dump(p);
    va_list args;
    va_start(args, fmt);
    if (vfprintf(p->of, fmt, args) < 0) {
        parser_set_error(p, ec_writefailed);
    }
    va_end(args);
}

/* Prints out contents of parser buffers */
static void debug_parser_buf_dump(parser* p) {
    LOG("Parser buffer: Count, contents\n");
    for (int i = 0; i < pb_count; ++i) {
        LOGF("%d | %s\n", i, p->buf[i]);
    }

    LOG("Token buffer: contents\n");
    LOGF("%s\n", p->tok_buf);
}

/* ============================================================ */
/* Token handling */

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

/* If character is considered as its own token */
static int istokenchar(char c) {
    switch (c) {
        /* TODO incomplete list of token chars */
        case '(': case ')': case '{': case '&': case '*': case ',': case ';':
            return 1;
        default:
            return 0;
    }
}

/* C keyword handling */

const char* token_storage_class_keyword[] = {
    /* See strbinfind for ordering requirements */
    "auto",
    "extern",
    "register",
    "static",
    "typedef"
};
/* Returns 1 if token c string is a storage class keyword, 0 otherwise */
static int tok_isstoreclass(const char* str) {
    return strbinfind(
            str,
            strlength(str),
            token_storage_class_keyword,
            ARRAY_SIZE(token_storage_class_keyword)) >= 0;
}

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
/* Returns 1 if token c string is a type specifier keyword, 0 otherwise */
static int tok_istypespec(const char* str) {
    return strbinfind(
            str,
            strlength(str),
            token_type_keyword,
            ARRAY_SIZE(token_type_keyword)) >= 0;
}

const char* token_type_qualifier_keyword[] = {
    /* See strbinfind for ordering requirements */
    "const",
    "restrict",
    "volatile"
};
/* Returns 1 if token c string is a type qualifier keyword, 0 otherwise */
static int tok_istypequal(const char* str) {
    return strbinfind(
            str,
            strlength(str),
            token_type_qualifier_keyword,
            ARRAY_SIZE(token_type_qualifier_keyword)) >= 0;
}

/* Returns 1 if token c string is a function specifier keyword, 0 otherwise */
static int tok_isfuncspec(const char* str) {
    return strequ(str, "inline");
}

/* Returns 1 if token c string is an identifier */
static int tok_isidentifier(const char* str) {
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

/* Indicates the pointed to token is no longer in use */
static void consume_token(char* token) {
    token[0] = '\0';
}

/* Reads a null terminated token */
/* Returns pointer to the token, NULL if EOF or errored */
static char* read_token(parser* p) {
    if (p->tok_buf[0] != '\0') {
        LOGF("%s ^Cached\n", p->tok_buf);
        return p->tok_buf;
    }

    int i = 0; /* Index into buf */
    int seen_token = 0;
    char c;
    while ((c = getc(p->rf)) != EOF) {
        if (i >= MAX_TOKEN_LEN) {
            parser_set_error(p, ec_tokbufexceed);
            break; /* Since MAX_TOKEN_LEN excludes null terminator, we can break and add null terminator */
        }

        if (iswhitespace(c)) {
            if (seen_token) {
                break;
            }
            /* Skip leading whitespace */
            continue;
        }
        if (istokenchar(c)) {
            /* Token char is a separate token, save it to be read again */
            if (seen_token) {
                fseek(p->rf, -1, SEEK_CUR);
                break;
            }
            else {
                /* A token char itself is a token */
                ASSERT(i == 0, "Expected token char to occupy index 0");
                p->tok_buf[i] = c;
                ++i;
                break;
            }
        }
        seen_token = 1;
        p->tok_buf[i] = c;
        ++i;
    }

    p->tok_buf[i] = '\0';
    LOGF("%s\n", p->tok_buf);

    if (c == EOF) {
        return NULL;
    }
    return p->tok_buf;
}


/* ============================================================ */
/* C source parsing functions */

/* Source parsing debug logging functions */
/* Shows beginning and end of each function, indented with recursion depth */

int debug_parse_func_recursion_depth = 0;
/* Call at beginning on function */
#define DEBUG_PARSE_FUNC_START(syntactic_category__)             \
    for (int i = 0; i < debug_parse_func_recursion_depth; ++i) { \
        LOG("\t");                                               \
    }                                                            \
    LOG("==>" #syntactic_category__ "\n");                       \
    debug_parse_func_recursion_depth++
/* Call at end on function */
#define DEBUG_PARSE_FUNC_END()                                   \
    debug_parse_func_recursion_depth--;                          \
    for (int i = 0; i < debug_parse_func_recursion_depth; ++i) { \
        LOG("\t");                                               \
    }                                                            \
    LOG("<==\n")

/* Sorted by Annex A.2 in C99 standard */

/* 6.5 Expressions */
static void parse_primaryexpr(parser* p);
static void parse_postfixexpr(parser* p);
static void parse_argumentexprlist(parser* p);
static void parse_unaryexpr(parser* p);
static void parse_castexpr(parser* p);
static void parse_multiplicativeexpr(parser* p);
static void parse_additiveexpr(parser* p);
static void parse_shiftexpr(parser* p);
static void parse_relationalexpr(parser* p);
static void parse_equalityexpr(parser* p);
static void parse_andexpr(parser* p);
static void parse_exclusiveorexpr(parser* p);
static void parse_inclusiveorexpr(parser* p);
static void parse_logicalandexpr(parser* p);
static void parse_logicalorexpr(parser* p);
static void parse_conditionalexpr(parser* p);
static void parse_assignmentexpr(parser* p);
static void parse_expr(parser* p);
/* 6.7 Declarators */
static void parse_decl(parser* p);
static void parse_declspec(parser* p);
static void parse_initdeclaratorlist(parser* p);
static void parse_initdeclarator(parser* p);
static void parse_declarator(parser* p);
static void parse_dirdeclarator(parser* p);
static void parse_pointer(parser* p);
static void parse_paramtypelist(parser* p);
static void parse_paramlist(parser* p);
static void parse_paramdecl(parser* p);
static void parse_initializer(parser* p);
/* 6.8 Statements and blocks */
static void parse_stat(parser* p);
static void parse_compoundstat(parser* p);
static void parse_blockitemlist(parser* p);
static void parse_blockitem(parser* p);
static void parse_jumpstat(parser* p);
/* Helpers */
static int parse_expecttoken(parser* p, const char* match_token);

/* primary-expression */
static void parse_primaryexpr(parser* p) {
    DEBUG_PARSE_FUNC_START(primary-expression);
    char* token;
    if ((token = read_token(p)) == NULL || parser_get_error(p) != ec_noerr) {
        goto exit;
    }

    if (tok_isidentifier(token)) {
        parser_buf_push(p, pb_op1, token);
        consume_token(token);
    }

exit:
    DEBUG_PARSE_FUNC_END();
}

/* postfix-expression */
static void parse_postfixexpr(parser* p) {
}

/* argument-expression-list */
static void parse_argumentexprlist(parser* p) {
}

/* unary-expression */
static void parse_unaryexpr(parser* p) {
}

/* cast-expression */
static void parse_castexpr(parser* p) {
}

/* multiplicative-expression */
static void parse_multiplicativeexpr(parser* p) {
}

/* additive-expression */
static void parse_additiveexpr(parser* p) {
}

/* shift-expression */
static void parse_shiftexpr(parser* p) {
}

/* relational-expression */
static void parse_relationalexpr(parser* p) {
}

/* equality-expression */
static void parse_equalityexpr(parser* p) {
}

/* and-expression */
static void parse_andexpr(parser* p) {
}

/* exclusive-or-expression */
static void parse_exclusiveorexpr(parser* p) {
}

/* inclusive-or-expression */
static void parse_inclusiveorexpr(parser* p) {
}

/* logical-and-expression */
static void parse_logicalandexpr(parser* p) {
}

/* logical-or-expression */
static void parse_logicalorexpr(parser* p) {
}

/* conditional-expression */
static void parse_conditionalexpr(parser* p) {
}

/* assignment-expression */
static void parse_assignmentexpr(parser* p) {
}

/* expression */
static void parse_expr(parser* p) {
    DEBUG_PARSE_FUNC_START(expression);
    parse_primaryexpr(p);
    DEBUG_PARSE_FUNC_END();
}

/* declaration */
static void parse_decl(parser* p) {
    DEBUG_PARSE_FUNC_START(declaration);

    parse_declspec(p);
    parse_initdeclaratorlist(p);

    int has_semicolon = parse_expecttoken(p, ";");
    if (parser_has_error(p)) goto exit;
    if (!has_semicolon) {
        ERRMSG("Expected semicolon after declaration\n");
        parser_set_error(p, ec_syntaxerr);
    }

exit:
    DEBUG_PARSE_FUNC_END();
}

/* declaration-specifiers */
static void parse_declspec(parser* p) {
    DEBUG_PARSE_FUNC_START(declspec);
    char* token;
    if ((token = read_token(p)) == NULL || parser_get_error(p) != ec_noerr) {
        goto exit;
    }

    int store_class = tok_isstoreclass(token);
    int type_spec = tok_istypespec(token);
    int type_qual = tok_istypequal(token);
    int func_spec = tok_isfuncspec(token);
    if (store_class) {
        consume_token(token);
        parse_declspec(p);
    }
    else if (type_spec) {
        if (p->state == ps_fdecl) {
            parser_buf_push(p, pb_declspec_type, token);
            consume_token(token);
        }
        else if (p->state == ps_paramtypelist) {
            parser_buf_push(p, pb_paramtypelist, token);
            consume_token(token);
        }
        else {
            ASSERTF(
                0,
                "Invalid parser state in declaration specifier: %d\n", p->state
            );
            goto exit;
        }
        parse_declspec(p);
    }
    else if (type_qual) {
        consume_token(token);
        parse_declspec(p);
    }
    else if (func_spec) {
        consume_token(token);
        parse_declspec(p);
    }

exit:
    DEBUG_PARSE_FUNC_END();
}

/* init-declarator-list */
static void parse_initdeclaratorlist(parser* p) {
    DEBUG_PARSE_FUNC_START(init-declarator-list);
    while (1) {
        parse_initdeclarator(p);

        /* init-declarators separated by commas */
        int has_token = parse_expecttoken(p, ",");
        if (parser_has_error(p)) goto exit;
        if (!has_token) {
            break;
        }
    }

exit:
    DEBUG_PARSE_FUNC_END();
}

/* init-declarator */
static void parse_initdeclarator(parser* p) {
    DEBUG_PARSE_FUNC_START(initdeclarator);

    /* declarator OR declarator = initializer */
    parse_declarator(p);

    int has_token = parse_expecttoken(p, "=");
    if (parser_has_error(p)) goto exit;
    if (has_token) {
        parse_initializer(p);
    }

exit:
    DEBUG_PARSE_FUNC_END();
}

/* declarator */
static void parse_declarator(parser* p) {
    DEBUG_PARSE_FUNC_START(declarator);
    parse_pointer(p);
    if (parser_get_error(p) != ec_noerr) {
        goto exit;
    }
    parse_dirdeclarator(p);

exit:
    DEBUG_PARSE_FUNC_END();
}

/* direct-declarator */
static void parse_dirdeclarator(parser* p) {
    DEBUG_PARSE_FUNC_START(direct-declarator);
    char* token;
    if ((token = read_token(p)) == NULL || parser_get_error(p) != ec_noerr) {
        goto exit;
    }
    if (tok_isidentifier(token)) {
        if (p->state == ps_fdecl) {
            parser_buf_push(p, pb_declarator_id, token);
        }
        else if (p->state == ps_paramtypelist) {
            parser_buf_push(p, pb_paramtypelist, " ");
            parser_buf_push(p, pb_paramtypelist, token);
        }
        else {
            ASSERTF(
                0,
                "Invalid parser state in direct declarator: %d\n", p->state
            );
            goto exit;
        }

        consume_token(token);
        /* Parse parameter-type-list, identifier-list, ... if it exists */
        if ((token = read_token(p)) == NULL || parser_get_error(p) != ec_noerr) {
            goto exit;
        }
        if (strequ(token, "(") || strequ(token, "[")) {
            /* Should have same closing bracket */
            char end_bracket = token[0] == '(' ? ')' : ']';
            consume_token(token);

            parse_paramtypelist(p);

            /* Match closing bracket */
            if ((token = read_token(p)) == NULL || parser_get_error(p)!= ec_noerr) {
                ERRMSGF("Expected closing bracket in direct-declarator " TOKEN_COLOR "%c\n", end_bracket);
                parser_set_error(p, ec_syntaxerr);
                goto exit;
            }
            if (strlength(token) != 1 || token[0] != end_bracket) {
                ERRMSGF("Mismatched brackets in direct-declarator. Expected: " TOKEN_COLOR "%c", end_bracket);
                ERRMSGF(" Got: " TOKEN_COLOR "%s\n", token);
                parser_set_error(p, ec_syntaxerr);
                goto exit;
            }
            consume_token(token);
        }
    }

exit:
    DEBUG_PARSE_FUNC_END();
}

/* pointer */
static void parse_pointer(parser* p) {
    DEBUG_PARSE_FUNC_START(pointer);
    char* token;
    if ((token = read_token(p)) == NULL || parser_get_error(p) != ec_noerr) {
        goto exit;
    }

    while (strequ(token, "*")) {
        if (p->state == ps_fdecl) {
            /* More convenient to just include * as part of type than declarator */
            parser_buf_push(p, pb_declspec_type, "*");
        }
        else if (p->state == ps_paramtypelist) {
            parser_buf_push(p, pb_paramtypelist, "*");
        }
        else {
            ASSERTF(
                0,
                "Invalid parser state in pointer: %d\n",
                p->state
            );
            goto exit;
        }

        consume_token(token);
        if ((token = read_token(p)) == NULL || parser_get_error(p) != ec_noerr) {
            goto exit;
        }
    }

exit:
    DEBUG_PARSE_FUNC_END();
}

/* parameter-type-list */
static void parse_paramtypelist(parser* p) {
    DEBUG_PARSE_FUNC_START(parameter-type-list);
    p->state = ps_paramtypelist;
    parse_paramlist(p);
    p->state = ps_fdecl;
    DEBUG_PARSE_FUNC_END();
}

/* parameter-list */
static void parse_paramlist(parser* p) {
    DEBUG_PARSE_FUNC_START(parameter-list);
    /* parameter-declaration */
    /* parameter-list , parameter-declaration */
    while (1) {
        parse_paramdecl(p);

        char* token;
        if ((token = read_token(p)) == NULL || parser_get_error(p) != ec_noerr) {
            break;
        }
        if (!strequ(token, ",")) {
            break;
        }
        parser_buf_push(p, pb_paramtypelist, ",");
        consume_token(token);
    }
    DEBUG_PARSE_FUNC_END();
}

/* parameter-declaration */
static void parse_paramdecl(parser* p) {
    DEBUG_PARSE_FUNC_START(parameter-declaration);
    /* declspec, declarator */
    parse_declspec(p);
    if (parser_get_error(p) != ec_noerr) {
        goto exit;
    }
    parse_declarator(p);

exit:
    DEBUG_PARSE_FUNC_END();
}

/* initializer */
static void parse_initializer(parser* p) {
    DEBUG_PARSE_FUNC_START(initializer);

    parse_assignmentexpr(p);

    DEBUG_PARSE_FUNC_END();
}

/* statement */
static void parse_stat(parser* p) {
    DEBUG_PARSE_FUNC_START(statement);
    parse_jumpstat(p);
    DEBUG_PARSE_FUNC_END();
}

/* compound statement */
static void parse_compoundstat(parser* p) {
    DEBUG_PARSE_FUNC_START(compound-statement);
    char* token;
    if ((token = read_token(p)) == NULL || parser_get_error(p) != ec_noerr) {
        goto exit;
    }

    if (token[0] == '{') {
        consume_token(token);
        parser_output_il(p,
            "func %s,%s,%s\n",
            parser_buf_rd(p, pb_declarator_id),
            parser_buf_rd(p, pb_declspec_type),
            parser_buf_rd(p, pb_paramtypelist)
        );
        parser_buf_clear(p);
        if (parser_get_error(p) != ec_noerr) {
            goto exit;
        }
        parse_blockitemlist(p);
    }

exit:
    DEBUG_PARSE_FUNC_END();
}

/* block-item-list */
static void parse_blockitemlist(parser* p) {
    DEBUG_PARSE_FUNC_START(block-item-list);
    /* block-item-list ends on } matching original scope reached */
    while (1) {
        char* token;
        if ((token = read_token(p)) == NULL || parser_get_error(p) != ec_noerr) {
            goto exit;
        }
        if (strequ(token, "}")) {
            consume_token(token);
            goto exit;
        }
        parse_blockitem(p);
    }

exit:
    DEBUG_PARSE_FUNC_END();
}

/* block-item */
static void parse_blockitem(parser* p) {
    DEBUG_PARSE_FUNC_START(block-item);
    /* TODO declaration OR statement, not both */
    parse_decl(p);
    parse_stat(p);
    DEBUG_PARSE_FUNC_END();
}

/* jump-statement */
static void parse_jumpstat(parser* p) {
    DEBUG_PARSE_FUNC_START(jump-statement);
    char* token;
    if ((token = read_token(p)) == NULL || parser_get_error(p) != ec_noerr) {
        goto exit;
    }

    if (strequ(token, "return")) {
        consume_token(token);
        parse_expr(p);
        int has_semicolon = parse_expecttoken(p, ";");
        if (parser_has_error(p)) goto exit;
        if (!has_semicolon) {
            ERRMSG("Expected semicolon after return expression\n");
            parser_set_error(p, ec_syntaxerr);
        }

        parser_output_il(p,
            "ret %s\n",
            parser_buf_rd(p, pb_op1)
        );
        parser_buf_clear(p);
    }

exit:
    DEBUG_PARSE_FUNC_END();
}

/* Return 1 if next token read matches provided token, 0 otherwise */
/* The token is consumed if it matches the provided token */
static int parse_expecttoken(parser * p, const char* match_token) {
    char* token;
    if ((token = read_token(p)) == NULL || parser_get_error(p) != ec_noerr) {
        return 0;
    }

    if (strequ(token, match_token)) {
        consume_token(token);
        return 1;
    }
    return 0;
}

static void parse(parser* p) {
    /* Function definition */
    /* declaration-specifiers declarator declaration-list(opt) compound-statement */
    parse_declspec(p);
    if (parser_get_error(p) != ec_noerr) {
        return;
    }
    parse_declarator(p);
    if (parser_get_error(p) != ec_noerr) {
        return;
    }
    parse_compoundstat(p);
    if (parser_get_error(p) != ec_noerr) {
        return;
    }
}

/* Parses cli args and processes them */
/* NOTE: will not clean up file handles at exit */
/* Returns non zero if error */
static int handle_cli_arg(parser* p, int argc, char** argv) {
    int rt_code = 0;
    /* Skip first argv since it is path */
    for (int i = 1; i < argc; ++i) {
        if (strequ(argv[i], "-o")) {
            if (p->of != NULL) {
                ERRMSG("Only one output file can be specified\n");
                rt_code = 1;
                break;
            }

            ++i;
            if (i >= argc) {
                ERRMSG("Expected output file path after -o\n");
                rt_code = 1;
                break;
            }
            p->of = fopen(argv[i], "w");
            if (p->of == NULL) {
                ERRMSGF("Failed to open output file" TOKEN_COLOR " %s\n", argv[i]);
                rt_code = 1;
                break;
            }
        }
        else {
            if (p->rf != NULL) {
                /* Incorrect flag */
                ERRMSGF("Unrecognized argument" TOKEN_COLOR " %s\n", argv[i]);
                rt_code = 1;
                break;
            }
            p->rf = fopen(argv[i], "r");
            if (p->rf == NULL) {
                ERRMSGF("Failed to open input file" TOKEN_COLOR " %s\n", argv[i]);
                rt_code = 1;
                break;
            }
        }
    }

    return rt_code;
}

int main(int argc, char** argv) {
    int rt_code = 0;
    parser p = {.rf = NULL, .of = NULL};
    p.ecode = ec_noerr;

    rt_code = handle_cli_arg(&p, argc, argv);
    if (rt_code != 0) {
        goto cleanup;
    }
    if (p.rf == NULL) {
        ERRMSG("No input file\n");
        goto cleanup;
    }
    if (p.of == NULL) {
        /* Default to opening imm2 */
        p.of = fopen("imm2", "w");
        if (p.of == NULL) {
            ERRMSG("Failed to open output file\n");
            rt_code = 1;
            goto cleanup;
        }
    }

    parse(&p);
    if (p.ecode != ec_noerr) {
        LOGF("Error during parsing: %d\n", p.ecode);
    }

cleanup:
    if (p.rf != NULL) {
        fclose(p.rf);
    }
    if (p.of != NULL) {
        fclose(p.of);
    }
    return rt_code;
}

