#include "cfg.h"

#include "common.h"
#include "symtab.h"

ErrorCode block_construct(Block* blk) {
	ASSERT(blk != NULL, "Block is null");
	ErrorCode ecode;

	vec_construct(&blk->labels);
	vec_construct(&blk->il_stats);
	blk->next[0] = NULL;
	blk->next[1] = NULL;
	blk->data = 0;

	if ((ecode = set_construct(&blk->use)) != ec_noerr) return ecode;
	if ((ecode = set_construct(&blk->def)) != ec_noerr) return ecode;

	return ec_noerr;
}

void block_destruct(Block* blk) {
	ASSERT(blk != NULL, "Block is null");
	set_destruct(&blk->def);
	set_destruct(&blk->use);

	vec_destruct(&blk->il_stats);
	vec_destruct(&blk->labels);
}

int block_lab_count(Block* blk) {
	ASSERT(blk != NULL, "Block is null");
	return vec_size(&blk->labels);
}

Symbol* block_lab(Block* blk, int i) {
	ASSERT(blk != NULL, "Block is null");
	ASSERT(i >= 0, "Index out of range");
	ASSERT(i < block_lab_count(blk), "Index out of range");
	return vec_at(&blk->labels, i);
}

ErrorCode block_add_label(Block* blk, Symbol* lab) {
	ASSERT(blk != NULL, "Block is null");
	if (!vec_push_back(&blk->labels, lab)) return ec_badalloc;
	return ec_noerr;
}

int block_ilstat_count(Block* blk) {
	ASSERT(blk != NULL, "Block is null");
	return vec_size(&blk->il_stats);
}

IL2Statement* block_ilstat(Block* blk, int i) {
	ASSERT(blk != NULL, "Block is null");
	ASSERT(i >= 0, "Index out of range");
	ASSERT(i < block_ilstat_count(blk), "Index out of range");
	return &vec_at(&blk->il_stats, i);
}

ErrorCode block_add_ilstat(Block* blk, IL2Statement stat) {
	ASSERT(blk != NULL, "Block is null");
	if (!vec_push_back(&blk->il_stats, stat)) return ec_badalloc;
	return ec_noerr;
}

int block_data(Block* blk) {
	ASSERT(blk != NULL, "Block is null");
	return blk->data;
}

void block_set_data(Block* blk, int data) {
	ASSERT(blk != NULL, "Block is null");
	blk->data = data;
}

/* Set of variables used by the block */
Set* block_use(Block* blk) {
	ASSERT(blk != NULL, "Block is null");
	return &blk->use;
}

/* Set of variables defined at the end of this block */
Set* block_def(Block* blk) {
	ASSERT(blk != NULL, "Block is null");
	return &blk->def;
}

ErrorCode block_compute_use_def(Block* blk) {
	ErrorCode ecode;
	for (int i = block_ilstat_count(blk) - 1; i >= 0; --i) {
		IL2Statement* stat = block_ilstat(blk, i);

		Symbol* syms[MAX_IL2_ARGS]; /* Buffer to hold symbols */

		/* Add 'def'ed symbol from statement to 'def' for block
		   Remove occurrences of 'def'd symbol from 'use' for block */
		int def_count = il2stat_def(stat, syms);
		for (int j = 0; j < def_count; ++j) {
			ASSERT(!symbol_is_constant(syms[j]), "Defined symbol should not be constant");

			if ((ecode = set_add(&blk->def, syms[j])) != ec_noerr) return ecode;
			if ((ecode = set_remove(&blk->use, syms[j])) != ec_noerr) return ecode;
		}

		/* Add 'use'd symbols from statement to 'use' for block */
		int use_count = il2stat_use(stat, syms);
		for (int j = 0; j < use_count; ++j) {
			if (symbol_is_constant(syms[j])) {
				continue;
			}
			if ((ecode = set_add(&blk->use, syms[j])) != ec_noerr) return ecode;
		}
	}
	return ec_noerr;
}

void block_link(Block* blk, Block* next) {
	ASSERT(blk != NULL, "Block is null");
	ASSERT(blk != next, "Cannot link block to self");
	for (int i = 0; i < MAX_BLOCK_LINK; ++i) {
		if (blk->next[i] == NULL) {
			blk->next[i] = next;
			return;
		}
	}
	ASSERT(0, "Too many links out of block");
}

Block* block_next(Block* blk, int i) {
	ASSERT(blk != NULL, "Block is null");
	ASSERT(i >= 0, "Index out of range");
	ASSERT(i < MAX_BLOCK_LINK, "Index out of range");

	return blk->next[i];
}

ErrorCode cfg_construct(Cfg* cfg) {
	vec_construct(&cfg->blocks);
	return ec_noerr;
}

void cfg_destruct(Cfg* cfg) {
	cfg_clear(cfg);
	vec_destruct(&cfg->blocks);
}

void cfg_clear(Cfg* cfg) {
	for (int i = 0; i < vec_size(&cfg->blocks); ++i) {
		Block* blk = vec_at(&cfg->blocks, i);
		block_destruct(blk);
		free(blk);
	}
	vec_clear(&cfg->blocks);
}

ErrorCode cfg_new_block(Cfg* cfg, Block** block_ptr) {
	ErrorCode ecode;

	/* Allocate new block */
	Block* blk = malloc(sizeof(Block));
	if (blk == NULL) {
		ecode = ec_badalloc;
		goto exit1;
	}
	if ((ecode = block_construct(blk)) != ec_noerr) goto exit2;

	/* Append block to cfg */
	if (!vec_push_backu(&cfg->blocks)) {
		ecode = ec_badalloc;
		goto exit3;
	}
	vec_back(&cfg->blocks) = blk;

	*block_ptr = blk;

	return ec_noerr;

exit3:
	block_destruct(blk);
exit2:
	free(blk);
exit1:
	return ecode;
}

int cfg_block_count(Cfg* cfg) {
	return vec_size(&cfg->blocks);
}

/* Returns block in CFG at index */
Block* cfg_block(Cfg* cfg, int i) {
	return vec_at(&cfg->blocks, i);
}

Block* cfg_find_labelled(Cfg* cfg, Symbol* lab) {
	for (int i = 0; i < vec_size(&cfg->blocks); ++i) {
		Block* blk = vec_at(&cfg->blocks, i);
		for (int j = 0; j < block_lab_count(blk); ++j) {
			if (block_lab(blk, j) == lab) {
				return blk;
			}
		}
	}
	return NULL;
}

void cfg_link_branch_dest(Cfg* cfg) {
	for (int blk_idx = 0; blk_idx < cfg_block_count(cfg); ++blk_idx) {
		Block* blk = cfg_block(cfg, blk_idx);
		if (block_ilstat_count(blk) == 0) continue;

		IL2Statement* ilstat = block_ilstat(blk, block_ilstat_count(blk) - 1);
		IL2Ins ins = il2stat_ins(ilstat);

		/* Get label branch instruction goes to
		   Exclude ret, it does not go to label */
		if (il2_is_branch(ins) && ins != il2_ret) {
			Symbol* target_lab = il2stat_arg(ilstat, 0);

			Block* target_blk = cfg_find_labelled(cfg, target_lab);
			block_link(blk, target_blk);
		}
	}
}

ErrorCode cfg_remove_unreachable(Cfg* cfg) {
	ErrorCode ecode = ec_noerr;

	/* Zero all marks on blocks */
	for (int blk_idx = 0; blk_idx < cfg_block_count(cfg); ++blk_idx) {
		Block* blk = cfg_block(cfg, blk_idx);
		block_set_data(blk, 0);
	}

	if (cfg_block_count(cfg) == 0) goto exit;

	/* Add the root block in buffer
	   At a block, add the next blocks to the buffer */

	/* Circular buffer for blocks to traverse */
	const int blk_buffer_capacity = cfg_block_count(cfg) + 1;
	Block** blk_buffer = malloc(blk_buffer_capacity * sizeof(Block*));
	if (blk_buffer == NULL) {
		ecode = ec_badalloc;
		goto exit;
	}

	/* Add root node (first block) */
	Block* root_block = cfg_block(cfg, 0);
	blk_buffer[0] = root_block;
	block_set_data(root_block, 1);

	int blk_buffer_start = 0; /* Index first block */
	int blk_buffer_end = 1;	  /* 1 past index last block */

	/* Traverse the Cfg, mark the reachable blocks */
	while (blk_buffer_start != blk_buffer_end) {
		Block* blk = blk_buffer[blk_buffer_start];

		/* Add the next blocks of this block, to the buffer to be traversed */
		for (int i = 0; i < MAX_BLOCK_LINK; ++i) {
			Block* next_blk = block_next(blk, i);
			if (next_blk == NULL) continue;

			/* Do not traverse next block if it has already been traversed
			   (cycles arising from loops) */
			if (block_data(next_blk) == 1) continue;

			/* Mark block as traversed, avoids adding the same block multiple times */
			block_set_data(next_blk, 1);

			ASSERT(blk_buffer_end + 1 != blk_buffer_start, "Circular buffer out of space");
			blk_buffer[blk_buffer_end] = next_blk;

			++blk_buffer_end;
			if (blk_buffer_end >= blk_buffer_capacity) blk_buffer_end = 0;
		}

		++blk_buffer_start;
		if (blk_buffer_start >= blk_buffer_capacity) blk_buffer_start = 0;
	}

	/* Remove the unreachable blocks */
	int blocks_removed = 0;
	int write_idx = 0;
	for (int blk_idx = 0; blk_idx < cfg_block_count(cfg); ++blk_idx) {
		Block* blk = cfg_block(cfg, blk_idx);

		/* Reachable block, shuffle it up to overwrite unreachable blocks */
		if (block_data(blk) == 1) {
			vec_at(&cfg->blocks, write_idx) = vec_at(&cfg->blocks, blk_idx);
			++write_idx;
		}
		else {
			/* Unreachable, delete the block */
			block_destruct(blk);
			free(blk);
			++blocks_removed;
		}
	}

	/* Remove any empty spaces at end of vector arising from the blocks removed */
	vec_splice(&cfg->blocks, vec_size(&cfg->blocks) - blocks_removed, blocks_removed);

	free(blk_buffer);
exit:
	return ecode;
}

ErrorCode cfg_compute_block_use_def(Cfg* cfg) {
	for (int i = 0; i < cfg_block_count(cfg); ++i) {
		Block* blk = cfg_block(cfg, i);

		ErrorCode ecode = block_compute_use_def(blk);
		if (ecode != ec_noerr) return ecode;
	}
	return ec_noerr;
}

void debug_print_cfg(Cfg* cfg) {
	LOGF("Control flow graph [%d]\n", vec_size(&cfg->blocks));
	for (int i = 0; i < vec_size(&cfg->blocks); ++i) {
		LOGF("  Block %d\n", i);
		Block* blk = vec_at(&cfg->blocks, i);

		/* Labels associated with block */
		if (block_lab_count(blk) > 0) {
			LOG("    Labels:");
			for (int j = 0; j < block_lab_count(blk); ++j) {
				LOGF(" %s", symbol_token(block_lab(blk, j)));
			}
			LOG("\n");
		}

		/* Print use/def */
		LOG("    USE:");
		uint64_t size;
		void** data = set_data(block_use(blk), &size);
		for (uint64_t j = 0; j < size; ++j) {
			Symbol* sym = data[j];
			LOGF(" %s", symbol_token(sym));
		}
		free(data);
		LOG("\n");

		LOG("    DEF:");
		data = set_data(block_def(blk), &size);
		for (uint64_t j = 0; j < size; ++j) {
			Symbol* sym = data[j];
			LOGF(" %s", symbol_token(sym));
		}
		free(data);
		LOG("\n");

		/* Print IL instruction and arguments */
		LOG("    IL:\n");
		for (int j = 0; j < block_ilstat_count(blk); ++j) {
			IL2Statement* stat = block_ilstat(blk, j);
			LOGF("    %3d %8s", j, il2_str(il2stat_ins(stat)));
			for (int k = 0; k < il2stat_argc(stat); ++k) {
				if (k == 0) {
					LOG(" ");
				}
				else {
					LOG(", ");
				}
				LOGF("%s", symbol_token(il2stat_arg(stat, k)));
			}
			LOG("\n");
		}

		/* Print next block index */
		LOG("    ->");
		for (int j = 0; j < MAX_BLOCK_LINK; ++j) {
			Block* next_blk = block_next(blk, j);
			if (next_blk == NULL) continue;

			/* Find index of block matching address */
			for (int k = 0; k < vec_size(&cfg->blocks); ++k) {
				if (vec_at(&cfg->blocks, k) == next_blk) {
					LOGF(" %d", k);
					break;
				}
			}
		}
		LOG("\n");
	}
}
