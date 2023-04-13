/*
 Common utilities across c files
*/
#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

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

/* ============================================================ */
/* Utility */

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
    buf[len] = '\0';
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

/* Creates a c sring on the stack with name__
   with str1__ and str2__ appended */
#define AAPPENDA(name__, str1__, str2__)      \
    int str1len__ = strlength(str1__);        \
    int str2len__ = strlength(str2__);        \
    int buflen__ = str1len__ + str2len__ + 1; \
    char buf[buflen__];                       \
    strcopy(str1__, buf);                     \
    strcopy(str2__, buf + str1len__)

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
/* Memory */

/* The c prefix stands for Compiler */

/* Allocates given bytes of uninitialized storage, returns NULL if error */
static inline void* cmalloc(size_t bytes) {
    return malloc(bytes);
}

/* Deallocates space allocated by cmalloc */
static inline void cfree(void* ptr) {
    free(ptr);
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
    (char*)&(v__)->data,      \
            &(v__)->length,   \
            &(v__)->capacity, \
            sizeof(*(v__)->data)

/* Because of strict aliasing rule, the T** (where T is the type of the vec's
   element) from vec_unpack_ is passed as a char*. The pointed to value of
   this char* is T*. The 2 macros below are provided to read the T* and write
   to the T* */

/* Loads T* from src__ (char*) into dest__ (void*) */
#define LOAD(dest__, src__)                               \
    for (unsigned i__ = 0; i__ < sizeof(dest__); ++i__) { \
        ((char*)&dest__)[i__] = src__[i__];               \
    }
/* Save T* from src__ (void*) into dest (char*) */
#define SAVE(dest__, src__)                              \
    for (unsigned i__ = 0; i__ < sizeof(src__); ++i__) { \
        dest__[i__] = ((char*)&src__)[i__];              \
    }

static inline int vec_expand_(
        char* pdata, int* length, int* capacity, int memsz) {
    char* data;
    LOAD(data, pdata);

    if (*length + 1 > *capacity) {
        void* ptr;
        int n = (*capacity == 0) ? 1 : *capacity * 2;
        ptr = realloc(data, (size_t)(n * memsz));
        if (ptr == NULL) return 0;
        SAVE(pdata, ptr);
        *capacity = n;
    }
    return 1;
}

static inline int vec_reserve_(
        char* pdata, int* length, int* capacity, int memsz, int n) {
    (void) length;
    char* data;
    LOAD(data, pdata);
    if (n > *capacity) {
        void* ptr = realloc(data, (size_t)(n * memsz));
        if (ptr == NULL) return 0;
        SAVE(pdata, ptr);
        *capacity = n;
    }
    return 1;
}

static inline int vec_insert_(
        char* pdata, int* length, int* capacity, int memsz, int idx) {
    if (!vec_expand_(pdata, length, capacity, memsz)) {
        return 0;
    }
    char* data;
    LOAD(data, pdata);
    /* |     |     |     | idx |     |     | length |
                        ^ end             ^ src    ^ ptr */
    char* ptr = data + (*length + 1) * memsz - 1;
    char* src = data + *length * memsz - 1;
    char* end = data + idx * memsz - 1;

    while (src != end) {
        *ptr = *src;
        --src;
        --ptr;
    }
    return 1;
}

static inline void vec_splice_(
        char* pdata, int* length, int* capacity, int memsz,
        int start, int count) {
    (void) capacity;
    char* data;
    LOAD(data, pdata);

    char* dest = data + start * memsz;
    char* src = data + (start + count) * memsz;
    size_t num_bytes =  (size_t)(*length - start - count) * (size_t)memsz;
    for (size_t i = 0; i < num_bytes; ++i) {
        dest[i] = src[i];
    }
    *length -= count;
}

/* ============================================================ */
/* Stores T via a handle, allows resizing without invaliding pointers.
   Behaves the same as a vec to be a drop in replacement
   _ _
   - - Elements
   ^ ^
   | |
   __________
   ---------- Handles
   ^
   | Index

   On resize, this container allocates memory for all the elements, i.e.,
   memory for the handles and memory for the element itself. It only frees
   memory when it is destructed. As a result, insert moves the element 1 past
   the last to the index where the insertion will take place (as the last
   element will be overwritten). Splice moves the elements to be discarded to
   the end of the hvec (after the last element) to be used later. */

/* Creates a hvector containing values of type T
   The members have a h_ prefix to prevent accidently using vec_ */
#define hvec_t(T__) \
    struct { vec_t(T__*) h_vec; }

/* Initializes the hvector, must be called before hvector is used */
#define hvec_construct(v__) \
    vec_construct(&(v__)->h_vec)

/* Frees memory allocated by bvector, call when finished using */
#define hvec_destruct(v__) \
    hvec_destruct_(hvec_unpack_(v__))

/* Access specified element withOUT bounds checking */
#define hvec_at(v__, idx__) \
    *vec_at(&(v__)->h_vec, (idx__))

/* Returns the first value in the hvector, do not use on empty hvector */
#define hvec_front(v__) \
    *vec_front(&(v__)->h_vec)

/* Returns the last value in the hvector, do not use on empty hvector */
#define hvec_back(v__) \
    *vec_back(&(v__)->h_vec)

/* Returns number of elements in hvector */
#define hvec_size(v__) \
    vec_size(&(v__)->h_vec)

/* Returns 1 if hvector is empty, 0 if not */
#define hvec_empty(v__) \
    vec_empty((&(v__)->h_vec)

/* Reserves capacity for at least n elements in hvector
   Returns 1 if successful, 0 if error */
#define hvec_reserve(v__, n__) \
    hvec_reserve_(hvec_unpack_(v__), (n__), hvec_value_size_(v__))

/* Clears all values from the hvector, new length is 0 */
#define hvec_clear(v__) \
    vec_clear(&(v__)->h_vec)

/* Pushes uninitialized value to end of hvector
   Returns 1 if successful, 0 if error */
#define hvec_push_backu(v__)                                  \
    (hvec_expand_(hvec_unpack_(v__), hvec_value_size_(v__)) ? \
    ((v__)->h_vec.length++, 1) : 0)

/* Pushes a value to the end of the hvector
   Returns 1 if successful, 0 if error */
#define hvec_push_back(v__, val__)                            \
    (hvec_expand_(hvec_unpack_(v__), hvec_value_size_(v__)) ? \
    ((v__)->h_vec.length++, hvec_back(v__) = (val__), 1) : 0)

/* Removes and returns the value at the end of the vector */
#define hvec_pop_back(v__) \
    *vec_pop((v__)->h_vec)

/* Removes count values starting at index start */
#define hvec_splice(v__, start__, count__) \
    hvec_splice_(hvec_unpack_(v__), (start__), (count__))

/* Inserts val at index shifting the elements after the index to make room
   1 if successful, 0 if error */
#define hvec_insert(v__, val__, idx__)                                 \
    (hvec_insert_(hvec_unpack_(v__), hvec_value_size_(v__), (idx__)) ? \
    (hvec_at((v__), (idx__)) = (val__), (v__)->h_vec.length++, 1) : 0)

#define hvec_unpack_(v__) \
    (char*)&(v__)->h_vec

/* vec holds T__**, 1 pointer added by hvec, 1 by vec */
#define hvec_value_size_(v__) \
    sizeof(**(v__)->h_vec.data)

/* The type to treat the vec as when accessing it via a function */
typedef vec_t(void*) hvec_vec_;

static inline void hvec_destruct_(char* vec_) {
    hvec_vec_* vec = (hvec_vec_*)vec_;
    for (int i = 0; i < vec->capacity; ++i) {
        cfree(vec_at(vec, i));
    }
    vec_destruct(vec);
}

/* Allocates storage for an n elements
   Each vec element (T__*) up to capacity is a valid pointer to memory to store
   hvec elements
   Does not change length */
static inline int hvec_reserve_(char* vec_, int n, int elem_bytes) {
    hvec_vec_* vec = (hvec_vec_*)vec_;
    if (n > vec->capacity) {
        int old_capacity = vec->capacity;
        vec_reserve(vec, n);
        for (int i = old_capacity; i < vec->capacity; ++i) {
            void* elem = cmalloc((unsigned)elem_bytes);
            if (elem == NULL) return 0;
            vec->data[i] = elem;
        }
    }
    return 1;
}

/* Allocates storage if necessary so an additional element can be stored */
static inline int hvec_expand_(char* vec_, int elem_bytes) {
    hvec_vec_* vec = (hvec_vec_*)vec_;
    if (vec->length == vec->capacity) {
        /* + 1 to guarantee new space is always reserved */
        return hvec_reserve_(vec_, vec->capacity * 2 + 1, elem_bytes);
    }
    return 1;
}

/* Moves elements at and after index backwards */
static inline int hvec_insert_(char* vec_, int elem_bytes, int idx) {
    hvec_vec_* vec = (hvec_vec_*)vec_;
    /* Since each vec element is a pointer to heap, take the last pointer and
       move it to idx (as it is still usable)

       Remember that vec_size (length) is the old length, not the
       length after insertion
       |     |     |     | idx |     |     | length |
                          ^ Put here        ^ Take this pointer
                                              (Will be overwritten)  */
    if (!hvec_expand_(vec_, elem_bytes)) return 0;
    void* elem = vec_at(vec, vec_size(vec));
    for (int i = vec_size(vec); i > idx; --i) {
        vec_at(vec, i) = vec_at(vec, i - 1);
    }
    vec_at(vec, idx) = elem;
    return 1;
}

/* Deletes count elements starting at index start */
static inline void hvec_splice_(char* vec_, int start, int count) {
    hvec_vec_* vec = (hvec_vec_*)vec_;
    /* Cannot allocate memory to temporarily hold the pointers which will be
       put at the end, thus we swap the pointers up to the end

       Since each vec element is a pointer to heap, take the overwritten
       pointers and put it at the end (as it is still usable)

       It doesn't matter what order we put the pointers which were
       overwritten as they contain no values
                         --------------- count
       |     |     |     | start |     |     |     | length
                          ^ Take        ^ Put */

    int i = start;
    while (1) {
        for (int j = 0; j < count; ++j) {
            if (i + count + j >= vec_size(vec)) {
                goto loop_exit;
            }
            void* temp = vec_at(vec, i + j);
            vec_at(vec, i + j) = vec_at(vec, i + count + j);
            vec_at(vec, i + count + j) = temp;
        }
        i += count;
    }
loop_exit:
    vec->length -= count;
}

#endif
