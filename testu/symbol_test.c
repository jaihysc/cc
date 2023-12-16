#include "CuTest.h"

#include "symbol.h"

static void SymbolConstruct(CuTest* tc) {
	Type type;
	type_construct(&type, ts_int, 0);

	Symbol sym;
	CuAssertIntEquals(tc, symbol_construct(&sym, "abc", &type), ec_noerr);

	CuAssertStrEquals(tc, symbol_token(&sym), "abc");

	symbol_destruct(&sym);
	type_destruct(&type);
}

static void SymbolNameTooLong(CuTest* tc) {
	const char* long_name = "abcdefghijklmnopqrstuvwxyz"
							"abcdefghijklmnopqrstuvwxyz"
							"abcdefghijklmnopqrstuvwxyz"
							"abcdefghijklmnopqrstuvwxyz"
							"abcdefghijklmnopqrstuvwxyz"
							"abcdefghijklmnopqrstuvwxyz"
							"abcdefghijklmnopqrstuvwxyz"
							"abcdefghijklmnopqrstuvwxyz"
							"abcdefghijklmnopqrstuvwxyz"
							"abcdefghijklmnopqrstuvwxyz"
							"abcdefghijklmnopqrstuvwxyz"
							"abcdefghijklmnopqrstuvwxyz"
							"abcdefghijklmnopqrstuvwxyz"
							"abcdefghijklmnopqrstuvwxyz"
							"abcdefghijklmnopqrstuvwxyz"
							"abcdefghijklmnopqrstuvwxyz"
							"abcdefghijklmnopqrstuvwxyz"
							"abcdefghijklmnopqrstuvwxyz"
							"abcdefghijklmnopqrstuvwxyz"
							"abcdefghijklmnopqrstuvwxyz";

	Type type;
	type_construct(&type, ts_int, 0);

	Symbol sym;
	CuAssertIntEquals(tc, symbol_construct(&sym, long_name, &type), ec_symbol_nametoolong);

	CuAssertStrEquals(tc, symbol_token(&sym), "");

	type_destruct(&type);
}

CuSuite* SymbolGetSuite(void) {
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, SymbolConstruct);
	SUITE_ADD_TEST(suite, SymbolNameTooLong);
	return suite;
}
