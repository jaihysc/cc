/*
 Common utilities across c files
*/
#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>

#define LOG(msg__) printf("%s", msg__);
#define LOGF(...) printf(__VA_ARGS__);
/* For error/diagnostic messages */
#define ERRMSG(msg__) printf("\033[1;31m%s\033[0m", msg__);
#define ERRMSGF(fmt__, ...) printf("\033[1;31m" fmt__ "\033[0m", __VA_ARGS__);

#define ASSERT(expr__, msg__)              \
    if (!(expr__)) {                       \
        ERRMSG("ICE: " msg__);             \
        ERRMSGF("  Line: %d\n", __LINE__); \
        exit(1);                           \
    } do {} while (0)
#define ASSERTF(expr__, fmt__, ...)          \
    if (!(expr__)) {                         \
        ERRMSGF("ICE: " fmt__, __VA_ARGS__); \
        ERRMSGF("  Line: %d\n", __LINE__);   \
        exit(1);                             \
    } do {} while (0)



#define ARRAY_SIZE(array__) (int)(sizeof(array__) / sizeof(array__[0]))

/* Performs binary search for string */
/* str:       c string to look for, does not need to be null terminated*/
/* match_amt: Number of characters in str to match */
/* strs:      pointer to c strings to look through */
/* str_count: number of c strings in strs */
/* Returns index where string was found, otherwise -1 */
/* strs String sorting requirements: */
/*   Alphabetical */
/*   If a longer string has a prefix which matches a shorter string: */
/*   the longer string comes after the shorter string */
static inline int strbinfind(const char* str, int match_amt, const char** strs, int str_count) {
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

#endif
