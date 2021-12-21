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

#define LOG(msg__) printf("%s", msg__);
#define LOGF(...) printf(__VA_ARGS__);

#define MAX_INSTRUCTION_LEN 256
#define MAX_ARG_LEN 2048   /* Excludes null terminator */
#define MAX_ARGS 256
#define MAX_SCOPE_DEPTH 16 /* Max number of scopes */
#define MAX_SCOPE_LEN 50   /* Max symbols per scope (simple for now, may be replaced with dynamic array in future) */


/* Own implementation of library functions */
/* The plan is to not have to use external headers when self compiling */

/* Return 1 if strings are equal, 0 if not */
static int strequ(const char* s1, const char* s2) {
    int i = 0;
    char c;
    while ((c = s1[i]) != '\0') {
        if (s1[i] != s2[i]) {
            return 0;
        }
        ++i;
    }
    /* Ensure both strings terminated */
    if (s2[i] == '\0') {
        return 1;
    }
    else {
        return 0;
    }
}


/* Compiler specific */


typedef enum {
    ec_noerr = 0,
    ec_insbufexceed,
    ec_argbufexceed,
    ec_scopedepexceed,
    ec_scopelenexceed,
    ec_invalidins,
    ec_invalidinsop,
    ec_badargs,
    ec_badmain,
    ec_writefailed
} errcode;

typedef enum {
    ps_rdins = 0,
    ps_rdarg
} pstate;

/* Data for parser */

/* All the registers in x86-64 (just 8 byte ones for now) */
typedef enum {
    rax = 0,
    rbc,
    rcx,
    rdx,
    rsi,
    rdi,
    rbp,
    rsp,
    r8,
    r9,
    r10,
    r11,
    r12,
    r13,
    r14,
    r15
} asm_register;

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

/* Sets up a new symbol scope*/
/* Error code returned via ecode pointer argument, otherwise ecode is unaffected */
static void parser_new_scope(parser* p, errcode* ecode) {
    if (p->i_scope + 1 >= MAX_SCOPE_DEPTH) {
        *ecode = ec_scopedepexceed;
        return;
    }
    /* Scope may have been previously used, reset index for symbol */
    p->i_symbol[++p->i_scope] = -1;
}

/* Acquires and returns memory for new symbol of current scope, contents of returned memory undefined */
/* Error code returned via ecode pointer argument, otherwise ecode is unaffected */
/* if an error occurred returned pointer is NULL */
static il_symbol* parser_sym_alloc(parser* p, errcode* ecode) {
    int i_scope = p->i_scope;
    if (p->i_symbol[i_scope] + 1 >= MAX_SCOPE_LEN) {
        *ecode = ec_scopelenexceed;
        return NULL;
    }
    return &p->symbol[i_scope][++p->i_symbol[i_scope]];
}


/* Writes provided assembly using format string and va_args into output */
/* Error code returned via ecode pointer argument, otherwise ecode is unaffected */
static void parser_output_asm(parser* p, errcode* ecode, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (vfprintf(p->of, fmt, args) < 0) {
        *ecode = ec_writefailed;
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


/* Alphabetical, short to long (important!) */
#define INSTRUCTIONS  \
    INSTRUCTION(func) \
    INSTRUCTION(mov)  \
    INSTRUCTION(ret)
typedef errcode(*instruction_handler) (parser*, char**, int);
/* pparg is pointer to array of size, each element is pointer to null terminated argument string */
#define INSTRUCTION_HANDLER(name__) errcode handler_ ## name__ (parser* p, char** pparg, int arg_count)

static INSTRUCTION_HANDLER(func) {
    LOG("func\n");
    for (int i = 0; i < arg_count; ++i) {
        LOGF("%s\n", pparg[i]);
    }

    if (arg_count < 2) {
        return ec_badargs;
    }
    errcode ecode = ec_noerr;

    /* Special handling of main function */
    if (strequ(pparg[0], "main")) {
        if (arg_count != 4) {
            return ec_badmain;
        }

        /* Load symbols argc argv*/
        parser_new_scope(p, &ecode);
        if (ecode != ec_noerr) {
            return ecode;
        }
        il_symbol* argc_sym = parser_sym_alloc(p, &ecode);
        if (ecode != ec_noerr) {
            return ecode;
        }
        il_symbol* argv_sym = parser_sym_alloc(p, &ecode);
        if (ecode != ec_noerr) {
            return ecode;
        }

        /* TODO set registers */
        extract_type_name(pparg[2], argc_sym->name);
        extract_type_name(pparg[3], argv_sym->name);

        /* Generate a _start function which calls main */
        parser_output_asm(p, &ecode, "%s"
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
        if (ecode != ec_noerr) {
            return ecode;
        }
    }

    /* Function labels always with prefix f@ */
    parser_output_asm(p, &ecode, "f@%s:\n", pparg[0]);
    if (ecode != ec_noerr) {
        return ecode;
    }

    return ecode;
}
static INSTRUCTION_HANDLER(mov) {
    LOG("mov\n");
    return 0;
}
static INSTRUCTION_HANDLER(ret) {
    LOG("ret\n");
    return 0;
}

/* Index of string is instruction handler index */
#define INSTRUCTION(name__) #name__,
const char* instruction_handler_index[] = {INSTRUCTIONS};
#undef INSTRUCTION
#define INSTRUCTION(name__) &handler_ ## name__,
const instruction_handler instruction_handler_table[] = {INSTRUCTIONS};
#undef INSTRUCTION
#define INSTRUCTION_COUNT (int)(sizeof(instruction_handler_index) / sizeof(instruction_handler_index[0]))

static errcode handle_instruction(parser* p, char* ins, int ins_len, char** arg, int arg_count) {
    int front = 0; /* Inclusive */
    int rear = INSTRUCTION_COUNT; /* Exclusive */
    /* Binary search for instruction */
    while (front != rear && rear != 0 && front < INSTRUCTION_COUNT) {
        int ins_i = (front+rear) / 2;
        const char* found = instruction_handler_index[ins_i];
        int match = 1;
        for (int i = 0; i < ins_len; ++i) {
            if (found[i] == '\0') {
                front = ins_i + 1;
                match = 0;
                break;
            }
            if (ins[i] < found[i]) {
                rear = ins_i;
                match = 0;
                break;
            }
            else if (ins[i] > found[i]) {
                front = ins_i + 1;
                match = 0;
                break;
            }
        }
        /* Ensure that the found instruction is same length (not just the prefix which matches) */
        if (match && found[ins_len] != '\0') {
            match = 0;
            rear = ins_i;
        }
        if (match) {
            LOGF("matched IL instruction %s\n", found);

            return instruction_handler_table[ins_i](p, arg, arg_count);
        }
        LOGF("%d %d %d\n", front, rear, ins_i);
    }

    return ec_invalidins;
}

static errcode parse(parser* p) {
    errcode rt_code = ec_noerr;

    char ins[MAX_INSTRUCTION_LEN];
    int i_ins = 0; /* Points to end of ins buffer */
    char arg[MAX_ARG_LEN + 1]; /* Needs to be 1 longer for final null terminator */
    int i_arg = 0; /* Points to end of arg buffer */

    pstate state = ps_rdins;
    char ch;
    while((ch = getc(p->rf)) != EOF) {
        if (state == ps_rdins) {
            if (i_ins == MAX_INSTRUCTION_LEN) {
                rt_code = ec_insbufexceed;
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
                rt_code = ec_argbufexceed;
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
                rt_code = handle_instruction(p, ins, i_ins, arg_table, arg_count);

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
        if (rt_code != ec_noerr) {
            return rt_code;
        }
    }
    return rt_code;
}

int main(int argc, char** argv) {
    int exitcode = 0;

    parser p = {.rf = NULL, .of = NULL, .i_scope = -1}; /* Initially have no active scope */
    p.rf = fopen("imm2", "r");
    if (p.rf == NULL) {
        printf("Failed to open input file\n");
        exitcode = 1;
        goto cleanup;
    }
    p.of = fopen("imm3", "w");
    if (p.of == NULL) {
        printf("Failed to open output file\n");
        exitcode = 1;
        goto cleanup;
    }

    /* Global scope */
    errcode ecode = ec_noerr;
    parser_new_scope(&p, &ecode);
    if (ecode != ec_noerr) {
        goto handle_ecode;
    }
    ecode = parse(&p);

handle_ecode:
    /* Indicate to the user cause for exiting if errored during parsing */
    if (ecode != ec_noerr) {
        LOGF("Error during parsing: %d\n", ecode);
    }
    debug_dump(&p);

cleanup:
    if (p.rf != NULL) {
        fclose(p.rf);
    }
    if (p.of != NULL) {
        fclose(p.of);
    }
    return exitcode;
}

