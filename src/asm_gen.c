/*
 Reads in intermediate language (imm2)
 Generated output x86-64 assembly (imm3)
*/

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include "common.h"

#define MAX_INSTRUCTION_LEN 256 /* Includes null terminator */
#define MAX_ARG_LEN 2048   /* Includes null terminator */
#define MAX_ARGS 256 /* Maximum arguments for il instruction */
#define MAX_BLOCK_LINK 2 /* Maximum links out of block to to other blocks */

/* ============================================================ */
/* Parser global configuration */

int g_debug_print_buffers = 0;

/* ============================================================ */
/* x86 Registers */

/* All the registers in x86-64 (just 8 byte ones for now) */
#define X86_REGISTER_COUNT 16 /* Number of registers to store variables */
#define X86_REGISTERS  \
    X86_REGISTER(al)   \
    X86_REGISTER(bl)   \
    X86_REGISTER(cl)   \
    X86_REGISTER(dl)   \
    X86_REGISTER(sil)  \
    X86_REGISTER(dil)  \
    X86_REGISTER(bpl)  \
    X86_REGISTER(spl)  \
    X86_REGISTER(r8b)  \
    X86_REGISTER(r9b)  \
    X86_REGISTER(r10b) \
    X86_REGISTER(r11b) \
    X86_REGISTER(r12b) \
    X86_REGISTER(r13b) \
    X86_REGISTER(r14b) \
    X86_REGISTER(r15b) \
                       \
    X86_REGISTER(ah)   \
    X86_REGISTER(bh)   \
    X86_REGISTER(ch)   \
    X86_REGISTER(dh)   \
                       \
    X86_REGISTER(ax)   \
    X86_REGISTER(bx)   \
    X86_REGISTER(cx)   \
    X86_REGISTER(dx)   \
    X86_REGISTER(si)   \
    X86_REGISTER(di)   \
    X86_REGISTER(bp)   \
    X86_REGISTER(sp)   \
    X86_REGISTER(r8w)  \
    X86_REGISTER(r9w)  \
    X86_REGISTER(r10w) \
    X86_REGISTER(r11w) \
    X86_REGISTER(r12w) \
    X86_REGISTER(r13w) \
    X86_REGISTER(r14w) \
    X86_REGISTER(r15w) \
                       \
    X86_REGISTER(eax)  \
    X86_REGISTER(ebx)  \
    X86_REGISTER(ecx)  \
    X86_REGISTER(edx)  \
    X86_REGISTER(esi)  \
    X86_REGISTER(edi)  \
    X86_REGISTER(ebp)  \
    X86_REGISTER(esp)  \
    X86_REGISTER(r8d)  \
    X86_REGISTER(r9d)  \
    X86_REGISTER(r10d) \
    X86_REGISTER(r11d) \
    X86_REGISTER(r12d) \
    X86_REGISTER(r13d) \
    X86_REGISTER(r14d) \
    X86_REGISTER(r15d) \
                       \
    X86_REGISTER(rax)  \
    X86_REGISTER(rbx)  \
    X86_REGISTER(rcx)  \
    X86_REGISTER(rdx)  \
    X86_REGISTER(rsi)  \
    X86_REGISTER(rdi)  \
    X86_REGISTER(rbp)  \
    X86_REGISTER(rsp)  \
    X86_REGISTER(r8)   \
    X86_REGISTER(r9)   \
    X86_REGISTER(r10)  \
    X86_REGISTER(r11)  \
    X86_REGISTER(r12)  \
    X86_REGISTER(r13)  \
    X86_REGISTER(r14)  \
    X86_REGISTER(r15)

#define X86_REGISTER(reg__) reg_ ## reg__,
typedef enum {reg_none = -2, reg_stack = -1, X86_REGISTERS} Register;
#undef X86_REGISTER
#define X86_REGISTER(reg__) #reg__ ,
const char* reg_strings[] = {X86_REGISTERS};
#undef X86_REGISTER

/* Refers to the various sizes of a register (GenericRegister)
   e.g., reg_a refers to: al, ah, ax eax, rax */
typedef enum {
    reg_a,
    reg_b,
    reg_c,
    reg_d
} GRegister;

/* Returns the name to access a given register with the indicates size
   -1 for upper byte (ah), 1 for lower byte (al) */
static Register reg_get(GRegister greg, int bytes) {
    const Register reg_1l[] = {reg_al, reg_bl, reg_cl, reg_dl};
    const Register reg_1h[] = {reg_ah, reg_bh, reg_ch, reg_dh};
    const Register reg_4[] = {reg_eax, reg_ebx, reg_ecx, reg_edx};
    const Register reg_8[] = {reg_rax, reg_rbx, reg_rcx, reg_rdx};
    switch (bytes) {
        case 1:
            return reg_1l[greg];
        case -1:
            return reg_1h[greg];
        case 4:
            return reg_4[greg];
        case 8:
            return reg_8[greg];
        default:
            ASSERT(0, "Bad byte size");
    }
}

/* Converts given asm_register into its corresponding cstr*/
static const char* reg_str(Register reg) {
    ASSERT(reg >= 0, "Invalid register");
    return reg_strings[reg];
}

/* Returns the cstr of the register corresponding to the provided
   register with the indicated size */
static const char* reg_get_str(GRegister greg, int bytes) {
    return reg_str(reg_get(greg, bytes));
}

/* ============================================================ */
/* Assembly manipulation */

/* Returns the size directive used to access bytes from memory location */
static const char* asm_size_directive(int bytes) {
    switch (bytes) {
        case 1:
            return "BYTE";
        case 2:
            return "WORD";
        case 4:
            return "DWORD";
        case 8:
            return "QWORD";
        default:
            ASSERT(0, "Bad byte size");
            return "";
    }
}

/* ============================================================ */
/* Parser data structure + functions */

typedef struct Parser Parser;
typedef struct Statement Statement;

#define ERROR_CODES            \
    ERROR_CODE(noerr)          \
    ERROR_CODE(insbufexceed)   \
    ERROR_CODE(argbufexceed)   \
    ERROR_CODE(scopelenexceed) \
    ERROR_CODE(invalidins)     \
    ERROR_CODE(invalidinsop)   \
    ERROR_CODE(invalidlabel)   \
    ERROR_CODE(badargs)        \
    ERROR_CODE(badmain)        \
    ERROR_CODE(writefailed)    \
    ERROR_CODE(seekfailed)     \
    ERROR_CODE(outofmemory)    \
    ERROR_CODE(unknownsym)

#define ERROR_CODE(name__) ec_ ## name__,
typedef enum {ERROR_CODES} ErrorCode;
#undef ERROR_CODE
#define ERROR_CODE(name__) #name__,
char* errcode_str[] = {ERROR_CODES};
#undef ERROR_CODE
#undef ERROR_CODES

typedef int SymbolId;
typedef struct {
    Type type;
    char name[MAX_ARG_LEN + 1]; /* +1 for null terminator */
    Register reg;
} Symbol;

/* Returns 1 if symbol name is considered a constant, 0 otherwise */
static int name_isconstant(const char* name) {
    /* '-' for negative numbers */
    return (('0' <= name[0] && name[0] <= '9') || name[0] == '-');
}

/* Returns type for given symbol */
static Type symbol_type(Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->type;
}

/* Returns name for given symbol */
static const char* symbol_name(Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->name;
}

/* Returns where symbol is stored */
static Register symbol_location(Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->reg;
}

/* Returns 1 if symbol is located on the stack, 0 otherwise */
static int symbol_on_stack(Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->reg == reg_stack;
}

/* Returns 1 if symbol is a constant */
static int symbol_is_constant(Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->reg == reg_none;
}

/* Turns this symbol into a special symbol for representing constants */
static void symbol_make_constant(Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    sym->reg = reg_none;
}

/* Returns bytes for symbol */
static int symbol_bytes(Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return type_bytes(sym->type);
}

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
typedef void(*InsCgHandler) (Parser*, Statement*);
#define INSTRUCTION_P(name__) \
    static void il_proc_ ## name__ (Parser*, char**, int);
#define INSTRUCTION_C(name__) \
    static void il_cg_ ## name__ (Parser*, Statement*);
#define INSTRUCTION_PC(name__)                             \
    static void il_proc_ ## name__ (Parser*, char**, int); \
    static void il_cg_ ## name__ (Parser*, Statement*);
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

struct Statement {
    ILIns ins;
    SymbolId arg[MAX_ARGS];
    int argc;
};

/* Returns ILIns for statement */
static ILIns stat_ins(Statement* stat) {
    ASSERT(stat != NULL, "Statement is null");
    return stat->ins;
}

/* Returns arg at index i for statement */
static ILIns stat_arg(Statement* stat, int i) {
    ASSERT(stat != NULL, "Statement is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < stat->argc, "Index out of range");
    return stat->arg[i];
}

/* Returns the number of arguments in statement */
static int stat_argc(Statement* stat) {
    ASSERT(stat != NULL, "Statement is null");
    return stat->argc;
}

/* Returns the number of symbols the instruction of this statement
   uses and puts the used symbols into provided symbol buffer.
   At most MAX_ARGS */
static int stat_use(Statement* stat, SymbolId* out_sym_id) {
    switch (stat->ins) {
        /* 2 use, argument index 1 and 2 */
        case il_add:
        case il_ce:
        case il_cl:
        case il_cle:
        case il_cne:
        case il_div:
        case il_mod:
        case il_mul:
        case il_sub:
            out_sym_id[0] = stat->arg[1];
            out_sym_id[1] = stat->arg[2];
            return 2;

        /* 1 use, argument index 0 */
        case il_ret:
            ASSERT(stat->argc == 1,
                    "ret requires 1 arg (0 arg handling not implemented)");
            out_sym_id[0] = stat->arg[0];
            return 1;

        /* 1 use, argument index 1 */
        case il_mov:
        case il_not:
        case il_jnz:
        case il_jz:
            out_sym_id[0] = stat->arg[1];
            return 1;

        /* No use */
        case il_lab:
        case il_def:
        case il_func:
        case il_jmp:
            return 0;

        default:
            ASSERT(0, "Unimplemented");
            return 0;
    }
}

/* Returns the number of symbols the instruction of this statement
   defines (sets) and puts the defined symbols into provided symbol buffer.
   At most 1 */
static int stat_def(Statement* stat, SymbolId* out_sym_id) {
    switch (stat->ins) {
        case il_add:
        case il_ce:
        case il_cl:
        case il_cle:
        case il_cne:
        case il_div:
        case il_mod:
        case il_mov:
        case il_mul:
        case il_not:
        case il_sub:
            out_sym_id[0] = stat->arg[0];
            return 1;

        case il_def:
        case il_func:
        case il_jmp:
        case il_jnz:
        case il_jz:
        case il_lab:
        case il_ret:
            return 0;

        default:
            ASSERT(0, "Unimplemented");
            return 0;
    }
}

/* Blocks are formed by partitioning il statements according to the rules:
   1. Control always enters at the start of the block
   2. Control always leaves at the last statement or end of the block */
typedef struct {
    /* Labels at the entry of this block */
    vec_t(SymbolId) labels;
    vec_t(Statement) stats;

    /* Symbols used, defined by this bock */
    vec_t(SymbolId) use;
    vec_t(SymbolId) def;

    /* Liveness IN[B] (Needed entering block)
       and OUT[B] (needed exiting block) */
    vec_t(SymbolId) in;
    vec_t(SymbolId) out;

    /* Loop nesting depth of block
       0 if not nested in any loop */
    int depth;

    /* At most 2 options:
       1. Flow through to next block
       2. Jump at end
       Is offset (in Block) from current location, cannot use pointer
       as container holding Block may resize */
    int next[MAX_BLOCK_LINK];
} Block;

static void block_construct(Block* blk) {
    ASSERT(blk != NULL, "Block is null");
    vec_construct(&blk->labels);
    vec_construct(&blk->stats);
    vec_construct(&blk->use);
    vec_construct(&blk->def);
    vec_construct(&blk->in);
    vec_construct(&blk->out);
    blk->depth = 0;
    blk->next[0] = 0;
    blk->next[1] = 0;
}

static void block_destruct(Block* blk) {
    ASSERT(blk != NULL, "Block is null");
    vec_destruct(&blk->out);
    vec_destruct(&blk->in);
    vec_destruct(&blk->def);
    vec_destruct(&blk->use);
    vec_destruct(&blk->stats);
    vec_destruct(&blk->labels);
}

/* Returns number of labels in block */
static int block_lab_count(Block* blk) {
    ASSERT(blk != NULL, "Block is null");
    return vec_size(&blk->labels);
}

/* Returns lab at index in block */
static SymbolId block_lab(Block* blk, int i) {
    ASSERT(blk != NULL, "Block is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < block_lab_count(blk), "Index out of range");
    return vec_at(&blk->labels, i);
}

/* Returns number of statements in block */
static int block_stat_count(Block* blk) {
    ASSERT(blk != NULL, "Block is null");
    return vec_size(&blk->stats);
}

/* Returns statement at index in block */
static Statement* block_stat(Block* blk, int i) {
    ASSERT(blk != NULL, "Block is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < block_stat_count(blk), "Index out of range");
    return &vec_at(&blk->stats, i);
}

/* Adds a new label at the entry of this block
   Returns 1 if successful, 0 if not */
static int block_add_label(Block* blk, SymbolId lab_id) {
    ASSERT(blk != NULL, "Block is null");
    return vec_push_back(&blk->labels, lab_id);
}

/* Adds statement to block
   Returns 1 if successful, 0 if not */
static int block_add_stat(Block* blk, Statement stat) {
    ASSERT(blk != NULL, "Block is null");
    return vec_push_back(&blk->stats, stat);
}

/* Adds provided SymbolId to liveness 'def'ed symbols for this block
   if it does not exist. Does nothing if does exist, returns 1
   Returns 1 if successful, 0 if not */
static int block_add_def(Block* blk, SymbolId sym_id) {
    ASSERT(blk != NULL, "Block is null");
    for (int i = 0; i < vec_size(&blk->def); ++i) {
        if (vec_at(&blk->def, i) == sym_id) {
            return 1;
        }
    }
    return vec_push_back(&blk->def, sym_id);
}

/* Returns 1 if SymbolId is in def(B), 0 otherwise */
static int block_in_def(Block* blk, SymbolId sym_id) {
    ASSERT(blk != NULL, "Block is null");
    for (int i = 0; i < vec_size(&blk->def); ++i) {
        if (vec_at(&blk->def, i) == sym_id) {
            return 1;
        }
    }
    return 0;
}

/* Returns number of entries use(B) */
static int block_use_count(Block* blk) {
    ASSERT(blk != NULL, "Block is null");
    return vec_size(&blk->use);
}

/* Returns symbol for use(B) at index i */
static SymbolId block_use(Block* blk, int i) {
    ASSERT(blk != NULL, "Block is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < block_use_count(blk), "Index out of range");
    return vec_at(&blk->use, i);
}

/* Adds provided SymbolId to liveness 'use'ed symbols for this block
   if it does not exist. Does nothing if does exist, returns 1
   Returns 1 if successful, 0 if not */
static int block_add_use(Block* blk, SymbolId sym_id) {
    ASSERT(blk != NULL, "Block is null");
    for (int i = 0; i < vec_size(&blk->use); ++i) {
        if (vec_at(&blk->use, i) == sym_id) {
            return 1;
        }
    }
    return vec_push_back(&blk->use, sym_id);
}

/* Removes provided SymbolId from liveness 'use'd symbols for this block
   Does nothing if symbol does not exist in block */
static void block_remove_use(Block* blk, SymbolId sym_id) {
    for (int i = 0; i < vec_size(&blk->use); ++i) {
        if (vec_at(&blk->use, i) == sym_id) {
            vec_splice(&blk->use, i, 1);
            return;
        }
    }
}

/* Returns number of entries for IN[B] */
static int block_in_count(Block* blk) {
    ASSERT(blk != NULL, "Block is null");
    return vec_size(&blk->in);
}

/* Returns symbol for IN[B] at index i */
static SymbolId block_in(Block* blk, int i) {
    ASSERT(blk != NULL, "Block is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < block_in_count(blk), "Index out of range");
    return vec_at(&blk->in, i);
}

/* Adds SymbolId to IN[B] if it does not already exist
   Does nothing if it does exist, returns 1
   Returns 1 if successful, 0 otherwise */
static int block_add_in(Block* blk, SymbolId sym_id) {
    ASSERT(blk != NULL, "Block is null");
    for (int i = 0; i < vec_size(&blk->in); ++i) {
        if (vec_at(&blk->in, i) == sym_id) {
            return 1;
        }
    }
    return vec_push_back(&blk->in, sym_id);
}

/* Returns number of entries for OUT[B] */
static int block_out_count(Block* blk) {
    ASSERT(blk != NULL, "Block is null");
    return vec_size(&blk->out);
}

/* Returns symbol for OUT[B] at index i */
static SymbolId block_out(Block* blk, int i) {
    ASSERT(blk != NULL, "Block is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < block_out_count(blk), "Index out of range");
    return vec_at(&blk->out, i);
}

/* Adds SymbolId to OUT[B] if it does not already exist
   Does nothing if it does exist, returns 1
   Returns 1 if successful, 0 otherwise */
static int block_add_out(Block* blk, SymbolId sym_id) {
    ASSERT(blk != NULL, "Block is null");
    for (int i = 0; i < vec_size(&blk->out); ++i) {
        if (vec_at(&blk->out, i) == sym_id) {
            return 1;
        }
    }
    return vec_push_back(&blk->out, sym_id);
}

/* Increments the loop nesting depth for block */
static void block_inc_depth(Block* blk) {
    ASSERT(blk != NULL, "Block is null");
    ++blk->depth;
}

/* Returns the loop nesting depth for block */
static int block_depth(Block* blk) {
    ASSERT(blk != NULL, "Block is null");
    return blk->depth;
}

/* Sets the loop nesting depth for block */
static void block_set_depth(Block* blk, int depth) {
    ASSERT(blk != NULL, "Block is null");
    blk->depth = depth;
}

/* Links block to next block */
static void block_link(Block* blk, Block* next) {
    ASSERT(blk != NULL, "Block is null");
    ASSERT(blk != next, "Cannot link block to self");
    for (int i = 0; i < MAX_BLOCK_LINK; ++i ) {
        if (blk->next[i] == 0) {
            blk->next[i] = next - blk;
            return;
        }
    }
    ASSERT(0, "Too many links out of block");
}

/* Returns pointer to ith next block */
static Block* block_next(Block* blk, int i) {
    ASSERT(blk != NULL, "Block is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < MAX_BLOCK_LINK, "Index out of range");

    if (blk->next[i] == 0) {
        return NULL;
    }
    return blk + blk->next[i];
}

/* Interference graph node */
typedef struct {
    /* Offset (in IGNode) to neighbor
       Pointers cannot be used as container holding nodes may resize */
    vec_t(int) neighbor;
    /* Performance impact if this variable is not in register,
       lower = less impact */
    uint64_t spill_cost;
} IGNode;

static void ignode_construct(IGNode* node) {
    ASSERT(node != NULL, "IGNode is null");
    vec_construct(&node->neighbor);
    node->spill_cost = 0;
}

static void ignode_destruct(IGNode* node) {
    ASSERT(node != NULL, "IGNode is null");
    vec_destruct(&node->neighbor);
}

/* Returns number of neighbors for IGNode */
static int ignode_neighbor_count(IGNode* node) {
    ASSERT(node != NULL, "Node is null");
    return vec_size(&node->neighbor);
}

/* Adds a neighbor to this node from node to other
   Does nothing if neighbor does exist, returns 1
   Returns 1 if successful, 0 otherwise */
static int ignode_link(IGNode* node, IGNode* other) {
    ASSERT(node != NULL, "Node is null");
    ASSERT(other != NULL, "Other node is null");
    ASSERT(node != other, "Cannot link node to self");
    int offset = other - node;
    for (int i = 0; i < vec_size(&node->neighbor); ++i) {
        if (vec_at(&node->neighbor, i) == offset) {
            return 1;
        }
    }
    return vec_push_back(&node->neighbor, offset);
}

/* Returns neighbors for IGNode at index */
static IGNode* ignode_neighbor(IGNode* node, int i) {
    ASSERT(node != NULL, "Node is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < ignode_neighbor_count(node), "Index out of range");
    return node + vec_at(&node->neighbor, i);
}

/* Returns spill cost for interference graph node */
static uint64_t ignode_cost(IGNode* node) {
    ASSERT(node != NULL, "Node is null");
    return node->spill_cost;
}

/* Adds provided spill cost to interference graph node */
static void ignode_add_cost(IGNode* node, uint64_t cost) {
    ASSERT(node != NULL, "Node is null");
    node->spill_cost += cost;
}

struct Parser {
    ErrorCode ecode;

    FILE* rf; /* Input file */
    FILE* of; /* Generated code goes in this file */

    /* For one function only for now */

    /* First symbol element is earliest in occurrence */
    vec_t(Symbol) symbol;
    char func_name[MAX_ARG_LEN]; /* Name of current function */
    int stack_bytes; /* Bytes stack needs */

    /* Control flow graph */
    vec_t(Block) cfg;
    Block* latest_blk;

    /* Interference graph
       Index of symbol in symbol table corresponds to index of its node
       in the interference graph */
    vec_t(IGNode) ig;
    /* Buffer of live symbols used when calculating interference graph */
    vec_t(SymbolId) ig_live;

    /* Instruction, argument */
    char ins[MAX_INSTRUCTION_LEN];
    int ins_len;
    char arg[MAX_ARG_LEN];
    char* arg_table[MAX_ARGS]; /* Points to beginning of each argument */
    int arg_count; /* Number of arguments */
};

static void parser_construct(Parser* p) {
    vec_construct(&p->symbol);
    vec_construct(&p->cfg);
    vec_construct(&p->ig);
    vec_construct(&p->ig_live);
}

static void parser_destruct(Parser* p) {
    vec_destruct(&p->ig_live);
    for (int i = 0; i < vec_size(&p->ig); ++i) {
        ignode_destruct(&vec_at(&p->ig, i));
    }
    vec_destruct(&p->ig);
    for (int i = 0; i < vec_size(&p->cfg); ++i) {
        block_destruct(&vec_at(&p->cfg, i));
    }
    vec_destruct(&p->cfg);
    vec_destruct(&p->symbol);
}

/* Return 1 if error is set, else 0 */
static int parser_has_error(Parser* p) {
    return p->ecode != ec_noerr;
}

/* Gets error in parser */
static ErrorCode parser_get_error(Parser* p) {
    return p->ecode;
}

/* Sets error in parser */
static void parser_set_error(Parser* p, ErrorCode ecode) {
    p->ecode = ecode;
    LOGF("Error set %d %s\n", ecode, errcode_str[ecode]);
}

/* Writes provided assembly using format string and va_args into output */
static void parser_output_asm(Parser* p, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (vfprintf(p->of, fmt, args) < 0) {
        parser_set_error(p, ec_writefailed);
    }
    va_end(args);
}

/* Writes comment using format string and va_args into output */
static void parser_output_comment(Parser* p, const char* fmt, ...) {
    parser_output_asm(p, "                                           ; ");
    va_list args;
    va_start(args, fmt);
    if (vfprintf(p->of, fmt, args) < 0) {
        parser_set_error(p, ec_writefailed);
    }
    va_end(args);
}

static SymbolId symtab_add(Parser* p, Type type, const char* name);
static Symbol* symtab_get(Parser* p, SymbolId sym_id);

/* Clears the symbol table */
static void symtab_clear(Parser* p) {
    vec_clear(&p->symbol);
}

/* Returns 1 if symbol is a constant, 0 otherwise */
static int symtab_isconstant(Parser* p, SymbolId sym_id) {
    return name_isconstant(symbol_name(symtab_get(p, sym_id)));
}

/* Returns 1 if name is within symbol table, 0 otherwise */
static int symtab_contains(Parser* p, const char* name) {
    for (int i = 0; i < vec_size(&p->symbol); ++i) {
        Symbol* symbol = &vec_at(&p->symbol, i);
        if (strequ(symbol->name, name)) {
            return 1;
        }
    }
    return 0;
}

/* Retrieves symbol with given name from symbol table
   Null if not found */
static SymbolId symtab_find(Parser* p, const char* name) {
    for (int i = 0; i < vec_size(&p->symbol); ++i) {
        Symbol* symbol = &vec_at(&p->symbol, i);
        if (strequ(symbol->name, name)) {
            return i;
        }
    }

    /* Special handling for constants, they always exist, thus add
       them to the table when not found to make them exist */
    if (name_isconstant(name)) {
        /* TODO calculate size of constant, assume integer for now */
        Type type = {.typespec = ts_i32};
        SymbolId sym_id = symtab_add(p, type, name);
        symbol_make_constant(symtab_get(p, sym_id));
        return sym_id;
    }

    if (g_debug_print_buffers) {
        LOGF("Cannot find %s\n", name);
    }
    return -1;
}

static Symbol* symtab_get(Parser* p, SymbolId sym_id) {
    ASSERT(sym_id >= 0, "Invalid symbol id");
    ASSERT(sym_id < vec_size(&p->symbol), "Symbol id out of range");
    return &vec_at(&p->symbol, sym_id);
}

/* Returns offset from base pointer to access symbol on the stack */
static int symtab_get_offset(Parser* p, SymbolId sym_id) {
    ASSERT(sym_id >= 0, "Symbol not found");

    int offset = 0;
    for (int i = 0; i <= sym_id; ++i) {
        Symbol* sym = &vec_at(&p->symbol, i);
        if (symbol_on_stack(sym)) {
            offset -= symbol_bytes(sym);
        }
    }
    return offset;
}

/* Adds symbol created with given arguments to symbol table
   Returns the added symbol */
static SymbolId symtab_add(Parser* p, Type type, const char* name) {
    ASSERTF(!symtab_contains(p, name), "Duplicate symbol %s", name);

    if (!vec_push_backu(&p->symbol)) {
        parser_set_error(p, ec_scopelenexceed);
        return -1;
    }
    Symbol* sym = &vec_back(&p->symbol);
    sym->type = type;
    strcopy(name, sym->name);
    sym->reg = reg_stack;
    return vec_size(&p->symbol) - 1;
}

/* Clears control flow graph */
static void cfg_clear(Parser* p) {
    for (int i = 0; i < vec_size(&p->cfg); ++i) {
        block_destruct(&vec_at(&p->cfg, i));
    }
    vec_clear(&p->cfg);
}

static Block* cfg_latest_block(Parser* p) {
    return p->latest_blk;
}

/* Adds a new, unlinked block to the cfg */
static Block* cfg_new_block(Parser* p) {
    if (vec_push_backu(&p->cfg)) {
        Block* blk = &vec_back(&p->cfg);
        block_construct(blk);
        p->latest_blk = blk;
        return blk;
    }
    parser_set_error(p, ec_outofmemory);
    return NULL;
}

/* Adds a new block to the cfg,
   links the current block to the new block
   Returns the new block, NULL if failed to allocate */
static Block* cfg_link_new_block(Parser* p) {
    Block* new = cfg_new_block(p);
    if (new) {
        Block* last = &vec_at(&p->cfg, vec_size(&p->cfg) - 2);
        block_link(last, new);
    }
    return new;
}

/* Finds the first block which has the provided label
   Returns null if not found */
static Block* cfg_find_labelled(Parser* p, SymbolId lab_id) {
    for (int i = 0; i < vec_size(&p->cfg); ++i) {
        Block* blk = &vec_at(&p->cfg, i);
        for (int j = 0; j < block_lab_count(blk); ++j) {
            if (block_lab(blk, j) == lab_id) {
                return blk;
            }
        }
    }
    return NULL;
}

/* Links jumps from blocks to other blocks */
static void cfg_link_jump_dest(Parser* p) {
    for (int i = 0; i < vec_size(&p->cfg); ++i) {
        Block* blk = &vec_at(&p->cfg, i);
        if (block_stat_count(blk) == 0) {
            continue;
        }

        Statement* last_stat = block_stat(blk, block_stat_count(blk) - 1);
        if (ins_isjump(stat_ins(last_stat))) {
            SymbolId lab_id = stat_arg(last_stat, 0);
            Block* target_blk = cfg_find_labelled(p, lab_id);
            if (target_blk == NULL) {
                Symbol* lab = symtab_get(p, lab_id);
                ERRMSGF("Could not find jump label %s\n", symbol_name(lab));
                parser_set_error(p, ec_invalidlabel);
                return;
            }
            block_link(blk, target_blk);
        }
    }
}

/* Appends statement into the latest block */
static void cfg_append_latest(Parser* p, Statement stat) {
    ASSERT(p->latest_blk, "No block exists");
    if (!block_add_stat(p->latest_blk, stat)) {
        parser_set_error(p, ec_outofmemory);
    }
}

/* Computes liveness use/def information for each block */
static void cfg_compute_use_def(Parser* p) {
    for (int i = 0; i < vec_size(&p->cfg); ++i) {
        Block* blk = &vec_at(&p->cfg, i);
        for (int j = block_stat_count(blk) - 1; j >= 0; --j) {
            Statement* stat = block_stat(blk, j);
            SymbolId syms[MAX_ARGS]; /* Buffer to hold symbols */

            /* Add 'def'ed symbol from statement to 'def' for block
               Remove occurrences of 'def'd symbol from 'use' for block */
            int def_count = stat_def(stat, syms);
            for (int k = 0; k < def_count; ++k) {
                ASSERT(!symtab_isconstant(p, syms[k]),
                        "Assigned symbol should not be constant");
                if (!block_add_def(blk, syms[k])) {
                    parser_set_error(p, ec_outofmemory);
                    return;
                }
                block_remove_use(blk, syms[k]);
            }

            /* Add 'use'd symbols from statement to 'use' for block */
            ASSERT(MAX_ARGS >= 1, "Need at least 1 to hold def");
            int use_count = stat_use(stat, syms);
            for (int k = 0; k < use_count; ++k) {
                if (symtab_isconstant(p, syms[k])) {
                    continue;
                }
                if (!block_add_use(blk, syms[k])) {
                    parser_set_error(p, ec_outofmemory);
                    return;
                }
            }
        }
    }
}

/* Performs recursive traversal to compute liveness for blocks in the cfg
   status: At index i holds whether a block at index i has been traversed or not
   block index is block - base_block (Both of type Block*)
     0 = Not traversed
     1 = Traversed
     2 = Traversed and modified
   base: The first block in the array which holds all the blocks, it is
   provided to compute an index to determine whether a block has been
   traversed or not
   blk: The current block */
static void cfg_compute_liveness_traverse(
        Parser* p, char* status, Block* base, Block* blk) {
    int index = blk - base;
    ASSERT(status[index] == 0, "Block should be untraversed");
    status[index] = 1; /* Block traversed */

    for (int i = 0; i < MAX_BLOCK_LINK; ++i) {
        if (parser_has_error(p)) return;

        Block* next = block_next(blk, i);
        if (!next) {
            continue;
        }

        /* Only traverse next if not traversed */
        if (status[next - base] == 0) {
            cfg_compute_liveness_traverse(p, status, base, next);
        }
        /* Compute IN[B] and OUT[B]

           Math below:
           This is block B
           OUT[B] is a union for all successor of B: S, IN[S]

           Check for changes by trying to add IN[S] to OUT[B],
           disallowing duplicates, if OUT[B] increased in size, it
           has changed
           IN[B] only changes with OUT[B], as use(B) and def(B) are constant,
           thus IN[B] can only change if OUT[B] changes - therefore
           checking if OUT[B] has increased in size always indicates
           whether the block has changed

           IN[B] = use(B) union (OUT[B] - def(B))
           use(B) and def(B) are constant, use(B) is added to IN[B]
           at the beginning of liveness calculation
           Thus add the new entries from OUT[B] to IN[B] if the entries
           are not in def(B) */
        int old_out_count = block_out_count(blk);
        for (int j = 0; j < block_in_count(next); ++j) {
            SymbolId new_in = block_in(next, j);
            if (!block_add_out(blk, new_in)) goto error;

            /* Check if is in def(B), if not add to IN[B] */
            if (!block_in_def(blk, new_in)) {
                if (!block_add_in(blk, new_in)) goto error;
            }
        }
        if (block_out_count(blk) != old_out_count) {
            status[index] = 2;
        }
    }
    return;

error:
    parser_set_error(p, ec_outofmemory);
}

/* Computes liveness (live-variable analysis) of blocks in the cfg
   Results saved are Block use/def and in/out */
static void cfg_compute_liveness(Parser* p) {
    /* Avoid segfault trying to traverse nothing for IN[B]/OUT[B] */
    if (vec_size(&p->cfg) == 0) {
        return;
    }

    cfg_compute_use_def(p);

    /* IN[B] = use(B) union (OUT[B] - def(B))
       Thus add use(B) to IN[B] for all blocks
       (Cannot do this while computing use/def as use may change
       during computation as new def are discovered, changing
       what is use upon entering block) */
    for (int i = 0; i < vec_size(&p->cfg); ++i) {
        Block* blk = &vec_at(&p->cfg, i);
        for (int j = 0; j < block_use_count(blk); ++j) {
            if (!block_add_in(blk, block_use(blk, j))) {
                parser_set_error(p, ec_outofmemory);
                return;
            }
        }
    }

    /* Because the blocks are cyclic, the best method I have is to
       repeatedly traverse the entire cfg until the results are stable */

    /* Give up trying to determine in/out for blocks
       if the results does not stabilize after some number of traversals
       and assume worst case */
    int max_traverse = 10;

    int block_count = vec_size(&p->cfg);
    char status[block_count];
    for (int i = 0; i < max_traverse; ++i) {
        /* Reset status to not traversed */
        for (int j = 0; j < block_count; ++j) {
            status[j] = 0;
        }

        Block* base = vec_data(&p->cfg);
        cfg_compute_liveness_traverse(p, status, base, base);

        /* Check if results are stable (no modifications) */
        int stable = 1;
        for (int j = 0; j < block_count; ++j) {
            if (status[j] == 2) {
                stable = 0;
                break;
            }
        }
        if (stable) {
            return;
        }
    }

    /* TODO implement something to handle liveness if give up */
    ASSERT(0, "Liveness behavior if stability not found unimplemented");
}

/* Performs recursive traversal to compute loop nesting depth for blocks
   The algorithm is as follows, I could not find any existing algorithms
   so I made my own which I *believe* is correct.

   Traverse depth first, keeping track of the path. A cycle is found if
   the current node is in the path, if so increment the depth for all the
   nodes along the path.
   A node is fully visited if the traversal visited and left the node.
   If a node visited is fully visited, scan backwards along the path:
   for each non fully visited node along the path, if the depth of the
   node is less than the depth of the fully visited node, set the depth
   to be equal to the depth of the fully visited node. Stop at the first
   node along the path that is a fully visited node.

   status: Set to 1 if node is visited, index into status is index of node,
           calculated as: block - base_block (Both of type Block*)
           0 = Not traversed
           1 = Traversed
   path: Array of Block* in sequential order to reach the current block,
         guarantee it has sufficient space to hold all blocks
   path_len: Size of path array
   base: Base block
   blk: Current block */
static void cfg_compute_loop_depth_traverse(
        Parser* p, char* status, Block** path, int path_len,
        Block* base, Block* blk) {
    int index = blk - base;

    /* Check if block already traversed,
       if so increment the nesting depth along the path
       The old path_len is used here (does not include the current
       Block*) as it would wrongly think it found a cycle */
    for (int i = 0; i < path_len; ++i) {
        if (path[i] != blk) {
            continue;
        }

        for (; i < path_len; ++i) {
            block_inc_depth(path[i]);
        }
        goto exit;
    }

    /* Check if node already visited,
       if so, scanning backwards, for each non fully visited node along
       the path, if its depth is less than the fully visited node's depth,
       set its depth to be equal to the fully visited node's depth.
       Stop at the first fully visited node */
    if (status[index] == 1) {
        const int depth = block_depth(blk); /* Fully traversed node */
        for (int i = path_len - 1; i >= 0; --i) {
            Block* path_blk = path[i]; /* ith block in path */
            int path_blk_index = path_blk - base;

            if (status[path_blk_index] == 1) {
                break;
            }
            if (block_depth(path_blk) < depth) {
                block_set_depth(path_blk, depth);
            }
        }
        goto exit;
    }

    ASSERT(status[index] == 0, "Expect block to be untraversed");
    ASSERT(path_len < vec_size(&p->cfg), "Too many blocks");
    path[path_len] = blk;

    for (int i = 0; i < MAX_BLOCK_LINK; ++i) {
        Block* next = block_next(blk, i);
        if (next) {
            cfg_compute_loop_depth_traverse(
                    p, status, path, path_len + 1, base, next);
        }
    }

exit:
    status[index] = 1;
}

/* Computes the loop nesting depth for blocks in the control flow graph
   Results are saved in Block member depth */
static void cfg_compute_loop_depth(Parser* p) {
    if (vec_size(&p->cfg) == 0) {
        return;
    }

    int block_count = vec_size(&p->cfg);
    char status[block_count];
    Block* path[block_count];
    for (int i = 0; i < block_count; ++i) {
        status[i] = 0;
    }

    Block* base = &vec_at(&p->cfg, 0);
    cfg_compute_loop_depth_traverse(p, status, path, 0, base, base);
}

/* Traverses the blocks in the control flow graph and emits assembly */
static void cfg_output_asm(Parser* p) {
    /* Function labels always with prefix f@ */
    parser_output_asm(p,
        "f@%s:\n"
        /* Function prologue */
        "push rbp\n"
        "mov rbp,rsp\n",
        p->func_name
    );
    /* Reserve stack space */
    if (p->stack_bytes != 0) {
        parser_output_asm(p, "sub rsp,%d\n", p->stack_bytes);
    }

    for (int i = 0; i < vec_size(&p->cfg); ++i) {
        parser_output_comment(p, "Block %d\n", i);

        Block* blk = &vec_at(&p->cfg, i);
        /* Labels for block */
        for (int j = 0; j < block_lab_count(blk); ++j) {
            SymbolId lab_id = block_lab(blk, j);
            parser_output_asm(p, "%s:\n", symbol_name(symtab_get(p, lab_id)));
        }

        /* Statements for block */
        for (int j = 0; j < block_stat_count(blk); ++j) {
            Statement* stat = block_stat(blk, j);
            InsCgHandler cg_handler = il_get_cgi(stat_ins(stat));
            ASSERT(cg_handler != NULL, "Failed to find handler for codegen");

            cg_handler(p, stat);
            if (parser_has_error(p)) return;
        }
    }
}

/* Clears nodes in interference graph */
static void ig_clear(Parser* p) {
    for (int i = 0; i < vec_size(&p->ig); ++i) {
        ignode_destruct(&vec_at(&p->ig, i));
    }
    vec_clear(&p->ig);
}

/* Returns interference graph node at index */
static IGNode* ig_node(Parser* p, int i) {
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < vec_size(&p->ig), "Index out of range");
    return &vec_at(&p->ig, i);
}

/* Adds symbol to buffer of live symbols
   Does nothing if already exists, returns 1
   Returns 1 if successful, 0 otherwise */
static int ig_add_live(Parser* p, SymbolId sym_id) {
    for (int i = 0; i < vec_size(&p->ig_live); ++i) {
        if (vec_at(&p->ig_live, i) == sym_id) {
            return 1;
        }
    }
    return vec_push_back(&p->ig_live, sym_id);
}

/* Removes symbol from buffer of live symbols,
   does nothing if symbol does not exist */
static void ig_remove_live(Parser* p, SymbolId sym_id) {
    for (int i = 0; i < vec_size(&p->ig_live); ++i) {
        if (vec_at(&p->ig_live, i) == sym_id) {
            vec_splice(&p->ig_live, i, 1);
            return;
        }
    }
    /* If symbol to remove is not in live symbol buffer,
       it is because the symbol is unused */
}

/* Remove all live symbols from buffer */
static void ig_clear_live(Parser* p) {
    vec_clear(&p->ig_live);
}

/* Links given symbol in interference graph with edges to all live symbols,
   2 way (live symbols also have edges to this symbol)
   Returns 1 if successful, 0 otherwise */
static int ig_link_live(Parser* p, SymbolId sym_id) {
    IGNode* node = &vec_at(&p->ig, sym_id);
    for (int i = 0 ; i < vec_size(&p->ig_live); ++i) {
        SymbolId other_id = vec_at(&p->ig_live, i);
        /* Do not link to self */
        if (other_id == sym_id) {
            continue;
        }

        IGNode* other_node = &vec_at(&p->ig, other_id);

        /* Link 2 way, this symbol to live symbol,
           live symbol to this symbol */
        if (!ignode_link(node, other_node)) return 0;
        if (!ignode_link(other_node, node)) return 0;
    }
    return 1;
}

/* Creates unlinked interference graph nodes for all the symbols
   in the symbol table
   The index of the symbol in the symbol table corresponds to the
   index of the node in the interference graph */
static void ig_create_nodes(Parser* p) {
    ASSERT(vec_size(&p->ig) == 0, "Interference graph nodes already exist");

    if (!vec_reserve(&p->ig, vec_size(&p->symbol))) goto error;
    for (int i = 0; i < vec_size(&p->symbol); ++i) {
        if (!vec_push_backu(&p->ig)) goto error;
        IGNode* node = &vec_back(&p->ig);
        ignode_construct(node);
    }

    return;

error:
    parser_set_error(p, ec_outofmemory);
}

/* Constructs edges for the interference graph using the control flow graph
   Requires interference graph nodes to have been created
   Requires liveness information for blocks to be computed */
static void ig_compute_edge(Parser* p) {
    for (int i = 0; i < vec_size(&p->cfg); ++i) {
        Block* blk = &vec_at(&p->cfg, i);

        /* New block, recalculate live symbols
           OUT[B] symbols are live for entire block,
           thus they are linked to each other */
        ig_clear_live(p);
        for (int j = 0; j < block_out_count(blk); ++j) {
            SymbolId sym_id = block_out(blk, j);
            if (!ig_link_live(p, sym_id)) goto error;
            if (!ig_add_live(p, block_out(blk, j))) goto error;
        }

        /* Calculate live symbols at each statement and
           link them */
        for (int j = block_stat_count(blk) - 1; j >= 0; --j) {
            Statement* stat = block_stat(blk, j);
            SymbolId syms[MAX_ARGS]; /* Buffer to hold symbols */

            ASSERT(MAX_ARGS >= 1, "Need at least 1 to hold def");
            int def_count = stat_def(stat, syms);
            for (int k = 0; k < def_count; ++k) {
                ASSERT(!symtab_isconstant(p, syms[k]),
                        "Assigned symbol should not be constant");
                ig_remove_live(p, syms[k]);
            }

            int use_count = stat_use(stat, syms);
            for (int k = 0; k < use_count; ++k) {
                if (symtab_isconstant(p, syms[k])) {
                    continue;
                }
                if (!ig_link_live(p, syms[k])) goto error;
                if (!ig_add_live(p, syms[k])) goto error;
            }
        }
    }

    return;

error:
    parser_set_error(p, ec_outofmemory);
}

/* Computes the spill cost for all nodes in the interference graph
   Requires interference graph nodes to have been created
   Requires loop nesting depth information for blocks to be computed */
static void ig_compute_spill_cost(Parser* p) {
    for (int i = 0; i < vec_size(&p->cfg); ++i) {
        Block* blk = &vec_at(&p->cfg, i);

        for (int j = 0; j < block_stat_count(blk); ++j) {
            Statement* stat = block_stat(blk, j);
            SymbolId syms[MAX_ARGS]; /* Buffer to hold symbols */

            int use_count = stat_use(stat, syms);
            for (int k = 0; k < use_count; ++k) {
                if (symtab_isconstant(p, syms[k])) {
                    continue;
                }
                unsigned operation_cost = 1;
                uint64_t weight = (uint64_t)powip(10, (unsigned)block_depth(blk));
                ignode_add_cost(ig_node(p, syms[k]), operation_cost * weight);
            }
        }
    }
}

/* Sorting function for use in ig_compute_color, lowest spill cost first */
static int ig_compute_color_sort_neighbor(const void* a, const void* b) {
    uint64_t l = ignode_cost(*(IGNode* const *)a);
    uint64_t r = ignode_cost(*(IGNode* const *)b);
    if (l < r) {
        return -1;
    }
    if (l == r) {
        return 0;
    }
    return 1;
}

/* Assigns register to each interference graph node or marks it for spilling
   Requires interference graph by created and linked
   Requires spill cost be computed */
static void ig_compute_color(Parser* p) {
    /* Mark each node for coloring or spilling */
    /* mark, index by IGNode index in container:
        0 = Unmarked
        1 = Marked for coloring
        2 = Marked for spilling */
    char mark[vec_size(&p->ig)];
    for (int i = 0; i < vec_size(&p->ig); ++i) {
        mark[i] = 0;
    }

    for (int i = 0; i < vec_size(&p->ig); ++i) {
        IGNode* node = &vec_at(&p->ig, i);
        /* Already marked */
        if (mark[i] != 0) {
            continue;
        }

        /* Count number of neighbors (unmarked and marked for coloring)
           Also obtain the neighbors for later use below */
        IGNode* neighbor[ignode_neighbor_count(node)];
        int neighbor_count = 0;
        for (int j = 0; j < ignode_neighbor_count(node); ++j) {
            IGNode* neigh = ignode_neighbor(node, j);
            int neighbor_index = neigh - &vec_at(&p->ig, 0);
            /* Not marked for spilling */
            if (mark[neighbor_index] != 2) {
                neighbor[neighbor_count++] = neigh;
            }
        }
        if (neighbor_count < X86_REGISTER_COUNT) {
            /* No spilling needed, this node gets colored */
            mark[i] = 1;
            continue;
        }

        /* Deciding whether to spill neighbors or self:
           Find n lowest spill cost neighbors, where n is the number of nodes
           which must be spilled to have (neighbors < registers)
           If sum of spill cost for n neighbors < spill cost of self: spill
           the neighbors.
           Otherwise, spill self */

        /* Sort neighbor by spill cost (lowest cost first) */
        quicksort(neighbor, (size_t)neighbor_count, sizeof(IGNode*),
                ig_compute_color_sort_neighbor);

        /* Compute neighbor spill cost */
        int spill_amount = neighbor_count - X86_REGISTER_COUNT + 1;
        uint64_t neighbor_spill_cost = 0;
        for (int j = 0; j < spill_amount; ++j) {
            neighbor_spill_cost += ignode_cost(neighbor[j]);
        }

        /* Decide which to spill */
        if (neighbor_spill_cost < ignode_cost(node)) {
            /* Spill neighbors */
            for (int j = 0; j < spill_amount; ++j) {
                IGNode* neigh = ignode_neighbor(node, j);
                int neighbor_index = neigh - &vec_at(&p->ig, 0);
                mark[neighbor_index] = 2;
            }
            /* self gets colored */
            mark[i] = 1;
        }
        else {
            /* Spill self */
            mark[i] = 2;
        }
    }

    for (int i = 0; i < vec_size(&p->ig); ++i) {
        ASSERTF(mark[i] != 0, "Node %d is unmarked", i);
        /* TODO color each node */
    }
}

/* Sets up parser data structures to begin parsing a new function */
static void parser_clear_func(Parser* p) {
    cfg_clear(p);
    ig_clear(p);
    symtab_clear(p);
}

/* Dumps contents stored in parser */
static void debug_dump(Parser* p) {
    LOGF("Symbol table: [%d]\n", vec_size(&p->symbol));
    for (int i = 0; i < vec_size(&p->symbol); ++i) {
        Symbol* sym = &vec_at(&p->symbol, i);
        Type type = symbol_type(sym);
        LOGF("  %s", type_specifiers_str(type.typespec));
        for (int j = 0; j < type.pointers; ++j) {
            LOG("*");
        }
        LOGF(" %s %d\n", symbol_name(sym), symbol_location(sym));
    }

    /* Print out instructions and arguments of each node */
    LOGF("Control flow graph [%d]\n", vec_size(&p->cfg));
    for (int i = 0; i < vec_size(&p->cfg); ++i) {
        LOGF("  Block %d\n", i);
        Block* blk = &vec_at(&p->cfg, i);

        /* Labels associated with block */
        if (block_lab_count(blk) > 0) {
            LOG("    Labels:");
            for (int j = 0; j < block_lab_count(blk); ++j) {
                Symbol* sym = symtab_get(p, block_lab(blk, j));
                LOGF(" %s", symbol_name(sym));
            }
            LOG("\n");
        }

        LOGF("    Loop depth: %d\n", block_depth(blk));

        /* Liveness use/def, in/out for block */
        LOG("    Liveness\n");
        LOG("      use:");
        for (int j = 0; j < vec_size(&blk->use); ++j) {
            Symbol* sym = symtab_get(p, vec_at(&blk->use, j));
            LOGF(" %s", symbol_name(sym));
        }
        LOG("\n");
        LOG("      def:");
        for (int j = 0; j < vec_size(&blk->def); ++j) {
            Symbol* sym = symtab_get(p, vec_at(&blk->def, j));
            LOGF(" %s", symbol_name(sym));
        }
        LOG("\n");
        LOG("      IN:");
        for (int j = 0; j < vec_size(&blk->in); ++j) {
            Symbol* sym = symtab_get(p, vec_at(&blk->in, j));
            LOGF(" %s", symbol_name(sym));
        }
        LOG("\n");
        LOG("      OUT:");
        for (int j = 0; j < vec_size(&blk->out); ++j) {
            Symbol* sym = symtab_get(p, vec_at(&blk->out, j));
            LOGF(" %s", symbol_name(sym));
        }
        LOG("\n");



        /* Print instruction and arguments */
        for (int j = 0; j < block_stat_count(blk); ++j) {
            Statement* stat = block_stat(blk, j);
            LOGF("    %d %s", j, ins_str(stat_ins(stat)));
            for (int k = 0; k < stat_argc(stat); ++k) {
                Symbol* arg_sym = symtab_get(p, stat_arg(stat, k));
                if (k == 0) {
                    LOG(" ");
                }
                else {
                    LOG(",");
                }
                LOGF("%s", symbol_name(arg_sym));
            }
            LOG("\n");
        }

        /* Print next block index */
        LOG("    ->");
        for (int j = 0; j < 2; ++j) {
            if (block_next(blk, j)) {
                LOGF(" %ld", block_next(blk, j) - vec_data(&p->cfg));
            }
        }
        LOG("\n");
    }

    LOGF("Interference graph [%d]\n", vec_size(&p->ig));
    for (int i = 0; i < vec_size(&p->ig); ++i) {
        LOGF("  Node %d %s\n", i, symbol_name(symtab_get(p, i)));
        IGNode* node = ig_node(p, i);

        /* Spill cost */
        LOGF("    Spill cost %ld\n", ignode_cost(node));

        /* Neighbors of node */
        LOGF("    Neighbors [%d]", ignode_neighbor_count(node));
        for (int j = 0; j < ignode_neighbor_count(node); ++j) {
            LOGF(" %ld", ignode_neighbor(node, j) - node + i);
        }
        LOG("\n");
    }

}


/* ============================================================ */
/* Instruction handlers */
/* INSTRUCTION_PROC if it exists checks the arguments for
   validity as it runs first, otherwise INSTRUCTION_CG checks */

/* Helpers */
static void cg_arithmetic(
        Parser* p,
        SymbolId dest_id, SymbolId op1_id, SymbolId op2_id, const char* ins);
static void cg_cmpsetcc(
        Parser* p,
        SymbolId dest_id, SymbolId op1_id, SymbolId op2_id, const char* set);
static void cg_testjmpcc(
        Parser* p,
        SymbolId label_id, SymbolId op1_id, const char* jmp);
static void cg_divide(
        Parser* p, SymbolId op1_id, SymbolId op2_id,
        const char* ins);
static void cg_mov_tor(Parser* p, SymbolId source, GRegister dest);
static void cg_mov_fromr(Parser* p, GRegister source, SymbolId dest);
static void cg_ref_symbol(Parser* p, SymbolId sym_id);
static void cg_extract_param(const char* str, char* type, char* name);
static void cg_validate_equal_size2(Parser* p,
        SymbolId lval_id, SymbolId rval_id);
static void cg_validate_equal_size3(Parser* p,
        SymbolId lval_id, SymbolId op1_id, SymbolId op2_id);

/* argv contains argc pointers, each pointer is to null terminated argument string */
#define INSTRUCTION_PROC(name__) \
    void il_proc_ ## name__ (Parser* p, char** pparg, int arg_count)
#define INSTRUCTION_CG(name__) \
    void il_cg_ ## name__ (Parser* p, Statement* stat)

static INSTRUCTION_PROC(def) {
    if (arg_count != 1) {
        parser_set_error(p, ec_badargs);
        return;
    }

    char type[MAX_ARG_LEN];
    char name[MAX_ARG_LEN];
    cg_extract_param(pparg[0], type, name);

    Symbol* sym = symtab_get(p, symtab_add(p, type_from_str(type), name));
    p->stack_bytes += symbol_bytes(sym);
}

static INSTRUCTION_PROC(func) {
    if (arg_count < 2) {
        parser_set_error(p, ec_badargs);
        return;
    }

    parser_clear_func(p);
    cfg_new_block(p);

    /* Special handling of main function */
    if (strequ(pparg[0], "main")) {
        if (arg_count != 4) {
            parser_set_error(p, ec_badmain);
            return;
        }

        if (parser_has_error(p)) return;
        char type[MAX_ARG_LEN];
        char name[MAX_ARG_LEN];

        /* Symbol argc */
        cg_extract_param(pparg[2], type, name);
        SymbolId argc_id = symtab_add(p, type_from_str(type), name);
        if (parser_has_error(p)) return;

        /* Symbol pparg */
        cg_extract_param(pparg[3], type, name);
        SymbolId pparg_id = symtab_add(p, type_from_str(type), name);
        if (parser_has_error(p)) return;

        /* Wait on obtaining pointers until all symbols added
           as add may invalidate pointer */
        symtab_get(p, argc_id)->reg = reg_edi;
        symtab_get(p, pparg_id)->reg = reg_esi;

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
    }

    strcopy(pparg[0], p->func_name);
}

static INSTRUCTION_PROC(jmp) {
    if (arg_count != 1) {
        parser_set_error(p, ec_badargs);
        return;
    }
    cfg_new_block(p);
}

static INSTRUCTION_PROC(jnz) {
    if (arg_count != 2) {
        parser_set_error(p, ec_badargs);
        return;
    }
    cfg_link_new_block(p);
}

static INSTRUCTION_PROC(jz) {
    if (arg_count != 2) {
        parser_set_error(p, ec_badargs);
        return;
    }
    cfg_link_new_block(p);
}

static INSTRUCTION_PROC(lab) {
    if (arg_count != 1) {
        parser_set_error(p, ec_badargs);
        return;
    }
    Block* blk = cfg_latest_block(p);
    /* Simplify cfg by making consecutive labels part of one block */
    if (block_stat_count(blk) != 0) {
        blk = cfg_link_new_block(p);
    }
    block_add_label(blk, symtab_find(p, pparg[0]));
}

static INSTRUCTION_PROC(ret) {
    if (arg_count != 1) {
        parser_set_error(p, ec_badargs);
        return;
    }
    cfg_new_block(p);
}

static INSTRUCTION_CG(add) {
    if (stat_argc(stat) != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
    SymbolId lval_id = stat_arg(stat, 0);
    SymbolId op1_id = stat_arg(stat, 1);
    SymbolId op2_id = stat_arg(stat, 2);

    cg_arithmetic(p, lval_id, op1_id, op2_id, "add");
}

static INSTRUCTION_CG(ce) {
    if (stat_argc(stat) != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
    SymbolId lval_id = stat_arg(stat, 0);
    SymbolId op1_id = stat_arg(stat, 1);
    SymbolId op2_id = stat_arg(stat, 2);

    cg_cmpsetcc(p, lval_id, op1_id, op2_id, "sete");
}

static INSTRUCTION_CG(cl) {
    if (stat_argc(stat) != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
    SymbolId lval_id = stat_arg(stat, 0);
    SymbolId op1_id = stat_arg(stat, 1);
    SymbolId op2_id = stat_arg(stat, 2);

    cg_cmpsetcc(p, lval_id, op1_id, op2_id, "setl");
}

static INSTRUCTION_CG(cle) {
    if (stat_argc(stat) != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
    SymbolId lval_id = stat_arg(stat, 0);
    SymbolId op1_id = stat_arg(stat, 1);
    SymbolId op2_id = stat_arg(stat, 2);

    cg_cmpsetcc(p, lval_id, op1_id, op2_id, "setle");
}

static INSTRUCTION_CG(cne) {
    if (stat_argc(stat) != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
    SymbolId lval_id = stat_arg(stat, 0);
    SymbolId op1_id = stat_arg(stat, 1);
    SymbolId op2_id = stat_arg(stat, 2);

    cg_cmpsetcc(p, lval_id, op1_id, op2_id, "setne");
}

static INSTRUCTION_CG(div) {
    if (stat_argc(stat) != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
    /* Division needs special handling, since it behaves differently
       Dividend in ax, Result in ax */
    SymbolId lval_id = stat_arg(stat, 0);
    SymbolId op1_id = stat_arg(stat, 1);
    SymbolId op2_id = stat_arg(stat, 2);
    cg_validate_equal_size3(p, lval_id, op1_id, op2_id);

    cg_divide(p, op1_id, op2_id, "idiv");
    cg_mov_fromr(p, reg_a, lval_id);
}

static INSTRUCTION_CG(jmp) {
    SymbolId label_id = stat_arg(stat, 0);
    Symbol* label = symtab_get(p, label_id);

    parser_output_asm(p, "jmp %s\n", symbol_name(label));
}

static INSTRUCTION_CG(jnz) {
    SymbolId label_id = stat_arg(stat, 0);
    SymbolId op1_id = stat_arg(stat, 1);

    cg_testjmpcc(p, label_id, op1_id, "jnz");
}

static INSTRUCTION_CG(jz) {
    SymbolId label_id = stat_arg(stat, 0);
    SymbolId op1_id = stat_arg(stat, 1);

    cg_testjmpcc(p, label_id, op1_id, "jz");
}

static INSTRUCTION_CG(mod) {
    if (stat_argc(stat) != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }

    /* Mod is division but returning different part of result
       Dividend in ax, Remainder in dx */
    SymbolId lval_id = stat_arg(stat, 0);
    SymbolId op1_id = stat_arg(stat, 1);
    SymbolId op2_id = stat_arg(stat, 2);
    cg_validate_equal_size3(p, lval_id, op1_id, op2_id);

    cg_divide(p, op1_id, op2_id, "idiv");
    cg_mov_fromr(p, reg_d, lval_id);
}

static INSTRUCTION_CG(mov) {
    if (stat_argc(stat) != 2) {
        parser_set_error(p, ec_badargs);
        return;
    }

    SymbolId lval_id = stat_arg(stat, 0);
    SymbolId rval_id = stat_arg(stat, 1);
    cg_validate_equal_size2(p, lval_id, rval_id);

    cg_mov_tor(p, rval_id, reg_a);
    cg_mov_fromr(p, reg_a, lval_id);
}

static INSTRUCTION_CG(mul) {
    if (stat_argc(stat) != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
    SymbolId lval_id = stat_arg(stat, 0);
    SymbolId op1_id = stat_arg(stat, 1);
    SymbolId op2_id = stat_arg(stat, 2);

    cg_arithmetic(p, lval_id, op1_id, op2_id, "imul");
}

static INSTRUCTION_CG(not) {
    if (stat_argc(stat) != 2) {
        parser_set_error(p, ec_badargs);
        return;
    }

    SymbolId lval_id = stat_arg(stat, 0);
    SymbolId rval_id = stat_arg(stat, 1);
    cg_validate_equal_size2(p, lval_id, rval_id);

    Symbol* rval = symtab_get(p, rval_id);
    GRegister reg = reg_a; /* Where to store intermediate value */
    int bytes = symbol_bytes(rval);
    const char* reg_name = reg_get_str(reg_a, bytes);
    const char* reg_lower_name = reg_get_str(reg_a, 1);

    cg_mov_tor(p, rval_id, reg);
    parser_output_asm(p, "test %s,%s\n", reg_name, reg_name);
    parser_output_asm(p, "setz %s\n", reg_lower_name);

    /* Setz only sets 1 byte, thus the the remaining register
       has to be cleared to obtain 1 or 0 */
    if (bytes > 1) {
        parser_output_asm(p, "movzx %s,%s\n", reg_name, reg_lower_name);
    }
    cg_mov_fromr(p, reg, lval_id);
}

static INSTRUCTION_CG(ret) {
    SymbolId sym_id = stat_arg(stat, 0);
    Symbol* sym = symtab_get(p, sym_id);
    if (sym == NULL) {
        parser_set_error(p, ec_unknownsym);
        goto exit;
    }

    int bytes = symbol_bytes(sym);
    /* Return integer types in rax */
    parser_output_asm(p, "mov %s,", reg_get_str(reg_a, bytes));
    cg_ref_symbol(p, sym_id);
    parser_output_asm(p, "\n");

    /* Function epilogue */
    parser_output_asm(p,
        "leave\n"
        "ret\n"
    );

exit:
    return;
}

static INSTRUCTION_CG(sub) {
    if (stat_argc(stat) != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
    SymbolId lval_id = stat_arg(stat, 0);
    SymbolId op1_id = stat_arg(stat, 1);
    SymbolId op2_id = stat_arg(stat, 2);

    cg_arithmetic(p, lval_id, op1_id, op2_id, "sub");
}

/* Generates the necessary instructions to implement each arithmetic operation
   Generates very inefficient code, but goal is to get something functioning
   right now.
   1. Moves both operands into registers
   2. Performs operation using instruction
   3. Moves register where op_1 used to be to destination */
static void cg_arithmetic(
        Parser* p,
        SymbolId dest_id, SymbolId op1_id, SymbolId op2_id, const char* ins) {
    cg_validate_equal_size3(p, dest_id, op1_id, op2_id);

    GRegister op1_reg = reg_a;
    GRegister op2_reg = reg_c;
    Symbol* dest = symtab_get(p, dest_id);
    int bytes = symbol_bytes(dest);

    cg_mov_tor(p, op1_id, op1_reg);
    cg_mov_tor(p, op2_id, op2_reg);
    parser_output_asm(p, "%s %s,%s\n",
        ins,
        reg_get_str(op1_reg, bytes), reg_get_str(op2_reg, bytes));
    cg_mov_fromr(p, op1_reg, dest_id);
}

/* Generates the necessary instructions to implement logical operators:
   1. Moves both operands into registers
   2. Compares (op1 cmp op2)
   3. Performs the indicated conditional set using the results from comparison
   4. Zero extends result (0 or 1) to bytes of destination
   5. Moves the result to destination */
static void cg_cmpsetcc(
        Parser* p,
        SymbolId dest_id, SymbolId op1_id, SymbolId op2_id, const char* set) {
    cg_validate_equal_size3(p, dest_id, op1_id, op2_id);

    GRegister op1_reg = reg_a;
    GRegister op2_reg = reg_c;
    Symbol* dest = symtab_get(p, dest_id);
    int bytes = symbol_bytes(dest);
    const char* op1_reg_name = reg_get_str(op1_reg, bytes);
    const char* op1_reg_lower_name = reg_get_str(op1_reg, 1);

    cg_mov_tor(p, op1_id, op1_reg);
    cg_mov_tor(p, op2_id, op2_reg);
    parser_output_asm(p, "cmp %s,%s\n",
        op1_reg_name, reg_get_str(op2_reg, bytes));
    parser_output_asm(p, "%s %s\n", set, op1_reg_lower_name);
    if (bytes > 1) {
        parser_output_asm(p, "movzx %s,%s\n", op1_reg_name, op1_reg_lower_name);
    }
    cg_mov_fromr(p, op1_reg, dest_id);
}

/* Generates the necessary instructions to implement conditional jump
   1. Moves operand into register
   2. Tests (op1, op1)
   3. Performs the conditional jump to label using the results from test */
static void cg_testjmpcc(
        Parser* p,
        SymbolId label_id, SymbolId op1_id, const char* jmp) {
    GRegister reg = reg_a;
    Symbol* label = symtab_get(p, label_id);
    Symbol* op1 = symtab_get(p, op1_id);
    const char* reg_name = reg_get_str(reg, symbol_bytes(op1));

    cg_mov_tor(p, op1_id, reg);
    parser_output_asm(p, "test %s,%s\n", reg_name, reg_name);
    parser_output_asm(p, "%s %s\n", jmp, symbol_name(label));
}

/* Places op1 and op2 in the appropriate locations for op1 / op2
   ins is the instruction to apply to perform the division */
static void cg_divide(
        Parser* p, SymbolId op1_id, SymbolId op2_id,
        const char* ins) {
    GRegister op2_reg = reg_c;

    Symbol* op1 = symtab_get(p, op1_id);
    int bytes = symbol_bytes(op1);

    cg_mov_tor(p, op1_id, reg_a);
    cg_mov_tor(p, op2_id, op2_reg);
    /* dx has to be zeroed, other it is interpreted as part of dividend */
    parser_output_asm(p, "xor %s,%s\n",
            reg_get_str(reg_d, bytes),
            reg_get_str(reg_d, bytes));

    parser_output_asm(p, "%s %s\n", ins, reg_get_str(op2_reg, bytes));
}

/* Generates code for reg/mem -> reg */
static void cg_mov_tor(Parser* p, SymbolId source, GRegister dest) {
    Symbol* sym = symtab_get(p, source);
    int bytes = symbol_bytes(sym);

    parser_output_asm(p, "mov %s,", reg_get_str(dest, bytes));
    cg_ref_symbol(p, source);
    parser_output_asm(p, "\n");
}

/* Generates code for reg -> reg/mem */
static void cg_mov_fromr(Parser* p, GRegister source, SymbolId dest) {
    Symbol* sym = symtab_get(p, dest);
    int bytes = symbol_bytes(sym);

    parser_output_asm(p, "mov ");
    cg_ref_symbol(p, dest);
    parser_output_asm(p, ",%s\n", reg_get_str(source, bytes));
}

/* Generates assembly code to reference symbol
   Example: "dword [rbp+10]" */
static void cg_ref_symbol(Parser* p, SymbolId sym_id) {
    Symbol* sym = symtab_get(p, sym_id);

    if (symbol_on_stack(sym)) {
        char sign = '+';
        int abs_offset = symtab_get_offset(p, sym_id);
        if (abs_offset < 0) {
            sign = '-';
            abs_offset = -abs_offset;
        }
        const char* size_dir = asm_size_directive(symbol_bytes(sym));
        parser_output_asm(p, "%s [rbp%c%d]", size_dir, sign, abs_offset);
    }
    else if (symbol_is_constant(sym)) {
        parser_output_asm(p, "%s", symbol_name(sym));
    }
    else {
        /* Symbol in register */
        parser_output_asm(p, "%s", reg_str(symbol_location(sym)));
    }
}

/* Extracts the type and the name from str into name and type
   Assumes type and name is of sufficient size */
static void cg_extract_param(const char* str, char* type, char* name) {
    char c;
    int i = 0;
    /* Extract type */
    {
        while ((c = str[i]) != ' ' && c != '\0') {
            type[i] = str[i];
            ++i;
        }
        type[i] = '\0';
    }
    /* Extract name */
    {
        if (c == '\0') { /* No name provided */
            name[0] = '\0';
            return;
        }
        ++i; /* Skip the space which was encountered */
        int i_out = 0;
        while ((c = str[i]) != '\0') {
            name[i_out] = str[i];
            ++i;
            ++i_out;
        }
        name[i_out] = '\0';
    }
}

/* Asserts the given symbols are the same size */
static void cg_validate_equal_size2(Parser* p,
        SymbolId lval_id, SymbolId rval_id) {
    Symbol* lval = symtab_get(p, lval_id);
    Symbol* rval = symtab_get(p, rval_id);
    ASSERT(symbol_bytes(lval) == symbol_bytes(rval), "Non equal type sizes");
}

/* Asserts the given symbols are the same size */
static void cg_validate_equal_size3(Parser* p,
        SymbolId lval_id, SymbolId op1_id, SymbolId op2_id) {
    Symbol* lval = symtab_get(p, lval_id);
    Symbol* op1 = symtab_get(p, op1_id);
    Symbol* op2 = symtab_get(p, op2_id);
    ASSERT(symbol_bytes(lval) == symbol_bytes(op1), "Non equal type sizes");
    ASSERT(symbol_bytes(op1) == symbol_bytes(op2), "Non equal type sizes");
}

/* ============================================================ */
/* Initialization and configuration */

/* Repositions file to begin reading from the beginning */
static void seek_to_start(Parser* p) {
    if (fseek(p->rf, 0, SEEK_SET) != 0) {
        parser_set_error(p, ec_seekfailed);
        return;
    }
}

/* Reads one instruction and its arguments,
   the result is stored in the parser
   Returns 1 if successfully read, 0 if EOF or error */
static int read_instruction(Parser* p) {
    /* Read instruction */
    p->ins_len = 0;
    char c;
    while(1) {
        c = getc(p->rf);
        if (c == EOF) {
            p->ins[p->ins_len] = '\0';
            return 0;
        }
        if (c == '\n') {
            p->ins[p->ins_len] = '\0';
            return 1;
        }
        if (c == ' ') {
            p->ins[p->ins_len] = '\0';
            break;
        }
        /* Also need space for null terminator */
        if (p->ins_len >= MAX_INSTRUCTION_LEN - 1) {
            p->ins[p->ins_len] = '\0';
            parser_set_error(p, ec_insbufexceed);
            return 0;
        }
        p->ins[p->ins_len] = c;
        ++p->ins_len;
    }

    /* Read argument
       Arguments are stored together in one buffer
       With \0 between them, arg_table[i] points to the beginning
       of each argument */
    /* Add argument to argument list upon seeing its beginning,
       guaranteed one argument since space seen */
    p->arg_count = 0;
    if (p->arg_count >= MAX_ARGS) {
        parser_set_error(p, ec_argbufexceed);
        return 0;
    }
    p->arg_table[0] = p->arg;
    ++p->arg_count;

    int i = 0;
    while(1) {
        c = getc(p->rf);
        if (c == EOF) {
            p->arg[i] = '\0';
            return 0;
        }
        if (c == '\n') {
            p->arg[i] = '\0';
            return 1;
        }

        if (c == ',') {
            p->arg[i] = '\0';
            if (p->arg_count >= MAX_ARGS) {
                parser_set_error(p, ec_argbufexceed);
                return 0;
            }
            p->arg_table[p->arg_count] = p->arg + i + 1;
            ++p->arg_count;
        }
        else {
            /* -1 as also need space for null terminator */
            if (i >= MAX_ARG_LEN - 1) {
                parser_set_error(p, ec_argbufexceed);
                p->arg[i] = '\0';
                return 0;
            }
            p->arg[i] = c;
        }
        ++i;
    }
}

static void parse(Parser* p) {
    while (read_instruction(p)) {
        InsProcHandler proc_handler = il_get_proc(p->ins);
        InsCgHandler cg_handler = il_get_cg(p->ins);
        /* Verify is valid instruction */
        if (proc_handler == NULL && cg_handler == NULL) {
            ERRMSGF("Unrecognized instruction %s\n", p->ins);
            parser_set_error(p, ec_invalidins);
            return;
        }

        if (ins_incfg(ins_from_str(p->ins))) {
            Statement stat =
                {.ins = ins_from_str(p->ins), .argc = p->arg_count};
            for (int i = 0; i < p->arg_count; ++i) {
                SymbolId sym_id = symtab_find(p, p->arg_table[i]);
                ASSERTF(sym_id >= 0, "Invalid SymbolId %s", p->arg_table[i]);
                stat.arg[i] = sym_id;
            }

            cfg_append_latest(p, stat);
        }
        /* Instruction gets added first
           then new block is made (e.g., jmp at last block, then new block) */
        if (proc_handler != NULL) {
            proc_handler(p, p->arg_table, p->arg_count);
        }
        if (parser_has_error(p)) return;
    }

    /* Process the last (most recent) function */
    cfg_link_jump_dest(p);
    cfg_compute_liveness(p);
    cfg_compute_loop_depth(p);
    ig_create_nodes(p);
    ig_compute_edge(p);
    ig_compute_spill_cost(p);
    ig_compute_color(p);
    cfg_output_asm(p);
}

/* Parses cli args and processes them */
/* NOTE: will not clean up file handles at exit */
/* Returns non zero if error */
static int handle_cli_arg(Parser* p, int argc, char** argv) {
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
        else if (strequ(argv[i], "-Zd4")) {
            g_debug_print_buffers = 1;
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

    Parser p = {.rf = NULL, .of = NULL};
    parser_construct(&p);

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

    parse(&p);

exit:
    /* Indicate to the user cause for exiting if errored during parsing */
    if (parser_has_error(&p)) {
        ErrorCode ecode = parser_get_error(&p);
        ERRMSGF("Error during parsing: %d %s\n", ecode, errcode_str[ecode]);
        exitcode = ecode;
    }
    if (g_debug_print_buffers) {
        debug_dump(&p);
    }

    if (p.rf != NULL) {
        fclose(p.rf);
    }
    if (p.of != NULL) {
        fclose(p.of);
    }
    parser_destruct(&p);
    return exitcode;
}

