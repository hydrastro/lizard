/* tests/reals_test.c — inexact (floating-point) reals and the exact/inexact
 * numeric tower: literals, contagion, predicates, conversions, math.
 * Assertions are written as boolean Lisp expressions so the existing helpers
 * suffice (no float-valued assertion needed). */
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* literals read back as inexact, exact stays exact */
  r = lizard_test_eval(&e, "(inexact? 3.14)"); TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(exact? 3)");      TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(inexact? 3)");    TEST_ASSERT(lizard_test_is_false(r));
  r = lizard_test_eval(&e, "(exact? 1.5)");    TEST_ASSERT(lizard_test_is_false(r));

  /* contagion: any inexact operand forces an inexact result */
  r = lizard_test_eval(&e, "(inexact? (+ 1 2.5))");      TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(= (+ 1 2.5) 3.5)");         TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(exact? (+ 1 2 3))");        TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(= (* 2 0.5) 1.0)");         TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(inexact? (/ 1 2.0))");      TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(exact? (/ 1 2))");          TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(= (+ 1/2 0.5) 1.0)");       TEST_ASSERT(lizard_test_is_true(r));

  /* numeric = with contagion vs structural equal? */
  r = lizard_test_eval(&e, "(= 1 1.0)");        TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(= 1/2 0.5)");      TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(equal? 1 1.0)");   TEST_ASSERT(lizard_test_is_false(r));
  r = lizard_test_eval(&e, "(< 1 1.5 2)");      TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(> 2.5 1/2)");      TEST_ASSERT(lizard_test_is_true(r));

  /* predicates */
  r = lizard_test_eval(&e, "(integer? 2.0)");   TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(integer? 2.5)");   TEST_ASSERT(lizard_test_is_false(r));
  r = lizard_test_eval(&e, "(real? 3)");        TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(rational? 2.0)");  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(exact-integer? 3)"); TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(exact-integer? 3.0)"); TEST_ASSERT(lizard_test_is_false(r));

  /* conversions round-trip exactly for dyadic rationals */
  r = lizard_test_eval(&e, "(= (exact->inexact 1/2) 0.5)");  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(equal? (inexact->exact 0.5) 1/2)"); TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(equal? (inexact->exact 0.25) 1/4)"); TEST_ASSERT(lizard_test_is_true(r));

  /* sqrt: exact for perfect squares, inexact otherwise */
  r = lizard_test_eval(&e, "(exact? (sqrt 16))");           TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(= (sqrt 16) 4)");              TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(inexact? (sqrt 2))");          TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(< (abs (- (* (sqrt 2) (sqrt 2)) 2)) 0.0000001)");
  TEST_ASSERT(lizard_test_is_true(r));

  /* exactness-preserving rounding on rationals; inexact in -> inexact out */
  r = lizard_test_eval(&e, "(equal? (floor 7/2) 3)");       TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(equal? (ceiling 7/2) 4)");     TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(equal? (truncate -7/2) -3)");  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(inexact? (floor 3.7))");       TEST_ASSERT(lizard_test_is_true(r));

  /* transcendental identities (within tolerance) */
  r = lizard_test_eval(&e, "(< (abs (- (exp 0) 1.0)) 0.0000001)"); TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(< (abs (sin 0)) 0.0000001)");         TEST_ASSERT(lizard_test_is_true(r));

  TEST_RETURN();
}
