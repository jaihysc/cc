#include "CuTest.h"

#include "common.h"
#include "parser.h"

static void ParserConstruct(CuTest* tc, Parser* p) {
    Lexer* lex = cmalloc(sizeof(Lexer));
    CuAssertIntEquals(tc, lexer_construct(lex, "testu/testfunc"), ec_noerr);

    Symtab* symtab = cmalloc(sizeof(Symtab));
    CuAssertIntEquals(tc, symtab_construct(symtab), ec_noerr);

    symtab_push_scope(symtab);

    Tree* tree = cmalloc(sizeof(Tree));
    CuAssertIntEquals(tc, tree_construct(tree), ec_noerr);

    CuAssertIntEquals(tc, parser_construct(p, lex, symtab, tree), ec_noerr);
}

static void ParserDestruct(CuTest* /*tc*/, Parser* p) {
    tree_destruct(p->tree);
    cfree(p->tree);

    symtab_destruct(p->symtab);
    cfree(p->symtab);

    lexer_destruct(p->lex);
    cfree(p->lex);
}

static void ParseFunction(CuTest* tc) {
    Parser p;
    ParserConstruct(tc, &p);

    parse_translation_unit(&p);

    ParserDestruct(tc, &p);
}

CuSuite* ParserGetSuite() {
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, ParseFunction);
    return suite;
}
