#include "CuTest.h"

#include "set.h"

static void SetAdd(CuTest* tc) {
	Set set;
	CuAssertIntEquals(tc, set_construct(&set), ec_noerr);

	int* a = NULL;
	int* b = NULL;
	int* c = NULL;

	set_add(&set, &a);
	set_add(&set, &a);
	CuAssertIntEquals(tc, set_size(&set), 1);

	set_add(&set, &b);
	CuAssertIntEquals(tc, set_size(&set), 2);
	CuAssertTrue(tc, set_contains(&set, &a));
	CuAssertTrue(tc, set_contains(&set, &b));
	CuAssertFalse(tc, set_contains(&set, &c));

	set_destruct(&set);
}

static void SetRemove(CuTest* tc) {
	Set set;
	CuAssertIntEquals(tc, set_construct(&set), ec_noerr);

	int* a = NULL;
	int* b = NULL;

	set_add(&set, &a);
	set_add(&set, &b);
	CuAssertIntEquals(tc, set_size(&set), 2);

	set_remove(&set, &b);
	set_remove(&set, &b);
	CuAssertIntEquals(tc, set_size(&set), 1);
	CuAssertTrue(tc, set_contains(&set, &a));
	CuAssertFalse(tc, set_contains(&set, &b));

	set_clear(&set);
	CuAssertIntEquals(tc, set_size(&set), 0);
	CuAssertFalse(tc, set_contains(&set, &a));
	CuAssertFalse(tc, set_contains(&set, &b));

	set_destruct(&set);
}

CuSuite* SetGetSuite(void) {
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, SetAdd);
	SUITE_ADD_TEST(suite, SetRemove);
	return suite;
}
