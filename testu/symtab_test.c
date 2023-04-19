#include "CuTest.h"

#include "symtab.h"

static void AddSymbol(CuTest* tc) {
    Symtab stab;
    symtab_construct(&stab);

    CuAssertIntEquals(tc, symtab_push_scope(&stab), ec_noerr);

    SymbolId symid;
    CuAssertIntEquals(tc, symtab_add(&stab, &symid, "ee", type_int), ec_noerr);

    Symbol* sym = symtab_get(&stab, symid);
    CuAssertPtrNotNull(tc, sym);
    CuAssertStrEquals(tc, symbol_token(sym), "ee");

    symtab_destruct(&stab);
}

static void DuplicateSymbol(CuTest* tc) {
    Symtab stab;
    symtab_construct(&stab);

    CuAssertIntEquals(tc, symtab_push_scope(&stab), ec_noerr);

    SymbolId symid;
    CuAssertIntEquals(tc, symtab_add(&stab, &symid, "abc", type_int), ec_noerr);
    CuAssertIntEquals(tc, symtab_add(&stab, &symid, "abc", type_int), ec_symtab_dupname);

    symtab_destruct(&stab);
}

static void FindSymbol(CuTest* tc) {
    Symtab stab;
    symtab_construct(&stab);

    CuAssertIntEquals(tc, symtab_push_scope(&stab), ec_noerr);

    SymbolId symid;
    CuAssertIntEquals(tc, symtab_add(&stab, &symid, "llvm", type_int), ec_noerr);

    SymbolId foundid = symtab_find(&stab, "llvm");

    CuAssertTrue(tc, symid_equal(symid, foundid));

    symtab_destruct(&stab);
}

CuSuite* SymtabGetSuite() {
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, AddSymbol);
    SUITE_ADD_TEST(suite, DuplicateSymbol);
    SUITE_ADD_TEST(suite, FindSymbol);
    return suite;
}
