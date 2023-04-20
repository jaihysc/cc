/* Represents a statement in IL2 */
#ifndef ILSTATEMENT_H
#define ILSTATEMENT_H

#define MAX_IL2_ARGS 5

#include "symbol.h"

/* See strbinfind for ordering requirements */
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

#define INSTRUCTION(name__) il2_ ## name__,
typedef enum {il2_none = -1, INSTRUCTIONS} ILIns;
#undef INSTRUCTION

/* Returns string for ILIns */
const char* il2_str(ILIns ins);

/* Converts string to IlIns, il_none if not found */
ILIns il2_from_str(const char* str);

/* Returns 1 if the instruction is a jump instruction
   0 otherwise */
int il2_isjump(ILIns ins);

/* Returns 1 if the instruction should be included
   as part of the control flow graph,
   0 otherwise */
int il2_incfg(ILIns ins);

typedef struct {
    ILIns ins;
    Symbol* arg[MAX_IL2_ARGS];
    int argc;
} ILStatement;

/* Returns ILIns for IL statement */
ILIns ilstat_ins(const ILStatement* stat);

/* Returns arg at index i for IL statement */
Symbol* ilstat_arg(const ILStatement* stat, int i);

/* Returns the number of arguments in IL statement */
int ilstat_argc(const ILStatement* stat);

#endif