#include "CuTest.h"

#include "type.h"

void TypeConstruct(CuTest* tc) {
	Type type;
	type_construct(&type, ts_uint, 1);

	CuAssertTrue(tc, type_typespec(&type) == ts_uint);
	CuAssertTrue(tc, type_pointer(&type) == 1);

	type_destruct(&type);
}

void TypeConstructFunction(CuTest* tc) {
	Type return_type;
	type_construct(&return_type, ts_uint, 2);

	Type type;
	type_constructf(&type, &return_type, 1);

	CuAssertTrue(tc, type_is_function(&type));
	CuAssertTrue(tc, type_pointer(&type) == 1);

	Type* got_return_type = type_return(&type);
	CuAssertTrue(tc, type_typespec(got_return_type) == ts_uint);
	CuAssertTrue(tc, type_pointer(got_return_type) == 2);

	type_destruct(&type);
	type_destruct(&return_type);
}

void TypeCopy(CuTest* tc) {
	Type type1;
	type_construct(&type1, ts_uint, 2);

	Type type2;
	type_constructf(&type2, &type1, 1);

	Type type1_copy;
	CuAssertTrue(tc, type_copy(&type1, &type1_copy) == ec_noerr);

	Type type2_copy;
	CuAssertTrue(tc, type_copy(&type2, &type2_copy) == ec_noerr);

	CuAssertTrue(tc, type_typespec(&type1_copy) == ts_uint);
	CuAssertTrue(tc, type_pointer(&type1_copy) == 2);

	CuAssertTrue(tc, type_typespec(type_return(&type2_copy)) == ts_uint);
	CuAssertTrue(tc, type_pointer(&type2_copy) == 1);

	type_destruct(&type2_copy);
	type_destruct(&type1_copy);
	type_destruct(&type2);
	type_destruct(&type1);
}

void TypeEqualPointers(CuTest* tc) {
	Type type1;
	type_construct(&type1, ts_uint, 2);

	Type type2;
	type_construct(&type2, ts_uint, 1);

	CuAssertFalse(tc, type_equal(&type1, &type2));

	type_destruct(&type2);
	type_destruct(&type1);
}

void TypeEqualFunction(CuTest* tc) {
	Type return_type;
	type_construct(&return_type, ts_uint, 2);

	Type type1;
	type_constructf(&type1, &return_type, 1);

	Type type2;
	type_constructf(&type2, &return_type, 1);

	CuAssertFalse(tc, type_equal(&return_type, &type2));
	CuAssertTrue(tc, type_equal(&type1, &type2));

	type_destruct(&type2);
	type_destruct(&type1);
	type_destruct(&return_type);
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

CuSuite* TypeGetSuite(void) {
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, TypeConstruct);
	SUITE_ADD_TEST(suite, TypeConstructFunction);
	SUITE_ADD_TEST(suite, TypeCopy);
	SUITE_ADD_TEST(suite, TypeEqualPointers);
	SUITE_ADD_TEST(suite, TypeEqualFunction);
	SUITE_ADD_TEST(suite, TypeSpecifierFromString);
	return suite;
}
