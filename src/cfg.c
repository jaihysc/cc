#include "cfg.h"

#include "common.h"

ErrorCode block_construct(Block* blk) {
	ASSERT(blk != NULL, "Block is null");
	vec_construct(&blk->labels);
	vec_construct(&blk->il_stats);
	blk->next[0] = NULL;
	blk->next[1] = NULL;
	return ec_noerr;
}

void block_destruct(Block* blk) {
	ASSERT(blk != NULL, "Block is null");
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
