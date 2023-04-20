#include "type.h"

#include <stddef.h>

#include "common.h"

#define TYPE_SPECIFIER(name__) #name__,
const char* type_specifiers_str[] = {TYPE_SPECIFIERS};
#undef TYPE_SPECIFIER

Type type_int = {.typespec = ts_int, .pointers = 0};
Type type_label = {.typespec = ts_void, .pointers = 0};
Type type_ptroffset = {.typespec = ts_longlong, .pointers = 0};
Type type_ptrdiff = {.typespec = ts_longlong, .pointers = 0};

const char* ts_str(TypeSpecifiers typespec) {
    return type_specifiers_str[typespec];
}

TypeSpecifiers ts_from_str(const char* str) {
    /* Index of string in arrangement corresponds to type specifier at index */
    char* arrangement[] = {
        "void",
        "char",
        "signed char",
        "unsigned char",
        "short", "signed short", "short int", "signed short int",
        "unsigned short", "unsigned short int",
        "int", "signed", "signed int",
        "unsigned", "unsigned int",
        "long", "signed long", "long int", "signed long int",
        "unsigned long", "unsigned long int",
        "long long", "signed long long", "long long int", "signed long long int",
        "unsigned long long", "unsigned long long int",
        "float",
        "double",
        "long double",
    };
    int count = ARRAY_SIZE(arrangement);
    TypeSpecifiers typespec[] = {
        ts_void,
        ts_char,
        ts_schar,
        ts_uchar,
        ts_short, ts_short, ts_short, ts_short,
        ts_ushort, ts_ushort,
        ts_int, ts_int, ts_int,
        ts_uint, ts_uint,
        ts_long, ts_long, ts_long, ts_long,
        ts_ulong, ts_ulong,
        ts_longlong, ts_longlong, ts_longlong, ts_longlong,
        ts_ulonglong, ts_ulonglong,
        ts_float,
        ts_double,
        ts_ldouble,
    };

    for (int i = 0; i < count; ++i) {
        if (strequ(arrangement[i], str)) {
            return typespec[i];
        }
    }
    return ts_none;
}

void type_construct(
        Type* type, TypeSpecifiers ts, int pointers) {
    type->category = 0;
    type->typespec = ts;
    type->pointers = pointers;
    type->dimension = 0;
    type->size[0] = 0;
}

void type_constructf(Type* type, const Type* ret_type) {
    *type = *ret_type;
    type->category = 1;
}

TypeSpecifiers type_typespec(const Type* type) {
    ASSERT(type != NULL, "Type is null");
    return type->typespec;
}

void type_set_typespec(Type* type, TypeSpecifiers typespec) {
    ASSERT(type != NULL, "Type is null");
    type->typespec = typespec;
}

int type_pointer(const Type* type) {
    ASSERT(type != NULL, "Type is null");
    return type->pointers;
}

void type_set_pointer(Type* type, int pointer) {
    ASSERT(type != NULL, "Type is null");
    type->pointers = pointer;
}

int type_inc_pointer(Type* type) {
    ASSERT(type != NULL, "Type is null");
    ++type->pointers;
    return type->pointers;
}

void type_dec_indirection(Type* type) {
    ASSERT(type != NULL, "Type is null");
    if (type->dimension > 0) {
        --type->dimension;
    }
    else {
        ASSERT(type->pointers > 0, "No pointers to decrement");
        --type->pointers;
    }
}

Type type_point_to(const Type* type) {
    ASSERT(type != NULL, "Type is null");
    Type t = *type;
    type_dec_indirection(&t);
    return t;
}

int type_dimension(const Type* type) {
    ASSERT(type != NULL, "Type is null");
    return type->dimension;
}

void type_add_dimension(Type* type, int size) {
    ASSERT(type != NULL, "Type is null");
    ASSERT(type->dimension == 0,
            "Only single dimension arrays supported for now");
    ++type->dimension;
    type->size[0] = size;
}

int type_dimension_size(const Type* type, int i) {
    ASSERT(type != NULL, "Type is null");
    return type->size[i];
}

int type_is_function(const Type* type) {
    return type->category == 1;
}

Type type_return(const Type* type) {
    return *type;
}

Type type_from_str(const char* str) {
    Type type;
    type_construct(&type, ts_none, 0);

    /* type at most 4 char, + 1 null terminator */
    char buf[5];
    int i = 0;
    for (; i < 4; ++i) {
        char c = str[i];
        if (c == '\0' || c == '*' || c == '[') {
            break;
        }
        buf[i] = c;
    }
    /* Insert null terminator so can compare */
    buf[i] = '\0';
    for (int j = 0; j < ts_count; ++j) {
        if (strequ(buf, type_specifiers_str[j])) {
            type.typespec = j;
        }
    }
    /* Pointers */
    char c;
    while ((c = str[i]) == '*') {
        ++type.pointers;
        ++i;
    }
    /* Arrays */
    ASSERT(c == '[' || c == '\0', "Expected array or end of string");

    while (c == '[') {
        /* Start at the character after [
           e.g., [100]
                  ^ i_start */
        int digits = 0;
        ++i;
        int i_start = i;
        while (1) {
            if (str[i] == ']') {
                break;
            }
            ASSERT(str[i] != '\0', "Expected ]");
            ++i;
            ++digits;
        }
        int size = strtoi2(str + i_start, digits);
        type_add_dimension(&type, size);

        /* [100][200]
               ^ Currently here, advance to next dimension or end of string */
        ++i;
        c = str[i];
    }

    return type;
}

int type_bytes(Type type) {
    ASSERT(type.typespec != ts_none, "Invalid type specifiers");

    if (type.pointers > 0) {
        return 8;
    }
    int bytes = 0;
    switch (type.typespec) {
        case ts_void:
            bytes = 0;
            break;
        case ts_char:
        case ts_schar:
        case ts_uchar:
            bytes = 1;
            break;
        case ts_short:
        case ts_ushort:
            bytes = 2;
            break;
        case ts_int:
        case ts_uint:
            bytes = 4;
            break;
        case ts_long:
        case ts_ulong:
            bytes = 4;
            break;
        case ts_longlong:
        case ts_ulonglong:
            bytes = 8;
            break;
        case ts_float:
            bytes = 4;
            break;
        case ts_double:
        case ts_ldouble:
            bytes = 8;
            break;
        case ts_none:
        case ts_count:
        default:
            ASSERT(0, "Bad type specifier");
            bytes = 0;
            break;
    }
    if (type.dimension > 0) {
        ASSERT(type.dimension == 1,
            "Only single dimension arrays supported for now");
        bytes *= type.size[0];
    }
    return bytes;
}

int type_equal(Type lhs, Type rhs) {
    if (lhs.typespec != rhs.typespec) {
        return 0;
    }
    if (lhs.pointers != rhs.pointers) {
        return 0;
    }
    return 1;
}

int type_rank(TypeSpecifiers typespec) {
    switch (typespec) {
        case ts_char:
        case ts_schar:
        case ts_uchar:
            return 1;
        case ts_short:
        case ts_ushort:
            return 2;
        case ts_int:
        case ts_uint:
            return 3;
        case ts_long:
        case ts_ulong:
            return 4;
        case ts_longlong:
        case ts_ulonglong:
            return 5;
        default:
            ASSERT(0, "Bad type specifier");
            return 0;
    }
}

int type_array(const Type* type) {
    ASSERT(type != NULL, "Type is null");
    return type->dimension > 0;
}

Type type_element(const Type* type) {
    ASSERT(type_array(type) || type_is_pointer(type),
            "Not an array or pointer");
    if (type_array(type)) {
        Type t;
        type_construct(&t, type_typespec(type), 0);
        return t;
    }
    else {
        return type_point_to(type);
    }
}

Type type_array_as_pointer(const Type* type) {
    ASSERT(type != NULL, "Type is null");
    ASSERT(type->dimension, "Expected dimension > 0");
    Type t = *type;
    --t.dimension;
    ++t.pointers;
    return t;
}

int type_is_pointer(const Type* type) {
    ASSERT(type != NULL, "Type is null");
    return type_pointer(type) > 0;
}

int type_is_arithmetic(const Type* type) {
    ASSERT(type != NULL, "Type is null");
    if (type_is_pointer(type) || type_array(type)) {
        return 0;
    }
    return 1;
}

int type_integral(Type type) {
    if (type_pointer(&type) != 0) {
        return 0;
    }
    switch (type_typespec(&type)) {
        case ts_char:
        case ts_schar:
        case ts_uchar:
        case ts_short:
        case ts_ushort:
        case ts_int:
        case ts_uint:
        case ts_long:
        case ts_ulong:
        case ts_longlong:
        case ts_ulonglong:
            return 1;
        default:
            return 0;
    }
}

int type_signed(TypeSpecifiers typespec) {
    switch (typespec) {
        case ts_void:
        case ts_float:
        case ts_double:
        case ts_ldouble:
        case ts_uchar:
        case ts_ushort:
        case ts_uint:
        case ts_ulong:
        case ts_ulonglong:
            return 0;
        case ts_char:
        case ts_schar:
        case ts_short:
        case ts_int:
        case ts_long:
        case ts_longlong:
            return 1;
        default:
            ASSERT(0, "Bad type specifier");
            return 0;
    }
}

int type_unsigned(TypeSpecifiers typespec) {
    switch (typespec) {
        case ts_void:
        case ts_float:
        case ts_double:
        case ts_ldouble:
        case ts_char:
        case ts_schar:
        case ts_short:
        case ts_int:
        case ts_long:
        case ts_longlong:
            return 0;
        case ts_uchar:
        case ts_ushort:
        case ts_uint:
        case ts_ulong:
        case ts_ulonglong:
            return 1;
        default:
            ASSERT(0, "Bad type specifier");
            return 0;
    }
}

int type_signed_represent_unsigned(
        TypeSpecifiers sign, TypeSpecifiers unsign) {
    switch (sign) {
        case ts_char:
        case ts_schar:
            return 0;
        case ts_short:
            switch (unsign) {
                case ts_uchar:
                    return 1;
                case ts_ushort:
                case ts_uint:
                case ts_ulong:
                case ts_ulonglong:
                    return 0;
                default:
                    ASSERT(0, "Bad type specifier");
                    return 0;
            }
        case ts_int:
        case ts_long:
            switch (unsign) {
                case ts_uchar:
                case ts_ushort:
                    return 1;
                case ts_uint:
                case ts_ulong:
                case ts_ulonglong:
                    return 0;
                default:
                    ASSERT(0, "Bad type specifier");
                    return 0;
            }
        case ts_longlong:
            switch (unsign) {
                case ts_uchar:
                case ts_ushort:
                case ts_uint:
                case ts_ulong:
                    return 1;
                case ts_ulonglong:
                    return 0;
                default:
                    ASSERT(0, "Bad type specifier");
                    return 0;
            }
        default:
            ASSERT(0, "Bad type specifier");
            return 0;
    }
}

TypeSpecifiers type_unsign_signed(TypeSpecifiers typespec) {
    switch (typespec) {
        case ts_char:
        case ts_schar:
            return ts_uchar;
        case ts_short:
            return ts_ushort;
        case ts_int:
            return ts_uint;
        case ts_long:
            return ts_ulong;
        case ts_longlong:
            return ts_ulonglong;
        default:
            ASSERT(0, "Bad type specifier");
            return 0;
    }
}

TypeSpecifiers type_common_ts(
        TypeSpecifiers lhs, TypeSpecifiers rhs) {
    if (lhs == ts_ldouble || rhs == ts_ldouble) {
        return ts_ldouble;
    }
    if (lhs == ts_double || rhs == ts_double) {
        return ts_double;
    }
    if (lhs == ts_float || rhs == ts_float) {
        return ts_float;
    }
    if (lhs == rhs) {
        return lhs;
    }
    if (type_signed(lhs)) {
        if (type_signed(rhs)) {
            goto case1;
        }
        else {
            ASSERT(type_unsigned(rhs), "Expected rhs unsigned");
            if (type_rank(rhs) >= type_rank(lhs)) {
                return rhs;
            }
            if (type_signed_represent_unsigned(lhs, rhs)) {
                return lhs;
            }
            return type_unsign_signed(lhs);
        }
    }
    else if (type_unsigned(lhs)) {
        if (type_signed(rhs)) {
            if (type_rank(lhs) >= type_rank(rhs)) {
                return lhs;
            }
            if (type_signed_represent_unsigned(rhs, lhs)) {
                return rhs;
            }
            return type_unsign_signed(rhs);
        }
        else {
            ASSERT(type_unsigned(rhs), "Expected rhs unsigned");
            goto case1;
        }
    }

case1: /* Both have signed or unsigned types */
    if (type_rank(lhs) < type_rank(rhs)) {
        return rhs;
    }
    return lhs;
}

Type type_common(Type type1, Type type2) {
    ASSERT(type_pointer(&type1) == 0,
            "Cannot determine common type for pointers");
    ASSERT(type_pointer(&type2) == 0,
            "Cannot determine common type for pointers");

    TypeSpecifiers lhs = type_typespec(&type1);
    TypeSpecifiers rhs = type_typespec(&type2);
    /* Can choose type1 or type2 to create the common type */
    type_set_typespec(&type1, type_common_ts(lhs, rhs));
    return type1;
}

Type type_promotion(Type type) {
    if (!type_integral(type)) {
        return type;
    }

    switch (type_typespec(&type)) {
        case ts_char:
        case ts_schar:
        case ts_uchar:
        case ts_short:
        case ts_ushort:
            type_set_typespec(&type, ts_int);
            break;
        default:
            break;
    }
    return type;
}

