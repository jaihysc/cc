/* Represents a statement in IL2 */
#ifndef IL2STATEMENT_H
#define IL2STATEMENT_H

#define MAX_IL2_ARGS 3

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
	INSTRUCTION(not ) \
	INSTRUCTION(ret)  \
	INSTRUCTION(sub)

#define INSTRUCTION(name__) il2_##name__,
typedef enum
{
	il2_none = -1,
	INSTRUCTIONS
} IL2Ins;
#undef INSTRUCTION

/* Returns string for IL2Ins */
const char* il2_str(IL2Ins ins);

/* Converts string to IlIns, il_none if not found */
IL2Ins il2_from_str(const char* str);

/* Returns 1 if the instruction is a branch instruction
   0 otherwise */
int il2_is_branch(IL2Ins ins);

/* Returns 1 if the instruction is an unconditional branch instruction
   0 otherwise */
int il2_is_unconditional_branch(IL2Ins ins);

/* Returns 1 if the instruction should be included
   as part of the control flow graph,
   0 otherwise */
int il2_incfg(IL2Ins ins);

typedef struct
{
	IL2Ins ins;
	Symbol* arg[MAX_IL2_ARGS];
	int argc;
} IL2Statement;

/* Makes IL2 statement with given arguments */
IL2Statement il2stat_make0(IL2Ins ins);

/* Makes IL2 statement with given arguments */
IL2Statement il2stat_make1(IL2Ins ins, Symbol* a0);

/* Makes IL2 statement with given arguments */
IL2Statement il2stat_make2(IL2Ins ins, Symbol* a0, Symbol* a1);

/* Makes IL2 statement with given arguments */
IL2Statement il2stat_make(IL2Ins ins, Symbol* a0, Symbol* a1, Symbol* a2);

/* Returns IL2Ins for IL statement */
IL2Ins il2stat_ins(const IL2Statement* stat);

/* Returns arg at index i for IL statement */
Symbol* il2stat_arg(const IL2Statement* stat, int i);

/* Returns the number of arguments in IL statement */
int il2stat_argc(const IL2Statement* stat);

/* Determines the variables which are used by the IL2 instruction stored in provided buffer
   Returns the number of symbols stored in provided buffer, maximum length MAX_IL2_ARGS */
int il2stat_use(const IL2Statement* stat, Symbol** used_symbols);

/* Determines the variables which are defined by the IL2 instruction stored in provided buffer
   Returns the number of symbols stored in provided buffer, maximum length MAX_IL2_ARGS */
int il2stat_def(const IL2Statement* stat, Symbol** defined_symbols);

#endif
