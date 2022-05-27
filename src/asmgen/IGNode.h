/* Assembly generator, struct IGNode */
#ifndef ASMGEN_IGNODE_H
#define ASMGEN_IGNODE_H

/* Interference graph node */
typedef struct {
    /* Offset (in IGNode) to neighbor
       Pointers cannot be used as container holding nodes may resize */
    vec_t(int) neighbor;
    /* Performance impact if this variable is not in register,
       lower = less impact */
    uint64_t spill_cost;
} IGNode;

static void ignode_construct(IGNode* node) {
    ASSERT(node != NULL, "IGNode is null");
    vec_construct(&node->neighbor);
    node->spill_cost = 0;
}

static void ignode_destruct(IGNode* node) {
    ASSERT(node != NULL, "IGNode is null");
    vec_destruct(&node->neighbor);
}

/* Returns number of neighbors for IGNode */
static int ignode_neighbor_count(IGNode* node) {
    ASSERT(node != NULL, "Node is null");
    return vec_size(&node->neighbor);
}

/* Adds a neighbor to this node from node to other
   Does nothing if neighbor does exist, returns 1
   Returns 1 if successful, 0 otherwise */
static int ignode_link(IGNode* node, IGNode* other) {
    ASSERT(node != NULL, "Node is null");
    ASSERT(other != NULL, "Other node is null");
    ASSERT(node != other, "Cannot link node to self");
    int offset = other - node;
    for (int i = 0; i < vec_size(&node->neighbor); ++i) {
        if (vec_at(&node->neighbor, i) == offset) {
            return 1;
        }
    }
    return vec_push_back(&node->neighbor, offset);
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

#endif
