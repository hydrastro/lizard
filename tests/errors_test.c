/* tests/errors_test.c
 *
 * Verifies that every error path surfaces as an AST_ERROR rather
 * than crashing or silently returning garbage. Errors must also
 * propagate through enclosing expressions instead of being
 * swallowed or rewritten.
 */
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* Unbound symbols. */
  r = lizard_test_eval(&e, "no-such-name");
  TEST_ASSERT(lizard_test_is_error(r));

  /* Unbound in operator position must propagate as unbound,
   * not get rewritten to "non-function". */
  r = lizard_test_eval(&e, "(no-such-function 1 2)");
  TEST_ASSERT(lizard_test_is_error(r));

  /* Type errors. */
  r = lizard_test_eval(&e, "(+ 1 \"two\")");
  TEST_ASSERT(lizard_test_is_error(r));
  r = lizard_test_eval(&e, "(* 'a 2)");
  TEST_ASSERT(lizard_test_is_error(r));
  r = lizard_test_eval(&e, "(< 'a 1)");
  TEST_ASSERT(lizard_test_is_error(r));

  /* car/cdr on non-pairs. */
  r = lizard_test_eval(&e, "(car 5)");
  TEST_ASSERT(lizard_test_is_error(r));
  r = lizard_test_eval(&e, "(cdr 5)");
  TEST_ASSERT(lizard_test_is_error(r));
  r = lizard_test_eval(&e, "(car '())");
  TEST_ASSERT(lizard_test_is_error(r));

  /* Division by zero. */
  r = lizard_test_eval(&e, "(/ 1 0)");
  TEST_ASSERT(lizard_test_is_error(r));
  r = lizard_test_eval(&e, "(% 10 0)");
  TEST_ASSERT(lizard_test_is_error(r));
  r = lizard_test_eval(&e, "(quotient 5 0)");
  TEST_ASSERT(lizard_test_is_error(r));
  r = lizard_test_eval(&e, "(remainder 5 0)");
  TEST_ASSERT(lizard_test_is_error(r));

  /* Arity errors on built-ins. */
  r = lizard_test_eval(&e, "(not)");
  TEST_ASSERT(lizard_test_is_error(r));
  r = lizard_test_eval(&e, "(not #t #f)");
  TEST_ASSERT(lizard_test_is_error(r));
  r = lizard_test_eval(&e, "(load)");
  TEST_ASSERT(lizard_test_is_error(r));
  r = lizard_test_eval(&e, "(load \"a\" \"b\")");
  TEST_ASSERT(lizard_test_is_error(r));

  /* Lambda arity mismatch. */
  r = lizard_test_eval(&e, "((lambda (x y) (+ x y)) 1)");
  TEST_ASSERT(lizard_test_is_error(r));
  r = lizard_test_eval(&e, "((lambda (x) x) 1 2)");
  TEST_ASSERT(lizard_test_is_error(r));

  /* Applying non-functions. */
  r = lizard_test_eval(&e, "(5 6)");
  TEST_ASSERT(lizard_test_is_error(r));
  r = lizard_test_eval(&e, "(\"hi\" 'x)");
  TEST_ASSERT(lizard_test_is_error(r));

  /* set! on undefined variable. */
  r = lizard_test_eval(&e, "(set! never-defined 42)");
  TEST_ASSERT(lizard_test_is_error(r));

  /* expt with negative exponent (we only support non-negative). */
  r = lizard_test_eval(&e, "(expt 2 -3)");
  TEST_ASSERT(lizard_test_is_error(r));

  /* Predicate type errors. */
  r = lizard_test_eval(&e, "(number?)");
  TEST_ASSERT(lizard_test_is_error(r));
  r = lizard_test_eval(&e, "(number? 1 2)");
  TEST_ASSERT(lizard_test_is_error(r));

  /* Error propagation: an error inside an argument should surface,
   * not get silently masked by the outer call. */
  r = lizard_test_eval(&e, "(+ 1 (/ 1 0))");
  TEST_ASSERT(lizard_test_is_error(r));

  /* Error in let binding propagates. */
  r = lizard_test_eval(&e, "(let ((x (/ 1 0))) x)");
  TEST_ASSERT(lizard_test_is_error(r));

  /* After an error, the environment must still be usable. */
  r = lizard_test_eval(&e, "(+ 1 2)");
  TEST_ASSERT(lizard_test_is_int(r, 3));

  /* Errors from inside lambdas surface to the caller. */
  r = lizard_test_eval(&e, "(define (boom) (/ 1 0))"
                           "(boom)");
  TEST_ASSERT(lizard_test_is_error(r));

  /* (load "/path/that/does/not/exist") returns an error rather than
   * crashing. */
  r = lizard_test_eval(&e, "(load \"/nonexistent/path/abcdef\")");
  TEST_ASSERT(lizard_test_is_error(r));

  /* (load 42) — wrong argument type. */
  r = lizard_test_eval(&e, "(load 42)");
  TEST_ASSERT(lizard_test_is_error(r));

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
