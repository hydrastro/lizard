/* tests/vectors_test.c
 *
 * Vector creation, indexing, mutation, conversion to/from lists.
 */
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* make-vector with fill. */
  r = lizard_test_eval(&e, "(make-vector 5 0)");
  TEST_ASSERT_STR(lizard_test_format(r), "#(0 0 0 0 0)");

  /* Default fill is nil. */
  r = lizard_test_eval(&e, "(make-vector 3)");
  TEST_ASSERT_STR(lizard_test_format(r), "#(() () ())");

  /* vector literal. */
  r = lizard_test_eval(&e, "(vector 1 2 3 4 5)");
  TEST_ASSERT_STR(lizard_test_format(r), "#(1 2 3 4 5)");
  /* Empty vector. */
  r = lizard_test_eval(&e, "(vector)");
  TEST_ASSERT_STR(lizard_test_format(r), "#()");

  /* vector? predicate. */
  r = lizard_test_eval(&e, "(vector? (vector 1 2 3))");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(vector? '(1 2 3))");
  TEST_ASSERT(lizard_test_is_false(r));
  r = lizard_test_eval(&e, "(vector? 42)");
  TEST_ASSERT(lizard_test_is_false(r));

  /* vector-length */
  r = lizard_test_eval(&e, "(vector-length (vector 10 20 30 40))");
  TEST_ASSERT(lizard_test_is_int(r, 4));
  r = lizard_test_eval(&e, "(vector-length (make-vector 1000))");
  TEST_ASSERT(lizard_test_is_int(r, 1000));

  /* vector-ref */
  r = lizard_test_eval(&e, "(vector-ref (vector 10 20 30) 0)");
  TEST_ASSERT(lizard_test_is_int(r, 10));
  r = lizard_test_eval(&e, "(vector-ref (vector 10 20 30) 2)");
  TEST_ASSERT(lizard_test_is_int(r, 30));

  /* Mutation. */
  r = lizard_test_eval(&e,
                       "(define v (vector 1 2 3 4 5))"
                       "(vector-set! v 2 999)"
                       "(vector-ref v 2)");
  TEST_ASSERT(lizard_test_is_int(r, 999));

  r = lizard_test_eval(&e, "v");
  TEST_ASSERT_STR(lizard_test_format(r), "#(1 2 999 4 5)");

  /* vector->list / list->vector round-trip. */
  r = lizard_test_eval(&e, "(vector->list (vector 1 2 3))");
  TEST_ASSERT_STR(lizard_test_format(r), "(1 2 3)");
  r = lizard_test_eval(&e, "(list->vector '(a b c))");
  TEST_ASSERT_STR(lizard_test_format(r), "#(a b c)");
  r = lizard_test_eval(&e, "(list->vector '())");
  TEST_ASSERT_STR(lizard_test_format(r), "#()");

  /* Round-trip through both directions. */
  r = lizard_test_eval(&e,
                       "(vector-length (list->vector"
                       "  (vector->list (vector 1 2 3 4 5 6 7 8 9 10))))");
  TEST_ASSERT(lizard_test_is_int(r, 10));

  /* Vectors as a backing store for accumulator. */
  r = lizard_test_eval(&e,
                       "(define acc (make-vector 1 0))"
                       "(define (bump!)"
                       "  (vector-set! acc 0 (+ (vector-ref acc 0) 1)))"
                       "(bump!) (bump!) (bump!) (bump!) (bump!)"
                       "(vector-ref acc 0)");
  TEST_ASSERT(lizard_test_is_int(r, 5));

  /* Vectors can hold mixed types. */
  r = lizard_test_eval(&e, "(vector-ref (vector 1 \"two\" 'three #t) 2)");
  TEST_ASSERT(lizard_test_is_symbol(r, "three"));

  /* Errors. */
  r = lizard_test_eval(&e, "(vector-ref (vector 1 2 3) 10)");
  TEST_ASSERT(lizard_test_is_error(r));
  r = lizard_test_eval(&e, "(vector-ref (vector 1 2 3) -1)");
  TEST_ASSERT(lizard_test_is_error(r));
  r = lizard_test_eval(&e, "(vector-length 42)");
  TEST_ASSERT(lizard_test_is_error(r));
  r = lizard_test_eval(&e, "(make-vector -1)");
  TEST_ASSERT(lizard_test_is_error(r));

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
