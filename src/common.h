/*
 Common utilities across c files
*/
#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG(msg__) printf("%s", msg__);
#define LOGF(...)  printf(__VA_ARGS__);
/* For error/diagnostic messages */
#define ERRMSG(msg__)		printf("\033[1;31m%s\033[0m", msg__);
#define ERRMSGF(fmt__, ...) printf("\033[1;31m" fmt__ "\033[0m", __VA_ARGS__);

#define ASSERT(expr__, msg__)                           \
	if (!(expr__)) {                                    \
		ERRMSG("ICE: " msg__);                          \
		ERRMSGF("  Line: %d %s\n", __LINE__, __FILE__); \
		exit(1);                                        \
	}                                                   \
	do {                                                \
	} while (0)
#define ASSERTF(expr__, fmt__, ...)                     \
	if (!(expr__)) {                                    \
		ERRMSGF("ICE: " fmt__, __VA_ARGS__);            \
		ERRMSGF("  Line: %d %s\n", __LINE__, __FILE__); \
		exit(1);                                        \
	}                                                   \
	do {                                                \
	} while (0)

#define TOKEN_COLOR "\033[1;97m" /* Color for tokens when printed */

#define ARRAY_SIZE(array__) (int)(sizeof(array__) / sizeof(array__[0]))

/* ============================================================ */
/* Utility */

#define INT_CHAR_BUF 11 /* Length of buffer for holding integer as string */

/* Checks that the given strings is sorted correctly to
   perform a binary search via strbinfind */
static inline void strbinfind_validate(const char** strs, int str_count) {
	/* Compare the string at i with string at i+1, n-1 times */
	for (int i = 0; i < str_count - 1; ++i) {
		const char* str1 = strs[i];
		const char* str2 = strs[i + 1];

		int j = 0; /* String index */
		while (1) {
			char c1 = str1[j];
			char c2 = str2[j];
			ASSERTF(c1 <= c2, "Incorrectly sorted at index %d, %s %s", j, str1, str2);
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

	int front = 0;		  /* Inclusive */
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

/* Appends prefix and string representation of number into buf, null terminated
   Assumes sufficient space exists */
static inline void aappendi(char* buf, const char* prefix, const int num) {
	int prefix_len = strlen(prefix);
	strcopy(prefix, buf);
	itostr(num, buf + prefix_len);
	buf[prefix_len + ichar(num)] = '\0';
}

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
static inline void quicksort(void* ptr, size_t count, size_t size, int (*comp)(const void*, const void*)) {
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

/* Allocates num * bytes of zeroed storage, returns NULL if error */
static inline void* ccalloc(size_t num, size_t bytes) {
	return calloc(num, bytes);
}

/* Deallocates space allocated by cmalloc */
static inline void cfree(void* ptr) {
	free(ptr);
}

/* Zeros ptr with num bytes */
static inline void cmemzero(void* ptr, size_t num) {
	memset(ptr, 0, num);
}

#endif
