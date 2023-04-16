#include "CuTest.h"

#include "tree.h"

static void AttachDetachNode(CuTest* tc) {
    Tree tree;
    CuAssertIntEquals(tc, tree_construct(&tree), ec_noerr);

    TNode* node;
    tnode_alloca(&node, tree_root(&tree));

    TNode* node_1;
    tnode_alloca(&node_1, node);

    TNode* node_2;
    tnode_alloc(&node_2);

    tnode_attach(node, node_2);

    CuAssertIntEquals(tc, tnode_count_child(node), 2);

    tree_destruct(&tree);
}

static void DeleteSingleChildNodes(CuTest* tc) {
    Tree tree;
    CuAssertIntEquals(tc, tree_construct(&tree), ec_noerr);

    TNode* n1_1;
    tnode_alloca(&n1_1, tree_root(&tree));

    TNode* n2_1;
    tnode_alloca(&n2_1, n1_1);

    TNode* n2_2;
    tnode_alloca(&n2_2, n1_1);

    TNode* n3_1;
    tnode_alloca(&n3_1, n2_1);

    tree_remove_single_child(tree_root(&tree));

    CuAssertPtrEquals(tc, tnode_child(n1_1, 0), n3_1);

    tree_destruct(&tree);
}

static void DeleteSingleChildNodes2(CuTest* tc) {
    Tree tree;
    CuAssertIntEquals(tc, tree_construct(&tree), ec_noerr);

    TNode* n1_1;
    tnode_alloca(&n1_1, tree_root(&tree));

    TNode* n2_1;
    tnode_alloca(&n2_1, n1_1);

    tree_remove_single_child(tree_root(&tree));

    CuAssertPtrEquals(tc, tnode_child(tree_root(&tree), 0), n2_1);

    tree_destruct(&tree);
}

CuSuite* TreeGetSuite() {
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, AttachDetachNode);
    SUITE_ADD_TEST(suite, DeleteSingleChildNodes);
    SUITE_ADD_TEST(suite, DeleteSingleChildNodes2);
    return suite;
}
