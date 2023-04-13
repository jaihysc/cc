#include "CuTest.h"

#include "lexer.h"

void ReadToken(CuTest* tc) {
    Lexer lex;
    CuAssertIntEquals(tc, lexer_construct(&lex, "testu/testcode"), 0);

    /* Does not consume */
    CuAssertStrEquals(tc, lexer_getc(&lex), "float");
    CuAssertStrEquals(tc, lexer_getc(&lex), "float");
    CuAssertStrEquals(tc, lexer_getc(&lex), "float");

    lexer_consume(&lex);
    /* Reads next token after consume */
    CuAssertStrEquals(tc, lexer_getc(&lex), "z");
    CuAssertStrEquals(tc, lexer_getc(&lex), "z");
    CuAssertStrEquals(tc, lexer_getc(&lex), "z");

    lexer_consume(&lex);
    CuAssertStrEquals(tc, lexer_getc(&lex), "=");

    lexer_consume(&lex);
    CuAssertStrEquals(tc, lexer_getc(&lex), "1");

    lexer_consume(&lex);
    CuAssertStrEquals(tc, lexer_getc(&lex), ";");

    lexer_destruct(&lex);
}

CuSuite* LexerGetSuite() {
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, ReadToken);
    return suite;
}
