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

/* ============================================================ */
/* Parser global configuration */

int g_debug_print_buffers = 0;

/* ============================================================ */
/* x86 Registers */

/* All the registers in x86-64 (just 8 byte ones for now) */
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
    INSTRUCTION_PC(func) \
    INSTRUCTION_PC(jmp)  \
    INSTRUCTION_PC(jnz)  \
    INSTRUCTION_PC(jz)   \
    INSTRUCTION_PC(lab)  \
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

typedef struct {
    ILIns ins;
    SymbolId arg[MAX_ARGS];
    int argc;
} Statement;

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

/* Blocks are formed by partitioning il statements according to the rules:
   1. Control always enters at the start of the block
   2. Control always leaves at the last statement or end of the block */
typedef struct {
    /* Labels at the entry of this block */
    vec_t(SymbolId) labels;
    vec_t(Statement) stats;
    /* At most 2 options:
       1. Flow through to next block
       2. Jump at end
       Is offset (in Block) from current location, cannot use pointer
       as container holding Block may resize */
    int next[2];
} Block;

static void block_construct(Block* blk) {
    ASSERT(blk != NULL, "Block is null");
    vec_construct(&blk->labels);
    vec_construct(&blk->stats);
    blk->next[0] = 0;
    blk->next[1] = 0;
}

static void block_destruct(Block* blk) {
    ASSERT(blk != NULL, "Block is null");
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

/* Links block to next block */
static void block_link(Block* blk, Block* next) {
    ASSERT(blk != NULL, "Block is null");
    ASSERT(blk != next, "Cannot link block to self");
    for (int i = 0; i < 2; ++i ) {
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
    ASSERT(i < 2, "Index out of range");

    if (blk->next[i] == 0) {
        return NULL;
    }
    return blk + blk->next[i];
}

typedef struct {
    ErrorCode ecode;

    FILE* rf; /* Input file */
    FILE* of; /* Generated code goes in this file */

    /* For one function only for now */

    /* First symbol element is earliest in occurrence */
    vec_t(Symbol) symbol;
    int stack_bytes; /* Bytes stack needs */

    /* Control flow graph */
    vec_t(Block) cfg;
    Block* latest_blk;

    /* Instruction, argument */
    char ins[MAX_INSTRUCTION_LEN];
    int ins_len;
    char arg[MAX_ARG_LEN];
    char* arg_table[MAX_ARGS]; /* Points to beginning of each argument */
    int arg_count; /* Number of arguments */
} Parser;

static void parser_construct(Parser* p) {
    vec_construct(&p->symbol);
    vec_construct(&p->cfg);
}

static void parser_destruct(Parser* p) {
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
       them to the table when not found to make them exist
       '-' for negative numbers */
    if (('0' <= name[0] && name[0] <= '9') || name[0] == '-') {
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
                Symbol* lab_sym = symtab_get(p, block_lab(blk, j));
                LOGF(" %s", symbol_name(lab_sym));
            }
            LOG("\n");
        }

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

typedef void(*InsHandler) (Parser*, char**, int);
/* argv contains argc pointers, each pointer is to null terminated argument string */
#define INSTRUCTION_PROC(name__) \
    void il_proc_ ## name__ (Parser* p, char** pparg, int arg_count)
#define INSTRUCTION_CG(name__) \
    void il_cg_ ## name__ (Parser* p, char** pparg, int arg_count)

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

    cfg_clear(p);
    symtab_clear(p);
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
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
    SymbolId lval_id = symtab_find(p, pparg[0]);
    SymbolId op1_id = symtab_find(p, pparg[1]);
    SymbolId op2_id = symtab_find(p, pparg[2]);

    cg_arithmetic(p, lval_id, op1_id, op2_id, "add");
}

static INSTRUCTION_CG(ce) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
    SymbolId lval_id = symtab_find(p, pparg[0]);
    SymbolId op1_id = symtab_find(p, pparg[1]);
    SymbolId op2_id = symtab_find(p, pparg[2]);

    cg_cmpsetcc(p, lval_id, op1_id, op2_id, "sete");
}

static INSTRUCTION_CG(cl) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
    SymbolId lval_id = symtab_find(p, pparg[0]);
    SymbolId op1_id = symtab_find(p, pparg[1]);
    SymbolId op2_id = symtab_find(p, pparg[2]);

    cg_cmpsetcc(p, lval_id, op1_id, op2_id, "setl");
}

static INSTRUCTION_CG(cle) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
    SymbolId lval_id = symtab_find(p, pparg[0]);
    SymbolId op1_id = symtab_find(p, pparg[1]);
    SymbolId op2_id = symtab_find(p, pparg[2]);

    cg_cmpsetcc(p, lval_id, op1_id, op2_id, "setle");
}

static INSTRUCTION_CG(cne) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
    SymbolId lval_id = symtab_find(p, pparg[0]);
    SymbolId op1_id = symtab_find(p, pparg[1]);
    SymbolId op2_id = symtab_find(p, pparg[2]);

    cg_cmpsetcc(p, lval_id, op1_id, op2_id, "setne");
}

static INSTRUCTION_CG(div) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
    /* Division needs special handling, since it behaves differently
       Dividend in ax, Result in ax */
    SymbolId lval_id = symtab_find(p, pparg[0]);
    SymbolId op1_id = symtab_find(p, pparg[1]);
    SymbolId op2_id = symtab_find(p, pparg[2]);
    cg_validate_equal_size3(p, lval_id, op1_id, op2_id);

    cg_divide(p, op1_id, op2_id, "idiv");
    cg_mov_fromr(p, reg_a, lval_id);
}

static INSTRUCTION_CG(func) {
    /* Function labels always with prefix f@ */
    parser_output_asm(p,
        "f@%s:\n"
        /* Function prologue */
        "push rbp\n"
        "mov rbp,rsp\n",
        pparg[0]
    );
    /* Reserve stack space */
    if (p->stack_bytes != 0) {
        parser_output_asm(p, "sub rsp,%d\n", p->stack_bytes);
    }
}

static INSTRUCTION_CG(jmp) {
    SymbolId label_id = symtab_find(p, pparg[0]);
    Symbol* label = symtab_get(p, label_id);

    parser_output_asm(p, "jmp %s\n", symbol_name(label));
}

static INSTRUCTION_CG(jnz) {
    SymbolId label_id = symtab_find(p, pparg[0]);
    SymbolId op1_id = symtab_find(p, pparg[1]);

    cg_testjmpcc(p, label_id, op1_id, "jnz");
}

static INSTRUCTION_CG(jz) {
    SymbolId label_id = symtab_find(p, pparg[0]);
    SymbolId op1_id = symtab_find(p, pparg[1]);

    cg_testjmpcc(p, label_id, op1_id, "jz");
}

static INSTRUCTION_CG(lab) {
    parser_output_asm(p, "%s:\n", pparg[0]);
}

static INSTRUCTION_CG(mod) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }

    /* Mod is division but returning different part of result
       Dividend in ax, Remainder in dx */
    SymbolId lval_id = symtab_find(p, pparg[0]);
    SymbolId op1_id = symtab_find(p, pparg[1]);
    SymbolId op2_id = symtab_find(p, pparg[2]);
    cg_validate_equal_size3(p, lval_id, op1_id, op2_id);

    cg_divide(p, op1_id, op2_id, "idiv");
    cg_mov_fromr(p, reg_d, lval_id);
}

static INSTRUCTION_CG(mov) {
    if (arg_count != 2) {
        parser_set_error(p, ec_badargs);
        return;
    }

    SymbolId lval_id = symtab_find(p, pparg[0]);
    SymbolId rval_id = symtab_find(p, pparg[1]);
    cg_validate_equal_size2(p, lval_id, rval_id);

    cg_mov_tor(p, rval_id, reg_a);
    cg_mov_fromr(p, reg_a, lval_id);
}

static INSTRUCTION_CG(mul) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
    SymbolId lval_id = symtab_find(p, pparg[0]);
    SymbolId op1_id = symtab_find(p, pparg[1]);
    SymbolId op2_id = symtab_find(p, pparg[2]);

    cg_arithmetic(p, lval_id, op1_id, op2_id, "imul");
}

static INSTRUCTION_CG(not) {
    if (arg_count != 2) {
        parser_set_error(p, ec_badargs);
        return;
    }

    SymbolId lval_id = symtab_find(p, pparg[0]);
    SymbolId rval_id = symtab_find(p, pparg[1]);
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
    SymbolId sym_id = symtab_find(p, pparg[0]);
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
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
    SymbolId lval_id = symtab_find(p, pparg[0]);
    SymbolId op1_id = symtab_find(p, pparg[1]);
    SymbolId op2_id = symtab_find(p, pparg[2]);

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

/* Index of string is instruction handler index */
#define INSTRUCTION_P(name__) #name__,
#define INSTRUCTION_C(name__)
#define INSTRUCTION_PC(name__) #name__,
const char* instruction_proc_index[] = {INSTRUCTIONS};
#undef INSTRUCTION_P
#undef INSTRUCTION_PC

#define INSTRUCTION_P(name__) &il_proc_ ## name__,
#define INSTRUCTION_PC(name__) &il_proc_ ## name__,
const InsHandler instruction_proc_table[] = {INSTRUCTIONS};
#undef INSTRUCTION_P
#undef INSTRUCTION_C
#undef INSTRUCTION_PC

#define INSTRUCTION_P(name__)
#define INSTRUCTION_C(name__) #name__,
#define INSTRUCTION_PC(name__) #name__,
const char* instruction_cg_index[] = {INSTRUCTIONS};
#undef INSTRUCTION_C
#undef INSTRUCTION_PC

#define INSTRUCTION_C(name__) &il_cg_ ## name__,
#define INSTRUCTION_PC(name__) &il_cg_ ## name__,
const InsHandler instruction_cg_table[] = {INSTRUCTIONS};
#undef INSTRUCTION_P
#undef INSTRUCTION_C
#undef INSTRUCTION_PC

/* Returns function for processing instruction with provided name
   Returns null if not found */
static InsHandler il_get_proc(const char* name) {
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
static InsHandler il_get_cg(const char* name) {
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
        InsHandler proc_handler = il_get_proc(p->ins);
        InsHandler cg_handler = il_get_cg(p->ins);
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
                ASSERT(sym_id >= 0, "Invalid SymbolId");
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

    seek_to_start(p);
    while (read_instruction(p)) {
        InsHandler cg_handler = il_get_cg(p->ins);
        if (cg_handler == NULL) {
            continue;
        }

        parser_output_comment(p, "%s", p->ins);
        if (0 < p->arg_count) {
            parser_output_asm(p, " %s", p->arg_table[0]);
        }
        for (int i = 1; i < p->arg_count; ++i) {
            parser_output_asm(p, ",%s", p->arg_table[i]);
        }
        parser_output_asm(p, "\n");

        cg_handler(p, p->arg_table, p->arg_count);
        if (parser_has_error(p)) return;
    }
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

