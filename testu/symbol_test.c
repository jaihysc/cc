#include "CuTest.h"

#include "symbol.h"

static void SymbolConstruct(CuTest* tc) {
    Symbol sym;
    CuAssertIntEquals(
            tc,
            symbol_construct(&sym, "abc", type_int, vc_lval, 0),
            ec_noerr
    );

    CuAssertStrEquals(tc, symbol_token(&sym), "abc");
}

static void SymbolNameTooLong(CuTest* tc) {
    const char* long_name =
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
        "abcdefghijklmnopqrstuvwxyz"
        "abcdefghijklmnopqrstuvwxyz";

    Symbol sym;
    CuAssertIntEquals(
            tc,
            symbol_construct(&sym, long_name, type_int, vc_lval, 0),
            ec_symbol_nametoolong
    );

    CuAssertStrEquals(tc, symbol_token(&sym), "");
}

CuSuite* SymbolGetSuite() {
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, SymbolConstruct);
    SUITE_ADD_TEST(suite, SymbolNameTooLong);
    return suite;
}
