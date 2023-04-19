#include "vec.h"

#include "common.h"

int vec_expand_(
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

int vec_reserve_(
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

int vec_insert_(
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

void vec_splice_(
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

void hvec_destruct_(char* vec_) {
    hvec_vec_* vec = (hvec_vec_*)vec_;
    for (int i = 0; i < vec->capacity; ++i) {
        cfree(vec_at(vec, i));
    }
    vec_destruct(vec);
}

int hvec_reserve_(char* vec_, int n, int elem_bytes) {
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

int hvec_expand_(char* vec_, int elem_bytes) {
    hvec_vec_* vec = (hvec_vec_*)vec_;
    if (vec->length == vec->capacity) {
        /* + 1 to guarantee new space is always reserved */
        return hvec_reserve_(vec_, vec->capacity * 2 + 1, elem_bytes);
    }
    return 1;
}

int hvec_insert_(char* vec_, int elem_bytes, int idx) {
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

void hvec_splice_(char* vec_, int start, int count) {
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
