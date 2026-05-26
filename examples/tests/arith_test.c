/* tests/arith_test.c
 *
 * Guards the headline regression: arithmetic primitives must not
 * mutate their first argument. Also covers variadic arithmetic,
 * bignum results, and division by zero.
 */
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* Variadic + - * / */
  r = lizard_test_eval(&e, "(+ 1 2 3 4 5)");
  TEST_ASSERT(lizard_test_is_int(r, 15));

  r = lizard_test_eval(&e, "(* 2 3 4)");
  TEST_ASSERT(lizard_test_is_int(r, 24));

  r = lizard_test_eval(&e, "(- 100 1 1 1)");
  TEST_ASSERT(lizard_test_is_int(r, 97));

  r = lizard_test_eval(&e, "(/ 100 5 2)");
  TEST_ASSERT(lizard_test_is_int(r, 10));

  /* Unary negation. */
  r = lizard_test_eval(&e, "(- 7)");
  TEST_ASSERT(lizard_test_is_int(r, -7));

  /* Modulo + power. */
  r = lizard_test_eval(&e, "(% 17 5)");
  TEST_ASSERT(lizard_test_is_int(r, 2));

  r = lizard_test_eval(&e, "(^ 2 10)");
  TEST_ASSERT(lizard_test_is_int(r, 1024));

  /* HEADLINE BUG: (* x 2) must not rewrite x. */
  r = lizard_test_eval(&e,
                       "(define x 5)"
                       "(* x 2)"
                       "x");
  TEST_ASSERT(lizard_test_is_int(r, 5));

  r = lizard_test_eval(&e,
                       "(define y 10)"
                       "(- y 3)"
                       "y");
  TEST_ASSERT(lizard_test_is_int(r, 10));

  r = lizard_test_eval(&e,
                       "(define z 100)"
                       "(/ z 4)"
                       "z");
  TEST_ASSERT(lizard_test_is_int(r, 100));

  /* set! still mutates. */
  r = lizard_test_eval(&e,
                       "(define n 1)"
                       "(set! n (+ n 1))"
                       "(set! n (* n 5))"
                       "n");
  TEST_ASSERT(lizard_test_is_int(r, 10));

  /* Division by zero surfaces as an error. */
  r = lizard_test_eval(&e, "(/ 1 0)");
  TEST_ASSERT(lizard_test_is_error(r));

  /* Type mismatch surfaces as an error. */
  r = lizard_test_eval(&e, "(+ 1 \"two\")");
  TEST_ASSERT(lizard_test_is_error(r));

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
