/* tests/pvector_test.c — persistent vector as a bit-partitioned trie.
 *
 * Checks correctness across the 32 / 1024 trie-growth boundaries, functional
 * update with the original preserved, pop, and — crucially — STRUCTURAL
 * SHARING: an update copies only the nodes on one root-to-leaf path, so every
 * leaf off that path is pointer-identical between the original and the update.
 */
#include "test_harness.h"
#include "test_helpers.h"
#include "pvector.h"
#include <stddef.h>

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* Build a 2050-element vector with repeated push (shift grows 5 -> 10). */
  lizard_test_eval(&e, "(define (build i n v) (if (< i n) (build (+ i 1) n (pvec-push v i)) v))");
  lizard_test_eval(&e, "(define big (build 0 2050 (pvec)))");

  r = lizard_test_eval(&e, "(= (pvec-count big) 2050)");
  TEST_ASSERT(lizard_test_is_true(r));

  /* Every index reads back its value (full trie navigation). */
  lizard_test_eval(&e, "(define (ok? i n v) (if (< i n) (if (= (pvec-ref v i) i) (ok? (+ i 1) n v) #f) #t))");
  r = lizard_test_eval(&e, "(ok? 0 2050 big)");
  TEST_ASSERT(lizard_test_is_true(r));

  /* Boundary indices. */
  r = lizard_test_eval(&e, "(and (= (pvec-ref big 31) 31) (= (pvec-ref big 32) 32) "
                           "(= (pvec-ref big 1023) 1023) (= (pvec-ref big 1024) 1024) "
                           "(= (pvec-ref big 2049) 2049))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* Functional update deep in the trie; original is unchanged. */
  lizard_test_eval(&e, "(define upd (pvec-set big 1000 -7))");
  r = lizard_test_eval(&e, "(= (pvec-ref upd 1000) -7)");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(= (pvec-ref big 1000) 1000)");   /* persistence */
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(= (pvec-count upd) 2050)");
  TEST_ASSERT(lizard_test_is_true(r));

  /* Pop across the 1024 boundary downward. */
  lizard_test_eval(&e, "(define (popn k v) (if (> k 0) (popn (- k 1) (pvec-pop v)) v))");
  lizard_test_eval(&e, "(define small (popn 1030 big))");
  r = lizard_test_eval(&e, "(and (= (pvec-count small) 1020) (= (pvec-ref small 1019) 1019) (= (pvec-ref small 0) 0))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* ---- structural sharing, checked at the node level ---- */
  {
    lizard_ast_node_t *base = lizard_test_eval(&e, "big");
    lizard_ast_node_t *up   = lizard_test_eval(&e, "upd");
    /* index 1000 lives in some leaf; index 40 and index 1500 live in other
     * leaves that are NOT on the updated path. */
    const void *base_hit  = lizard_pvec_leaf_ptr(base, 1000);
    const void *up_hit    = lizard_pvec_leaf_ptr(up,   1000);
    const void *base_far1 = lizard_pvec_leaf_ptr(base, 40);
    const void *up_far1   = lizard_pvec_leaf_ptr(up,   40);
    const void *base_far2 = lizard_pvec_leaf_ptr(base, 1500);
    const void *up_far2   = lizard_pvec_leaf_ptr(up,   1500);

    /* The leaf on the updated path is a fresh copy... */
    TEST_ASSERT(base_hit != up_hit);
    /* ...while every off-path leaf is shared (identical pointer). */
    TEST_ASSERT(base_far1 == up_far1);
    TEST_ASSERT(base_far2 == up_far2);
  }

  TEST_RETURN();
}
