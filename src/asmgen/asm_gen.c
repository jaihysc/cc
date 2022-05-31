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
#include "X86.h"
#include "ErrorCode.h"
#include "Symbol.h"
#include "Statement.h"
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

typedef struct {
    AsmIns ins;
    /* See INSSEL_MACRO_REPLACE for usage of type */
    int op1_type;
    int op2_type;
    union {
        Location loc;
        /* Index of the argument in IL instruction */
        int index;
    } op1;
    union {
        Location loc;
        int index;
    } op2;
} InsSelMacroReplace;

/* Returns AsmIns */
static AsmIns ismr_ins(const InsSelMacroReplace* ismr) {
    ASSERT(ismr != NULL, "Macro replace is null");
    return ismr->ins;
}

/* Returns 1 if operand 1 is a new virtual register, 0 if not */
static int ismr_op1_new(const InsSelMacroReplace* ismr) {
    ASSERT(ismr != NULL, "Macro replace is null");
    return ismr->op1_type == 0;
}

/* Returns 1 if operand 1 is a virtual register, 0 if not */
static int ismr_op1_virtual(const InsSelMacroReplace* ismr) {
    ASSERT(ismr != NULL, "Macro replace is null");
    return ismr->op1_type == 1;
}

/* Returns 1 if operand 1 is a physical register, 0 if not */
static int ismr_op1_physical(const InsSelMacroReplace* ismr) {
    ASSERT(ismr != NULL, "Macro replace is null");
    return ismr->op1_type == 2;
}

/* Returns 1 if operand 2 is a new virtual register, 0 if not */
static int ismr_op2_new(const InsSelMacroReplace* ismr) {
    ASSERT(ismr != NULL, "Macro replace is null");
    return ismr->op2_type == 0;
}

/* Returns 1 if operand 2 is a virtual register, 0 if not */
static int ismr_op2_virtual(const InsSelMacroReplace* ismr) {
    ASSERT(ismr != NULL, "Macro replace is null");
    return ismr->op2_type == 1;
}

/* Returns 1 if operand 2 is a physical register, 0 if not */
static int ismr_op2_physical(const InsSelMacroReplace* ismr) {
    ASSERT(ismr != NULL, "Macro replace is null");
    return ismr->op2_type == 2;
}

/* Interprets op1 as a virtual register (Symbol) and returns the index of the
   argument (SymbolId) in the IL instruction (to perform macro replacement) */
static int ismr_op1_index(const InsSelMacroReplace* ismr) {
    ASSERT(ismr != NULL, "Macro replace is null");
    ASSERT(ismr_op1_virtual(ismr), "Incorrect op1 type");
    return ismr->op1.index;
}

/* Interprets op1 as a physical register (Location) and returns it */
static Location ismr_op1_loc(const InsSelMacroReplace* ismr) {
    ASSERT(ismr != NULL, "Macro replace is null");
    ASSERT(ismr_op1_physical(ismr), "Incorrect op1 type");
    return ismr->op1.loc;
}

/* Interprets op2 as a virtual register (Symbol) and returns the index of the
   argument (SymbolId) in the IL instruction (to perform macro replacement) */
static int ismr_op2_index(const InsSelMacroReplace* ismr) {
    ASSERT(ismr != NULL, "Macro replace is null");
    ASSERT(ismr_op2_virtual(ismr), "Incorrect op2 type");
    return ismr->op2.index;
}

/* Interprets op2 as a physical register (Location) and returns it */
static Location ismr_op2_loc(const InsSelMacroReplace* ismr) {
    ASSERT(ismr != NULL, "Macro replace is null");
    ASSERT(ismr_op2_physical(ismr), "Incorrect op2 type");
    return ismr->op2.loc;
}

typedef struct {
    /* See INSSEL_MACRO_CASE for usage of constraint string */
    const char* constraint;
    int reg_pref[X86_REGISTER_COUNT];
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
    char func_name[MAX_ARG_LEN]; /* Name of current function */

    /* Instruction selection */
    vec_InsSelMacro inssel_macro;

    /* Control flow graph */
    vec_t(Block) cfg;
    Block* latest_blk;

    /* Interference graph
       Index of symbol in symbol table corresponds to index of its node
       in the interference graph */
    vec_t(IGNode) ig;
    /* Buffer of live symbols used when calculating interference graph */
    vec_t(SymbolId) ig_live;
    /* The registers which can be assigned (colored) to nodes */
    Location ig_palette[X86_REGISTER_COUNT];
    int ig_palette_size;

    /* Instruction, argument */
    char ins[MAX_INSTRUCTION_LEN];
    int ins_len;
    char arg[MAX_ARG_LEN];
    char* arg_table[MAX_ARGS]; /* Points to beginning of each argument */
    int arg_count; /* Number of arguments */
};

/* Returns 1 if succeeded, 0 if error */
static int parser_construct(Parser* p) {
    vec_construct(&p->symbol);
    if (!inssel_macro_construct(&p->inssel_macro)) goto error;
    vec_construct(&p->cfg);
    vec_construct(&p->ig);
    vec_construct(&p->ig_live);

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

    return 1;

error:
    parser_set_error(p, ec_outofmemory);
    return 0;
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
    int index = (int)(blk - base);
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

/* Calculates the SymbolId for the given IGNode */
static SymbolId ig_ignode_symbolid(Parser* p, IGNode* node) {
    return (SymbolId)(node - &vec_at(&p->ig, 0));
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
        SymbolId node_id = ig_ignode_symbolid(p, node);
        Symbol* node_sym = symtab_get(p, node_id);

        /* Preassigned location */
        if (symbol_location(node_sym) != loc_none ||
            symbol_is_constant(node_sym)) {
            continue;
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
            SymbolId neighbor_id = ig_ignode_symbolid(p, neighbor);
            Symbol* neighbor_sym = symtab_get(p, neighbor_id);

            Location loc = symbol_location(neighbor_sym);
            if (loc != loc_none && loc != loc_stack) {
                ++used_loc[loc];
            }
        }

        /* Find first available register to use */
        int found_reg = 0;
        for (int j = 0; j < X86_REGISTER_COUNT; ++j) {
            if (used_loc[j] == 0) {
                symbol_set_location(node_sym, j);
                found_reg = 1;
                break;
            }
        }
        if (found_reg) {
            continue;
        }

        /* Always spill self as this has the lowest spill cost, neighbors were
           assigned earlier, thus they have a higher spill cost */

        symbol_set_location(node_sym, loc_stack);
    }

    /* Spilled variables are guaranteed to remain spilled, as nodes
       which were assigned registers never give them away. Any unassigned
       neighbors when the variable was spilled will have been assigned or
       spilled, offering no openings for the spilled variable to be assigned
       a register. */
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

static void debug_print_ig(Parser* p) {
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
static void cg_cmpsetcc(
        Parser* p,
        SymbolId dest_id, SymbolId op1_id, SymbolId op2_id, const char* set);
static void cg_testjmpcc(
        Parser* p,
        SymbolId label_id, SymbolId op1_id, const char* jmp);
/* Suffix meaning:
    s: Symbol
    r: Register
    e.g., movss is mov symbol to register
    insss is Instruction with 2 symbols */
static void cg_inss(Parser* p, const char* ins, SymbolId op1_id);
static void cg_insr(Parser* p, const char* ins, Register op1_reg);
static void cg_insss(
        Parser* p, const char* ins, SymbolId op1_id, SymbolId op2_id);
static void cg_inssr(
        Parser* p, const char* ins, SymbolId op1_id, Register op2_reg);
static void cg_insrs(
        Parser* p, const char* ins, Register op1_reg, SymbolId op2_id);
static void cg_movss(Parser* p, SymbolId source, SymbolId dest);
static void cg_movsr(Parser* p, SymbolId source, Location dest);
static void cg_movrs(Parser* p, Location source, SymbolId dest);
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
    symtab_get(p, symtab_add(p, type_from_str(type), name));
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

/* ============================================================ */
/* Instruction selector */

#include "x86_inssel_macro.h"

/* Finds and returns the lowest cost macro case for the provided
   IL statement
   Returns null if not found */
static InsSelMacroCase* inssel_find(Parser* p, const Statement* stat) {
    for (int i = 0; i < vec_size(&p->inssel_macro); ++i) {
        InsSelMacro* ism = &vec_at(&p->inssel_macro, i);

        /* Macro must be for IL instruction */
        if (ism_il_ins(ism) != stat_ins(stat)) {
            continue;
        }

        /* Pick the first case which matches (picks lowest cost) */
        for (int j = 0; j < ism_case_count(ism); ++j) {
            InsSelMacroCase* ismc = ism_case(ism, j);

            /* Parse the constraint string */
            const char* constraint = ismc_constraint(ismc);
            int i_constraint = 0; /* Current location in constraint str */
            for (int k = 0; k < stat_argc(stat); ++k) {
                ASSERT(constraint[i_constraint] != '\0',
                        "Constraint string ended too early");
                int isconstant = constraint[i_constraint] == 'i';
                ++i_constraint;

                SymbolId arg_id = stat_arg(stat, k);
                if (isconstant != symtab_isconstant(p, arg_id)) {
                    goto no_match;
                }
            }
            /* Match made */
            return ismc;
no_match:
            ;
        }

        /* Only one macro is for an IL instructions, after the macro
           is found, no need to keep searching */
        break;
    }
    return NULL;
}

/* Computes pseudo-assembly for statements in blocks
   Requires statements in blocks */
static void cfg_compute_pasm(Parser* p) {
    for (int i = 0; i < vec_size(&p->cfg); ++i) {
        Block* blk = &vec_at(&p->cfg, i);
        for (int j = 0; j < block_stat_count(blk); ++j) {
            Statement* stat = block_stat(blk, j);
            InsSelMacroCase* ismc = inssel_find(p, stat);
            ASSERTF(ismc != NULL,
                    "Could not find macro for IL statement %s",
                    ins_str(stat_ins(stat)));

            /* Print out the replacement for now as a test */
            for (int k = 0; k < ismc_replace_count(ismc); ++k) {
                InsSelMacroReplace* ismr = ismc_replace(ismc, k);
                LOGF("%s ", asmins_str(ismr_ins(ismr)));
                if (ismr_op1_new(ismr)) {
                    LOG("%new");
                }
                else if (ismr_op1_virtual(ismr)) {
                    LOGF("%%%d", ismr_op1_index(ismr));
                }
                else if (ismr_op1_physical(ismr)) {
                    LOGF("%s", loc_str(ismr_op1_loc(ismr)));
                }

                if (ismr_op2_new(ismr)) {
                    LOG(",%new");
                }
                else if (ismr_op2_virtual(ismr)) {
                    LOGF(",%%%d", ismr_op2_index(ismr));
                }
                else if (ismr_op2_physical(ismr)) {
                    LOGF(",%s", loc_str(ismr_op2_loc(ismr)));
                }
                LOG("\n");
            }
        }
    }
}


static INSTRUCTION_CG(add) {
    if (stat_argc(stat) != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
    SymbolId lval_id = stat_arg(stat, 0);
    SymbolId op1_id = stat_arg(stat, 1);
    SymbolId op2_id = stat_arg(stat, 2);
    cg_validate_equal_size3(p, lval_id, op1_id, op2_id);

    Symbol* lval = symtab_get(p, lval_id);
    Symbol* op1 = symtab_get(p, op1_id);
    Symbol* op2 = symtab_get(p, op2_id);

    if (symbol_location(lval) == symbol_location(op1)) {
        cg_insss(p, "add", op1_id, op2_id);
    }
    else if (symbol_location(lval) == symbol_location(op2)) {
        cg_insss(p, "add", op2_id, op1_id);
    }
    else {
        cg_movss(p, op1_id, lval_id);
        cg_insss(p, "add", lval_id, op2_id);
    }
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
    SymbolId lval_id = stat_arg(stat, 0);
    SymbolId op1_id = stat_arg(stat, 1);
    SymbolId op2_id = stat_arg(stat, 2);
    cg_validate_equal_size3(p, lval_id, op1_id, op2_id);

    Symbol* lval = symtab_get(p, lval_id);
    Symbol* op2 = symtab_get(p, op2_id);
    int bytes = symbol_bytes(lval);

    /* rax:rdx is dividend,
       rdx is unused and zeroed */

    /* TODO in the future, less registers need to be saved
       depending on the size of the division */
    if (symbol_location(lval) != loc_a) {
        cg_insr(p, "push", reg_rax);
    }
    if (symbol_location(lval) != loc_d) {
        cg_insr(p, "push", reg_rdx);
    }
    /* Move first, op1 might be in dx */
    cg_movsr(p, op1_id, loc_a);
    parser_output_asm(p, "xor %s,%s\n",
            reg_get_str(loc_d, bytes),
            reg_get_str(loc_d, bytes));

    /* Put the immediate into a register to divide */
    if (symbol_is_constant(op2)) {
        cg_insr(p, "push", reg_rbx);
        cg_insrs(p, "mov", reg_get(loc_b, bytes), op2_id);
        cg_insr(p, "idiv", reg_get(loc_b, bytes));
        cg_insr(p, "pop", reg_rbx);
    }
    else {
        cg_inss(p, "idiv", op2_id);
    }
    cg_movrs(p, loc_a, lval_id);

    if (symbol_location(lval) != loc_d) {
        cg_insr(p, "pop", reg_rdx);
    }
    if (symbol_location(lval) != loc_a) {
        cg_insr(p, "pop", reg_rax);
    }
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

    Symbol* lval = symtab_get(p, lval_id);
    Symbol* op2 = symtab_get(p, op2_id);
    int bytes = symbol_bytes(lval);

    /* rax:rdx is dividend,
       rdx is unused and zeroed */

    if (symbol_location(lval) != loc_a) {
        cg_insr(p, "push", reg_rax);
    }
    if (symbol_location(lval) != loc_d) {
        cg_insr(p, "push", reg_rdx);
    }
    /* Move first, op1 might be in dx */
    cg_movsr(p, op1_id, loc_a);
    parser_output_asm(p, "xor %s,%s\n",
            reg_get_str(loc_d, bytes),
            reg_get_str(loc_d, bytes));

    /* Put the immediate into a register to divide */
    if (symbol_is_constant(op2)) {
        cg_insr(p, "push", reg_rbx);
        cg_insrs(p, "mov", reg_get(loc_b, bytes), op2_id);
        cg_insr(p, "idiv", reg_get(loc_b, bytes));
        cg_insr(p, "pop", reg_rbx);
    }
    else {
        cg_inss(p, "idiv", op2_id);
    }
    cg_movrs(p, loc_d, lval_id);

    if (symbol_location(lval) != loc_d) {
        cg_insr(p, "pop", reg_rdx);
    }
    if (symbol_location(lval) != loc_a) {
        cg_insr(p, "pop", reg_rax);
    }
}

static INSTRUCTION_CG(mov) {
    if (stat_argc(stat) != 2) {
        parser_set_error(p, ec_badargs);
        return;
    }

    SymbolId lval_id = stat_arg(stat, 0);
    SymbolId rval_id = stat_arg(stat, 1);
    cg_validate_equal_size2(p, lval_id, rval_id);

    cg_movss(p, rval_id, lval_id);
}

static INSTRUCTION_CG(mul) {
    if (stat_argc(stat) != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
    SymbolId lval_id = stat_arg(stat, 0);
    SymbolId op1_id = stat_arg(stat, 1);
    SymbolId op2_id = stat_arg(stat, 2);
    cg_validate_equal_size3(p, lval_id, op1_id, op2_id);

    Symbol* lval = symtab_get(p, lval_id);
    Symbol* op1 = symtab_get(p, op1_id);
    Symbol* op2 = symtab_get(p, op2_id);

    if (symbol_location(lval) == symbol_location(op1)) {
        cg_insss(p, "imul", op1_id, op2_id);
    }
    else if (symbol_location(lval) == symbol_location(op2)) {
        cg_insss(p, "imul", op2_id, op1_id);
    }
    else {
        cg_movss(p, op1_id, lval_id);
        cg_insss(p, "imul", lval_id, op2_id);
    }
}

static INSTRUCTION_CG(not) {
    if (stat_argc(stat) != 2) {
        parser_set_error(p, ec_badargs);
        return;
    }

    SymbolId lval_id = stat_arg(stat, 0);
    SymbolId rval_id = stat_arg(stat, 1);
    cg_validate_equal_size2(p, lval_id, rval_id);

    Symbol* lval = symtab_get(p, lval_id);
    int bytes = symbol_bytes(lval);

    cg_insss(p, "test", rval_id, rval_id);

    /* Setz only sets 1 byte, thus the the remaining register
       has to be cleared to obtain 1 or 0 */
    if (bytes > 1) {
        Register lval_reg_lower = reg_get(symbol_location(lval), 1);
        if (symbol_on_stack(lval)) {
            ASSERT(0, "Unimplemented");
        }
        else {
            cg_insr(p, "setz", lval_reg_lower);
            cg_inssr(p, "movzx", lval_id, lval_reg_lower);
        }
    }
    else {
        cg_inss(p, "setz", lval_id);
    }
}

static INSTRUCTION_CG(ret) {
    SymbolId sym_id = stat_arg(stat, 0);
    Symbol* sym = symtab_get(p, sym_id);
    if (sym == NULL) {
        parser_set_error(p, ec_unknownsym);
        goto exit;
    }

    /* Return integer types in rax */
    cg_movsr(p, sym_id, loc_a);

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
    cg_validate_equal_size3(p, lval_id, op1_id, op2_id);

    Symbol* lval = symtab_get(p, lval_id);
    Symbol* op1 = symtab_get(p, op1_id);
    Symbol* op2 = symtab_get(p, op2_id);

    if (symbol_location(lval) == symbol_location(op1)) {
        cg_insss(p, "sub", op1_id, op2_id);
    }
    else if (symbol_location(lval) == symbol_location(op2)) {
        cg_insss(p, "sub", op2_id, op1_id);
        cg_inss(p, "neg", lval_id);
    }
    else {
        cg_movss(p, op1_id, lval_id);
        cg_insss(p, "sub", lval_id, op2_id);
    }
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

    Symbol* dest = symtab_get(p, dest_id);
    int bytes = symbol_bytes(dest);

    cg_insss(p, "cmp", op1_id, op2_id);
    if (bytes > 1) {
        Register dest_reg_lower = reg_get(symbol_location(dest), 1);

        if (symbol_on_stack(dest)) {
            ASSERT(0, "Unimplemented");
        }
        else {
            cg_insr(p, set, dest_reg_lower);
            cg_inssr(p, "movzx", dest_id, dest_reg_lower);
        }
    }
    else {
        cg_inss(p, set, dest_id);
    }
}

/* Generates the necessary instructions to implement conditional jump
   1. Moves operand into register
   2. Tests (op1, op1)
   3. Performs the conditional jump to label using the results from test */
static void cg_testjmpcc(
        Parser* p,
        SymbolId label_id, SymbolId op1_id, const char* jmp) {
    Symbol* label = symtab_get(p, label_id);

    cg_insss(p, "test", op1_id, op1_id);
    parser_output_asm(p, "%s %s\n", jmp, symbol_name(label));
}

/* Generates assembly for instruction, with additional assembly as necessary
   to reference operand 1 (symbol) */
static void cg_inss(Parser* p, const char* ins, SymbolId op1_id) {
    parser_output_asm(p, "%s ", ins);
    cg_ref_symbol(p, op1_id);
    parser_output_asm(p, "\n");
}

/* Generates assembly for instruction, with additional assembly as necessary
   to reference operand 1 (register) */
static void cg_insr(Parser* p, const char* ins, Register op1_reg) {
    parser_output_asm(p, "%s %s\n", ins, reg_str(op1_reg));
}

/* Generates assembly for instruction, with additional assembly as necessary
   to reference operand 1 (symbol) and operand 2 (symbol) */
static void cg_insss(
        Parser* p, const char* ins, SymbolId op1_id, SymbolId op2_id) {
    parser_output_asm(p, "%s ", ins);
    cg_ref_symbol(p, op1_id);
    parser_output_asm(p, ",");
    cg_ref_symbol(p, op2_id);
    parser_output_asm(p, "\n");
}

/* Generates assembly for instruction, with additional assembly as necessary
   to reference operand 1 (symbol) and operand 2 (register) */
static void cg_inssr(
        Parser* p, const char* ins, SymbolId op1_id, Register op2_reg) {
    parser_output_asm(p, "%s ", ins);
    cg_ref_symbol(p, op1_id);
    parser_output_asm(p, ",%s\n", reg_str(op2_reg));
}

/* Generates assembly for instruction, with additional assembly as necessary
   to reference operand 1 (register) and operand 2 (symbol) */
static void cg_insrs(Parser* p, const char* ins, Register op1_reg, SymbolId op2_id) {
    parser_output_asm(p, "%s %s,", ins, reg_str(op1_reg));
    cg_ref_symbol(p, op2_id);
    parser_output_asm(p, "\n");
}

/* Generates the required assembly to copy from source (symbol) to
   destination (symbol) */
static void cg_movss(Parser* p, SymbolId src, SymbolId dest) {
    /* src already in dest */
    Symbol* dest_sym = symtab_get(p, dest);
    Symbol* src_sym = symtab_get(p, src);
    if (symbol_location(dest_sym) == symbol_location(src_sym)) {
        return;
    }
    cg_insss(p, "mov", dest, src);
}

/* Generates the required assembly to copy from source (symbol) to
   destination (register) */
static void cg_movsr(Parser* p, SymbolId source, Location dest) {
    Symbol* sym = symtab_get(p, source);
    if (dest == symbol_location(sym)) {
        return;
    }
    int bytes = symbol_bytes(sym);

    parser_output_asm(p, "mov %s,", reg_get_str(dest, bytes));
    cg_ref_symbol(p, source);
    parser_output_asm(p, "\n");
}

/* Generates the required assembly to copy from source (register) to
   destination (symbol) */
static void cg_movrs(Parser* p, Location source, SymbolId dest) {
    Symbol* sym = symtab_get(p, dest);
    if (source == symbol_location(sym)) {
        return;
    }
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
        parser_output_asm(p, "%s", reg_str(symbol_register(sym)));
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
    cfg_compute_pasm(p);
    cfg_output_asm(p);
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

    Parser p = {.rf = NULL, .of = NULL};
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

    parse(&p);

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

