/* tests/gc_objects_test.c — Phase 5 per-object conservative mark & sweep.
 *
 * Verifies the three properties that define the milestone:
 *   1. individual dead objects are reclaimed   (collect returns freed > 0),
 *   2. diverse live data survives collection AND subsequent chunk reuse
 *      (lists, HAMT maps, sets, rationals, closures — checked every cycle),
 *   3. survivors do not move    (a live object's address is stable across GC).
 */
#include "test_harness.h"
#include "test_helpers.h"
#include "gc.h"
#include <stddef.h>

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  void *addr_before, *addr_after;
  size_t freed;
  int cyc;

  lizard_test_env_init(&e);

  /* Diverse live data, bound in the global env so it stays reachable. */
  lizard_test_eval(&e, "(define L (list 11 22 33 44 55))");
  lizard_test_eval(&e, "(define M (phash-map \"x\" 1 \"y\" 2 \"z\" 3))");
  lizard_test_eval(&e, "(define S (pset 7 8 9))");
  lizard_test_eval(&e, "(define R (/ 22 7))");
  lizard_test_eval(&e, "(define F (let ((k 100)) (lambda (n) (+ n k))))");
  /* burn makes garbage that is genuinely dropped (not accumulated). */
  lizard_test_eval(&e,
      "(define (burn n) (if (> n 0) (begin (cons n (list n n)) "
      "(burn (- n 1))) 'ok))");

  /* Address of L's head node before any collection (proves non-moving). */
  addr_before = (void *)lizard_test_eval(&e, "L");

  for (cyc = 0; cyc < 5; cyc++) {
    lizard_test_eval(&e, "(burn 2000)");                 /* generate garbage */
    freed = lizard_gc_collect_objects(e.heap, e.env);    /* reclaim it       */
    TEST_ASSERT(freed > 0U);
    lizard_test_eval(&e, "(burn 2000)");                 /* reuse free chunks */

    r = lizard_test_eval(&e, "(equal? L (list 11 22 33 44 55))");
    TEST_ASSERT(lizard_test_is_true(r));
    r = lizard_test_eval(&e, "(= (phash-get M \"z\") 3)");
    TEST_ASSERT(lizard_test_is_true(r));
    r = lizard_test_eval(&e, "(pset-contains? S 8)");
    TEST_ASSERT(lizard_test_is_true(r));
    r = lizard_test_eval(&e, "(= R (/ 22 7))");
    TEST_ASSERT(lizard_test_is_true(r));
    r = lizard_test_eval(&e, "(= (F 5) 105)");
    TEST_ASSERT(lizard_test_is_true(r));
  }

  /* The collector is non-moving: the survivor's address is unchanged. */
  addr_after = (void *)lizard_test_eval(&e, "L");
  TEST_ASSERT(addr_before == addr_after);

  /* And it is still structurally correct after all those cycles. */
  r = lizard_test_eval(&e, "(equal? L (list 11 22 33 44 55))");
  TEST_ASSERT(lizard_test_is_true(r));

  TEST_RETURN();
}
