/*
 Reads in intermediate language (imm2)
 Generated output x86-64 assembly (imm3)
*/

/*
 Discard the curly braces at the very end
 Create a stack for the scopes, do something on scope end
 Create a context struct (current context (scope, etc))
*/

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include "common.h"

#define MAX_INSTRUCTION_LEN 256
#define MAX_ARG_LEN 2048   /* Excludes null terminator */
#define MAX_ARGS 256
#define MAX_SCOPE_DEPTH 16 /* Max number of scopes */
#define MAX_SCOPE_LEN 50   /* Max symbols per scope (simple for now, may be replaced with dynamic array in future) */

#define ERROR_CODES            \
    ERROR_CODE(noerr)          \
    ERROR_CODE(insbufexceed)   \
    ERROR_CODE(argbufexceed)   \
    ERROR_CODE(scopedepexceed) \
    ERROR_CODE(scopelenexceed) \
    ERROR_CODE(invalidins)     \
    ERROR_CODE(invalidinsop)   \
    ERROR_CODE(badargs)        \
    ERROR_CODE(badmain)        \
    ERROR_CODE(writefailed)    \
    ERROR_CODE(unknownsym)

#define ERROR_CODE(name__) ec_ ## name__,
typedef enum {ERROR_CODES} errcode;
#undef ERROR_CODE
#define ERROR_CODE(name__) #name__,
char* errcode_str[] = {ERROR_CODES};
#undef ERROR_CODE
#undef ERROR_CODES

typedef enum {
    ps_rdins = 0,
    ps_rdarg
} pstate;

/* Data for parser */

/* All the registers in x86-64 (just 8 byte ones for now) */
#define X86_REGISTERS \
    X86_REGISTER(rax) \
    X86_REGISTER(rbx) \
    X86_REGISTER(rcx) \
    X86_REGISTER(rdx) \
    X86_REGISTER(rsi) \
    X86_REGISTER(rdi) \
    X86_REGISTER(rbp) \
    X86_REGISTER(rsp) \
    X86_REGISTER(r8)  \
    X86_REGISTER(r9)  \
    X86_REGISTER(r10) \
    X86_REGISTER(r11) \
    X86_REGISTER(r12) \
    X86_REGISTER(r13) \
    X86_REGISTER(r14) \
    X86_REGISTER(r15)

#define X86_REGISTER(reg__) asm_ ## reg__,
typedef enum {X86_REGISTERS} asm_register;
#undef X86_REGISTER
#define X86_REGISTER(reg__) #reg__ ,
const char* asm_register_strings[] = {X86_REGISTERS};
#undef X86_REGISTER

/* Converts given asm_register into its corresponding cstr*/
static const char* asm_register_str(asm_register reg) {
    return asm_register_strings[reg];
}

/* Location of symbol */
typedef struct {
    asm_register reg;
    int isptr; /* Register holds value if 0, register points to value if 1 and offset may be used*/
    uint64_t offset; /* Offset applied to register if reg is pointer */
} il_symloc;

typedef struct {
    char name[MAX_ARG_LEN + 1]; /* +1 for null terminator */
    il_symloc loc;
} il_symbol;

typedef struct {
    errcode ecode;

    FILE* rf; /* Input file */
    FILE* of; /* Generated code goes in this file */

    /* [Array of scopes][Array of symbols in each scope] */
    /* First scope element is highest scope (most global) */
    /* First symbol element is earliest in occurrence */
    il_symbol symbol[MAX_SCOPE_DEPTH][MAX_SCOPE_LEN];

    /*
     Point to last so createing new scope,symbol is one function, then caller code, otherwise if pointed one past:
        create new -> caller code to initialize new -> increment index
    */
    int i_scope; /* Index of latest(deepest) scope. */
    int i_symbol[MAX_SCOPE_DEPTH]; /* Index of latest symbol for each scope */
} parser;

/* Return 1 if error is set, else 0 */
static int parser_has_error(parser* p) {
    return p->ecode != ec_noerr;
}

/* Gets error in parser */
static errcode parser_get_error(parser* p) {
    return p->ecode;
}

/* Sets error in parser */
static void parser_set_error(parser* p, errcode ecode) {
    p->ecode = ecode;
    LOGF("Error set %d\n", ecode);
}

/* Sets up a new symbol scope*/
static void parser_new_scope(parser* p) {
    if (p->i_scope + 1 >= MAX_SCOPE_DEPTH) {
        parser_set_error(p, ec_scopedepexceed);
        return;
    }
    /* Scope may have been previously used, reset index for symbol */
    p->i_symbol[++p->i_scope] = -1;
}

/* Acquires and returns memory for new symbol of current scope, contents of returned memory undefined */
/* if an error occurred returned pointer is NULL */
static il_symbol* parser_sym_alloc(parser* p) {
    int i_scope = p->i_scope;
    if (p->i_symbol[i_scope] + 1 >= MAX_SCOPE_LEN) {
        parser_set_error(p, ec_scopelenexceed);
        return NULL;
    }
    return &p->symbol[i_scope][++p->i_symbol[i_scope]];
}

/* Looks for symbol with given cstr name from deepest scope first*/
/* Return NULL if not found */
static il_symbol* parser_sym_lookup(parser* p, const char* name) {
    for (int i_scope = p->i_scope; i_scope >= 0; --i_scope) {
        for (int i_sym = 0; i_sym <= p->i_symbol[i_scope]; ++i_sym) {
            il_symbol* sym = &p->symbol[i_scope][i_sym];
            if (strequ(sym->name, name)) {
                return sym;
            }
        }
    }
    return NULL;
}


/* Writes provided assembly using format string and va_args into output */
static void parser_output_asm(parser* p, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (vfprintf(p->of, fmt, args) < 0) {
        parser_set_error(p, ec_writefailed);
    }
    va_end(args);
}
/* Extracts the type and the name from <arg>ts stored in cstr */
/* Ensure out_name is of sufficient size to hold all names which can be contained in arguments */
/* Currently only extracts the name */
static void extract_type_name(const char* arg, char* out_name) {
    char c;
    int i = 0;
    /* Extract type */
    while ((c = arg[i]) != ' ' && c != '\0') {
        ++i;
    }
    if (c == '\0') { /* No name provided */
        out_name[0] = '\0';
        return;
    }
    /* Extract name */
    ++i; /* Skip the space which was encountered */
    int i_out = 0;
    while ((c = arg[i]) != '\0') {
        out_name[i_out] = arg[i];
        ++i;
        ++i_out;
    }
    out_name[i_out] = '\0';
}

/* Dumps contents stored in parser */
static void debug_dump(parser* p) {
    LOGF("Scopes: [%d]\n", p->i_scope + 1);
    for (int i = 0; i <= p->i_scope; ++i) {
        LOGF("  [%d]\n", p->i_symbol[i] + 1);
        for (int j = 0; j <= p->i_symbol[i]; ++j) {
            LOGF("    %s\n", p->symbol[i][j].name);
        }
    }
}


/* Instruction handlers */


/* See strbinfind for ordering requirements */
#define INSTRUCTIONS  \
    INSTRUCTION(func) \
    INSTRUCTION(mov)  \
    INSTRUCTION(ret)
typedef void(*instruction_handler) (parser*, char**, int);
/* pparg is pointer to array of size, each element is pointer to null terminated argument string */
#define INSTRUCTION_HANDLER(name__) void handler_ ## name__ (parser* p, char** pparg, int arg_count)

static INSTRUCTION_HANDLER(func) {
    for (int i = 0; i < arg_count; ++i) {
        LOGF("%s\n", pparg[i]);
    }

    if (arg_count < 2) {
        parser_set_error(p, ec_badargs);
        goto exit;
    }

    /* Special handling of main function */
    if (strequ(pparg[0], "main")) {
        if (arg_count != 4) {
            parser_set_error(p, ec_badmain);
            goto exit;
        }

        /* Load symbols argc argv*/
        parser_new_scope(p);
        if (parser_has_error(p)) goto exit;
        il_symbol* argc_sym = parser_sym_alloc(p);
        if (parser_has_error(p)) goto exit;
        il_symbol* argv_sym = parser_sym_alloc(p);
        if (parser_has_error(p)) goto exit;

        extract_type_name(pparg[2], argc_sym->name);
        extract_type_name(pparg[3], argv_sym->name);

        argc_sym->loc.reg = asm_rdi;
        argv_sym->loc.reg = asm_rsi;
        argv_sym->loc.isptr = 1;

        /* Generate a _start function which calls main */
        parser_output_asm(p, "%s",
            "\n"
            "    global _start\n"
            "_start:\n"
            "    mov             rdi, QWORD [rsp]\n" /* argc */
            "    mov             rsi, QWORD [rsp+8]\n" /* argv */
            "    call            f@main\n"
            "    mov             rdi, rax\n" /* exit call */
            "    mov             rax, 60\n"
            "    syscall\n"
        );
        if (parser_has_error(p)) goto exit;
    }

    /* Function labels always with prefix f@ */
    parser_output_asm(p, "f@%s:\n", pparg[0]);

exit:
    return;
}

static INSTRUCTION_HANDLER(mov) {
    return;
}
static INSTRUCTION_HANDLER(ret) {
    if (arg_count != 1) {
        parser_set_error(p, ec_badargs);
        goto exit;
    }

    il_symbol* sym = parser_sym_lookup(p, pparg[0]);
    if (sym == NULL) {
        parser_set_error(p, ec_unknownsym);
        goto exit;
    }
    parser_output_asm(p,
        "    mov             rax, %s\n" /* Return integer types in rax */
        "    ret\n",
        asm_register_str(sym->loc.reg)
    );

exit:
    return;
}

/* Index of string is instruction handler index */
#define INSTRUCTION(name__) #name__,
const char* instruction_handler_index[] = {INSTRUCTIONS};
#undef INSTRUCTION
#define INSTRUCTION(name__) &handler_ ## name__,
const instruction_handler instruction_handler_table[] = {INSTRUCTIONS};
#undef INSTRUCTION


static void parse(parser* p) {
    char ins[MAX_INSTRUCTION_LEN];
    int i_ins = 0; /* Points to end of ins buffer */
    char arg[MAX_ARG_LEN + 1]; /* Needs to be 1 longer for final null terminator */
    int i_arg = 0; /* Points to end of arg buffer */

    pstate state = ps_rdins;
    char ch;
    while((ch = getc(p->rf)) != EOF) {
        if (state == ps_rdins) {
            if (i_ins == MAX_INSTRUCTION_LEN) {
                parser_set_error(p, ec_insbufexceed);
                goto exit;
            }
            else if (ch == ' ') {
                state = ps_rdarg;
            }
            else {
                ins[i_ins] = ch;
                i_ins++;
            }
        }
        else if (state == ps_rdarg) {
            if (i_arg == MAX_ARG_LEN) {
                parser_set_error(p, ec_argbufexceed);
                goto exit;
            }
            else if (ch == '\n') {
                LOGF("Instruction buffer:\n%.*s\nArgument buffer:\n%.*s\n", i_ins, ins, i_arg, arg);

                /* Separate each arg into array of pointers below, pass the array */
                int arg_count = 0;
                char* arg_table[MAX_ARGS];
                for (int i = 0; i < i_arg; ++i) {
                    arg_table[arg_count] = arg + i;
                    arg_count++;
                    do {
                        ++i;
                        if (arg[i] == ',') {
                            break;
                        }
                    }
                    while (i < i_arg);
                    arg[i] = '\0';
                }

                /* Run handler for instruction */
                int i_handler = strbinfind(ins, i_ins, instruction_handler_index, ARRAY_SIZE(instruction_handler_index));
                if (i_handler < 0) {
                    parser_set_error(p, ec_invalidins);
                    goto exit;
                }
                instruction_handler_table[i_handler](p, arg_table, arg_count);

                /* Prepare for reading new instruction again */
                i_ins = 0;
                i_arg = 0;
                state = ps_rdins;
            }
            else {
                arg[i_arg] = ch;
                i_arg++;
            }
        }
        /* Check, maybe need to exit in error */
        if (parser_has_error(p)) goto exit;
    }

exit:
    return;
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
                /* Maybe user meant for additional arg to go with flag */
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
    int exitcode = 0;

    parser p = {.rf = NULL, .of = NULL, .i_scope = -1}; /* Initially have no active scope */

    exitcode = handle_cli_arg(&p, argc, argv);
    if (exitcode != 0) {
        goto exit;
    }
    if (p.rf == NULL) {
        ERRMSG("No input file\n");
        goto exit;
    }
    if (p.of == NULL) {
        /* Default to opening imm3 */
        p.of = fopen("imm3", "w");
        if (p.of == NULL) {
            ERRMSG("Failed to open output file\n");
            exitcode = 1;
            goto exit;
        }
    }

    /* Global scope */
    parser_new_scope(&p);
    if (parser_has_error(&p)) goto exit;
    parse(&p);

exit:
    /* Indicate to the user cause for exiting if errored during parsing */
    if (parser_has_error(&p)) {
        errcode ecode = parser_get_error(&p);
        ERRMSGF("Error during parsing: %d %s\n", ecode, errcode_str[ecode]);
    }
    debug_dump(&p);

    if (p.rf != NULL) {
        fclose(p.rf);
    }
    if (p.of != NULL) {
        fclose(p.of);
    }
    return exitcode;
}

