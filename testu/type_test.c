#include "CuTest.h"

#include "type.h"

void TypeConstruct(CuTest* tc) {
    Type type;
    type_construct(&type, ts_uint, 1);

    CuAssertTrue(tc, type_typespec(&type) == ts_uint);
    CuAssertTrue(tc, type_pointer(&type) == 1);
}

void TypeSpecifierFromString(CuTest* tc) {
    CuAssertTrue(tc, ts_from_str("void") == ts_void);

    CuAssertTrue(tc, ts_from_str("char") == ts_char);

    CuAssertTrue(tc, ts_from_str("signed char") == ts_schar);

    CuAssertTrue(tc, ts_from_str("unsigned char") == ts_uchar);

    CuAssertTrue(tc, ts_from_str("short") == ts_short);
    CuAssertTrue(tc, ts_from_str("short int") == ts_short);
    CuAssertTrue(tc, ts_from_str("signed short") == ts_short);
    CuAssertTrue(tc, ts_from_str("signed short int") == ts_short);

    CuAssertTrue(tc, ts_from_str("unsigned short") == ts_ushort);
    CuAssertTrue(tc, ts_from_str("unsigned short int") == ts_ushort);

    CuAssertTrue(tc, ts_from_str("int") == ts_int);
    CuAssertTrue(tc, ts_from_str("signed") == ts_int);
    CuAssertTrue(tc, ts_from_str("signed int") == ts_int);

    CuAssertTrue(tc, ts_from_str("unsigned") == ts_uint);
    CuAssertTrue(tc, ts_from_str("unsigned int") == ts_uint);

    CuAssertTrue(tc, ts_from_str("long") == ts_long);
    CuAssertTrue(tc, ts_from_str("long int") == ts_long);
    CuAssertTrue(tc, ts_from_str("signed long") == ts_long);
    CuAssertTrue(tc, ts_from_str("signed long int") == ts_long);

    CuAssertTrue(tc, ts_from_str("unsigned long") == ts_ulong);
    CuAssertTrue(tc, ts_from_str("unsigned long int") == ts_ulong);

    CuAssertTrue(tc, ts_from_str("long long") == ts_longlong);
    CuAssertTrue(tc, ts_from_str("long long int") == ts_longlong);
    CuAssertTrue(tc, ts_from_str("signed long long") == ts_longlong);
    CuAssertTrue(tc, ts_from_str("signed long long int") == ts_longlong);

    CuAssertTrue(tc, ts_from_str("unsigned long long") == ts_ulonglong);
    CuAssertTrue(tc, ts_from_str("unsigned long long int") == ts_ulonglong);

    CuAssertTrue(tc, ts_from_str("float") == ts_float);
    CuAssertTrue(tc, ts_from_str("double") == ts_double);
    CuAssertTrue(tc, ts_from_str("long double") == ts_ldouble);

    /* Invalid types */
    CuAssertTrue(tc, ts_from_str("voiD") == ts_none);
    CuAssertTrue(tc, ts_from_str("cha") == ts_none);
    CuAssertTrue(tc, ts_from_str("shark") == ts_none);
    CuAssertTrue(tc, ts_from_str("in t") == ts_none);
    CuAssertTrue(tc, ts_from_str("INT") == ts_none);
    CuAssertTrue(tc, ts_from_str("long double long") == ts_none);
    CuAssertTrue(tc, ts_from_str("long long double") == ts_none);
}

CuSuite* TypeGetSuite() {
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, TypeConstruct);
    SUITE_ADD_TEST(suite, TypeSpecifierFromString);
    return suite;
}
