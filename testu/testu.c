#include <stdio.h>

#include "CuTest.h"

CuSuite* CfgGetSuite(void);
CuSuite* LexerGetSuite(void);
CuSuite* ParserGetSuite(void);
CuSuite* SymbolGetSuite(void);
CuSuite* SymtabGetSuite(void);
CuSuite* TreeGetSuite(void);
CuSuite* TypeGetSuite(void);

int RunAllTests(void) {
	CuString* output = CuStringNew();
	CuSuite* suite = CuSuiteNew();

	CuSuiteAddSuite(suite, CfgGetSuite());
	CuSuiteAddSuite(suite, LexerGetSuite());
	CuSuiteAddSuite(suite, ParserGetSuite());
	CuSuiteAddSuite(suite, SymbolGetSuite());
	CuSuiteAddSuite(suite, SymtabGetSuite());
	CuSuiteAddSuite(suite, TreeGetSuite());
	CuSuiteAddSuite(suite, TypeGetSuite());

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	printf("%s\n", output->buffer);
	int failCount = suite->failCount;

	CuSuiteDelete(suite);
	CuStringDelete(output);

	return failCount;
}

int main(void) {
	return RunAllTests();
}
