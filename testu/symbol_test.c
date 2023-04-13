#include "CuTest.h"

#include "symbol.h"

void SymbolConstruct(CuTest* tc) {
}

CuSuite* SymbolGetSuite() {
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, SymbolConstruct);
    return suite;
}
