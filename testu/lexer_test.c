#include "CuTest.h"

#include "lexer.h"

static void ReadToken(CuTest* tc) {
    Lexer lex;
    CuAssertIntEquals(tc, lexer_construct(&lex, "testu/testcode"), ec_noerr);

    /* Does not consume */
    const char* token;
    CuAssertIntEquals(tc, lexer_getc(&lex, &token), ec_noerr);
    CuAssertStrEquals(tc, token, "float");
    CuAssertIntEquals(tc, lexer_getc(&lex, &token), ec_noerr);
    CuAssertStrEquals(tc, token, "float");
    CuAssertIntEquals(tc, lexer_getc2(&lex, &token), ec_noerr);
    CuAssertStrEquals(tc, token, "z");
    CuAssertIntEquals(tc, lexer_getc2(&lex, &token), ec_noerr);
    CuAssertStrEquals(tc, token, "z");

    lexer_consume(&lex);
    /* Reads next token after consume */
    CuAssertIntEquals(tc, lexer_getc(&lex, &token), ec_noerr);
    CuAssertStrEquals(tc, token, "z");
    CuAssertIntEquals(tc, lexer_getc(&lex, &token), ec_noerr);
    CuAssertStrEquals(tc, token, "z");
    CuAssertIntEquals(tc, lexer_getc2(&lex, &token), ec_noerr);
    CuAssertStrEquals(tc, token, "=");
    CuAssertIntEquals(tc, lexer_getc2(&lex, &token), ec_noerr);
    CuAssertStrEquals(tc, token, "=");

    lexer_consume(&lex);
    CuAssertIntEquals(tc, lexer_getc(&lex, &token), ec_noerr);
    CuAssertStrEquals(tc, token, "=");
    CuAssertIntEquals(tc, lexer_getc2(&lex, &token), ec_noerr);
    CuAssertStrEquals(tc, token, "1");

    lexer_consume(&lex);
    CuAssertIntEquals(tc, lexer_getc(&lex, &token), ec_noerr);
    CuAssertStrEquals(tc, token, "1");
    CuAssertIntEquals(tc, lexer_getc2(&lex, &token), ec_noerr);
    CuAssertStrEquals(tc, token, ";");

    lexer_consume(&lex);
    CuAssertIntEquals(tc, lexer_getc(&lex, &token), ec_noerr);
    CuAssertStrEquals(tc, token, ";");
    CuAssertIntEquals(tc, lexer_getc2(&lex, &token), ec_noerr);
    CuAssertStrEquals(tc, token, "int");

    lexer_destruct(&lex);
}

CuSuite* LexerGetSuite() {
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, ReadToken);
    return suite;
}
