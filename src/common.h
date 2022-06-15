/*
 Common utilities across c files
*/
#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include <stdint.h>

#define LOG(msg__) printf("%s", msg__);
#define LOGF(...) printf(__VA_ARGS__);
/* For error/diagnostic messages */
#define ERRMSG(msg__) printf("\033[1;31m%s\033[0m", msg__);
#define ERRMSGF(fmt__, ...) printf("\033[1;31m" fmt__ "\033[0m", __VA_ARGS__);

#define ASSERT(expr__, msg__)                           \
    if (!(expr__)) {                                    \
        ERRMSG("ICE: " msg__);                          \
        ERRMSGF("  Line: %d %s\n", __LINE__, __FILE__); \
        exit(1);                                        \
    } do {} while (0)
#define ASSERTF(expr__, fmt__, ...)                     \
    if (!(expr__)) {                                    \
        ERRMSGF("ICE: " fmt__, __VA_ARGS__);            \
        ERRMSGF("  Line: %d %s\n", __LINE__, __FILE__); \
        exit(1);                                        \
    } do {} while (0)

#define TOKEN_COLOR "\033[1;97m" /* Color for tokens when printed */

#define ARRAY_SIZE(array__) (int)(sizeof(array__) / sizeof(array__[0]))

/* Checks that the given strings is sorted correctly to
   perform a binary search via strbinfind */
static inline void strbinfind_validate(const char** strs, int str_count) {
    /* Compare the string at i with string at i+1, n-1 times */
    for (int i = 0; i < str_count - 1; ++i) {
        const char* str1 = strs[i];
        const char* str2 = strs[i+1];

        int j = 0; /* String index */
        while (1) {
            char c1 = str1[j];
            char c2 = str2[j];
            ASSERTF(c1 <= c2,
                    "Incorrectly sorted at index %d, %s %s", j, str1, str2);
            if (c1 != '\0' && c2 == '\0') {
                ASSERTF(0, "String 2 ended before string 1, %s %s", str1, str2);
            }

            if (c1 == '\0') {
                break;
            }
            if (c1 < c2) {
                break;
            }
            ++j;
        }
    }
}

/* Performs binary search for string */
/* str:       c string to look for, does not need to be null terminated*/
/* match_amt: Number of characters in str to match */
/* strs:      pointer to c strings to look through */
/* str_count: number of c strings in strs */
/* Returns index where string was found, otherwise -1 */
/* strs string sorting requirements:
     Order by numeric value of first letter, smallest first. For those with
     the same valued first letter, order by numeric value of second letter,
     smallest first. If same letter, repeat for third letter, continue until
     all strings sorted.
     If a longer string has a prefix which matches a shorter string:
     the longer string comes after the shorter string */
static inline int strbinfind(const char* str, int match_amt, const char** strs, int str_count) {
    strbinfind_validate(strs, str_count);

    int front = 0; /* Inclusive */
    int rear = str_count; /* Exclusive */
    while (front < rear) {
        int i = (front + rear) / 2; /* Index for string */
        const char* found = strs[i];
        int match = 1;
        /* j is index of char in found string */
        for (int j = 0; j < match_amt; ++j) {
            if (found[j] == '\0') {
                front = i + 1;
                match = 0;
                break;
            }
            if (str[j] < found[j]) {
                rear = i;
                match = 0;
                break;
            }
            else if (str[j] > found[j]) {
                front = i + 1;
                match = 0;
                break;
            }
        }
        /* Ensure found string is same length (not just the prefix which matches) */
        if (match) {
            if (found[match_amt] != '\0') {
                match = 0;
                rear = i;
            }
            else {
                return i;
            }
        }
    }
    return -1;
}

/* Returns length of null terminated c string */
static inline int strlength(const char* str) {
    int i = 0;
    char c;
    while ((c = str[i]) != '\0') {
        ++i;
    }
    return i;
}

/* Return 1 if strings are equal, 0 if not */
static inline int strequ(const char* s1, const char* s2) {
    int i = 0;
    char c;
    while ((c = s1[i]) != '\0') {
        if (s1[i] != s2[i]) {
            return 0;
        }
        ++i;
    }
    /* Ensure both strings terminated */
    if (s2[i] == '\0') {
        return 1;
    }
    else {
        return 0;
    }
}

/* Copies string into target, assumes sufficent space exists */
static inline void strcopy(const char* str, char* target) {
    int i = 0;
    char c;
    while ((c = str[i]) != '\0') {
        target[i] = c;
        ++i;
    }
    target[i] = '\0';
}

/* Number of chars to represent integer */
static inline int ichar(int i) {
    /* Always have 1 digit */
    int digits = 1;
    if (i < 0) {
        /* For minus sign */
        ++digits;
    }
    i /= 10;
    while (i != 0) {
        i /= 10;
        ++digits;
    }
    return digits;
}

/* Converts integer to str representation written in buf
   Assumes buf has sufficient space */
static inline void itostr(int i, char* buf) {
    int len = ichar(i);
    if (i < 0) {
        buf[0] = '-';
        i = -i;
    }
    /* Take off digit at a time from end of number (right) */
    buf += len - 1;
    *buf = (char)(i % 10) + '0';
    i /= 10;
    while (i != 0) {
        --buf;
        *buf = (char)(i % 10) + '0';
        i /= 10;
    }
}

/* Converts str representation of integer to integer */
static inline int strtoi(const char* str) {
    int val = 0;
    int sign = 1;

    int i = 0;
    char c;

    if (str[0] == '-') {
        sign = -1;
        i = 1;
    }
    while ((c = str[i]) != '\0') {
        ASSERTF('0' <= c && c <= '9', "Invalid char %c", c);
        val *= 10;
        val += c - '0';
        ++i;
    }
    return val * sign;
}

/* Converts str representation of integer with given length to integer */
static inline int strtoi2(const char* str, int len) {
    int val = 0;
    int sign = 1;

    int i = 0;
    char c;

    ASSERT(len > 0, "No digits");
    if (str[0] == '-') {
        sign = -1;
        i = 1;
    }
    for (; i < len; ++i) {
        c = str[i];
        ASSERTF('0' <= c && c <= '9', "Invalid char %c", c);
        val *= 10;
        val += c - '0';
    }
    return val * sign;
}

/* Creates a c string on the stack with name,
   has given prefix string with number appended */
#define AAPPENDI(name__, prefix__, num__)                \
    int numlen__ = ichar(num__);                         \
    char name__[(int)sizeof(prefix__) + numlen__];       \
    strcopy(prefix__, name__);                           \
    itostr(num__, name__ + (int)sizeof(prefix__) - 1);   \
    name__[(int)sizeof(prefix__) - 1 + numlen__] = '\0'

/* Raises integer base to positive integer exponent */
static inline int64_t powip(int base, unsigned exponent) {
    int64_t result = 1;
    for (unsigned i = 0; i < exponent; ++i) {
        result *= base;
    }
    return result;
}

/* Quicksort implementation */
static inline void quicksort(
        void* ptr, size_t count, size_t size,
        int (*comp)(const void*, const void*)) {
    /* Write your own quicksort implementation when this
       compiler has to self compile */
    qsort(ptr, count, size, comp);
}

/* ============================================================ */
/* Dynamic array, similar to the one in C++,
   adapted from https://github.com/rxi/vec */

/* Creates a vector containing values of type T */
#define vec_t(T__) \
    struct { T__* data; int length; int capacity; }

/* Initializes the vector, must be called before vector is used */
#define vec_construct(v__) \
    ((v__)->data = 0, (v__)->length = 0, (v__)->capacity = 0)

/* Frees memory allocated by vector, call when finished using */
#define vec_destruct(v__) \
    free((v__)->data)

/* Access specified element withOUT bounds checking */
#define vec_at(v__, idx__) \
    (v__)->data[(idx__)]

/* Returns the first value in the vector, do not use on empty vector */
#define vec_front(v__) \
    (v__)->data[0]

/* Returns the last value in the vector, do not use on empty vector */
#define vec_back(v__) \
    (v__)->data[(v__)->length - 1]

/* Returns direct access to the underlying array */
#define vec_data(v__) \
    (v__)->data

/* Returns number of elements in vector */
#define vec_size(v__) \
    (v__)->length

/* Returns 1 if vec is empty, 0 if not */
#define vec_empty(v__) \
    (v__)->length == 0

/* Reserses capacity for at least n elements in vector
   Pointers to elements are invalidated if resize occurs
   Returns 1 if successful, 0 if error (vector remains unchanged) */
#define vec_reserve(v__, n__) \
    vec_reserve_(vec_unpack_(v__), n__)

/* Clears all values from the vector, new length is 0 */
#define vec_clear(v__) \
    ((v__)->length = 0)

/* Pushes uninitialized value to end of vector
   Pointers to elements are invalidated if resize occurs
   1 if successful, 0 if error and vector remains unchanged */
#define vec_push_backu(v__) \
    (vec_expand_(vec_unpack_(v__)) ? ((v__)->length++, 1) : 0)

/* Pushes a value to the end of the vector
   Pointers to elements are invalidated if resize occurs
   1 if successful, 0 if error and vector remains unchanged */
#define vec_push_back(v__, val__)                 \
    (vec_expand_(vec_unpack_(v__)) ?              \
    ((v__)->data[(v__)->length++] = (val__), 1) : \
    0)

/* Removes and returns the value at the end of the vector */
#define vec_pop_back(v__) \
    (v__)->data[--(v__)->length]

/* Removes count values starting at index start */
#define vec_splice(v__, start__, count__) \
    vec_splice_(vec_unpack_(v__), (start__), (count__))

/* Inserts val at index shifting the elements after the index to make room
   1 if successful, 0 if error and vector remains unchanged */
#define vec_insert(v__, val__, idx__)       \
    (vec_insert_(vec_unpack_(v__), idx__) ? \
    ((v__)->data[idx__] = (val__), (v__)->length++, 1) : 0)

#define vec_unpack_(v__)      \
    (char**)&(v__)->data,     \
            &(v__)->length,   \
            &(v__)->capacity, \
            sizeof(*(v__)->data)

static inline int vec_expand_(
        char** data, int* length, int* capacity, int memsz) {
    if (*length + 1 > *capacity) {
        void* ptr;
        int n = (*capacity == 0) ? 1 : *capacity * 2;
        ptr = realloc(*data, (size_t)(n * memsz));
        if (ptr == NULL) return 0;
        *data = ptr;
        *capacity = n;
    }
    return 1;
}

static inline int vec_reserve_(
        char** data, int* length, int* capacity, int memsz, int n) {
    (void) length;
    if (n > *capacity) {
        void* ptr = realloc(*data, (size_t)(n * memsz));
        if (ptr == NULL) return 0;
        *data = ptr;
        *capacity = n;
    }
    return 1;
}

static inline int vec_insert_(
        char** data, int* length, int* capacity, int memsz, int idx) {
    if (!vec_expand_(data, length, capacity, memsz)) {
        return 0;
    }
    /* |     |     |     | idx |     |     | length |
                        ^ end             ^ src    ^ ptr */
    char* ptr = *data + (*length + 1) * memsz - 1;
    char* src = *data + *length * memsz - 1;
    char* end = *data + idx * memsz - 1;

    while (src != end) {
        *ptr = *src;
        --src;
        --ptr;
    }
    return 1;
}

static inline void vec_splice_(
        char** data, int* length, int* capacity, int memsz,
        int start, int count) {
    (void) capacity;

    char* dest = *data + start * memsz;
    char* src = *data + (start + count) * memsz;
    size_t num_bytes =  (size_t)(*length - start - count) * (size_t)memsz;
    for (size_t i = 0; i < num_bytes; ++i) {
        dest[i] = src[i];
    }
    *length -= count;
}

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

/* Constructs a type at given location */
static inline void type_construct(
        Type* type, TypeSpecifiers ts, int pointers) {
    type->typespec = ts;
    type->pointers = pointers;
    type->dimension = 0;
    type->size[0] = 0;
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

/* Returns the type of the array's element */
static inline Type type_element(const Type* type) {
    ASSERT(type_array(type), "Not an array");
    Type t;
    type_construct(&t, type_typespec(type), 0);
    return t;
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
