#include "CuTest.h"

#include "cfg.h"

static void CreateNewBlock(CuTest* tc) {
	Cfg cfg;
	CuAssertIntEquals(tc, cfg_construct(&cfg), ec_noerr);

	Block* block = NULL;
	CuAssertIntEquals(tc, cfg_new_block(&cfg, &block), ec_noerr);

	CuAssertPtrNotNull(tc, block);

	cfg_destruct(&cfg);
}

static void ComputeBranchDestination(CuTest* tc) {
	Cfg cfg;
	CuAssertIntEquals(tc, cfg_construct(&cfg), ec_noerr);

	/* Setup CFG blocks */
	Block* block1 = NULL;
	CuAssertIntEquals(tc, cfg_new_block(&cfg, &block1), ec_noerr);

	Block* block2 = NULL;
	CuAssertIntEquals(tc, cfg_new_block(&cfg, &block2), ec_noerr);

	Block* block3 = NULL;
	CuAssertIntEquals(tc, cfg_new_block(&cfg, &block3), ec_noerr);

	/* Setup symbols */
	Type type;
	CuAssertIntEquals(tc, type_construct(&type, ts_none, 0), ec_noerr);

	Symbol label;
	CuAssertIntEquals(tc, symbol_construct(&label, "a", &type), ec_noerr);

	/* Should link block 1 to 3 */
	IL2Statement branchInstruction = il2stat_make1(il2_jmp, &label);
	CuAssertIntEquals(tc, block_add_ilstat(block1, branchInstruction), ec_noerr);

	CuAssertIntEquals(tc, block_add_label(block3, &label), ec_noerr);

	cfg_link_branch_dest(&cfg);

	CuAssertPtrEquals(tc, block_next(block1, 0), block3);
	CuAssertPtrEquals(tc, block_next(block2, 0), NULL);
	CuAssertPtrEquals(tc, block_next(block3, 0), NULL);

	symbol_destruct(&label);
	type_destruct(&type);
	cfg_destruct(&cfg);
}

static void RemoveUnreachableBlock(CuTest* tc) {
	Cfg cfg;
	CuAssertIntEquals(tc, cfg_construct(&cfg), ec_noerr);

	Block* block1 = NULL;
	CuAssertIntEquals(tc, cfg_new_block(&cfg, &block1), ec_noerr);

	Block* block2 = NULL;
	CuAssertIntEquals(tc, cfg_new_block(&cfg, &block2), ec_noerr);

	Block* block3 = NULL;
	CuAssertIntEquals(tc, cfg_new_block(&cfg, &block3), ec_noerr);

	Block* block4 = NULL;
	CuAssertIntEquals(tc, cfg_new_block(&cfg, &block4), ec_noerr);

	block_link(block1, block3);
	block_link(block2, block4);

	cfg_remove_unreachable(&cfg);

	CuAssertIntEquals(tc, cfg_block_count(&cfg), 2); /* Only block 1 and 3 reachable */

	cfg_destruct(&cfg);
}

static void ComputeBlockUseDef(CuTest* tc) {
	Cfg cfg;
	CuAssertIntEquals(tc, cfg_construct(&cfg), ec_noerr);

	Block* block = NULL;
	CuAssertIntEquals(tc, cfg_new_block(&cfg, &block), ec_noerr);

	/* Setup types */
	Type type;
	CuAssertIntEquals(tc, type_construct(&type, ts_int, 0), ec_noerr);

	Symbol dest;
	Symbol arg1;
	Symbol arg2;
	CuAssertIntEquals(tc, symbol_construct(&dest, "a", &type), ec_noerr);
	CuAssertIntEquals(tc, symbol_construct(&arg1, "b", &type), ec_noerr);
	CuAssertIntEquals(tc, symbol_construct(&arg2, "c", &type), ec_noerr);

	/* Add statement to block */
	CuAssertIntEquals(tc, block_add_ilstat(block, il2stat_make(il2_add, &dest, &arg1, &arg2)), ec_noerr);

	CuAssertIntEquals(tc, cfg_compute_block_use_def(&cfg), ec_noerr);
	CuAssertTrue(tc, set_contains(block_def(block), &dest));
	CuAssertTrue(tc, set_contains(block_use(block), &arg1));
	CuAssertTrue(tc, set_contains(block_use(block), &arg2));

	symbol_destruct(&arg2);
	symbol_destruct(&arg1);
	symbol_destruct(&dest);

	type_destruct(&type);

	cfg_destruct(&cfg);
}

CuSuite* CfgGetSuite(void) {
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, CreateNewBlock);
	SUITE_ADD_TEST(suite, ComputeBranchDestination);
	SUITE_ADD_TEST(suite, RemoveUnreachableBlock);
	SUITE_ADD_TEST(suite, ComputeBlockUseDef);
	return suite;
}
