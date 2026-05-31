/* tests/rationals_test.c — exact rational numeric tower, end to end through the
 * reader, evaluator, arithmetic primitives, and predicates. */
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* exact division + rational literal + equal? */
  r = lizard_test_eval(&e, "(equal? (/ 7 2) 7/2)");
  TEST_ASSERT(lizard_test_is_true(r));

  /* division reduces to lowest terms */
  r = lizard_test_eval(&e, "(= (/ 6 4) 3/2)");
  TEST_ASSERT(lizard_test_is_true(r));

  /* division that comes out whole demotes to an integer */
  r = lizard_test_eval(&e, "(integer? (/ 6 3))");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(integer? (/ 7 2))");
  TEST_ASSERT(lizard_test_is_false(r));

  /* literal 4/2 reads as the integer 2 */
  r = lizard_test_eval(&e, "(integer? 4/2)");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(equal? 4/2 2)");
  TEST_ASSERT(lizard_test_is_true(r));

  /* mixed integer/rational arithmetic */
  r = lizard_test_eval(&e, "(= (+ 1 1/2) 3/2)");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(= (- 5 1/3) 14/3)");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(= (* 2/3 3/4) 1/2)");
  TEST_ASSERT(lizard_test_is_true(r));

  /* exponentiation with a rational base */
  r = lizard_test_eval(&e, "(= (expt 1/2 3) 1/8)");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(= (^ 2/3 2) 4/9)");
  TEST_ASSERT(lizard_test_is_true(r));

  /* comparisons across integers and rationals */
  r = lizard_test_eval(&e, "(< 1/4 1/3 1/2 2/3 1)");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(= 1/2 2/4)");
  TEST_ASSERT(lizard_test_is_true(r));

  /* numerator / denominator return integers in lowest terms */
  r = lizard_test_eval(&e, "(numerator 6/8)");
  TEST_ASSERT(lizard_test_is_int(r, 3));
  r = lizard_test_eval(&e, "(denominator 6/8)");
  TEST_ASSERT(lizard_test_is_int(r, 4));
  r = lizard_test_eval(&e, "(denominator 5)");
  TEST_ASSERT(lizard_test_is_int(r, 1));

  /* predicates */
  r = lizard_test_eval(&e, "(rational? 5)");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(exact? 1/3)");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(negative? -1/2)");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(zero? (- 1/2 1/2))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* exactness at scale — no floating-point rounding */
  r = lizard_test_eval(&e, "(= (* 1/1000000 1/1000000) 1/1000000000000)");
  TEST_ASSERT(lizard_test_is_true(r));

  /* exact harmonic sum 1 + 1/2 + 1/3 + 1/4 = 25/12 */
  r = lizard_test_eval(&e,
      "(define (h n) (if (= n 0) 0 (+ (/ 1 n) (h (- n 1)))))"
      "(= (h 4) 25/12)");
  TEST_ASSERT(lizard_test_is_true(r));

  /* division by zero is still an error */
  r = lizard_test_eval(&e, "(/ 1 0)");
  TEST_ASSERT(lizard_test_is_error(r));

  /* integer arithmetic is unaffected (no accidental rationalization) */
  r = lizard_test_eval(&e, "(+ 2 3)");
  TEST_ASSERT(lizard_test_is_int(r, 5));
  r = lizard_test_eval(&e, "(* 6 7)");
  TEST_ASSERT(lizard_test_is_int(r, 42));

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
