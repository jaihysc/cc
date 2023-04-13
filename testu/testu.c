#include <stdio.h>

#include "CuTest.h"

CuSuite* LexerGetSuite(void);
CuSuite* SymbolGetSuite(void);
CuSuite* SymtabGetSuite(void);
CuSuite* TreeGetSuite(void);
CuSuite* TypeGetSuite(void);

int RunAllTests(void) {
	CuString* output = CuStringNew();
	CuSuite* suite = CuSuiteNew();

	CuSuiteAddSuite(suite, LexerGetSuite());
	CuSuiteAddSuite(suite, SymbolGetSuite());
	CuSuiteAddSuite(suite, SymtabGetSuite());
	CuSuiteAddSuite(suite, TreeGetSuite());
	CuSuiteAddSuite(suite, TypeGetSuite());

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	printf("%s\n", output->buffer);
	return suite->failCount;
}

int main(void) {
	return RunAllTests();
}
