#include "sonLibGlobalsTest.h"

struct stEulerTour *et;

static void setup(void) {
	et = stEulerTour_construct();
}
static void teardown(void) {
	stEulerTour_destruct(et);
}
static void test_stEulerTour_makeRootTrivial(CuTest *testCase) {
	setup();
	struct stEulerVertex *v = stEulerTour_createVertex(et, (void*)"v");
	stEulerTour_makeRoot(et, v);
	teardown();
}

static void test_stEulerTour_link(CuTest *testCase) {
	setup();
	struct stEulerVertex *a = stEulerTour_createVertex(et, (void*)"a");
	struct stEulerVertex *b = stEulerTour_createVertex(et, (void*)"b");
	struct stEulerVertex *c = stEulerTour_createVertex(et, (void*)"c");
	struct stEulerVertex *d = stEulerTour_createVertex(et, (void*)"d");

	stEulerTour_link(et, a, b);
	CuAssertTrue(testCase, stEulerVertex_connected(a, b));
	CuAssertTrue(testCase, !stEulerVertex_connected(b, c));
	CuAssertTrue(testCase, !stEulerVertex_connected(a, c));

	CuAssertTrue(testCase, a->leftOut->to == b);
	CuAssertTrue(testCase, a->leftOut->from == a);
	CuAssertTrue(testCase, b->rightIn->from == b);
	CuAssertTrue(testCase, a->rightIn->to == a);
	CuAssertTrue(testCase, treap_compare(a->leftOut->node, a->rightIn->node) < 0);
	CuAssertTrue(testCase, treap_compare(a->leftOut->node, b->leftOut->node) == 0);

	CuAssertTrue(testCase, treap_next(a->leftOut->node) == b->rightIn->node);

	CuAssertTrue(testCase, stEulerVertex_findRoot(a) == a);
	CuAssertTrue(testCase, stEulerVertex_findRoot(b) == a);

	
	/*
	 *  d--a--b
	 *      \
	 *       c
	 */
	
	stEulerTour_link(et, a, c);
	stEulerTour_link(et, d, a);
	CuAssertStrEquals(testCase, stEulerVertex_print(a), "dacaba");
	teardown();
}
static void test_stEulerTour_cut(CuTest *testCase) {
	setup();
	struct stEulerVertex *a = stEulerTour_createVertex(et, (void*)"a");
	struct stEulerVertex *b = stEulerTour_createVertex(et, (void*)"b");
	struct stEulerVertex *c = stEulerTour_createVertex(et, (void*)"c");
	struct stEulerVertex *d = stEulerTour_createVertex(et, (void*)"d");
	struct stEulerVertex *e = stEulerTour_createVertex(et, (void*)"e");
	stEulerTour_link(et, a, b);
	stEulerTour_link(et, a, c);
	stEulerTour_link(et, a, d);
	stEulerTour_link(et, b, e);
	stEulerTour_cut(et, 0);
	CuAssertTrue(testCase, et->nComponents == 2);
	CuAssertTrue(testCase, !stEulerVertex_connected(a, b));
	printf("%s\n", stEulerVertex_print(a));
	printf("%s\n", stEulerVertex_print(b));
	teardown();
}



CuSuite *sonLib_stEulerTestSuite(void) {
	CuSuite *suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, test_stEulerTour_makeRootTrivial);
	SUITE_ADD_TEST(suite, test_stEulerTour_link);
	SUITE_ADD_TEST(suite, test_stEulerTour_cut);
	return suite;
}
