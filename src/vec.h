#ifndef VEC_H
#define VEC_H

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

int vec_expand_(
        char* pdata, int* length, int* capacity, int memsz);

int vec_reserve_(
        char* pdata, int* length, int* capacity, int memsz, int n);

int vec_insert_(
        char* pdata, int* length, int* capacity, int memsz, int idx);

void vec_splice_(
        char* pdata, int* length, int* capacity, int memsz,
        int start, int count);

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

void hvec_destruct_(char* vec_);

/* Allocates storage for an n elements
   Each vec element (T__*) up to capacity is a valid pointer to memory to store
   hvec elements
   Does not change length */
int hvec_reserve_(char* vec_, int n, int elem_bytes);

/* Allocates storage if necessary so an additional element can be stored */
int hvec_expand_(char* vec_, int elem_bytes);

/* Moves elements at and after index backwards */
int hvec_insert_(char* vec_, int elem_bytes, int idx);

/* Deletes count elements starting at index start */
void hvec_splice_(char* vec_, int start, int count);

#endif
