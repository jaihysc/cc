/* Assembly generator, defines IL instructions
   Defines macro:
     INSTRUCTIONS: A list of macros for all the IL instructions */
#ifndef ASMGEN_ILINS_H
#define ASMGEN_ILINS_H

/* Each instruction has a handler called INSTRUCTION_PROC
   used to validate the arguments and perform additional
   behaviour as required.

   See strbinfind for ordering requirements */
#define INSTRUCTIONS  \
    INSTRUCTION(add)  \
    INSTRUCTION(call) \
    INSTRUCTION(ce)   \
    INSTRUCTION(cl)   \
    INSTRUCTION(cle)  \
    INSTRUCTION(cne)  \
    INSTRUCTION(def)  \
    INSTRUCTION(div)  \
    INSTRUCTION(func) \
    INSTRUCTION(jmp)  \
    INSTRUCTION(jnz)  \
    INSTRUCTION(jz)   \
    INSTRUCTION(lab)  \
    INSTRUCTION(mad)  \
    INSTRUCTION(mfi)  \
    INSTRUCTION(mod)  \
    INSTRUCTION(mov)  \
    INSTRUCTION(mtc)  \
    INSTRUCTION(mti)  \
    INSTRUCTION(mul)  \
    INSTRUCTION(not)  \
    INSTRUCTION(ret)  \
    INSTRUCTION(sub)

#define INSTRUCTION(name__) il_ ## name__,
typedef enum {il_none = -1, INSTRUCTIONS} ILIns;
#undef INSTRUCTION

#define INSTRUCTION(name__) #name__,
const char* il_string[] = {INSTRUCTIONS};
#undef INSTRUCTION

/* Forward declarations */
typedef void(*InsProcHandler) (Parser*, char**, int);
#define INSTRUCTION(name__) \
    static void il_proc_ ## name__ (Parser*, char**, int);
INSTRUCTIONS
#undef INSTRUCTION

/* Index of string/ILIns is index of corresponding instruction handler */
#define INSTRUCTION(name__) #name__,
const char* instruction_proc_index[] = {INSTRUCTIONS};
#undef INSTRUCTION

#define INSTRUCTION(name__) &il_proc_ ## name__,
const InsProcHandler instruction_proc_table[] = {INSTRUCTIONS};
#undef INSTRUCTION

/* Returns function for processing instruction with provided name
   Returns null if not found */
static InsProcHandler il_get_proc(const char* name) {
    int i_handler = strbinfind(
            name,
            strlength(name),
            instruction_proc_index,
            ARRAY_SIZE(instruction_proc_index));
    if (i_handler == -1) {
        return NULL;
    }
    return instruction_proc_table[i_handler];
}

/* Returns string for ILIns */
static const char* ins_str(ILIns ins) {
    ASSERT(ins >= 0, "Invalid ILIns");
    return il_string[ins];
}

/* Converts string to IlIns, il_none if not found */
static ILIns ins_from_str(const char* str) {
    return strbinfind(
           str,
           strlength(str),
           il_string,
           ARRAY_SIZE(il_string));
}

/* Returns 1 if the instruction is a jump instruction
   0 otherwise */
static int ins_isjump(ILIns ins) {
    switch (ins) {
        case il_jmp:
        case il_jnz:
        case il_jz:
            return 1;
        default:
            return 0;
    }
}

/* Returns 1 if the instruction should be included
   as part of the control flow graph,
   0 otherwise */
static int ins_incfg(ILIns ins) {
    switch (ins) {
        case il_def:
        case il_func:
        case il_lab:
            return 0;
        default:
            return 1;
    }
}

#endif
