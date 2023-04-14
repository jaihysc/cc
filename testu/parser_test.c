#include "CuTest.h"

#include "common.h"
#include "parser.h"

static void ParserConstruct(CuTest* tc, Parser* p) {
    Lexer* lex = cmalloc(sizeof(Lexer));
    CuAssertIntEquals(tc, lexer_construct(lex, "testu/testcode"), ec_noerr);

    Tree* tree = cmalloc(sizeof(Tree));
    CuAssertIntEquals(tc, tree_construct(tree), ec_noerr);

    CuAssertIntEquals(tc, parser_construct(p, lex, tree), ec_noerr);
}

static void ParserDestruct(CuTest* /*tc*/, Parser* p) {
    cfree(p->tree);

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
