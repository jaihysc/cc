/* Preprocessed c file parser */
/* Generated output is imm2 */

#include <stdio.h>
#include "common.h"

#define MAX_TOKEN_LEN 255 /* Excluding null terminator, Tokens is string with no whitespace */

/* Should always be initialized to ec_noerr */
/* Since functions will only sets if error occurred */
typedef enum {
    ec_noerr,
    ec_tokbufexceed,
    ec_syntaxerr
} errcode;

/* Indicates what parser currently reading */
typedef enum {
    ps_declspec, /* Reading any declaration specifier */
    ps_declspec_type, /* Type specifiers in declaration-specifiers */
    ps_declarator,
    ps_fdecllist, /* Function declaration list */
    ps_fdecllist_type,
    ps_fdecllist_declarator,
    ps_fdecllist_term, /* Declaration list ended, expect ; or } */
    ps_fbody, /* Function body */
} pstate;

typedef struct {
    FILE* rf; /* Input file */
    FILE* of; /* Output file */

    /* Read buffer, used by read_token to hold extra characters read */
    /* Since in order to determine the end of a token, it must read 1 past */
    char rf_buf[1];
    int i_rf_buf; /* Index 1 past end in rf_buf */
} parser;

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
    char tok_buf[MAX_TOKEN_LEN + 1]; /* This buffer is null terminated */
    while (read_token(p, ecode, tok_buf)) {
        if (*ecode != ec_noerr) {
            return;
        }

        if (state == ps_declspec) {
            int sc = tok_isstoreclass(tok_buf);
            int type = tok_istype(tok_buf);
            int type_qual = tok_istypequal(tok_buf);
            int func_spec = tok_isfuncspec(tok_buf);
            if (sc) {
                LOG("storage class!\n");
            }
            else if (type) {
                LOG("type!\n");
                state = ps_declspec_type;
            }
            else if (type_qual) {
                LOG("type qualifier!\n");
            }
            else if (func_spec) {
                LOG("function specifier!\n");
            }
            else {
                *ecode = ec_syntaxerr;
            }
        }
        else if (state == ps_declspec_type) {
            if (tok_istype(tok_buf)) {
                LOG("another type!\n");
            }
            else {
                state = ps_declspec;
            }
        }

        LOGF("%s\n", tok_buf);
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

    // Output something for now so the compilation can be tested
    fprintf(p.of, "%s\n", "func main,i32,i32 argc,u8** argv");

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

