/* Control Flow Graph (CFG) */
#ifndef CFG_H
#define CFG_H

#include "il2statement.h"
#include "symbol.h"
#include "vec.h"

#define MAX_BLOCK_LINK 2

/* Blocks are formed by partitioning il statements according to the rules:
   1. Control always enters at the start of the block
   2. Control always leaves at the last statement or end of the block */
typedef struct Block
{
	/* Labels at the entry of this block */
	vec_t(Symbol*) labels;
	vec_t(IL2Statement) il_stats;

	/* At most 2 options:
	   1. Flow through to next block
	   2. Jump at end
	   Is offset (in Block) from current location, cannot use pointer
	   as container holding Block may resize */
	struct Block* next[MAX_BLOCK_LINK];
} Block;

ErrorCode block_construct(Block* blk);

void block_destruct(Block* blk);

/* Returns number of labels in block */
int block_lab_count(Block* blk);

/* Returns label at index in block */
Symbol* block_lab(Block* blk, int i);

/* Adds a new label at the entry of this block */
ErrorCode block_add_label(Block* blk, Symbol* lab);

/* Returns number of IL statements in block */
int block_ilstat_count(Block* blk);

/* Returns IL statement at index in block */
IL2Statement* block_ilstat(Block* blk, int i);

/* Adds IL statement to block */
ErrorCode block_add_ilstat(Block* blk, IL2Statement stat);

/* Links block to next block */
void block_link(Block* blk, Block* next);

/* Returns pointer to ith next block */
Block* block_next(Block* blk, int i);

typedef struct
{
	vec_t(Block*) blocks;
} Cfg;

ErrorCode cfg_construct(Cfg* cfg);

void cfg_destruct(Cfg* cfg);

/* Clears control flow graph */
void cfg_clear(Cfg* cfg);

/* Adds a new, unlinked block to the cfg
   Block saved to provided pointer */
ErrorCode cfg_new_block(Cfg* cfg, Block** block_ptr);

/* Returns number of blocks in CFG */
int cfg_block_count(Cfg* cfg);

/* Returns block in CFG at index */
Block* cfg_block(Cfg* cfg, int i);

/* Finds the first block which has the provided label
   Returns null if not found */
Block* cfg_find_labelled(Cfg* cfg, Symbol* lab);

void debug_print_cfg(Cfg* cfg);

#endif
