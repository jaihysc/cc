// Reads in intermediate language (imm2)
// Generated output x86-64 assembly (imm3)

// Push all arguments into one big buffer, the handler for each insturction can read it back using a macro

#include <stdio.h>
#include <unistd.h>

#define LOG(msg__) printf("%s", msg__);
#define LOGF(...) printf(__VA_ARGS__);

typedef int errcode;
#define ERR_NO_ERROR 0
#define ERR_INSTRUCTION_BUF_EXCEED 1
#define ERR_ARG_BUF_EXCEED 2
#define ERR_INVALID_INSTRUCTION 100

#define MAX_INSTRUCTION_LEN 256
#define MAX_ARG_LEN 2048

typedef int pstate;
#define PSTATE_RD_INSTRUCTION 0
#define PSTATE_RD_ARG 1

// Alphabetical, short to long (important!)
#define INSTRUCTIONS  \
    INSTRUCTION(func) \
    INSTRUCTION(mov)  \
    INSTRUCTION(ret)
typedef errcode(*instruction_handler) (char *, int);
#define INSTRUCTION_HANDLER(name__) errcode handler_ ## name__ (char* handler__arg, int handler__arg_size)
// handler__ will be referenced by other macros

INSTRUCTION_HANDLER(func) {
    LOG("func\n");
    return 0;
}
INSTRUCTION_HANDLER(mov) {
    LOG("mov\n");
    return 0;
}
INSTRUCTION_HANDLER(ret) {
    LOG("ret\n");
    return 0;
}

// Index of string is instruction handler index
#define INSTRUCTION(name__) #name__,
const char* instruction_handler_index[] = {INSTRUCTIONS};
#undef INSTRUCTION
#define INSTRUCTION(name__) &handler_ ## name__,
const instruction_handler instruction_handler_table[] = {INSTRUCTIONS};
#define INSTRUCTION_COUNT (int)(sizeof(instruction_handler_index) / sizeof(instruction_handler_index[0]))

errcode handle_instruction(char* ins, int ins_size, char* arg, int arg_size) {
    LOGF("Instruction buffer:\n%.*s\nArgument buffer:\n%.*s\n", ins_size, ins, arg_size, arg);

    int front = 0; // Inclusive
    int rear = INSTRUCTION_COUNT; // Exclusive
    // Binary search for instruction
    while (front != rear && rear != 0 && front < INSTRUCTION_COUNT) {
        int ins_i = (front+rear) / 2;
        const char* found = instruction_handler_index[ins_i];
        int match = 1;
        for (int i = 0; i < ins_size; ++i) {
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
        // Ensure that the found instruction is same length (not just the prefix which matches)
        if (match && found[ins_size] != '\0') {
            match = 0;
            rear = ins_i;
        }
        if (match) {
            LOGF("matched IL instruction %s\n", found);
            instruction_handler_table[ins_i](arg, arg_size);
            return ERR_NO_ERROR;
        }
        LOGF("%d %d %d\n", front, rear, ins_i);
    }

    return ERR_INVALID_INSTRUCTION;
}

errcode parse() {
    int fildes = STDIN_FILENO;
    errcode rt_code = ERR_NO_ERROR;

    char ins[MAX_INSTRUCTION_LEN];
    int i_ins = 0; // Points to end of ins buffer
    char arg[MAX_ARG_LEN];
    int i_arg = 0; // Points to end of arg buffer

    pstate state = PSTATE_RD_INSTRUCTION;
    char ch;
    while(read(fildes, &ch, 1) > 0) {
        if (state == PSTATE_RD_INSTRUCTION) {
            if (i_ins == MAX_INSTRUCTION_LEN) {
                rt_code = ERR_INSTRUCTION_BUF_EXCEED;
            }
            else if (ch == ' ') {
                state = PSTATE_RD_ARG;
            }
            else {
                ins[i_ins] = ch;
                i_ins++;
            }
        }
        else if (state == PSTATE_RD_ARG) {
            if (i_arg == MAX_ARG_LEN) {
                rt_code = ERR_ARG_BUF_EXCEED;
            }
            else if (ch == '\n') {
                rt_code = handle_instruction(ins, i_ins, arg, i_arg);

                // Prepare for reading new instruction again
                i_ins = 0;
                i_arg = 0;
                state = PSTATE_RD_INSTRUCTION;
            }
            else {
                arg[i_arg] = ch;
                i_arg++;
            }
        }
        // Check, maybe need to exit in error
        if (rt_code != ERR_NO_ERROR) {
            return rt_code;
        }
    }
    return rt_code;
}

int main(int argc, char** argv) {
    int rt_code = parse();
    // Indicate to the user cause for exiting if errored during parsing
    switch (rt_code) {
        case ERR_NO_ERROR:
            break;
        case ERR_INSTRUCTION_BUF_EXCEED:
            LOG("Parse instruction buffer exceeded\n");
            break;
        case ERR_INVALID_INSTRUCTION:
            LOG("Invalid IL instruction encountered\n");
            break;
        default:
            LOG("Error during parsing\n");
            break;
    }
    return 0;
}

