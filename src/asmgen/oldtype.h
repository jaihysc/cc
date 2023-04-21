/* Old type header to get the assembly generator to work */
#ifndef OLDTYPE_H
#define OLDTYPE_H

/* ============================================================ */
/* Common data types */

/* long needs to map to its own type to correctly calculate implicit
   conversions, hence the i32_ and u32_
   Same for long double and f64_ */
#define TYPE_SPECIFIERS  \
    TYPE_SPECIFIER(void) \
    TYPE_SPECIFIER(i8)   \
    TYPE_SPECIFIER(i16)  \
    TYPE_SPECIFIER(i32)  \
    TYPE_SPECIFIER(i32_) \
    TYPE_SPECIFIER(i64)  \
    TYPE_SPECIFIER(u8)   \
    TYPE_SPECIFIER(u16)  \
    TYPE_SPECIFIER(u32)  \
    TYPE_SPECIFIER(u32_) \
    TYPE_SPECIFIER(u64)  \
    TYPE_SPECIFIER(f32)  \
    TYPE_SPECIFIER(f64)  \
    TYPE_SPECIFIER(f64_)
#define TYPE_SPECIFIER(name__) ts_ ## name__,
/* ts_none for indicating error */
typedef enum {ts_none = -1, TYPE_SPECIFIERS ts_count} TypeSpecifiers;
#undef TYPE_SPECIFIER
#define TYPE_SPECIFIER(name__) #name__,
char* ts_str[] = {TYPE_SPECIFIERS};
#undef TYPE_SPECIFIER

/* Converts a type specifier to string */
static inline const char* type_specifiers_str(TypeSpecifiers typespec) {
    return ts_str[typespec];
}

typedef struct {
    /* 0 = Standard types, e.g., int, float, long long*
       1 = Function */
    int category;

    TypeSpecifiers typespec;
    int pointers;

    /* Arrays */
    int dimension; /* Dimension of array, e.g., [123][321] has dimension 2 */
    int size[1]; /* Number of elements for each dimension */
    /* FIXME assume dimension 1 arrays for now to get something implemented */
} Type;

/* Returned by relational, equality, logical and, logical or as required by
   6.5.8.6
   6.5.9.3
   6.5.13.3
   6.5.14.3 */
Type type_int = {.typespec = ts_i32, .pointers = 0};
/* Type for labels */
Type type_label = {.typespec = ts_void, .pointers = 0};
/* Type for offsetting pointers */
Type type_ptroffset = {.typespec = ts_i64, .pointers = 0};
Type type_ptrdiff = {.typespec = ts_i64, .pointers = 0};

static inline int type_is_pointer(const Type*);

/* Constructs a type at given location */
static inline void type_construct(
        Type* type, TypeSpecifiers ts, int pointers) {
    type->category = 0;
    type->typespec = ts;
    type->pointers = pointers;
    type->dimension = 0;
    type->size[0] = 0;
}

/* Constructs a function type at the given location */
static inline void type_constructf(Type* type, const Type* ret_type) {
    *type = *ret_type;
    type->category = 1;
}

/* Returns type specifiers for Type */
static inline TypeSpecifiers type_typespec(const Type* type) {
    ASSERT(type != NULL, "Type is null");
    return type->typespec;
}

/* Sets type specifiers for Type */
static inline void type_set_typespec(Type* type, TypeSpecifiers typespec) {
    ASSERT(type != NULL, "Type is null");
    type->typespec = typespec;
}

/* Returns number of pointers for Type */
static inline int type_pointer(const Type* type) {
    ASSERT(type != NULL, "Type is null");
    return type->pointers;
}

/* Sets number of pointers for Type */
static inline void type_set_pointer(Type* type, int pointer) {
    ASSERT(type != NULL, "Type is null");
    type->pointers = pointer;
}

/* Increments the number of pointers for Type, returns the new number of
   pointers */
static inline int type_inc_pointer(Type* type) {
    ASSERT(type != NULL, "Type is null");
    ++type->pointers;
    return type->pointers;
}

/* Sets the current type to the type obtained from dereferencing the current
   type (thus decreasing one level of indirection) */
static inline void type_dec_indirection(Type* type) {
    ASSERT(type != NULL, "Type is null");
    if (type->dimension > 0) {
        --type->dimension;
    }
    else {
        ASSERT(type->pointers > 0, "No pointers to decrement");
        --type->pointers;
    }
}

/* Returns Type of what this pointer points to */
static inline Type type_point_to(const Type* type) {
    ASSERT(type != NULL, "Type is null");
    Type t = *type;
    type_dec_indirection(&t);
    return t;
}

/* Returns the dimension of this type */
static inline int type_dimension(const Type* type) {
    ASSERT(type != NULL, "Type is null");
    return type->dimension;
}

/* Adds a dimension to the type
   size: Number of elements the dimension can hold in total
         e.g., int[10][60] has count 600 */
static inline void type_add_dimension(Type* type, int size) {
    ASSERT(type != NULL, "Type is null");
    ASSERT(type->dimension == 0,
            "Only single dimension arrays supported for now");
    ++type->dimension;
    type->size[0] = size;
}

/* Returns the number of elements held in the dimension at index i */
static inline int type_dimension_size(const Type* type, int i) {
    ASSERT(type != NULL, "Type is null");
    return type->size[i];
}


/* Function types */


/* Returns 1 if given type is a function, 0 if not */
static inline int type_is_function(const Type* type) {
    return type->category == 1;
}

/* Returns the return type of given function type */
static inline Type type_return(const Type* type) {
    return *type;
}

/* Converts a string into a type */
static inline Type type_from_str(const char* str) {
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
        if (strequ(buf, ts_str[j])) {
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

/* Number of bytes this type takes up
   For arrays, the element count are part of the type's size */
static inline int type_bytes(Type type) {
    ASSERT(type.typespec != ts_none, "Invalid type specifiers");

    if (type.pointers > 0) {
        return 8;
    }
    int bytes = 0;
    switch (type.typespec) {
        case ts_void:
            bytes = 0;
            break;
        case ts_i8:
            bytes = 1;
            break;
        case ts_i16:
            bytes = 2;
            break;
        case ts_i32:
            bytes = 4;
            break;
        case ts_i32_:
            bytes = 4;
            break;
        case ts_i64:
            bytes = 8;
            break;
        case ts_u8:
            bytes = 1;
            break;
        case ts_u16:
            bytes = 2;
            break;
        case ts_u32:
            bytes = 4;
            break;
        case ts_u32_:
            bytes = 4;
            break;
        case ts_u64:
            bytes = 8;
            break;
        case ts_f32:
            bytes = 4;
            break;
        case ts_f64:
            bytes = 8;
            break;
        case ts_f64_:
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

/* Return 1 if both types are equal, 0 if not */
static inline int type_equal(Type lhs, Type rhs) {
    if (lhs.typespec != rhs.typespec) {
        return 0;
    }
    if (lhs.pointers != rhs.pointers) {
        return 0;
    }
    return 1;
}

/* Returns the integer conversion rank for a given integer type */
static inline int type_rank(TypeSpecifiers typespec) {
    switch (typespec) {
        case ts_i8:
        case ts_u8:
            return 1;
        case ts_i16:
        case ts_u16:
            return 2;
        case ts_i32:
        case ts_u32:
            return 3;
        case ts_i32_:
        case ts_u32_:
            return 4;
        case ts_i64:
        case ts_u64:
            return 5;
        default:
            ASSERT(0, "Bad type specifier");
            return 0;
    }
}

/* Returns 1 if provided type is an array type, 0 if not */
static inline int type_array(const Type* type) {
    ASSERT(type != NULL, "Type is null");
    return type->dimension > 0;
}

/* Returns the type of the array / pointer 's element */
static inline Type type_element(const Type* type) {
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

/* Converts an array of type to pointer to type
   e.g., int[] -> int*, int[2][10] -> int (*)[10] */
static inline Type type_array_as_pointer(const Type* type) {
    ASSERT(type != NULL, "Type is null");
    ASSERT(type->dimension, "Expected dimension > 0");
    Type t = *type;
    --t.dimension;
    ++t.pointers;
    return t;
}

/* Returns 1 if given type is a pointer type */
static inline int type_is_pointer(const Type* type) {
    ASSERT(type != NULL, "Type is null");
    return type_pointer(type) > 0;
}

/* Returns 1 if given type is an arithmetic type */
static inline int type_is_arithmetic(const Type* type) {
    ASSERT(type != NULL, "Type is null");
    if (type_is_pointer(type) || type_array(type)) {
        return 0;
    }
    return 1;
}

/* Returns 1 if the provided type is an integer type, 0 if not */
static inline int type_integral(Type type) {
    if (type_pointer(&type) != 0) {
        return 0;
    }
    switch (type_typespec(&type)) {
        case ts_i8:
        case ts_u8:
        case ts_i16:
        case ts_u16:
        case ts_i32:
        case ts_u32:
        case ts_i32_:
        case ts_u32_:
        case ts_i64:
        case ts_u64:
            return 1;
        default:
            return 0;
    }
}

/* Returns 1 if the provided type is signed, 0 if not */
static inline int type_signed(TypeSpecifiers typespec) {
    switch (typespec) {
        case ts_void:
        case ts_f32:
        case ts_f64:
        case ts_f64_:
        case ts_u8:
        case ts_u16:
        case ts_u32:
        case ts_u32_:
        case ts_u64:
            return 0;
        case ts_i8:
        case ts_i16:
        case ts_i32:
        case ts_i32_:
        case ts_i64:
            return 1;
        default:
            ASSERT(0, "Bad type specifier");
            return 0;
    }
}

/* Returns 1 if the provided type is unsigned, 0 if not */
static inline int type_unsigned(TypeSpecifiers typespec) {
    switch (typespec) {
        case ts_void:
        case ts_f32:
        case ts_f64:
        case ts_f64_:
        case ts_i8:
        case ts_i16:
        case ts_i32:
        case ts_i32_:
        case ts_i64:
            return 0;
        case ts_u8:
        case ts_u16:
        case ts_u32:
        case ts_u32_:
        case ts_u64:
            return 1;
        default:
            ASSERT(0, "Bad type specifier");
            return 0;
    }
}

/* Returns 1 if the given signed type can represent to given unsigned type */
static inline int type_signed_represent_unsigned(
        TypeSpecifiers sign, TypeSpecifiers unsign) {
    switch (sign) {
        case ts_i8:
            return 0;
        case ts_i16:
            switch (unsign) {
                case ts_u8:
                    return 1;
                case ts_u16:
                case ts_u32:
                case ts_u32_:
                case ts_u64:
                    return 0;
                default:
                    ASSERT(0, "Bad type specifier");
                    return 0;
            }
        case ts_i32:
        case ts_i32_:
            switch (unsign) {
                case ts_u8:
                case ts_u16:
                    return 1;
                case ts_u32:
                case ts_u32_:
                case ts_u64:
                    return 0;
                default:
                    ASSERT(0, "Bad type specifier");
                    return 0;
            }
        case ts_i64:
            switch (unsign) {
                case ts_u8:
                case ts_u16:
                case ts_u32:
                case ts_u32_:
                    return 1;
                case ts_u64:
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

/* Returns the unsigned type for the given signed type */
static inline TypeSpecifiers type_unsign_signed(TypeSpecifiers typespec) {
    switch (typespec) {
        case ts_i8:
            return ts_u8;
        case ts_i16:
            return ts_u16;
        case ts_i32:
            return ts_u32;
        case ts_i32_:
            return ts_u32_;
        case ts_i64:
            return ts_u64;
        default:
            ASSERT(0, "Bad type specifier");
            return 0;
    }
}

/* Helper for type_common to calculate the common TypeSpecifiers */
static inline TypeSpecifiers type_common_ts(
        TypeSpecifiers lhs, TypeSpecifiers rhs) {
    if (lhs == ts_f64_ || rhs == ts_f64_) {
        return ts_f64;
    }
    if (lhs == ts_f64 || rhs == ts_f64) {
        return ts_f64;
    }
    if (lhs == ts_f32 || rhs == ts_f32) {
        return ts_f32;
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

/* Applies the C 6.3.1.8 Usual arithmetic conversions to obtain a common real
   type for the operands and result */
static inline Type type_common(Type type1, Type type2) {
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

/* Applies the C 6.3.1.1 integer promotions on the provided type
   if applicable and returns the new type, or the old type if no promotion  */
static inline Type type_promotion(Type type) {
    if (!type_integral(type)) {
        return type;
    }

    switch (type_typespec(&type)) {
        case ts_i8:
        case ts_i16:
        case ts_u8:
        case ts_u16:
            type_set_typespec(&type, ts_i32);
            break;
        default:
            break;
    }
    return type;
}

#endif
