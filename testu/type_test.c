#include "CuTest.h"

#include "type.h"

void TypeConstruct(CuTest* tc) {
    Type type;
    type_construct(&type, ts_uint, 1);

    CuAssertTrue(tc, type_typespec(&type) == ts_uint);
    CuAssertTrue(tc, type_pointer(&type) == 1);
}

CuSuite* TypeGetSuite() {
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, TypeConstruct);
    return suite;
}
