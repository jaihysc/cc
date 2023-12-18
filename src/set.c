#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "set.h"

#include "common.h"

#define MAX_FULLNESS_PERCENT 0.25 /* arbitrary */

static uint64_t default_hash_(const void* key);
/* Finds index, stored at pointer, return 1 if found, 0 if not found - index at first open slot */
static int get_index_(Set* set, const void* key, uint64_t hash, uint64_t* index);
static ErrorCode assign_node_(Set* set, const void* key, uint64_t hash, uint64_t index);
static void free_index_(Set* set, uint64_t index);
static int set_contains_(Set* set, const void* key, uint64_t hash);
static ErrorCode set_add_(Set* set, const void* key, uint64_t hash);
static ErrorCode relayout_nodes_(Set* set, uint64_t start, short end_on_null);

ErrorCode set_construct(Set* set) {
	return set_construct2(set, 1024, NULL);
}

ErrorCode set_construct2(Set* set, uint64_t num_els, SetHashFunction hash) {
	set->nodes = malloc(num_els * sizeof(SetNode*));
	if (set->nodes == NULL) {
		return ec_badalloc;
	}

	set->number_nodes = num_els;
	for (uint64_t i = 0; i < set->number_nodes; ++i) {
		set->nodes[i] = NULL;
	}

	set->used_nodes = 0;
	set->hash_function = (hash == NULL) ? &default_hash_ : hash;
	return ec_noerr;
}

ErrorCode set_clear(Set* set) {
	for (uint64_t i = 0; i < set->number_nodes; ++i) {
		if (set->nodes[i] != NULL) {
			free_index_(set, i);
		}
	}
	set->used_nodes = 0;
	return ec_noerr;
}

void set_destruct(Set* set) {
	set_clear(set);
	free(set->nodes);
	set->number_nodes = 0;
	set->used_nodes = 0;
	set->hash_function = NULL;
}

ErrorCode set_add(Set* set, const void* key) {
	uint64_t hash = set->hash_function(key);
	return set_add_(set, key, hash);
}

int set_contains(Set* set, const void* key) {
	uint64_t index, hash = set->hash_function(key);
	return get_index_(set, key, hash, &index);
}

ErrorCode set_remove(Set* set, const void* key) {
	uint64_t index, hash = set->hash_function(key);
	if (!get_index_(set, key, hash, &index)) {
		/* Not found */
		return ec_noerr;
	}

	/* remove this node */
	free_index_(set, index);

	ErrorCode ecode = relayout_nodes_(set, index, 0);
	if (ecode != ec_noerr) return ecode;

	--set->used_nodes;
	return ec_noerr;
}

uint64_t set_size(Set* set) {
	return set->used_nodes;
}

void** set_data(Set* set, uint64_t* size) {
	*size = 0;
	void** results = malloc(set->used_nodes + 1, sizeof(void*));
	if (results == NULL) return NULL;

	uint64_t j = 0;
	for (uint64_t i = 0; i < set->number_nodes; ++i) {
		if (set->nodes[i] != NULL) {
			results[j] = (void*)set->nodes[i]->key_;
			++j;
		}
	}

	*size = set->used_nodes;
	return results;
}

ErrorCode set_union(Set* res, Set* s1, Set* s2) {
	ASSERT(res->used_nodes == 0, "Set occupied");

	// loop over both s1 and s2 and get keys and insert them into res
	for (uint64_t i = 0; i < s1->number_nodes; ++i) {
		if (s1->nodes[i] != NULL) {
			ErrorCode code = set_add_(res, s1->nodes[i]->key_, s1->nodes[i]->hash_);
			if (code != ec_noerr) return code;
		}
	}
	for (uint64_t i = 0; i < s2->number_nodes; ++i) {
		if (s2->nodes[i] != NULL) {
			ErrorCode code = set_add_(res, s2->nodes[i]->key_, s2->nodes[i]->hash_);
			if (code != ec_noerr) return code;
		}
	}
	return ec_noerr;
}

ErrorCode set_intersection(Set* res, Set* s1, Set* s2) {
	ASSERT(res->used_nodes == 0, "Set occupied");

	// loop over both one of s1 and s2: get keys, check the other, and insert them into res if it is
	for (uint64_t i = 0; i < s1->number_nodes; ++i) {
		if (s1->nodes[i] != NULL) {
			if (set_contains_(s2, s1->nodes[i]->key_, s1->nodes[i]->hash_)) {
				ErrorCode code = set_add_(res, s1->nodes[i]->key_, s1->nodes[i]->hash_);
				if (code != ec_noerr) return code;
			}
		}
	}
	return ec_noerr;
}

/* difference is s1 - s2 */
ErrorCode set_difference(Set* res, Set* s1, Set* s2) {
	ASSERT(res->used_nodes == 0, "Set occupied");

	// loop over s1 and keep only things not in s2
	for (uint64_t i = 0; i < s1->number_nodes; ++i) {
		if (s1->nodes[i] != NULL) {
			if (!set_contains_(s2, s1->nodes[i]->key_, s1->nodes[i]->hash_)) {
				ErrorCode code = set_add_(res, s1->nodes[i]->key_, s1->nodes[i]->hash_);
				if (code != ec_noerr) return code;
			}
		}
	}
	return ec_noerr;
}

ErrorCode set_symmetric_difference(Set* res, Set* s1, Set* s2) {
	ASSERT(res->used_nodes == 0, "Set occupied");

	// loop over set 1 and add elements that are unique to set 1
	for (uint64_t i = 0; i < s1->number_nodes; ++i) {
		if (s1->nodes[i] != NULL) {
			if (!set_contains_(s2, s1->nodes[i]->key_, s1->nodes[i]->hash_)) {
				ErrorCode code = set_add_(res, s1->nodes[i]->key_, s1->nodes[i]->hash_);
				if (code != ec_noerr) return code;
			}
		}
	}
	// loop over set 2 and add elements that are unique to set 2
	for (uint64_t i = 0; i < s2->number_nodes; ++i) {
		if (s2->nodes[i] != NULL) {
			if (!set_contains_(s1, s2->nodes[i]->key_, s2->nodes[i]->hash_)) {
				ErrorCode code = set_add_(res, s2->nodes[i]->key_, s2->nodes[i]->hash_);
				if (code != ec_noerr) return code;
			}
		}
	}
	return ec_noerr;
}

int set_is_subset(Set* test, Set* against) {
	for (uint64_t i = 0; i < test->number_nodes; ++i) {
		if (test->nodes[i] != NULL) {
			if (!set_contains_(against, test->nodes[i]->key_, test->nodes[i]->hash_)) {
				return 1;
			}
		}
	}
	return 0;
}

int set_is_superset(Set* test, Set* against) {
	return set_is_subset(against, test);
}

int set_is_subset_strict(Set* test, Set* against) {
	if (test->used_nodes >= against->used_nodes) {
		return 0;
	}
	return set_is_subset(test, against);
}

int set_is_superset_strict(Set* test, Set* against) {
	return set_is_subset_strict(against, test);
}

SetCmpResult set_cmp(Set* left, Set* right) {
	if (left->used_nodes < right->used_nodes) {
		return set_right_greater;
	}
	else if (right->used_nodes < left->used_nodes) {
		return set_left_greater;
	}
	for (uint64_t i = 0; i < left->number_nodes; ++i) {
		if (left->nodes[i] != NULL) {
			if (!set_contains(right, left->nodes[i]->key_)) {
				return set_unequal;
			}
		}
	}

	return set_equal;
}

static uint64_t default_hash_(const void* key) {
	// FNV-1a hash (http://www.isthe.com/chongo/tech/comp/fnv/)
	uint64_t h = 14695981039346656037ULL; // FNV_OFFSET 64 bit
	for (size_t i = 0; i < sizeof(void*); ++i) {
		h = h ^ ((char*)&key)[i];
		h = h * 1099511628211ULL; // FNV_PRIME 64 bit
	}
	return h;
}

static int set_contains_(Set* set, const void* key, uint64_t hash) {
	uint64_t index;
	return get_index_(set, key, hash, &index);
}

static ErrorCode set_add_(Set* set, const void* key, uint64_t hash) {
	uint64_t index;
	if (set_contains_(set, key, hash)) return ec_noerr;

	// Expand nodes if we are close to our desired fullness
	ErrorCode ecode;
	if ((float)set->used_nodes / set->number_nodes > MAX_FULLNESS_PERCENT) {
		uint64_t num_els = set->number_nodes * 2; // we want to double each time
		SetNode** tmp = realloc(set->nodes, num_els * sizeof(SetNode*));
		if (tmp == NULL || set->nodes == NULL) // malloc failure
			return ec_badalloc;

		set->nodes = tmp;
		uint64_t i, orig_num_els = set->number_nodes;
		for (i = orig_num_els; i < num_els; ++i) set->nodes[i] = NULL;

		set->number_nodes = num_els;
		// re-layout all nodes
		ecode = relayout_nodes_(set, 0, 1);
		if (ecode != ec_noerr) return ecode;
	}
	// add element in
	get_index_(set, key, hash, &index); // this is the first open slot

	ecode = assign_node_(set, key, hash, index);
	if (ecode != ec_noerr) return ecode;

	++set->used_nodes;
	return ec_noerr;
}

static int get_index_(Set* set, const void* key, uint64_t hash, uint64_t* index) {
	uint64_t i, idx;
	idx = hash % set->number_nodes;
	i = idx;
	while (1) {
		if (set->nodes[i] == NULL) {
			*index = i;
			return 0; // not here OR first open slot
		}
		else if (hash == set->nodes[i]->hash_ && key == set->nodes[i]->key_) {
			*index = i;
			return 1;
		}
		++i;
		if (i == set->number_nodes) i = 0;
		ASSERT(i != idx, "Set circular"); // this means we went all the way around and the set is full
	}
}

static ErrorCode assign_node_(Set* set, const void* key, uint64_t hash, uint64_t index) {
	set->nodes[index] = malloc(sizeof(SetNode));
	if (set->nodes[index] == NULL) return ec_badalloc;

	set->nodes[index]->key_ = key;
	set->nodes[index]->hash_ = hash;
	return ec_noerr;
}

static void free_index_(Set* set, uint64_t index) {
	free(set->nodes[index]);
	set->nodes[index] = NULL;
}

static ErrorCode relayout_nodes_(Set* set, uint64_t start, short end_on_null) {
	uint64_t index = 0, i;
	for (i = start; i < set->number_nodes; ++i) {
		if (set->nodes[i] != NULL) {
			ASSERT(get_index_(set, set->nodes[i]->key_, set->nodes[i]->hash_, &index), "Could not find index");
			if (i != index) { // we are moving this node
				ErrorCode ecode = assign_node_(set, set->nodes[i]->key_, set->nodes[i]->hash_, index);
				if (ecode != ec_noerr) return ecode;

				free_index_(set, i);
			}
		}
		else if (end_on_null == 0 && i != start) {
			break;
		}
	}
	return ec_noerr;
}