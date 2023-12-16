#include "CuTest.h"

#include "lexer.h"


static const char* testcode = "float z = 1;\n"
							  "\n"
							  "int\n"
							  "main(int argc, char** argv) {\n"
							  "    return 1;\n"
							  "}\n";

static const char* testcode2 = "// This is line 1\n"
							   "// This is another comment\n"
							   "\n"
							   "\n"
							   "int a = 0;\n"
							   "\n"
							   "// This is line 2\n"
							   "/* Yet another comment here\n"
							   "// Spanning this line too */\n"
							   "\n"
							   "float b = 0;\n"
							   "// This is line 3";

static void ReadToken(CuTest* tc) {
	Lexer lex;
	CuAssertIntEquals(tc, lexer_construct(&lex, testcode), ec_noerr);

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

static void SkipComments(CuTest* tc) {
	Lexer lex;
	CuAssertIntEquals(tc, lexer_construct(&lex, testcode2), ec_noerr);

	const char* token;

	CuAssertIntEquals(tc, lexer_getc(&lex, &token), ec_noerr);
	CuAssertStrEquals(tc, token, "int");
	lexer_consume(&lex);

	CuAssertIntEquals(tc, lexer_getc(&lex, &token), ec_noerr);
	CuAssertStrEquals(tc, token, "a");
	lexer_consume(&lex);

	CuAssertIntEquals(tc, lexer_getc(&lex, &token), ec_noerr);
	CuAssertStrEquals(tc, token, "=");
	lexer_consume(&lex);

	CuAssertIntEquals(tc, lexer_getc(&lex, &token), ec_noerr);
	CuAssertStrEquals(tc, token, "0");
	lexer_consume(&lex);

	CuAssertIntEquals(tc, lexer_getc(&lex, &token), ec_noerr);
	CuAssertStrEquals(tc, token, ";");
	lexer_consume(&lex);

	CuAssertIntEquals(tc, lexer_getc(&lex, &token), ec_noerr);
	CuAssertStrEquals(tc, token, "float");
	lexer_consume(&lex);

	CuAssertIntEquals(tc, lexer_getc(&lex, &token), ec_noerr);
	CuAssertStrEquals(tc, token, "b");
	lexer_consume(&lex);

	CuAssertIntEquals(tc, lexer_getc(&lex, &token), ec_noerr);
	CuAssertStrEquals(tc, token, "=");
	lexer_consume(&lex);

	CuAssertIntEquals(tc, lexer_getc(&lex, &token), ec_noerr);
	CuAssertStrEquals(tc, token, "0");
	lexer_consume(&lex);

	CuAssertIntEquals(tc, lexer_getc(&lex, &token), ec_noerr);
	CuAssertStrEquals(tc, token, ";");
	lexer_consume(&lex);

	CuAssertIntEquals(tc, lexer_getc(&lex, &token), ec_noerr);
	CuAssertStrEquals(tc, token, "");

	lexer_destruct(&lex);
}

CuSuite* LexerGetSuite(void) {
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, ReadToken);
	SUITE_ADD_TEST(suite, SkipComments);
	return suite;
}
