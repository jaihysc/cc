#ifndef SET_H
#define SET_H

/* Set data type, holds pointers
   Adapted from https://github.com/barrust/set
   Implemented as hash table with linear probing */

#include <inttypes.h> /* uint64_t */

#include "errorcode.h"

typedef uint64_t (*SetHashFunction)(const void* key);

typedef struct
{
	const void* key_;
	uint64_t hash_;
} SetNode;

typedef struct
{
	SetNode** nodes;
	uint64_t number_nodes;
	uint64_t used_nodes;
	SetHashFunction hash_function;
} Set;

/*  Initialize the set either with default parameters (hash function and space)

	Returns:
		ec_badalloc: If an error occured setting up the memory */
ErrorCode set_construct(Set* set);

/*  Initialize the set either with provided parameters (hash function and space)

	Returns:
		ec_badalloc: If an error occured setting up the memory */
ErrorCode set_construct2(Set* set, uint64_t num_els, SetHashFunction hash);

/* Utility function to clear out the set */
ErrorCode set_clear(Set* set);

/* Free all memory that is part of the set */
void set_destruct(Set* set);

/*  Add element to set

	Returns:
		ec_noerr:         If added or already present
		ec_badalloc:      If unable to grow the set */
ErrorCode set_add(Set* set, const void* key);

/*  Remove element from the set

	Returns:
		ec_badalloc: If unable to modify the set */
ErrorCode set_remove(Set* set, const void* key);

/*  Check if key in set

	Returns:
		1 if present,
		0 if not found */
int set_contains(Set* set, const void* key);

/* Return the number of elements in the set */
uint64_t set_size(Set* set);

/*  Return an array of the elements in the set, size stored in provided pointer
    Returns NULL, size 0 if failed to allocate memory
	NOTE: Up to the caller to free the memory */
void** set_data(Set* set, uint64_t* size);

/*  Set res to the union of s1 and s2
	res = s1 ∪ s2

	Returns:
		ec_badalloc: If unable to grow the set */
ErrorCode set_union(Set* res, Set* s1, Set* s2);

/*  Set res to the intersection of s1 and s2
	res = s1 ∩ s2

	Returns:
		ec_badalloc: If unable to grow the set */
ErrorCode set_intersection(Set* res, Set* s1, Set* s2);

/*  Set res to the difference between s1 and s2
	res = s1 ∖ s2

	The set difference between two sets A and B is written A ∖ B, and means
	the set that consists of the elements of A which are not elements
	of B: x ∈ A ∖ B ⟺ x ∈ A ∧ x ∉ B. Another frequently seen notation
	for S ∖ T is S − T

	Returns:
		ec_badalloc: If unable to grow the set */
ErrorCode set_difference(Set* res, Set* s1, Set* s2);

/*  Set res to the symmetric difference between s1 and s2
	res = s1 △ s2

	The symmetric difference of two sets A and B is the set of elements either
	in A or in B but not in both. Symmetric difference is denoted
	A △ B or A * B

	Returns:
		ec_badalloc: If unable to grow the set */
ErrorCode set_symmetric_difference(Set* res, Set* s1, Set* s2);

/*  Return 1 if test is fully contained in s2; returns 0 otherwise
	test ⊆ against

	A set A is a subset of another set B if all elements of the set A are
	elements of the set B. In other words, the set A is contained inside
	the set B. The subset relationship is denoted as A ⊆ B */
int set_is_subset(Set* test, Set* against);

/*  Inverse of subset; return 1 if set test fully contains
	(including equal to) set against; return 0 otherwise
	test ⊇ against

	Superset Definition: A set A is a superset of another set B if all
	elements of the set B are elements of the set A. The superset
	relationship is denoted as A ⊇ B */
int set_is_superset(Set* test, Set* against);

/*  Strict subset ensures that the test is a subset of against, but that
	the two are also not equal. Return 1 if is strict subset, 0 otherwise
	test ⊂ against

	Set A is a strict subset of another set B if all elements of the set A
	are elements of the set B. In other words, the set A is contained inside
	the set B. A ≠ B is required. The strict subset relationship is denoted
	as A ⊂ B */
int set_is_subset_strict(Set* test, Set* against);

/*  Strict superset ensures that the test is a superset of against, but that
	the two are also not equal. Return 1 if is strict superset, 0 otherwise
	test ⊃ against

	Strict Superset Definition: A set A is a superset of another set B if
	all elements of the set B are elements of the set A. A ≠ B is required.
	The superset relationship is denoted as A ⊃ B */
int set_is_superset_strict(Set* test, Set* against);


typedef enum
{
	set_right_greater = 3,
	set_left_greater = 1,
	set_equal = 0,
	set_unequal = 2,
} SetCmpResult;

/*  Compare two sets for equality (size, keys same, etc)

	Returns:
		set_right_greater if left is less than right
		set_left_greater if right is less than left
		set_equal if left is the same size as right and keys match
		set_unequal if size is the same but elements are different */
SetCmpResult set_cmp(Set* left, Set* right);

#endif
