/* Assembly generator, struct IGNode */
#ifndef ASMGEN_IGNODE_H
#define ASMGEN_IGNODE_H

/* Interference graph node */
typedef struct {
    vec_t(SymbolId) symbol_id;
    /* Offset (in IGNode) to neighbor
       Pointers cannot be used as container holding nodes may resize */
    vec_t(int) neighbor;
    /* Performance impact if this variable is not in register,
       lower = less impact */
    uint64_t spill_cost;
    /* Holds a register preference score for each register */
    int reg_pref[X86_REGISTER_COUNT];
} IGNode;

/* registers: Number of registers on the target machine */
static void ignode_construct(IGNode* node) {
    ASSERT(node != NULL, "IGNode is null");
    vec_construct(&node->symbol_id);
    vec_construct(&node->neighbor);
    node->spill_cost = 0;
    for (int i = 0; i < X86_REGISTER_COUNT; ++i) {
        node->reg_pref[i] = 0;
    }
}

static void ignode_destruct(IGNode* node) {
    ASSERT(node != NULL, "IGNode is null");
    vec_destruct(&node->neighbor);
    vec_destruct(&node->symbol_id);
}

/* Clears the SymbolId which are represented by this interference graph node */
static void ignode_symid_clear(IGNode* node) {
    ASSERT(node != NULL, "IGNode is null");
    vec_clear(&node->symbol_id);
}

/* Returns the number of SymbolId which are represented by this interference
   graph node */
static int ignode_symid_count(const IGNode* node) {
    ASSERT(node != NULL, "IGNode is null");
    return vec_size(&node->symbol_id);
}

/* Returns SymbolId which is represented by this interference graph node
   at index */
static SymbolId ignode_symid(const IGNode* node, int i) {
    ASSERT(node != NULL, "IGNode is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < ignode_symid_count(node), "Index out of range");
    return vec_at(&node->symbol_id, i);
}

/* Adds a new SymbolId which is represented by this interference graph node
   Returns 1 if successful, 0 otherwise */
static int ignode_add_symid(IGNode* node, SymbolId id) {
    ASSERT(node != NULL, "IGNode is null");
    return vec_push_back(&node->symbol_id, id);
}

/* Returns number of neighbors for IGNode */
static int ignode_neighbor_count(const IGNode* node) {
    ASSERT(node != NULL, "Node is null");
    return vec_size(&node->neighbor);
}

/* Returns 1 if node has an link to other, 0 if not */
static int ignode_has_link(const IGNode* node, const IGNode* other) {
    int offset = (int)(other - node);
    for (int i = 0; i < ignode_neighbor_count(node); ++i) {
        if (vec_at(&node->neighbor, i) == offset) {
            return 1;
        }
    }
    return 0;
}

/* Adds a neighbor to this node from node to other
   Does nothing if neighbor does exist, returns 1
   Returns 1 if successful, 0 otherwise */
static int ignode_link(IGNode* node, IGNode* other) {
    ASSERT(node != NULL, "Node is null");
    ASSERT(other != NULL, "Other node is null");
    ASSERT(node != other, "Cannot link node to self");
    int offset = (int)(other - node);
    for (int i = 0; i < vec_size(&node->neighbor); ++i) {
        if (vec_at(&node->neighbor, i) == offset) {
            return 1;
        }
    }
    return vec_push_back(&node->neighbor, offset);
}

/* Removes a neighbor to this node from node to other
   Does nothing if neighbor does not exist */
static void ignode_unlink(IGNode* node, const IGNode* other) {
    ASSERT(node != NULL, "Node is null");
    ASSERT(other != NULL, "Other node is null");
    int offset = (int)(other - node);
    for (int i = 0; i < vec_size(&node->neighbor); ++i) {
        if (vec_at(&node->neighbor, i) == offset) {
            vec_splice(&node->neighbor, i, 1);
        }
    }
}

/* Returns neighbors for IGNode at index */
static IGNode* ignode_neighbor(IGNode* node, int i) {
    ASSERT(node != NULL, "Node is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < ignode_neighbor_count(node), "Index out of range");
    return node + vec_at(&node->neighbor, i);
}

/* Returns spill cost for interference graph node */
static uint64_t ignode_cost(IGNode* node) {
    ASSERT(node != NULL, "Node is null");
    return node->spill_cost;
}

/* Adds provided spill cost to interference graph node */
static void ignode_add_cost(IGNode* node, uint64_t cost) {
    ASSERT(node != NULL, "Node is null");
    node->spill_cost += cost;
}

/* Returns the number of register preferences for interference graph
   node */
static int ignode_reg_pref_count(const IGNode* node) {
    return X86_REGISTER_COUNT;
}

/* Returns register preference score at index for interference graph node */
static int ignode_reg_pref(const IGNode* node, int i) {
    ASSERT(node != NULL, "IGNode is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < ignode_reg_pref_count(node), "Index out of range");
    return node->reg_pref[i];
}

/* Increments register preference score at index by given score for
   interference graph node */
static void ignode_inc_reg_pref(IGNode* node, int i, int score) {
    ASSERT(node != NULL, "IGNode is null");
    ASSERT(i >= 0, "Invalid register");
    ASSERT(i < ignode_reg_pref_count(node), "Index out of range");
    node->reg_pref[i] += score;
}

#endif
