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

static int cmp_func1(TNode* node) {
    return tnode_count_child(node) == 1;
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

    tnode_remove_if(tree_root(&tree), cmp_func1);

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

    tnode_remove_if(tree_root(&tree), cmp_func1);

    CuAssertPtrEquals(tc, tnode_child(tree_root(&tree), 0), n2_1);

    tree_destruct(&tree);
}

static int cmp_func2(TNode* node) {
    return tnode_count_child(node) > 0;
}

static void DeleteMultipleChild(CuTest* tc) {
    Tree tree;
    CuAssertIntEquals(tc, tree_construct(&tree), ec_noerr);

    TNode* n1_1;
    tnode_alloca(&n1_1, tree_root(&tree));
    TNode* n1_2;
    tnode_alloca(&n1_2, tree_root(&tree));
    TNode* n1_3;
    tnode_alloca(&n1_3, tree_root(&tree));

    TNode* n2_1;
    tnode_alloca(&n2_1, n1_1);
    TNode* n2_2;
    tnode_alloca(&n2_2, n1_2);
    TNode* n2_3;
    tnode_alloca(&n2_3, n1_3);

    tnode_remove_if(tree_root(&tree), cmp_func2);

    CuAssertIntEquals(tc, tnode_count_child(tree_root(&tree)), 3);
    CuAssertPtrEquals(tc, tnode_child(tree_root(&tree), 0), n2_1);
    CuAssertPtrEquals(tc, tnode_child(tree_root(&tree), 1), n2_2);
    CuAssertPtrEquals(tc, tnode_child(tree_root(&tree), 2), n2_3);

    tree_destruct(&tree);
}

static void DeleteMultipleChild2(CuTest* tc) {
    Tree tree;
    CuAssertIntEquals(tc, tree_construct(&tree), ec_noerr);

    TNode* n1_1;
    tnode_alloca(&n1_1, tree_root(&tree));
    TNode* n1_2;
    tnode_alloca(&n1_2, tree_root(&tree));
    TNode* n1_3;
    tnode_alloca(&n1_3, tree_root(&tree));

    TNode* n2_1;
    tnode_alloca(&n2_1, n1_1);
    TNode* n2_2;
    tnode_alloca(&n2_2, n1_1);
    TNode* n2_3;
    tnode_alloca(&n2_3, n1_1);

    tnode_remove_if(tree_root(&tree), cmp_func2);

    CuAssertIntEquals(tc, tnode_count_child(tree_root(&tree)), 5);
    CuAssertPtrEquals(tc, tnode_child(tree_root(&tree), 0), n2_1);
    CuAssertPtrEquals(tc, tnode_child(tree_root(&tree), 1), n2_2);
    CuAssertPtrEquals(tc, tnode_child(tree_root(&tree), 2), n2_3);
    CuAssertPtrEquals(tc, tnode_child(tree_root(&tree), 3), n1_2);
    CuAssertPtrEquals(tc, tnode_child(tree_root(&tree), 4), n1_3);

    tree_destruct(&tree);
}

static int cmp_func3(TNode* node) {
    if (tnode_type(node) != tt_root) return 0;
    return tnode_count_child(node) == 0;
}

static void DeleteLeaf(CuTest* tc) {
    Tree tree;
    CuAssertIntEquals(tc, tree_construct(&tree), ec_noerr);

    TNode* n1_1;
    tnode_alloca(&n1_1, tree_root(&tree));
    TNode* n1_2;
    tnode_alloca(&n1_2, tree_root(&tree));
    TNode* n1_3;
    tnode_alloca(&n1_3, tree_root(&tree));

    TNode* n2_1;
    tnode_alloca(&n2_1, n1_3);
    TNodeIdentifier data;
    tnode_set(n2_1, tt_identifier, &data);

    tnode_remove_if(tree_root(&tree), cmp_func3);

    CuAssertIntEquals(tc, tnode_count_child(tree_root(&tree)), 1);
    CuAssertPtrEquals(tc, tnode_child(tree_root(&tree), 0), n1_3);
    CuAssertPtrEquals(tc, tnode_child(n1_3, 0), n2_1);

    tree_destruct(&tree);
}

static void ReplaceChild(CuTest* tc) {
    Tree tree;
    CuAssertIntEquals(tc, tree_construct(&tree), ec_noerr);

    TNode* n1_1;
    tnode_alloca(&n1_1, tree_root(&tree));
    TNode* n1_2;
    tnode_alloca(&n1_2, tree_root(&tree));
    TNode* n1_3;
    tnode_alloca(&n1_3, tree_root(&tree));

    TNode* new_child;
    tnode_replace_child(tree_root(&tree), &new_child, -2);

    CuAssertPtrNotNull(tc, new_child);

    TNodeIdentifier data;
    tnode_set(new_child, tt_identifier, &data);

    CuAssertIntEquals(tc, tnode_count_child(tree_root(&tree)), 3);
    CuAssertPtrEquals(tc, tnode_child(tree_root(&tree), 0), n1_1);
    CuAssertPtrEquals(tc, tnode_child(tree_root(&tree), 1), new_child);
    CuAssertPtrEquals(tc, tnode_child(tree_root(&tree), 2), n1_3);

    tree_destruct(&tree);
}

CuSuite* TreeGetSuite() {
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, AttachDetachNode);
    SUITE_ADD_TEST(suite, DeleteSingleChildNodes);
    SUITE_ADD_TEST(suite, DeleteSingleChildNodes2);
    SUITE_ADD_TEST(suite, DeleteMultipleChild);
    SUITE_ADD_TEST(suite, DeleteMultipleChild2);
    SUITE_ADD_TEST(suite, DeleteLeaf);
    SUITE_ADD_TEST(suite, ReplaceChild);
    return suite;
}
