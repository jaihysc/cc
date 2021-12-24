/* Preprocessed c file parser */
/* Generated output is imm2 */

#include <stdio.h>
#include <stdarg.h>
#include "common.h"

#define MAX_TOKEN_LEN 255 /* Excluding null terminator, Tokens is string with no whitespace */
#define MAX_PARSER_BUFFER_LEN 255 /* Excluding null terminator */

#define TOKEN_COLOR "\033[1;97m" /* Color for tokens when printed */

/* Should always be initialized to ec_noerr */
/* Since functions will only sets if error occurred */
typedef enum {
    ec_noerr,
    ec_tokbufexceed,
    ec_pbufexceed, /* Parser buffer exceeded */
    ec_syntaxerr,
    ec_writefailed
} errcode;

/* Indicates what parser currently reading */
typedef enum {
    ps_declspec, /* Reading any declaration specifier */
    ps_declspec_type, /* Type specifiers in declaration-specifiers */
    ps_declarator,
    /* Function declaration list */
    ps_fdecllist,
    ps_fdecllist_pend, /* End of parameter */
    ps_fdecllist_term, /* Declaration list ended, expect ; or } */
    ps_fbody, /* Function body */
} pstate;

/* The text buffers of the parser */
/* Holds text until enough information is known to generate an il instruction */
typedef enum {
    pb_declspec_type = 0,
    pb_declarator,
    pb_fdecllist,
    pb_count, /* Count of entries in enum */
} pbuffer;

typedef struct {
    FILE* rf; /* Input file */
    FILE* of; /* Output file */

    /* Token read buffer, used by read_token to hold extra characters read */
    /* Since in order to determine the end of a token, it must read 1 past */
    char rf_buf[1];
    int i_rf_buf; /* Index 1 past end in rf_buf */

    char buf[pb_count][MAX_PARSER_BUFFER_LEN + 1];
    int i_buf[pb_count]; /* Index 1 past end in buf */
} parser;

/* Reads contents of parser buffer specified by target */
/* Read is null terminated string */
static char* parser_buf_rd(parser* p, pbuffer target) {
    return p->buf[target];
}

/* Adds null terminated string into buffer specified by target */
/* The null terminator is not included */
/* ecode is set if error occurred, otherwise ecode unmodified */
/* Can be called again even if error occurred previously */
static void parser_buf_push(parser* p, errcode* ecode, pbuffer target, const char* str) {
    int i = 0;
    char c;
    while ((c = str[i]) != '\0') {
        if (p->i_buf[target] >= MAX_PARSER_BUFFER_LEN) {
            *ecode = ec_pbufexceed;
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
/* ecode is set if error occurred, otherwise ecode unmodified */
static void parser_output_il(parser* p, errcode* ecode, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (vfprintf(p->of, fmt, args) < 0) {
        *ecode = ec_writefailed;
    }
    va_end(args);
}

/* Prints out contents of parser buffers */
static void debug_parser_buf_dump(parser* p) {
    for (int i = 0; i < pb_count; ++i) {
        LOGF("%d | %s\n", i, p->buf[i]);
    }
}


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
/* Returns 1 if token c string is a type keyword, 0 otherwise */
static int tok_istype(const char* str) {
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

/* Reads a null terminated token into buf */
/* ecode is set if error occurred, otherwise ecode unmodified */
/* Returns 0 if EOF, 1 otherwise */
static int read_token(parser* p, errcode* ecode, char* buf) {
    int i = 0;
    int seen_token = 0;
    char c;
    while (1) {
        if (i >= MAX_TOKEN_LEN) {
            *ecode = ec_tokbufexceed;
            break; /* Since MAX_TOKEN_LEN does no include null terminator, we can break and add null terminator */
        }
        /* Read character out of buffer first from previous read_token calls */
        if (p->i_rf_buf > 0) {
            --p->i_rf_buf;
            c = p->rf_buf[p->i_rf_buf];
        }
        else {
            c = getc(p->rf);
        }
        if (c == EOF) {
            break;
        }

        if (iswhitespace(c)) {
            if (seen_token) {
                break;
            }
            /* Skip leading whitespace */
            continue;
        }
        /* TODO incomplete list of token chars */
        switch (c) {
            case '(':
            case ')':
            case '{':
            case '&':
            case '*':
            case ',':
            case ';':
                /* Return final character read since it is token, for next call */
                if (seen_token) {
                    p->rf_buf[0] = c; /* We only need to hold 1 character for now */
                    p->i_rf_buf++;
                    goto while_exit; /* Break the while */
                }
                else {
                    /* A token char itself is a token */
                    buf[i] = c;
                    ++i;
                    goto while_exit;
                }
            default:
                break;
        }
        seen_token = 1;
        buf[i] = c;
        ++i;
    }
while_exit:
    buf[i] = '\0';
    return c != EOF;
}

/* ecode is set if error occurred, otherwise ecode unmodified */
static void parse(parser* p, errcode* ecode) {
    pstate state = ps_declspec;
    int reuse_token = 0; /* If 1, do not perform read_token and use existing token */
    char tok_buf[MAX_TOKEN_LEN + 1]; /* This buffer is null terminated */
    while (1) {
        if (!reuse_token && !read_token(p, ecode, tok_buf)) {
            break;
        }
        if (*ecode != ec_noerr) {
            return;
        }
        reuse_token = 0;

        if (state == ps_declspec) {
            int sc = tok_isstoreclass(tok_buf);
            int type = tok_istype(tok_buf);
            int type_qual = tok_istypequal(tok_buf);
            int func_spec = tok_isfuncspec(tok_buf);
            if (sc) {
                LOG("storage class!\n");
            }
            else if (type) {
                state = ps_declspec_type;
                reuse_token = 1;
            }
            else if (type_qual) {
                LOG("type qualifier!\n");
            }
            else if (func_spec) {
                LOG("function specifier!\n");
            }
            else {
                state = ps_declarator;
                reuse_token = 1;
            }
        }
        /* Types can be multiple keywords long so it handled separately */
        else if (state == ps_declspec_type) {
            if (tok_istype(tok_buf)) {
                LOG("type!\n");
                parser_buf_push(p, ecode, pb_declspec_type, tok_buf);
            }
            else {
                state = ps_declspec;
                reuse_token = 1;
            }
        }
        else if (state == ps_declarator) {
            if (tok_isidentifier(tok_buf)) {
                LOG("Declarator!\n");
                parser_buf_push(p, ecode, pb_declarator, tok_buf);
            }
            else if (strequ(tok_buf, "(")) {
                state = ps_fdecllist;
            }
            else {
                ERRMSGF("Expected declarator, got " TOKEN_COLOR "%s\n", tok_buf);
                *ecode = ec_syntaxerr;
            }
        }
        else if (state == ps_fdecllist) {
            if (tok_istype(tok_buf) || strequ(tok_buf, "*")) {
                LOG("Function parameter type!\n");
                parser_buf_push(p, ecode, pb_fdecllist, tok_buf);
            }
            else if (tok_isidentifier(tok_buf)) {
                LOG("Function parameter name!\n");
                parser_buf_push(p, ecode, pb_fdecllist, " ");
                parser_buf_push(p, ecode, pb_fdecllist, tok_buf);
                state = ps_fdecllist_pend;
            }
            else if (strequ(tok_buf, ")")) { /* Empty param list */
                state = ps_fdecllist_term;
            }
            else {
                ERRMSGF(
                    "Expected parameter type or name, got " TOKEN_COLOR "%s\n",
                    tok_buf
                );
                *ecode = ec_syntaxerr;
            }
        }
        else if (state == ps_fdecllist_pend) {
            if (strequ(tok_buf, ",")) {
                parser_buf_push(p, ecode, pb_fdecllist, ",");
                state = ps_fdecllist;
            }
            else if (strequ(tok_buf, ")")) {
                state = ps_fdecllist_term;
            }
            else {
                ERRMSGF(
                    "Unrecognized token after function parameter " TOKEN_COLOR "%s\n",
                    tok_buf
                );
                *ecode = ec_syntaxerr;
            }
        }
        else if (state == ps_fdecllist_term) {
            if (strequ(tok_buf, ";")) {
                LOG("Function prototype!\n");
                state = ps_declspec;
            }
            else if (strequ(tok_buf, "{")) {
                LOG("Function body!\n");
                parser_output_il(p, ecode,
                    "func %s,%s,%s\n",
                    parser_buf_rd(p, pb_declarator),
                    parser_buf_rd(p, pb_declspec_type),
                    parser_buf_rd(p, pb_fdecllist)
                );
                parser_buf_clear(p);
                state = ps_fbody;
            }
            else {
                ERRMSGF(
                    "Unrecognized token after function parameter list " TOKEN_COLOR "%s\n",
                    tok_buf
                );
                *ecode = ec_syntaxerr;
            }
        }
        else if (state == ps_fbody) {
        }

        LOGF("  %s\n", tok_buf);
        debug_parser_buf_dump(p);
        /* Error may have occurred during parsing */
        if (*ecode != ec_noerr) {
            return;
        }
    }
}

int main(int argc, char** argv) {
    int rt_code = 0;
    parser p = {.rf = NULL, .of = NULL, .i_rf_buf = 0};

    p.rf = fopen("imm1", "r");
    if (p.rf == NULL) {
        LOG("Failed to open input file\n");
        rt_code = 1;
        goto cleanup;
    }
    p.of = fopen("imm2", "w");
    if (p.of == NULL) {
        LOG("Failed to open output file\n");
        rt_code = 1;
        goto cleanup;
    }

    errcode ecode = ec_noerr;
    parse(&p, &ecode);
    if (ecode != ec_noerr) {
        LOGF("Error during parsing: %d\n", ecode);
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

