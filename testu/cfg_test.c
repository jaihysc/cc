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

CuSuite* CfgGetSuite() {
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, CreateNewBlock);
	return suite;
}
