/* Assembly generator, struct Block */
#ifndef ASMGEN_BLOCK_H
#define ASMGEN_BLOCK_H

/* Blocks are formed by partitioning il statements according to the rules:
   1. Control always enters at the start of the block
   2. Control always leaves at the last statement or end of the block */
typedef struct {
    /* Labels at the entry of this block */
    vec_t(SymbolId) labels;
    vec_t(ILStatement) il_stats;
    vec_t(PasmStatement) pasm_stats;

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
    vec_construct(&blk->il_stats);
    vec_construct(&blk->pasm_stats);
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
    for (int i = 0; i < vec_size(&blk->pasm_stats); ++i) {
        pasmstat_destruct(&vec_at(&blk->pasm_stats, i));
    }
    vec_destruct(&blk->pasm_stats);
    vec_destruct(&blk->il_stats);
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

/* Adds a new label at the entry of this block
   Returns 1 if successful, 0 if not */
static int block_add_label(Block* blk, SymbolId lab_id) {
    ASSERT(blk != NULL, "Block is null");
    return vec_push_back(&blk->labels, lab_id);
}

/* Returns number of IL statements in block */
static int block_ilstat_count(Block* blk) {
    ASSERT(blk != NULL, "Block is null");
    return vec_size(&blk->il_stats);
}

/* Returns IL statement at index in block */
static ILStatement* block_ilstat(Block* blk, int i) {
    ASSERT(blk != NULL, "Block is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < block_ilstat_count(blk), "Index out of range");
    return &vec_at(&blk->il_stats, i);
}

/* Adds IL statement to block
   Returns 1 if successful, 0 if not */
static int block_add_ilstat(Block* blk, ILStatement stat) {
    ASSERT(blk != NULL, "Block is null");
    return vec_push_back(&blk->il_stats, stat);
}

/* Returns number of pseudo-assembly statements in block */
static int block_pasmstat_count(Block* blk) {
    ASSERT(blk != NULL, "Block is null");
    return vec_size(&blk->pasm_stats);
}

/* Returns pseudo-assembly statement at index in block */
static PasmStatement* block_pasmstat(Block* blk, int i) {
    ASSERT(blk != NULL, "Block is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < block_pasmstat_count(blk), "Index out of range");
    return &vec_at(&blk->pasm_stats, i);
}

/* Adds pseudo-assembly statement to block
   Returns 1 if successful, 0 if not */
static int block_add_pasmstat(Block* blk, PasmStatement stat) {
    ASSERT(blk != NULL, "Block is null");
    return vec_push_back(&blk->pasm_stats, stat);
}

/* Adds pseudo-assembly statement to block at index, shifting
   statements after the index to make room
   Returns 1 if successful, 0 if not */
static int block_insert_pasmstat(Block* blk, PasmStatement stat, int i) {
    ASSERT(blk != NULL, "Block is null");
    return vec_insert(&blk->pasm_stats, stat, i);
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
            blk->next[i] = (int)(next - blk);
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

#endif
