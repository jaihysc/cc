/*
 Reads in intermediate language (imm2)
 Generated output x86-64 assembly (imm3)
*/

/*
 Generate a label with f@func_name
 Recognize the name main and generate special output
 Discard the curly braces at the very end
 Create a stack for the scopes, do something on scope end
 Create a context struct (current context (scope, etc))
*/

#include <stdio.h>

#define LOG(msg__) printf("%s", msg__);
#define LOGF(...) printf(__VA_ARGS__);

#define MAX_INSTRUCTION_LEN 256
#define MAX_ARG_LEN 2048
#define MAX_ARGS 256

typedef enum {
    ec_noerr = 0,
    ec_insbufexceed,
    ec_argbufexceed,
    ec_invalidins,
    ec_invalidinsop
} errcode;

typedef enum {
    ps_rdins = 0,
    ps_rdarg
} pstate;

/* Alphabetical, short to long (important!) */
#define INSTRUCTIONS  \
    INSTRUCTION(func) \
    INSTRUCTION(mov)  \
    INSTRUCTION(ret)
typedef errcode(*instruction_handler) (char**, int);
/* pparg is pointer to array of size, each element is pointer to null terminated argument string */
#define INSTRUCTION_HANDLER(name__) errcode handler_ ## name__ (char** pparg, int arg_count)

/* Data for parser */
typedef struct {
    FILE* rf; /* Input file */
    FILE* of; /* Generated code goes in this file */
} parser;

static INSTRUCTION_HANDLER(func) {
    LOG("func\n");

    for (int i = 0; i < arg_count; ++i) {
        LOGF("%s\n", pparg[i]);
    }
    return ec_noerr;
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

static errcode handle_instruction(char* ins, int ins_len, char** arg, int arg_count) {
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

            instruction_handler_table[ins_i](arg, arg_count);
            return ec_noerr;
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
                rt_code = handle_instruction(ins, i_ins, arg_table, arg_count);

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

    parser p = {.rf = NULL, .of = NULL};
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

    /* Output something for now so the compilation can be tested */
    fprintf(p.of, "%s\n", "global _start");
    fprintf(p.of, "%s\n", "_start:");
    fprintf(p.of, "%s\n", "mov rax, 60");
    fprintf(p.of, "%s\n", "xor rdi, rdi");
    fprintf(p.of, "%s\n", "syscall");

    errcode rt_code = parse(&p);
    /* Indicate to the user cause for exiting if errored during parsing */
    switch (rt_code) {
        case ec_noerr:
            break;
        case ec_insbufexceed:
            LOG("Parse instruction buffer exceeded\n");
            break;
        case ec_invalidins:
            LOG("Invalid IL instruction encountered\n");
            break;
        default:
            LOG("Error during parsing\n");
            break;
    }

cleanup:
    if (p.rf != NULL) {
        fclose(p.rf);
    }
    if (p.of != NULL) {
        fclose(p.of);
    }
    return exitcode;
}

