#include "CuTest.h"

#include "symtab.h"

static void AddSymbol(CuTest* tc) {
    Symtab stab;
    symtab_construct(&stab);

    CuAssertIntEquals(tc, symtab_push_scope(&stab), ec_noerr);

    Symbol* sym;
    CuAssertIntEquals(tc, symtab_add(&stab, &sym, "ee", type_int), ec_noerr);

    CuAssertPtrNotNull(tc, sym);
    CuAssertStrEquals(tc, symbol_token(sym), "ee");

    symtab_destruct(&stab);
}

static void DuplicateSymbol(CuTest* tc) {
    Symtab stab;
    symtab_construct(&stab);

    CuAssertIntEquals(tc, symtab_push_scope(&stab), ec_noerr);

    Symbol* sym;
    CuAssertIntEquals(tc, symtab_add(&stab, &sym, "abc", type_int), ec_noerr);
    CuAssertIntEquals(tc, symtab_add(&stab, &sym, "abc", type_int), ec_symtab_dupname);

    symtab_destruct(&stab);
}

static void FindSymbol(CuTest* tc) {
    Symtab stab;
    symtab_construct(&stab);

    CuAssertIntEquals(tc, symtab_push_scope(&stab), ec_noerr);

    Symbol* sym;
    CuAssertIntEquals(tc, symtab_add(&stab, &sym, "llvm", type_int), ec_noerr);

    Symbol* found = symtab_find(&stab, "llvm");

    CuAssertPtrEquals(tc, sym, found);

    symtab_destruct(&stab);
}

static void AccessSymbolOutOfScope(CuTest* tc) {
    Symtab stab;
    symtab_construct(&stab);

    CuAssertIntEquals(tc, symtab_push_scope(&stab), ec_noerr);
    Symbol* sym;
    CuAssertIntEquals(tc, symtab_add(&stab, &sym, "symbol", type_int), ec_noerr);
    symtab_pop_scope(&stab);

    CuAssertIntEquals(tc, symtab_push_scope(&stab), ec_noerr);
    Symbol* sym2;
    CuAssertIntEquals(tc, symtab_add(&stab, &sym2, "symbol2", type_int), ec_noerr);
    symtab_pop_scope(&stab);

    /* Scope popped, can still access via the symbol id, but not name */
    CuAssertPtrEquals(tc, symtab_find(&stab, "symbol"), NULL);
    CuAssertPtrEquals(tc, symtab_find(&stab, "symbol2"), NULL);

    CuAssertPtrNotNull(tc, sym);
    CuAssertStrEquals(tc, symbol_token(sym), "symbol");

    CuAssertPtrNotNull(tc, sym2);
    CuAssertStrEquals(tc, symbol_token(sym2), "symbol2");

    symtab_destruct(&stab);
}

static void AddConstant(CuTest* tc) {
    Symtab stab;
    symtab_construct(&stab);

    Symbol* sym = NULL;
    symtab_add_constant(&stab, &sym, "1234", type_int);

    CuAssertPtrNotNull(tc, sym);
    CuAssertStrEquals(tc, symbol_token(sym), "1234");

    symtab_destruct(&stab);
}

static void ConstantZero(CuTest* tc) {
    Symtab stab;
    symtab_construct(&stab);

    Symbol* sym = symtab_constant_zero(&stab);

    CuAssertPtrNotNull(tc, sym);
    CuAssertStrEquals(tc, symbol_token(sym), "0");

    symtab_destruct(&stab);
}

CuSuite* SymtabGetSuite() {
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, AddSymbol);
    SUITE_ADD_TEST(suite, DuplicateSymbol);
    SUITE_ADD_TEST(suite, FindSymbol);
    SUITE_ADD_TEST(suite, AccessSymbolOutOfScope);
    SUITE_ADD_TEST(suite, AddConstant);
    SUITE_ADD_TEST(suite, ConstantZero);
    return suite;
}
