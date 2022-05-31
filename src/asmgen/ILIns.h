/* Assembly generator, defines IL instructions
   Defines macro:
     INSTRUCTIONS: A list of macros for all the IL instructions */
#ifndef ASMGEN_ILINS_H
#define ASMGEN_ILINS_H

/* _P (Process):
   Instructions which get handled to perform processes
   e.g., to build data structures

   _C (Code generation):
   Instructions which have assembly generation behavior

   See strbinfind for ordering requirements */
#define INSTRUCTIONS     \
    INSTRUCTION_C(add)   \
    INSTRUCTION_C(ce)    \
    INSTRUCTION_C(cl)    \
    INSTRUCTION_C(cle)   \
    INSTRUCTION_C(cne)   \
    INSTRUCTION_P(def)   \
    INSTRUCTION_C(div)   \
    INSTRUCTION_P(func)  \
    INSTRUCTION_PC(jmp)  \
    INSTRUCTION_PC(jnz)  \
    INSTRUCTION_PC(jz)   \
    INSTRUCTION_P(lab)   \
    INSTRUCTION_C(mod)   \
    INSTRUCTION_C(mov)   \
    INSTRUCTION_C(mul)   \
    INSTRUCTION_C(not)   \
    INSTRUCTION_PC(ret)  \
    INSTRUCTION_C(sub)

#define INSTRUCTION_P(name__) il_ ## name__,
#define INSTRUCTION_C(name__) il_ ## name__,
#define INSTRUCTION_PC(name__) il_ ## name__,
typedef enum {il_none = -1, INSTRUCTIONS} ILIns;
#undef INSTRUCTION_P
#undef INSTRUCTION_C
#undef INSTRUCTION_PC

#define INSTRUCTION_P(name__) #name__,
#define INSTRUCTION_C(name__) #name__,
#define INSTRUCTION_PC(name__) #name__,
const char* il_string[] = {INSTRUCTIONS};
#undef INSTRUCTION_P
#undef INSTRUCTION_C
#undef INSTRUCTION_PC

/* Forward declarations */
typedef void(*InsProcHandler) (Parser*, char**, int);
typedef void(*InsCgHandler) (Parser*, ILStatement*);
#define INSTRUCTION_P(name__) \
    static void il_proc_ ## name__ (Parser*, char**, int);
#define INSTRUCTION_C(name__) \
    static void il_cg_ ## name__ (Parser*, ILStatement*);
#define INSTRUCTION_PC(name__)                             \
    static void il_proc_ ## name__ (Parser*, char**, int); \
    static void il_cg_ ## name__ (Parser*, ILStatement*);
INSTRUCTIONS
#undef INSTRUCTION_P
#undef INSTRUCTION_C
#undef INSTRUCTION_PC

/* Index of string/ILIns is index of corresponding instruction handler */
#define INSTRUCTION_P(name__) #name__,
#define INSTRUCTION_C(name__)
#define INSTRUCTION_PC(name__) #name__,
const char* instruction_proc_index[] = {INSTRUCTIONS};
#undef INSTRUCTION_P
#undef INSTRUCTION_PC

#define INSTRUCTION_P(name__) &il_proc_ ## name__,
#define INSTRUCTION_PC(name__) &il_proc_ ## name__,
const InsProcHandler instruction_proc_table[] = {INSTRUCTIONS};
#undef INSTRUCTION_P
#undef INSTRUCTION_C
#undef INSTRUCTION_PC

#define INSTRUCTION_P(name__)
#define INSTRUCTION_C(name__) #name__,
#define INSTRUCTION_PC(name__) #name__,
const char* instruction_cg_index[] = {INSTRUCTIONS};
#undef INSTRUCTION_C
#undef INSTRUCTION_PC
#define INSTRUCTION_C(name__) il_ ## name__,
#define INSTRUCTION_PC(name__) il_ ## name__,
const ILIns instruction_cg_indexi[] = {INSTRUCTIONS};
#undef INSTRUCTION_C
#undef INSTRUCTION_PC

#define INSTRUCTION_C(name__) &il_cg_ ## name__,
#define INSTRUCTION_PC(name__) &il_cg_ ## name__,
const InsCgHandler instruction_cg_table[] = {INSTRUCTIONS};
#undef INSTRUCTION_P
#undef INSTRUCTION_C
#undef INSTRUCTION_PC

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

/* Returns function for assembly generation for il instruction
   with provided name
   Returns null if not found */
static InsCgHandler il_get_cg(const char* name) {
    int i_handler = strbinfind(
            name,
            strlength(name),
            instruction_cg_index,
            ARRAY_SIZE(instruction_cg_index));
    if (i_handler == -1) {
        return NULL;
    }
    return instruction_cg_table[i_handler];
}

/* Returns function for assembly generation for il instruction
   with provided ILIns
   Returns null if not found */
static InsCgHandler il_get_cgi(ILIns ins) {
    ASSERT(ins >= 0, "Invalid ILIns");
    for (int i = 0; i < ARRAY_SIZE(instruction_cg_indexi); ++i) {
        if (instruction_cg_indexi[i] == ins) {
            return instruction_cg_table[i];
        }
    }
    return NULL;
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
