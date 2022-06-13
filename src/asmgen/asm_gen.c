/*
 Reads in intermediate language (imm2)
 Generated output x86-64 assembly (imm3)
*/

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>

#include "../common.h"
#include "fwddecl.h"
#include "constant.h"
#include "ILIns.h"
#include "x86.h"
#include "ErrorCode.h"
#include "Symbol.h"
#include "ILStatement.h"
#include "PasmStatement.h"
#include "Block.h"
#include "IGNode.h"

/* ============================================================ */
/* Parser global configuration */

int g_debug_print_cfg = 0;
int g_debug_print_ig = 0;
int g_debug_print_info = 0;
int g_debug_print_symtab = 0;

/* ============================================================ */
/* Parser data structure + functions */

static IGNode* ig_node(Parser* p, int i);

typedef struct {
    AsmIns ins;
    /* See INSSEL_MACRO_REPLACE for usage of type */
    int op_type[MAX_ASM_OP];
    union {
        Location loc;
        Register reg;
        /* Index of the argument in IL instruction */
        int index;
    } op[MAX_ASM_OP];
    int op_count;
    /* See declaration of ISMRFlag in fwddecl.h */
    ISMRFlag flag[MAX_ASM_OP];
} InsSelMacroReplace;

/* Returns AsmIns */
static AsmIns ismr_ins(const InsSelMacroReplace* ismr) {
    ASSERT(ismr != NULL, "Macro replace is null");
    return ismr->ins;
}

/* Returns 1 if operand at index i is a new virtual register, 0 if not */
static int ismr_op_new(const InsSelMacroReplace* ismr, int i) {
    ASSERT(ismr != NULL, "Macro replace is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < ismr->op_count, "Index out of range");
    return ismr->op_type[i] == 0;
}

/* Returns 1 if operand at index i is a virtual register, 0 if not */
static int ismr_op_virtual(const InsSelMacroReplace* ismr, int i) {
    ASSERT(ismr != NULL, "Macro replace is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < ismr->op_count, "Index out of range");
    return ismr->op_type[i] == 1;
}

/* Returns 1 if operand at index i is a physical register location, 0 if not */
static int ismr_op_location(const InsSelMacroReplace* ismr, int i) {
    ASSERT(ismr != NULL, "Macro replace is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < ismr->op_count, "Index out of range");
    return ismr->op_type[i] == 2;
}

/* Returns 1 if operand at index i is a physical register, 0 if not */
static int ismr_op_physical(const InsSelMacroReplace* ismr, int i) {
    ASSERT(ismr != NULL, "Macro replace is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < ismr->op_count, "Index out of range");
    return ismr->op_type[i] == 3;
}

/* Interprets operand at index i as a virtual register (Symbol) and returns the
   index of the argument (SymbolId) in the IL instruction (to perform macro
   replacement) */
static int ismr_op_index(const InsSelMacroReplace* ismr, int i) {
    ASSERT(ismr != NULL, "Macro replace is null");
    ASSERT(ismr_op_new(ismr, i) || ismr_op_virtual(ismr, i),
            "Incorrect op1 type");
    return ismr->op[i].index;
}

/* Interprets operand at index i as a physical register location and returns
   it */
static Location ismr_op_loc(const InsSelMacroReplace* ismr, int i) {
    ASSERT(ismr != NULL, "Macro replace is null");
    ASSERT(ismr_op_location(ismr, i), "Incorrect op1 type");
    return ismr->op[i].loc;
}

/* Interprets operand at index i as a physical register and returns it */
static Register ismr_op_reg(const InsSelMacroReplace* ismr, int i) {
    ASSERT(ismr != NULL, "Macro replace is null");
    ASSERT(ismr_op_physical(ismr, i), "Incorrect op1 type");
    return ismr->op[i].reg;
}

/* Returns number of operands this replacement specifies */
static int ismr_op_count(const InsSelMacroReplace* ismr) {
    ASSERT(ismr != NULL, "Macro replace is null");
    return ismr->op_count;
}

/* Returns flag at index i */
static ISMRFlag ismr_flag(const InsSelMacroReplace* ismr, int i) {
    ASSERT(ismr != NULL, "Macro replace is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < ismr->op_count, "Index out of range");
    return ismr->flag[i];
}

/* For reading InsSelMacroReplace flags */

/* Returns flag size override InsSelMacroReplace */
static int ismr_size_override(ISMRFlag flag) {
    return flag & 0x0F;
}

/* Returns flag dereference InsSelMacroReplace flag */
static int ismr_dereference(ISMRFlag flag) {
    return (flag & 0x10) / 16;
}


typedef struct {
    /* See INSSEL_MACRO_CASE for usage of constraint string */
    const char* constraint;
    vec_t(InsSelMacroReplace) replace;
} InsSelMacroCase;

/* Returns constraint string for instruction selection macro case */
static const char* ismc_constraint(const InsSelMacroCase* ismc) {
    ASSERT(ismc != NULL, "Macro case is null");
    return ismc->constraint;
}

/* Returns the number of pseudo-assembly replacements for instruction selection
   macro case */
static int ismc_replace_count(const InsSelMacroCase* ismc) {
    ASSERT(ismc != NULL, "Macro case is null");
    return vec_size(&ismc->replace);
}

/* Returns the replacement at index for the instruction selection macro case */
static InsSelMacroReplace* ismc_replace(InsSelMacroCase* ismc, int index) {
    ASSERT(ismc != NULL, "Macro case is null");
    ASSERT(index >= 0, "Index out of range");
    ASSERT(index < ismc_replace_count(ismc), "Index out of range");
    return &vec_at(&ismc->replace, index);
}

typedef struct {
    ILIns ins;
    vec_t(InsSelMacroCase) cases;
} InsSelMacro;

/* Returns the IL instruction that the instruction selection macro is for */
static ILIns ism_il_ins(const InsSelMacro* macro) {
    ASSERT(macro != NULL, "Macro is null");
    return macro->ins;
}

/* Returns the number of cases the instruction selection macro has */
static int ism_case_count(const InsSelMacro* macro) {
    ASSERT(macro != NULL, "Macro is null");
    return vec_size(&macro->cases);
}

/* Returns the case at index for the instruction selection macro */
static InsSelMacroCase* ism_case(InsSelMacro* macro, int index) {
    ASSERT(macro != NULL, "Macro is null");
    ASSERT(index >= 0, "Index out of range");
    ASSERT(index < ism_case_count(macro), "Index out of range");
    return &vec_at(&macro->cases, index);
}

typedef vec_t(InsSelMacro) vec_InsSelMacro;

static int inssel_macro_construct(vec_InsSelMacro* macros);
static void inssel_macro_destruct(vec_InsSelMacro* macros);
static void parser_set_error(Parser* p, ErrorCode ecode);

struct Parser {
    ErrorCode ecode;

    FILE* rf; /* Input file */
    FILE* of; /* Generated code goes in this file */

    /* For one function only for now */

    /* First symbol element is earliest in occurrence */
    vec_t(Symbol) symbol;
    /* Used to create unique compiler generated symbols */
    int symtab_temp_num; /* Temporaries */

    char func_name[MAX_ARG_LEN]; /* Name of current function */

    /* Instruction selection */
    vec_InsSelMacro inssel_macro;

    /* Control flow graph */
    vec_t(Block) cfg;
    Block* latest_blk;
    /* Buffer of live symbols used when calculating per statement liveness */
    vec_t(SymbolId) cfg_live_buf;

    /* Interference graph
       Index of symbol in symbol table corresponds to index of its node
       in the interference graph */
    vec_t(IGNode) ig;
    /* The registers which can be assigned (colored) to nodes */
    Location ig_palette[X86_REGISTER_COUNT];
    int ig_palette_size;

    /* Buffer of PasmStatement to calculate save + restore range for register
       preference */
    vec_t(PasmStatement*) cfg_pasm_stack;

    /* Instruction, argument */
    char ins[MAX_INSTRUCTION_LEN];
    int ins_len;
    char arg[MAX_ARG_LEN];
    char* arg_table[MAX_ARGS]; /* Points to beginning of each argument */
    int arg_count; /* Number of arguments */
};

/* Returns 1 if succeeded, 0 if error */
static int parser_construct(Parser* p) {
    p->ecode = ec_noerr;
    p->rf = NULL;
    p->of = NULL;
    vec_construct(&p->symbol);
    p->symtab_temp_num = 0;
    p->func_name[0] = '\0';
    if (!inssel_macro_construct(&p->inssel_macro)) goto newerr;
    vec_construct(&p->cfg);
    vec_construct(&p->cfg_live_buf);
    p->latest_blk = NULL;
    vec_construct(&p->ig);

    p->ig_palette[0] = loc_a;
    p->ig_palette[1] = loc_b;
    p->ig_palette[2] = loc_c;
    p->ig_palette[3] = loc_d;
    p->ig_palette[4] = loc_si;
    p->ig_palette[5] = loc_di;
    /* Assume stack registers not usuable for now
    p->ig_palette[6] = loc_bp;
    p->ig_palette[7] = loc_sp; */
    p->ig_palette[6] = loc_8;
    p->ig_palette[7] = loc_9;
    p->ig_palette[8] = loc_10;
    p->ig_palette[9] = loc_11;
    p->ig_palette[10] = loc_12;
    p->ig_palette[11] = loc_13;
    p->ig_palette[12] = loc_14;
    p->ig_palette[13] = loc_15;
    p->ig_palette_size = 14;

    vec_construct(&p->cfg_pasm_stack);
    /* Instruction and argument does not need to be initialized
       it is set when reading from file */
    return 1;

newerr:
    parser_set_error(p, ec_outofmemory);
    return 0;
}

static void parser_destruct(Parser* p) {
    vec_destruct(&p->cfg_pasm_stack);
    for (int i = 0; i < vec_size(&p->ig); ++i) {
        ignode_destruct(&vec_at(&p->ig, i));
    }
    vec_destruct(&p->ig);
    vec_destruct(&p->cfg_live_buf);
    for (int i = 0; i < vec_size(&p->cfg); ++i) {
        block_destruct(&vec_at(&p->cfg, i));
    }
    vec_destruct(&p->cfg);
    inssel_macro_destruct(&p->inssel_macro);
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
static int symtab_is_constant(Parser* p, SymbolId sym_id) {
    return symbol_is_constant(symtab_get(p, sym_id));
}

/* Returns 1 if symbol is a label, 0 otherwise */
static int symtab_is_label(Parser* p, SymbolId sym_id) {
    return symbol_is_label(symtab_get(p, sym_id));
}

/* Returns 1 if symbol is a variable (requiring storage), 0 otherwise */
static int symtab_is_var(Parser* p, SymbolId sym_id) {
    return symbol_is_var(symtab_get(p, sym_id));
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

    if (g_debug_print_info) {
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
    sym->loc = loc_none;
    return vec_size(&p->symbol) - 1;
}

/* Creates a new compiler generated Symbol of given type */
static SymbolId symtab_add_temporary(Parser* p, Type type) {
    /* Double t so know this is from asm_gen */
    AAPPENDI(name, "__tt", p->symtab_temp_num);
    ++p->symtab_temp_num;
    return symtab_add(p, type, name);
}

/* Returns the size of the stack for the current function */
static int symtab_stack_bytes(Parser* p) {
    int size = 0;
    for (int i = 0; i < vec_size(&p->symbol); ++i) {
        Symbol* sym = &vec_at(&p->symbol, i);
        if (symbol_location(sym) == loc_stack) {
            size += symbol_bytes(sym);
        }
    }
    return size;
}

/* ============================================================ */
/* Control flow graph */

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

/* Links jumps from blocks to other blocks
   Returns 1 if successful, 0 if error */
static int cfg_link_jump_dest(Parser* p) {
    for (int i = 0; i < vec_size(&p->cfg); ++i) {
        Block* blk = &vec_at(&p->cfg, i);
        if (block_ilstat_count(blk) == 0) {
            continue;
        }

        ILStatement* last_stat = block_ilstat(blk, block_ilstat_count(blk) - 1);
        if (ins_isjump(ilstat_ins(last_stat))) {
            SymbolId lab_id = ilstat_arg(last_stat, 0);
            Block* target_blk = cfg_find_labelled(p, lab_id);
            if (target_blk == NULL) {
                Symbol* lab = symtab_get(p, lab_id);
                ERRMSGF("Could not find jump label %s\n", symbol_name(lab));
                parser_set_error(p, ec_invalidlabel);
                return 0;
            }
            block_link(blk, target_blk);
        }
    }
    return 1;
}

/* Appends statement into the latest block
   Returns 1 if successful, 0 if error */
static int cfg_append_latest(Parser* p, ILStatement stat) {
    ASSERT(p->latest_blk, "No block exists");
    if (!block_add_ilstat(p->latest_blk, stat)) {
        parser_set_error(p, ec_outofmemory);
        return 0;
    }
    return 1;
}

/* Computes liveness use/def information for each block
   Returns 1 if successful, 0 if error */
static int cfg_compute_use_def(Parser* p) {
    for (int i = 0; i < vec_size(&p->cfg); ++i) {
        Block* blk = &vec_at(&p->cfg, i);
        for (int j = block_pasmstat_count(blk) - 1; j >= 0; --j) {
            PasmStatement* stat = block_pasmstat(blk, j);
            SymbolId syms[MAX_ASM_OP]; /* Buffer to hold symbols */

            /* Add 'def'ed symbol from statement to 'def' for block
               Remove occurrences of 'def'd symbol from 'use' for block */
            int def_count = pasmstat_def(stat, syms);
            for (int k = 0; k < def_count; ++k) {
                ASSERT(symtab_is_var(p, syms[k]),
                        "Assigned symbol should be variable");
                if (!block_add_def(blk, syms[k])) goto newerr;
                block_remove_use(blk, syms[k]);
            }

            /* Add 'use'd symbols from statement to 'use' for block */
            ASSERT(MAX_ASM_OP >= 1, "Need at least 1 to hold def");
            int use_count = pasmstat_use(stat, syms);
            for (int k = 0; k < use_count; ++k) {
                if (!symtab_is_var(p, syms[k])) {
                    continue;
                }
                if (!block_add_use(blk, syms[k])) goto newerr;
            }
        }
    }
    return 1;

newerr:
    parser_set_error(p, ec_outofmemory);
    return 0;
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
   blk: The current block
   Returns 1 if successful, 0 if error */
static int cfg_compute_liveness_traverse(
        Parser* p, char* status, Block* base, Block* blk) {
    int index = (int)(blk - base);
    ASSERT(status[index] == 0, "Block should be untraversed");
    status[index] = 1; /* Block traversed */

    for (int i = 0; i < MAX_BLOCK_LINK; ++i) {
        Block* next = block_next(blk, i);
        if (!next) {
            continue;
        }

        /* Only traverse next if not traversed */
        if (status[next - base] == 0) {
            if (!cfg_compute_liveness_traverse(p, status, base, next)) {
                goto error;
            }
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
            if (!block_add_out(blk, new_in)) goto newerr;

            /* Check if is in def(B), if not add to IN[B] */
            if (!block_in_def(blk, new_in)) {
                if (!block_add_in(blk, new_in)) goto newerr;
            }
        }
        if (block_out_count(blk) != old_out_count) {
            status[index] = 2;
        }
    }
    return 1;

newerr:
    parser_set_error(p, ec_outofmemory);
error:
    return 0;
}

/* Adds symbol to buffer of live symbols
   Does nothing if already exists, returns 1
   Returns 1 if successful, 0 otherwise */
static int cfg_live_buf_add(Parser* p, SymbolId sym_id) {
    for (int i = 0; i < vec_size(&p->cfg_live_buf); ++i) {
        if (vec_at(&p->cfg_live_buf, i) == sym_id) {
            return 1;
        }
    }
    return vec_push_back(&p->cfg_live_buf, sym_id);
}

/* Removes symbol from buffer of live symbols,
   does nothing if symbol does not exist */
static void cfg_live_buf_remove(Parser* p, SymbolId sym_id) {
    for (int i = 0; i < vec_size(&p->cfg_live_buf); ++i) {
        if (vec_at(&p->cfg_live_buf, i) == sym_id) {
            vec_splice(&p->cfg_live_buf, i, 1);
            return;
        }
    }
}

/* Remove all live symbols from buffer */
static void cfg_live_buf_clear(Parser* p) {
    vec_clear(&p->cfg_live_buf);
}

/* Computes liveness (live-variable analysis) of blocks in the cfg
   Results saved are Block use/def and in/out
   Requires pseudo-assembly statements in blocks
   Returns 1 if successful, 0 if error */
static int cfg_compute_liveness(Parser* p) {
    /* Avoid segfault trying to traverse nothing for IN[B]/OUT[B] */
    if (vec_size(&p->cfg) == 0) {
        return 1;
    }

    int block_count = vec_size(&p->cfg);
    char status[block_count]; /* Used by cfg_compute_liveness_traverse */

    if (!cfg_compute_use_def(p)) goto error;

    /* IN[B] = use(B) union (OUT[B] - def(B))
       Thus add use(B) to IN[B] for all blocks
       (Cannot do this while computing use/def as use may change
       during computation as new def are discovered, changing
       what is use upon entering block) */
    for (int i = 0; i < vec_size(&p->cfg); ++i) {
        Block* blk = &vec_at(&p->cfg, i);
        for (int j = 0; j < block_use_count(blk); ++j) {
            if (!block_add_in(blk, block_use(blk, j))) goto newerr;
        }
    }

    /* Because the blocks are cyclic, the best method I have is to
       repeatedly traverse the entire cfg until the results are stable */

    /* Give up trying to determine in/out for blocks
       if the results does not stabilize after some number of traversals
       and assume worst case */
    int max_traverse = 10;
    for (int i = 0; i < max_traverse; ++i) {
        /* Reset status to not traversed */
        for (int j = 0; j < block_count; ++j) {
            status[j] = 0;
        }

        Block* base = vec_data(&p->cfg);
        if (!cfg_compute_liveness_traverse(p, status, base, base)) goto error;

        /* Check if results are stable (no modifications) */
        for (int j = 0; j < block_count; ++j) {
            if (status[j] == 2) {
                goto instable;
            }
        }
        goto stable;
instable:
        ;
    }

    /* TODO implement something to handle liveness if give up */
    ASSERT(0, "Liveness behavior if stability not found unimplemented");

stable:
    /* Compute the liveness per statement */
    for (int i = 0; i < vec_size(&p->cfg); ++i) {
        Block* blk = &vec_at(&p->cfg, i);

        /* Start with OUT[B] live */
        cfg_live_buf_clear(p);
        for (int j = 0; j < block_out_count(blk); ++j) {
            if (!cfg_live_buf_add(p, block_out(blk, j))) goto newerr;
        }

        /* Calculate live symbols before/after each statement */
        for (int j = block_pasmstat_count(blk) - 1; j >= 0; --j) {
            PasmStatement* stat = block_pasmstat(blk, j);
            SymbolId syms[MAX_ASM_OP]; /* Buffer to hold symbols */

            if (!pasmstat_set_live_out(
                        stat,
                        vec_data(&p->cfg_live_buf),
                        vec_size(&p->cfg_live_buf))) goto newerr;

            ASSERT(MAX_ASM_OP >= 1, "Need at least 1 to hold def");
            if (pasmstat_def(stat, syms) == 1) {
                ASSERT(symtab_is_var(p, syms[0]),
                        "Assigned symbol should be variable");
                cfg_live_buf_remove(p, syms[0]);
            }

            int use_count = pasmstat_use(stat, syms);
            for (int k = 0; k < use_count; ++k) {
                if (!symtab_is_var(p, syms[k])) {
                    continue;
                }
                if (!cfg_live_buf_add(p, syms[k])) goto newerr;
            }

            if (!pasmstat_set_live_in(
                        stat,
                        vec_data(&p->cfg_live_buf),
                        vec_size(&p->cfg_live_buf))) goto newerr;
        }
    }
    return 1;

newerr:
    parser_set_error(p, ec_outofmemory);
error:
    return 0;
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
    int index = (int)(blk - base);

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
            int path_blk_index = (int)(path_blk - base);

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

/* Computes register preference scores to avoid save + restore of registers
   Returns 1 if succeeded, 0 otherwise */
static int cfg_compute_reg_pref(Parser* p, PasmStatement* stat) {
    /* Save + restore range identification
       by simulating a stack and pushing and popping PasmStatement */

    if (asmins_is_push(pasmstat_ins(stat))) {
        if (!vec_push_back(&p->cfg_pasm_stack, stat)) return 0;
    }
    /* May have pop without push in a block */
    else if (asmins_is_pop(pasmstat_ins(stat)) &&
            !vec_empty(&p->cfg_pasm_stack)) {
        PasmStatement* pushed_stat = vec_back(&p->cfg_pasm_stack);

        /* Check that the statements use the same arguments,
           Only save + restore of physical registers are handled
           for now */
        ASSERT(pasmstat_op_count(pushed_stat) == 1, "Only 1 operand supported");
        ASSERT(pasmstat_op_count(stat) == 1, "Only 1 operand supported");
        if (pasmstat_is_reg(pushed_stat, 0) == pasmstat_is_reg(stat, 0)) {
            if (pasmstat_op_reg(pushed_stat, 0) != pasmstat_op_reg(stat, 0)) {
                return 1;
            }
        }
        else {
            return 1;
        }

        /* Range found */

        cfg_live_buf_clear(p);
        /* Union the symbols which are live in the range from the statement
           after push to pop
           pop is included as we store the liveness BEFORE a statement, thus
           including pop accounts for definitions from the statement before
           pop */
        for (PasmStatement* s = pushed_stat + 1; s != stat + 1; ++s) {
            for (int i = 0; i < pasmstat_live_in_count(s); ++i) {
                cfg_live_buf_add(p, pasmstat_live_in(s, i));
            }
        }

        /* Apply decrement in register preference score to live variables */
        Register reg = pasmstat_op_reg(pushed_stat, 0);
        for (int i = 0; i < vec_size(&p->cfg_live_buf); ++i) {
            SymbolId id = vec_at(&p->cfg_live_buf, i);
            IGNode* node = ig_node(p, id);
            ignode_inc_reg_pref(node, reg_loc(reg), -1);
        }

        (void)vec_pop_back(&p->cfg_pasm_stack);
    }
    return 1;
}

/* Replaces symbol in pseudo-assembly statement with register */
static void cfg_compute_spill_code_replace(
        PasmStatement* stat, SymbolId symid, Register reg) {
    for (int i = 0; i < pasmstat_op_count(stat); ++i) {
        if (!pasmstat_is_sym(stat, i)) {
            continue;
        }
        if (pasmstat_op_sym(stat, i) == symid) {
            pasmstat_set_op_reg(stat, i, reg);
            return;
        }
    }
    ASSERT(0,
            "Failed to replace operand with reloaded register");
}

/* Computes spill code between pseudo-assembly statements in blocks
   Requires statements in blocks
   Requires register assignments
   Returns 1 if successful, 0 if error */
static int cfg_compute_spill_code(Parser* p) {
    /* Construct the PasmStatements which will be used for spill code */
    PasmStatement pasm_push;
    pasmstat_construct(&pasm_push, asmins_push, 1);
    pasmstat_set_op_reg(&pasm_push, 0, reg_rbx);

    PasmStatement pasm_load;
    pasmstat_construct(&pasm_load, asmins_mov, 2);
    /* src,dest operand set when generating for statement */

    PasmStatement pasm_save;
    pasmstat_construct(&pasm_save, asmins_mov, 2);
    /* src,dest operand set when generating for statement */

    PasmStatement pasm_pop;
    pasmstat_construct(&pasm_pop, asmins_pop, 1);
    pasmstat_set_op_reg(&pasm_pop, 0, reg_rbx);

    for (int i = 0; i < vec_size(&p->cfg); ++i) {
        Block* blk = &vec_at(&p->cfg, i);
        for (int j = 0; j < block_pasmstat_count(blk); ++j) {
            PasmStatement* stat = block_pasmstat(blk, j);

            /* Determine the operators which need spill code generated */

            int best_mode = -1;
            /* Number of spill code which have to be generated
               Big number so first found mode is always better than no mode */
            int best_spill_code = 16;
            /* Indices of operands which need spill code */
            int best_spill_index[MAX_ASM_OP];

            AsmIns asmins = pasmstat_ins(stat);
            for (int k = 0; k < asmins_mode_count(asmins); ++k) {
                /* Search for the addressing mode which requires the least
                   operators be spilled */

                AddressMode mode = asmins_mode(asmins, k);
                int spill_code = 0; /* Number of operands spilled */
                int spill_index[MAX_ASM_OP]; /*  Indices of operand spilled */
                for (int l = 0; l < pasmstat_op_count(stat); ++l) {
                    /* If a register was explicitly specified, it is assumed
                       that it forms a valid addressing mode */
                    if (!pasmstat_is_sym(stat, l)) {
                        continue;
                    }

                   /* Check if operand can be used with the addressing mode
                      that is currently being examined */
                    Symbol* sym = symtab_get(p, pasmstat_op_sym(stat, l));
                    switch (symbol_location(sym)) {
                        case loc_stack:
                            if (!addressmode_mem(&mode, l)) {
                                /* Addressing most must be usuable with a
                                   register as spill code reloads memory into a
                                   register */
                                if (!addressmode_reg(&mode, l)) goto no_match;
                                spill_index[spill_code] = l;
                                ++spill_code;
                            }
                            break;
                        case loc_constant:
                            if (!addressmode_imm(&mode, l)) goto no_match;
                            break;
                        case loc_a: case loc_b: case loc_c: case loc_d:
                        case loc_bp: case loc_sp: case loc_si: case loc_di:
                        case loc_8: case loc_9: case loc_10: case loc_11:
                        case loc_12: case loc_13: case loc_14: case loc_15:
                            if (!addressmode_reg(&mode, l)) goto no_match;
                            break;
                        default:
                            /* Labels and others */
                            break;
                    }
                }

                if (spill_code < best_spill_code) {
                    best_spill_code = spill_code;
                    for (int l = 0; l < MAX_ASM_OP; ++l) {
                        best_spill_index[l] = spill_index[l];
                    }
                    best_mode = k;
                }
no_match:
                ;
            }
            ASSERT(best_mode >= 0, "Failed to find suitable addressing mode");


            /* Generate spill code for spilled operands */


            /* During the iteration: j will be adjusted so it is always the
               index of the current statement, so the generated code is
               correctly inserted before and after the statement */

            /* Will be set to the index of the last generated so statement,
               so on reloop it increments to the next statement */
            int j_last_stat = j;
            Location loc_to_use = loc_a;

            SymbolId syms[MAX_ASM_OP];
            int def_count = pasmstat_def(stat, syms);

            for (int k = 0; k < best_spill_code; ++k) {
                int i_operand = best_spill_index[k];
                SymbolId operand_id = pasmstat_op_sym(stat, i_operand);
                Symbol* operand_sym = symtab_get(p, operand_id);

                /* Determine whether the operand is a def'ed or use'd symbol
                   to as they have different patterns for spill code */
                int def = 0; /* 1 for def, 0 for use */
                for (int l = 0; l < def_count; ++l) {
                    if (operand_id == syms[l]) {
                        def = 1;
                        break;
                    }
                }

                /* Replace original statement operand with reloaded register */
                int bytes = symbol_bytes(operand_sym);
                Register temp_reg = reg_get(loc_to_use, bytes);
                cfg_compute_spill_code_replace(stat, operand_id, temp_reg);

                /* Setup the operands for register reload */
                pasmstat_set_op_reg(&pasm_load, 0, temp_reg);
                pasmstat_set_op_sym(&pasm_load, 1, operand_id);
                pasmstat_set_op_sym(&pasm_save, 0, operand_id);
                pasmstat_set_op_reg(&pasm_save, 1, temp_reg);
                /* x64 must push the full 8 byte register */
                pasmstat_set_op_reg(&pasm_push, 0, reg_get(loc_to_use, 8));
                pasmstat_set_op_reg(&pasm_pop, 0, reg_get(loc_to_use, 8));

                if (loc_to_use == loc_15) {
                    ASSERT(0, "Out of registers to use for spill code");
                }
                ++loc_to_use;

                if (def) {
                    /* Is in this order because it is being inserted at index,
                       moving existing contents back */
                    if (!block_insert_pasmstat(blk, pasm_pop, j + 1)) goto newerr;
                    if (!block_insert_pasmstat(blk, pasm_save, j + 1)) goto newerr;
                    if (!block_insert_pasmstat(blk, pasm_load, j)) goto newerr;
                    if (!block_insert_pasmstat(blk, pasm_push, j)) goto newerr;

                    /* push mov (some statement) mov pop
                       ^        ^ Move j here        ^ Last stat here
                       ^ Previously here */
                    j += 2;
                    j_last_stat += 4;
                }
                else {
                    /* Generate for use'ed symbol */
                    if (!block_insert_pasmstat(blk, pasm_pop, j + 1)) goto newerr;
                    if (!block_insert_pasmstat(blk, pasm_load, j)) goto newerr;
                    if (!block_insert_pasmstat(blk, pasm_push, j)) goto newerr;

                    /* push mov (some statement) pop
                       ^        ^ Move j here    ^ Last stat here
                       ^ Previously here */
                    j += 2;
                    j_last_stat += 3;
                }
            }
            j = j_last_stat;
        }
    }
    return 1;
newerr:
    parser_set_error(p, ec_outofmemory);
    return 0;
}

/* Generates assembly for pseudo-assembly operand i */
static void cfg_output_asm_op(Parser* p, const PasmStatement* stat, int i) {
    ISMRFlag flag = pasmstat_flag(stat, i);
    int override = ismr_size_override(flag);
    int deref = ismr_dereference(flag);

    if (pasmstat_is_reg(stat, i)) {
        /* Generate asm to reference register */
        Register reg = pasmstat_op_reg(stat, i);
        const char* str;
        if (override > 0) {
            str = reg_get_str(reg_loc(reg), override);
        }
        else {
            str = reg_str(reg);
        }

        if (deref) {
            parser_output_asm(p, "[%s]", str);
        }
        else {
            parser_output_asm(p, "%s", str);
        }
    }
    else {
        /* Generate asm to reference symbol which could
           be on the stack, register, or constant */
        ASSERT(pasmstat_is_sym(stat, i),
                "Expected PasmStatement operand as symbol");

        SymbolId sym_id = pasmstat_op_sym(stat, i);
        Symbol* sym = symtab_get(p, sym_id);

        int bytes = symbol_bytes(sym);
        if (override > 0) {
            bytes = override;
        }
        if (symbol_on_stack(sym)) {
            /* Generates assembly code to reference symbol
               Example: "dword [rbp+10]" */
            ASSERT(!deref, "Cannot dereference symbol on stack");

            char sign = '+';
            int abs_offset = symtab_get_offset(p, sym_id);
            if (abs_offset < 0) {
                sign = '-';
                abs_offset = -abs_offset;
            }

            const char* size_dir = asm_size_directive(bytes);
            parser_output_asm(p, "%s [rbp%c%d]", size_dir, sign, abs_offset);
        }
        else {
            /* label is for jump destinations */
            if (symbol_is_label(sym) || symbol_is_constant(sym)) {
                ASSERT(!deref, "Cannot dereference constant");
                parser_output_asm(p, "%s", symbol_name(sym));
            }
            else {
                ASSERT(symbol_in_register(sym),
                            "Expected symbol in register");
                const char* str = reg_get_str(symbol_location(sym), bytes);
                if (deref) {
                    /* Size directive is size of pointed to Type,
                       e.g., int* p; *p = 100;
                       mov DWORD [rax], 100 */
                    Type sym_type = symbol_type(sym);
                    int dest_bytes = type_bytes(type_point_to(&sym_type));
                    const char* size_dir = asm_size_directive(dest_bytes);

                    parser_output_asm(p, "%s [%s]", size_dir, str);
                }
                else {
                    parser_output_asm(p, "%s", str);
                }
            }
        }
    }
}

/* Traverses the blocks in the control flow graph and emits assembly
   Requires register allocation decision */
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
    int stack_bytes = symtab_stack_bytes(p);
    if (stack_bytes != 0) {
        parser_output_asm(p, "sub rsp,%d\n",stack_bytes);
    }

    for (int i = 0; i < vec_size(&p->cfg); ++i) {
        parser_output_comment(p, "Block %d\n", i);

        Block* blk = &vec_at(&p->cfg, i);
        /* Labels for block */
        for (int j = 0; j < block_lab_count(blk); ++j) {
            SymbolId lab_id = block_lab(blk, j);
            parser_output_asm(p, "%s:\n", symbol_name(symtab_get(p, lab_id)));
        }

        /* Convert PasmStatement to assembly */
        for (int j = 0; j < block_pasmstat_count(blk); ++j) {
            /* Instruction */
            for (int k = 0; k < ASM_INS_INDENT; ++k) {
                parser_output_asm(p, " ");
            }
            PasmStatement* stat = block_pasmstat(blk, j);
            const char* ins = asmins_str(pasmstat_ins(stat));
            int ins_len = strlength(ins);
            parser_output_asm(p, "%s", ins);

            /* Operands */

            /* Need at least 1 space to separate instruction from operand */
            parser_output_asm(p, " ");
            for (int k = 1; k < ASM_OP_INDENT - ASM_INS_INDENT - ins_len; ++k) {
                parser_output_asm(p, " ");
            }
            for (int k = 0; k < pasmstat_op_count(stat); ++k) {
                if (k != 0) {
                    parser_output_asm(p, ", ");
                }
                cfg_output_asm_op(p, stat, k);
            }
            parser_output_asm(p, "\n");
        }
    }
}

/* Pseudo-assembly peephole optimizer */
static void cfg_pasm_po(Parser* p) {
    for (int i = 0; i < vec_size(&p->cfg); ++i) {
        Block* blk = &vec_at(&p->cfg, i);
        for (int j = 0; j < block_pasmstat_count(blk); ++j) {
            PasmStatement* stat = block_pasmstat(blk, j);

            /* Eliminate mov between the same register */
            if (pasmstat_ins(stat) == asmins_mov) {
                /* Using location because symbol may be on stack */
                Location loc[2];
                int deref[2];
                for (int k = 0; k < 2; ++k) {
                    if (pasmstat_is_reg(stat, k)) {
                        loc[k] = reg_loc(pasmstat_op_reg(stat, k));
                    }
                    else {
                        Symbol* sym = symtab_get(p, pasmstat_op_sym(stat, k));
                        loc[k] = symbol_location(sym);
                    }
                    deref[k] = ismr_dereference(pasmstat_flag(stat, k));
                }

                /* Cannot remove if on stack */
                if (loc[0] < 0 || loc[1] < 0) {
                    continue;
                }
                /* Must either both not dereference or both dereference */
                if (deref[0] != deref[1]) {
                    continue;
                }

                if (loc[0] == loc[1]) {
                    block_remove_pasmstat(blk, j);
                    /* Since current statement removed, move index back,
                       when reloop, it increments to the next statement */
                    --j;
                }
            }
        }
    }
}

/* ============================================================ */
/* Interference graph */

/* Clears nodes in interference graph */
static void ig_clear(Parser* p) {
    for (int i = 0; i < vec_size(&p->ig); ++i) {
        ignode_destruct(&vec_at(&p->ig, i));
    }
    vec_clear(&p->ig);
}

/* Returns interference graph node at SymbolId */
static IGNode* ig_node(Parser* p, SymbolId id) {
    ASSERT(id >= 0, "Invalid SymbolId");
    for (int i = 0; i < vec_size(&p->ig); ++i) {
        IGNode* node = &vec_at(&p->ig, i);
        for (int j = 0; j < ignode_symid_count(node); ++j) {
            if (ignode_symid(node, j) == id) {
                return node;
            }
        }
    }
    ASSERT(0, "Could not find IGNode for SymbolId");
    return NULL;
}

/* Creates unlinked interference graph nodes for non constant symbols
   in the symbol table
   The index of the symbol in the symbol table corresponds to the
   index of the node in the interference graph
   Requires that the symbol table contents be finalized
   (will not be changed in the future)
   Returns 1 if successful, 0 if error */
static int ig_create_nodes(Parser* p) {
    ASSERT(vec_size(&p->ig) == 0, "Interference graph nodes already exist");

    if (!vec_reserve(&p->ig, vec_size(&p->symbol))) goto newerr;
    for (int i = 0; i < vec_size(&p->symbol); ++i) {
        Symbol* sym = symtab_get(p, i);
        if (!symbol_is_var(sym)) {
            continue;
        }

        if (!vec_push_backu(&p->ig)) goto newerr;
        IGNode* node = &vec_back(&p->ig);
        ignode_construct(node);
        if (!ignode_add_symid(node, i)) goto newerr;
    }

    return 1;

newerr:
    parser_set_error(p, ec_outofmemory);
    return 0;
}

/* Constructs edges for the interference graph using the control flow graph
   Requires interference graph nodes to have been created
   Requires liveness information for blocks to be computed
   Returns 1 if successful, 0 if error */
static int ig_compute_edge(Parser* p) {
    for (int i = 0; i < vec_size(&p->cfg); ++i) {
        Block* blk = &vec_at(&p->cfg, i);

        /* An edge connects two nodes if one is live at a point where
           the other is defined */
        for (int j = 0; j < block_pasmstat_count(blk); ++j) {
            PasmStatement* stat = block_pasmstat(blk, j);
            SymbolId def_id;
            int def_count = pasmstat_def(stat, &def_id);
            if (def_count == 0) {
                continue;
            }
            ASSERT(symtab_is_var(p, def_id),
                    "Assigned symbol should be variable");

            /* Make the link */
            IGNode* node = ig_node(p, def_id);
            for (int k = 0; k < pasmstat_live_out_count(stat); ++k) {
                SymbolId other_id = pasmstat_live_out(stat, k);

                /* Do not link to self */
                if (def_id == other_id) {
                    continue;
                }

                IGNode* other_node = ig_node(p, other_id);

                /* Link 2 way, this symbol to live symbol,
                   live symbol to this symbol */
                if (!ignode_link(node, other_node)) goto newerr;
                if (!ignode_link(other_node, node)) goto newerr;
            }
        }
    }

    return 1;

newerr:
    parser_set_error(p, ec_outofmemory);
    return 0;
}

/* Computes register assignments / spills which are required by the
   instruction set / operations to be performed
   (e.g., taking address requires variable to be in memory) */
static void ig_precolor(Parser* p) {
    for (int i = 0; i < vec_size(&p->cfg); ++i) {
        Block* blk = &vec_at(&p->cfg, i);
        for (int j = 0; j < block_pasmstat_count(blk); ++j) {
            PasmStatement* stat = block_pasmstat(blk, j);
            if (pasmstat_ins(stat) == asmins_lea) {
                /* Must be a symbol, instruction cannot have register for
                   operand 2 */
                SymbolId id = pasmstat_op_sym(stat, 1);
                Symbol* sym = symtab_get(p, id);

                /* If the symbol is in a register because it is a function
                   parameter, it has to be copied over to the stack.
                   Assembly generated is:
                   mov %param, <param-register>
                   param is now on the stack, so it moves from its former
                   register to its location on the stack */
                if (symbol_in_register(sym)) {
                    PasmStatement mov;
                    pasmstat_construct(&mov, asmins_mov, 2);
                    pasmstat_set_op_sym(&mov, 0, id);
                    pasmstat_set_op_reg(&mov, 1, symbol_register(sym));

                    block_insert_pasmstat(blk, mov, j);
                    /* Skip the inserted statement
                       j moved to index of inserted statement, on reloop it
                       moves to statement after */
                    j += 1;
                }
                symbol_set_location(sym, loc_stack);
            }
        }
    }
}

/* Computes candidates for coalescing to eliminate copies
   Candidates are copy instructions where the source and destination nodes
   are not connected by an edge (non overlapping lifetime)
   Destination is coalesced into source
   Returns 1 if successful, 0 if error */
static int ig_compute_coalesce(Parser* p, PasmStatement* stat) {
    if (!asmins_is_copy(pasmstat_ins(stat))) {
        return 1;
    }

    /* Only symbols are supported for now */
    int i_src = asmins_copy_src_index();
    int i_dest = asmins_copy_dest_index();
    if (!pasmstat_is_sym(stat, i_src) || !pasmstat_is_sym(stat, i_dest)) {
        return 1;
    }
    /* Do not coalesce move to self
       (Leads to infinite loop trying to make node links) */
    SymbolId src_id = pasmstat_op_sym(stat, i_src);
    SymbolId dest_id = pasmstat_op_sym(stat, i_dest);
    if (src_id == dest_id) {
        return 1;
    }
    /* Cannot coalesce non variables, (they have no location) */
    if (!symbol_is_var(symtab_get(p, src_id))) {
        return 1;
    }
    /* source and destination nodes do not overlap in lifetime
       (no IGNode edge) */
    IGNode* src_node = ig_node(p, src_id);
    IGNode* dest_node = ig_node(p, dest_id);
    if (ignode_has_link(src_node, dest_node)) {
        return 1;
    }
    /* Determine the location to put coalesced node's symbols */
    ASSERT(ignode_symid_count(src_node) != 0, "Source node has no symbols");
    ASSERT(ignode_symid_count(dest_node) != 0, "Source node has no symbols");
    Location src_loc =
        symbol_location(symtab_get(p, ignode_symid(src_node, 0)));
    Location dest_loc =
        symbol_location(symtab_get(p, ignode_symid(dest_node, 0)));
    Location loc; /* Assigned to all the symbols of the source node */
    /* Cannot coalesce those on the stack, as the assembly generator always
       gives symbols on the stack distinct locations */
    if (src_loc == loc_stack || dest_loc == loc_stack) {
        return 1;
    }
    if (src_loc != loc_none && dest_loc != loc_none) {
        /* Cannot coalesce if both are precolored different locations */
        if (src_loc != dest_loc) {
            return 1;
        }
        /* Both are the same location */
        loc = src_loc;
    }
    else if (src_loc != loc_none) {
        loc = src_loc;
    }
    else {
        /* dest_loc could be a location or no location */
        loc = dest_loc;
    }

    /* Coalesce destination node into source node */

    /* Union source node's neighbors with destination node's neighbors */
    for (int i = 0; i < ignode_neighbor_count(dest_node); ++i) {
        IGNode* neighbor = ignode_neighbor(dest_node, i);
        if (!ignode_link(src_node, neighbor)) return 0;
        if (!ignode_link(neighbor, src_node)) return 0;
        ignode_unlink(neighbor, dest_node);
    }
    /* Source node now represents all the symbol(s) of the destination
       node */
    for (int i = 0; i < ignode_symid_count(dest_node); ++i) {
        SymbolId dest_symid = ignode_symid(dest_node, i);
        if (!ignode_add_symid(src_node, dest_symid)) return 0;
    }
    ignode_symid_clear(dest_node);

    /* Set the locations, any of the possibilities:
       - Destination node location set on source symbols
       - Source node location set on newly acquired symbols
       - No location is set (loc_none) */
    for (int i = 0; i < ignode_symid_count(src_node); ++i) {
        SymbolId src_symid = ignode_symid(src_node, i);
        symbol_set_location(symtab_get(p, src_symid), loc);
    }

    if (g_debug_print_info) {
        LOGF("Coalesce %s -> %s\n",
                symbol_name(symtab_get(p, dest_id)),
                symbol_name(symtab_get(p, src_id)));
    }
    return 1;
}

/* Computes the spill cost for all nodes in the interference graph
   Requires statements in blocks
   Requires interference graph nodes to have been created
   Requires loop nesting depth information for blocks to be computed */
static void ig_compute_spill_cost(Parser* p) {
    for (int i = 0; i < vec_size(&p->cfg); ++i) {
        Block* blk = &vec_at(&p->cfg, i);

        for (int j = 0; j < block_pasmstat_count(blk); ++j) {
            PasmStatement* stat = block_pasmstat(blk, j);
            SymbolId syms[MAX_ARGS]; /* Buffer to hold symbols */

            int use_count = pasmstat_use(stat, syms);
            for (int k = 0; k < use_count; ++k) {
                if (!symtab_is_var(p, syms[k])) {
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
static int ig_compute_color_sort(const void* a, const void* b) {
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
    /* Index corresponds to Location, number of times register is used

       Disallow registers not in the palette by setting the use count as 1
       This array is copied to reset the number of times registers were used
       when computing the register to assign to a node */
    int used_loc_init[X86_REGISTER_COUNT];
    for (int j = 0; j < X86_REGISTER_COUNT; ++j) {
        used_loc_init[j] = 1;
    }
    for (int j = 0; j < p->ig_palette_size; ++j) {
        used_loc_init[p->ig_palette[j]] = 0;
    }

    /* Copy the nodes and sort them, cannot sort in place as it would
       invalidate the links between nodes */
    IGNode* nodes[vec_size(&p->ig)];
    for (int i = 0; i < vec_size(&p->ig); ++i) {
        nodes[i] = &vec_at(&p->ig, i);
    }
    quicksort(nodes, (size_t)vec_size(&p->ig), sizeof(IGNode*),
            ig_compute_color_sort);

    /* Lowest spill cost is first in the array, thus iterate backwards
       for highest spill cost first */
    for (int i = vec_size(&p->ig) - 1; i >= 0; --i) {
        IGNode* node = nodes[i];

        /* Node was coalesced */
        if (ignode_symid_count(node) == 0) {
            continue;
        }
        /* Skip if preassigned location (precolored)
           All the symbols have the same location,
           only need to look at the first */
        {
            Symbol* node_sym = symtab_get(p, ignode_symid(node, 0));
            if (symbol_location(node_sym) != loc_none) {
                continue;
            }
        }

        /* Attempt to assign a register to the node */

        /* Initialize number of times register used */
        int used_loc[X86_REGISTER_COUNT];
        for (int j = 0; j < X86_REGISTER_COUNT; ++j) {
            used_loc[j] = used_loc_init[j];
        }

        /* Count registers used by neighbors */
        for (int j = 0; j < ignode_neighbor_count(node); ++j) {
            IGNode* neighbor = ignode_neighbor(node, j);
            /* All the symbols have the same location,
               only need to look at the first */
            ASSERT(ignode_symid_count(neighbor) != 0,
                    "Neighbor node has no symbols");
            SymbolId neighbor_id = ignode_symid(neighbor, 0);
            Symbol* neighbor_sym = symtab_get(p, neighbor_id);
            Location loc = symbol_location(neighbor_sym);
            if (loc != loc_none && loc != loc_stack) {
                ++used_loc[loc];
            }
        }

        /* Find register with highest preference score to use */
        int found_reg = 0;
        int highest_pref_score;
        Location location;
        for (int j = 0; j < X86_REGISTER_COUNT; ++j) {
            if (used_loc[j] == 0) {
                int pref_score = ignode_reg_pref(node, j);
                if (!found_reg) {
                    highest_pref_score = pref_score;
                    location = j;
                    found_reg = 1;
                }
                else if (pref_score > highest_pref_score) {
                    highest_pref_score = pref_score;
                    location = j;
                }
            }
        }
        /* Always spill self as this has the lowest spill cost, neighbors were
           assigned earlier, thus they have a higher spill cost */
        if (!found_reg) {
            location = loc_stack;
        }

        /* Store the location assignment for the symbols */
        for (int j = 0; j < ignode_symid_count(node); ++j) {
            Symbol* node_sym = symtab_get(p, ignode_symid(node, j));
            symbol_set_location(node_sym, location);
        }
    }

    /* Spilled variables are guaranteed to remain spilled, as nodes
       which were assigned registers never give them away. Any unassigned
       neighbors when the variable was spilled will have been assigned or
       spilled, offering no openings for the spilled variable to be assigned
       a register. */
}

/* Computes the register to assign to symbols
   Requires pseudo-asembly statements in blocks
   Returns 1 if successful, 0 if error */
static int compute_register(Parser* p) {
    if (!cfg_compute_liveness(p)) goto error;
    cfg_compute_loop_depth(p);
    if (!ig_create_nodes(p)) goto error;
    if (!ig_compute_edge(p)) goto error;

    /* Precolor has to run by itself through all the statements
       first, otherwise symbols which should be precolored may
       be coalesced before it gets precolored
       Example:
       mov p1, t0 <- Coalesce sees this, p1 not precolored yet
                     thus p1 gets incorrectly coalesced
       lea p2, p1 <- p1 precolored here */
    ig_precolor(p);
    for (int i = 0; i < vec_size(&p->cfg); ++i) {
        Block* blk = &vec_at(&p->cfg, i);

        /* Trace save + restore range per block only to avoid complexities
           of crossing blocks */
        vec_clear(&p->cfg_pasm_stack);

        for (int j = 0; j < block_pasmstat_count(blk); ++j) {
            PasmStatement* stat = block_pasmstat(blk, j);
            if (!cfg_compute_reg_pref(p, stat)) goto error;
            if (!ig_compute_coalesce(p, stat)) goto error;
        }
    }
    ig_compute_spill_cost(p);
    ig_compute_color(p);
    return 1;

error:
    return 0;
}

/* Sets up parser data structures to begin parsing a new function */
static void parser_clear_func(Parser* p) {
    cfg_clear(p);
    ig_clear(p);
    symtab_clear(p);
}

/* Dumps contents stored in parser */

static void debug_print_symtab(Parser* p) {
    LOGF("Symbol table: [%d]\n", vec_size(&p->symbol));
    for (int i = 0; i < vec_size(&p->symbol); ++i) {
        Symbol* sym = &vec_at(&p->symbol, i);
        Type type = symbol_type(sym);
        LOGF("  %s", type_specifiers_str(type.typespec));
        for (int j = 0; j < type.pointers; ++j) {
            LOG("*");
        }
        LOGF(" %s (%s)\n", symbol_name(sym), loc_str(symbol_location(sym)));
    }
}

static void debug_print_cfg(Parser* p) {
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

        /* Print IL instruction and arguments */
        LOG("    IL:\n");
        for (int j = 0; j < block_ilstat_count(blk); ++j) {
            ILStatement* stat = block_ilstat(blk, j);
            LOGF("    %d %s", j, ins_str(ilstat_ins(stat)));
            for (int k = 0; k < ilstat_argc(stat); ++k) {
                Symbol* arg_sym = symtab_get(p, ilstat_arg(stat, k));
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

        /* Print pseudo assembly statement */
        LOG("    Pseudo-assembly:\n");
        for (int j = 0; j < block_pasmstat_count(blk); ++j) {
            PasmStatement* stat = block_pasmstat(blk, j);

            /* Pseudo-assembly */
            LOGF("    %d %s", j, asmins_str(pasmstat_ins(stat)));
            for (int k = 0; k < pasmstat_op_count(stat); ++k) {
                if (k == 0) {
                    LOG(" ");
                }
                else {
                    LOG(",");
                }
                if (pasmstat_is_reg(stat, k)) {
                    Register reg = pasmstat_op_reg(stat, k);
                    LOGF("%s", reg_str(reg));
                }
                else if (pasmstat_is_sym(stat, k)) {
                    SymbolId id = pasmstat_op_sym(stat, k);
                    Symbol* sym = symtab_get(p, id);
                    LOGF("%%%s", symbol_name(sym));
                }
            }
            LOG("\n");

            /* Live information */
            /* Align with the "Live" */
            int spaces = ichar(j);
            for (int k = 0; k < spaces; ++k) {
                LOG(" ");
            }

            LOG("     Live in:");
            for (int k = 0; k < pasmstat_live_in_count(stat); ++k) {
                SymbolId id = pasmstat_live_in(stat, k);
                Symbol* sym = symtab_get(p, id);
                LOGF(" %s", symbol_name(sym));
            }
            LOG("\n");

            for (int k = 0; k < spaces; ++k) {
                LOG(" ");
            }
            LOG("     Live out:");
            for (int k = 0; k < pasmstat_live_out_count(stat); ++k) {
                SymbolId id = pasmstat_live_out(stat, k);
                Symbol* sym = symtab_get(p, id);
                LOGF(" %s", symbol_name(sym));
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

static void debug_print_ig(Parser* p) {
    LOGF("Interference graph [%d]\n", vec_size(&p->ig));
    for (int i = 0; i < vec_size(&p->ig); ++i) {
        IGNode* node = &vec_at(&p->ig, i);
        /* Omit nodes which were coalesced */
        if (ignode_symid_count(node) == 0) {
            continue;
        }

        LOGF("  Node %d", i);
        for (int j = 0; j < ignode_symid_count(node); ++j) {
            LOGF(" %s", symbol_name(symtab_get(p, ignode_symid(node, j))));
        }
        LOG("\n");

        /* Spill cost */
        LOGF("    Spill cost %ld\n", ignode_cost(node));

        /* Register preference score */
        LOG("    Register preference");
        for (int j = 0; j < ignode_reg_pref_count(node); ++j) {
            LOGF(" %d", ignode_reg_pref(node, j));
        }
        LOG("\n");

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
/* INSTRUCTION_PROC checks the arguments for validity */

/* Helpers */
static void cg_extract_param(const char* str, char* type, char* name);

/* argv contains argc pointers, each pointer is to null terminated argument string */
#define INSTRUCTION_PROC(name__) \
    void il_proc_ ## name__ (Parser* p, char** pparg, int arg_count)

static INSTRUCTION_PROC(add) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
}

static INSTRUCTION_PROC(ce) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
}

static INSTRUCTION_PROC(cl) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
}

static INSTRUCTION_PROC(cle) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
}

static INSTRUCTION_PROC(cne) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
}
static INSTRUCTION_PROC(def) {
    if (arg_count != 1) {
        parser_set_error(p, ec_badargs);
        return;
    }

    char type[MAX_ARG_LEN];
    char name[MAX_ARG_LEN];
    cg_extract_param(pparg[0], type, name);
    symtab_get(p, symtab_add(p, type_from_str(type), name));
}

static INSTRUCTION_PROC(div) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
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
        symbol_set_location(symtab_get(p, argc_id), loc_di);
        symbol_set_location(symtab_get(p, pparg_id), loc_si);

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
    if (block_ilstat_count(blk) != 0) {
        blk = cfg_link_new_block(p);
    }
    block_add_label(blk, symtab_find(p, pparg[0]));
}

static INSTRUCTION_PROC(mad) {
    if (arg_count != 2) {
        parser_set_error(p, ec_badargs);
        return;
    }
}

static INSTRUCTION_PROC(mfi) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
}

static INSTRUCTION_PROC(mod) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
}

static INSTRUCTION_PROC(mov) {
    if (arg_count != 2) {
        parser_set_error(p, ec_badargs);
        return;
    }
}

static INSTRUCTION_PROC(mtc) {
    if (arg_count != 2) {
        parser_set_error(p, ec_badargs);
        return;
    }
}

static INSTRUCTION_PROC(mti) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
}

static INSTRUCTION_PROC(mul) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
}

static INSTRUCTION_PROC(not) {
    if (arg_count != 2) {
        parser_set_error(p, ec_badargs);
        return;
    }
}

static INSTRUCTION_PROC(ret) {
    if (arg_count != 1) {
        parser_set_error(p, ec_badargs);
        return;
    }
    cfg_new_block(p);
}

static INSTRUCTION_PROC(sub) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
}

/* ============================================================ */
/* Instruction selector */

#include "x86_inssel_macro.h"

/* Finds and returns the lowest cost macro case for the provided
   IL statement
   Returns null if not found */
static InsSelMacroCase* inssel_find(Parser* p, const ILStatement* stat) {
    for (int i = 0; i < vec_size(&p->inssel_macro); ++i) {
        InsSelMacro* ism = &vec_at(&p->inssel_macro, i);

        /* Macro must be for IL instruction */
        if (ism_il_ins(ism) != ilstat_ins(stat)) {
            continue;
        }

        /* Pick the first case which matches (picks lowest cost) */
        for (int j = 0; j < ism_case_count(ism); ++j) {
            InsSelMacroCase* ismc = ism_case(ism, j);

            /* Attempt to match a constraint in the constraint string,
               the reader is reminded that it matches A constraint of
               all the constraints separated by spaces */
            const char* constraint = ismc_constraint(ismc);
            int i_constraint = 0; /* Current location in constraint str */
            while (1) {
                for (int k = 0; k < ilstat_argc(stat); ++k) {
                    ASSERT(constraint[i_constraint] != '\0',
                            "Constraint string ended too early");
                    const char c = constraint[i_constraint];
                    ++i_constraint;

                    /* Constrain signedness */
                    char cn = constraint[i_constraint];
                    int must_sign = 0;
                    int must_unsign = 0;
                    if (cn == 'u') {
                        ++i_constraint;
                        must_unsign = 1;
                    }
                    else if (cn == 'U') {
                        ++i_constraint;
                        must_sign = 1;
                    }

                    /* Constrain byte size */
                    SymbolId arg_id = ilstat_arg(stat, k);
                    cn = constraint[i_constraint];
                    if ('0' <= cn && cn <= '9') {
                        ++i_constraint;
                        int bytes = cn - '0';
                        const Symbol* sym = symtab_get(p, arg_id);
                        if (symbol_bytes(sym) != bytes) goto no_match;
                    }

                    /* Constrain symbol type */
                    if (c == 'i') {
                        Symbol* sym = symtab_get(p, arg_id);
                        Type type = symbol_type(sym);
                        TypeSpecifiers ts = type_typespec(&type);
                        if (!symbol_is_constant(sym)) goto no_match;
                        if (must_sign && !type_signed(ts)) goto no_match;
                        if (must_unsign && !type_unsigned(ts)) goto no_match;
                    }
                    else if (c == 'l') {
                        if (!symtab_is_label(p, arg_id)) goto no_match;
                    }
                    else if (c == 's') {
                        Symbol* sym = symtab_get(p, arg_id);
                        Type type = symbol_type(sym);
                        TypeSpecifiers ts = type_typespec(&type);
                        if (!symbol_is_var(sym)) goto no_match;
                        if (must_sign && !type_signed(ts)) goto no_match;
                        if (must_unsign && !type_unsigned(ts)) goto no_match;
                    }
                    else {
                        ASSERTF(0, "Invalid constraint character %c", c);
                    }
                }
                /* Match made */
                return ismc;
no_match:
                ;
                /* Attempt to match the next constraint in the string if it exists
                   otherwise this case is disqualified, e.g.,
                   s1s2 s3s4
                    ^   ^ Move to here
                    Was here */
                char c = constraint[i_constraint];
                while (1) {
                    ++i_constraint;
                    if (c == '\0') {
                        goto disqualify_case;
                    }
                    if (c == ' ') {
                        break;
                    }
                    c = constraint[i_constraint];
                }
            }
disqualify_case:
            ;
        }

        /* Only one macro is for an IL instructions, after the macro
           is found, no need to keep searching */
        break;
    }
    return NULL;
}

/* Computes pseudo-assembly for statements in blocks
   Requires statements in blocks
   Returns 1 if successful, 0 if error */
static int cfg_compute_pasm(Parser* p) {
    for (int i = 0; i < vec_size(&p->cfg); ++i) {
        Block* blk = &vec_at(&p->cfg, i);
        for (int j = 0; j < block_ilstat_count(blk); ++j) {
            ILStatement* ilstat = block_ilstat(blk, j);
            InsSelMacroCase* ismc = inssel_find(p, ilstat);
            ASSERTF(ismc != NULL,
                    "Could not find macro for IL statement %s",
                    ins_str(ilstat_ins(ilstat)));

            /* Holds newly created symbols for this macro

               This is setup such that in the future multiple
               registers can be created. The index can be stored
               as part of the operand, i.e., half of the operand
               is the SymbolId, other half is the index */
            SymbolId created_id[1];
            char created[1]; /* 0 if not created, 1 if created */
            for (int k = 0; k < 1; ++k) {
                created[k] = 0;
            }

            /* Add pseudo-assembly to block */
            for (int k = 0; k < ismc_replace_count(ismc); ++k) {
                InsSelMacroReplace* ismr = ismc_replace(ismc, k);

                PasmStatement pasmstat;
                pasmstat_construct(&pasmstat,
                        ismr_ins(ismr), ismr_op_count(ismr));
                for (int l = 0; l < ismr_op_count(ismr); ++l) {
                    /* Transfer the flags over */
                    pasmstat_set_flag(&pasmstat, l, ismr_flag(ismr, l));

                    if (ismr_op_new(ismr, l)) {
                        /* Use the newly created symbol if it exists
                           otherwise create it */
                        SymbolId id;
                        if (created[0] == 0) {
                            /* Make a new Symbol with the same type */
                            int index = ismr_op_index(ismr, l);
                            SymbolId arg_id = ilstat_arg(ilstat, index);
                            Symbol* arg_sym = symtab_get(p, arg_id);

                            ASSERT(symbol_bytes(arg_sym) > 0,
                                    "Attempted to create temporary of 0 bytes");
                            id = symtab_add_temporary(p, symbol_type(arg_sym));
                            created_id[0] = id;
                            created[0] = 1;
                        }
                        else {
                            id = created_id[0];
                        }
                        pasmstat_set_op_sym(&pasmstat, l, id);
                    }
                    else if (ismr_op_virtual(ismr, l)) {
                        int index = ismr_op_index(ismr, l);
                        SymbolId id = ilstat_arg(ilstat, index);
                        pasmstat_set_op_sym(&pasmstat, l, id);
                    }
                    else if (ismr_op_location(ismr, l)) {
                        /* Find the first symbol of the IL and use its size
                           to convert location to a register */
                        int bytes = 0;
                        for (int m = 0; m < ilstat_argc(ilstat); ++m) {
                            SymbolId id = ilstat_arg(ilstat, m);
                            Symbol* sym = symtab_get(p, id);
                            bytes = symbol_bytes(sym);
                            if (bytes != 0) {
                                break;
                            }
                        }
                        ASSERT(bytes != 0,
                                "Failed to calculate size for register");
                        pasmstat_set_op_reg(
                                &pasmstat,
                                l,
                                reg_get(ismr_op_loc(ismr, l), bytes));
                    }
                    else {
                        ASSERT(ismr_op_physical(ismr, l),
                                "Incorrect InsSelMacroReplace operand type");
                        pasmstat_set_op_reg(&pasmstat, l, ismr_op_reg(ismr, l));
                    }
                }

                if (!block_add_pasmstat(blk, pasmstat)) goto newerr;
            }
        }
    }
    return 1;
newerr:
    parser_set_error(p, ec_outofmemory);
    return 0;
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

/* ============================================================ */
/* Initialization and configuration */

/* Reads one instruction and its arguments,
   the result is stored in the parser
   Returns 1 if successfully read, 0 if EOF or error */
static int read_instruction(Parser* p) {
    /* Read instruction */
    p->ins_len = 0;
    char c;
    while(1) {
        c = (char)getc(p->rf);
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
        c = (char)getc(p->rf);
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

/* Parses the IL and generates assembly
   Returns 1 if successful, 0 if error */
static int parse(Parser* p) {
    while (read_instruction(p)) {
        InsProcHandler proc_handler = il_get_proc(p->ins);
        /* Verify is valid instruction */
        if (proc_handler == NULL) {
            ERRMSGF("Unrecognized instruction %s\n", p->ins);
            parser_set_error(p, ec_invalidins);
            goto error;
        }

        if (ins_incfg(ins_from_str(p->ins))) {
            ILStatement stat =
                {.ins = ins_from_str(p->ins), .argc = p->arg_count};
            for (int i = 0; i < p->arg_count; ++i) {
                SymbolId sym_id = symtab_find(p, p->arg_table[i]);
                ASSERTF(sym_id >= 0, "Invalid SymbolId %s", p->arg_table[i]);
                stat.arg[i] = sym_id;
            }

            if (!cfg_append_latest(p, stat)) goto error;
        }
        /* Instruction gets added first
           then new block is made (e.g., jmp at last block, then new block) */
        if (proc_handler != NULL) {
            proc_handler(p, p->arg_table, p->arg_count);
        }
    }

    /* Process the last (most recent) function */
    if (!cfg_link_jump_dest(p)) goto error;

    /* Instruction selection */
    if (!cfg_compute_pasm(p)) goto error;

    /* Register allocation */
    if (!compute_register(p)) goto error;
    cfg_pasm_po(p);

    /* Code generation */
    if (!cfg_compute_spill_code(p)) goto error;
    cfg_output_asm(p);
    return 1;
error:
    return 0;
}

/* The index of the option's string in the string array
   is the index of the pointer for the variable corresponding to the option

   SWITCH_OPTION(option string, variable to set)
   Order by option string, see strbinfind for ordering requirements */
#define SWITCH_OPTIONS                                  \
    SWITCH_OPTION(-dprint-cfg, g_debug_print_cfg)       \
    SWITCH_OPTION(-dprint-ig, g_debug_print_ig)         \
    SWITCH_OPTION(-dprint-info, g_debug_print_info)     \
    SWITCH_OPTION(-dprint-symtab, g_debug_print_symtab)

#define SWITCH_OPTION(str__, var__) #str__,
const char* option_switch_str[] = {SWITCH_OPTIONS};
#undef SWITCH_OPTION
#define SWITCH_OPTION(str__, var__) &var__,
int* option_switch_value[] = {SWITCH_OPTIONS};
#undef SWITCH_OPTION

/* Parses cli args and processes them */
/* NOTE: will not clean up file handles at exit */
/* Returns non zero if error */
static int handle_cli_arg(Parser* p, int argc, char** argv) {
    int rt_code = 0;
    /* Skip first argv since it is path */
    for (int i = 1; i < argc; ++i) {
        /* Handle switch options */
        int i_switch = strbinfind(
                argv[i],
                strlength(argv[i]),
                option_switch_str,
                ARRAY_SIZE(option_switch_str));
        if (i_switch >= 0) {
            *option_switch_value[i_switch] = 1;
            continue;
        }

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

    Parser p;
    if (!parser_construct(&p)) goto exit;

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

    if (!parse(&p)) goto exit;

exit:
    /* Indicate to the user cause for exiting if errored during parsing */
    if (parser_has_error(&p)) {
        ErrorCode ecode = parser_get_error(&p);
        ERRMSGF("Error during parsing: %d %s\n", ecode, errcode_str[ecode]);
        exitcode = ecode;
    }

    if (g_debug_print_symtab) {
        debug_print_symtab(&p);
    }
    if (g_debug_print_cfg) {
        debug_print_cfg(&p);
    }
    if (g_debug_print_ig) {
        debug_print_ig(&p);
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

