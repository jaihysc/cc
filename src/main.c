/* Entry point of compiler */

#include <stdio.h>
#include <stdarg.h>
#include "common.h"

#define MAX_TOKEN_LEN 255 /* Excluding null terminator, Tokens is string with no whitespace */
#define MAX_PARSE_TREE_NODE 2000 /* Maximum nodes in parser parse tree */
#define MAX_PARSE_NODE_CHILD 4 /* Maximum children nodes for a parse tree node */
#define MAX_TOKEN_BUFFER_CHAR 8192 /* Max characters in token buffer */
#define MAX_SCOPES 32 /* Max number of scopes */
#define MAX_SCOPE_LEN 500   /* Max symbols per scope */

/* Global configuration */

int g_debug_print_buffers = 0;
int g_debug_print_cg_recursion = 0;
int g_debug_print_parse_recursion = 0;
int g_debug_print_parse_tree = 0;
int g_debug_print_symtab = 0;

/* Include the C file back in for now for compiler to work */
#include "compiler.c"
#include "lexer.c"
#include "ilgen.c"
#include "parser.c"

/* ============================================================ */
/* Initialization and configuration */

/* The index of the option's string in the string array
   is the index of the pointer for the variable corresponding to the option

   SWITCH_OPTION(option string, variable to set)
   Order by option string, see strbinfind for ordering requirements */
#define SWITCH_OPTIONS                                                    \
    SWITCH_OPTION(-dprint-buffers, g_debug_print_buffers)                 \
    SWITCH_OPTION(-dprint-cg-recursion, g_debug_print_cg_recursion)       \
    SWITCH_OPTION(-dprint-parse-recursion, g_debug_print_parse_recursion) \
    SWITCH_OPTION(-dprint-parse-tree, g_debug_print_parse_tree)           \
    SWITCH_OPTION(-dprint-symtab, g_debug_print_symtab)

#define SWITCH_OPTION(str__, var__) #str__,
const char* option_switch_str[] = {SWITCH_OPTIONS};
#undef SWITCH_OPTION
#define SWITCH_OPTION(str__, var__) &var__,
int* option_switch_value[] = {SWITCH_OPTIONS};
#undef SWITCH_OPTION

/* Parses cli args and processes them */
/* NOTE: will not clean up file handles at exit */
/* Returns non zero if error */
static int handle_cli_arg(Parser* p, int argc, char** argv) {
    int rt_code = 0;
    /* Skip first argv since it is path */
    for (int i = 1; i < argc; ++i) {
        /* Handle switch options */
        int i_switch = strbinfind(
                argv[i],
                strlength(argv[i]),
                option_switch_str,
                ARRAY_SIZE(option_switch_str));
        if (i_switch >= 0) {
            *option_switch_value[i_switch] = 1;
            continue;
        }

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
    Parser p = {.rf = NULL, .of = NULL, .line_num = 1, .char_num = 1};
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

    symtab_push_scope(&p);

    while (1) {
        if (!parse_function_definition(&p, &p.parse_node_root)) {
            parser_set_error(&p, ec_syntaxerr);
            ERRMSGF("Failed to build parse tree. Line %d, around char %d\n",
                    p.last_line_num, p.last_char_num);
            break;
        }
        if (*read_token(&p) == '\0') {
            break;
        }
        tree_detach_node_child(&p, &p.parse_node_root);
    }

    symtab_pop_scope(&p);
    ASSERT(p.i_scope == 0, "Scopes not empty on parse end");
    for (int i = 0; i < sc_count; ++i) {
        ASSERTF(p.i_symtab_cat[i] == 0,
                "Symbol category stack %d not empty on parse end", i);
    }

    /* Debug options */
    if (g_debug_print_parse_tree) {
        LOG("Remaining ");
        debug_print_parse_tree(&p);
    }

    if (parser_has_error(&p)) {
        ErrorCode ecode = parser_get_error(&p);
        ERRMSGF("Error during parsing: %d %s\n", ecode, errcode_str[ecode]);
        rt_code = ecode;
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

