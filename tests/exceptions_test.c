/* tests/exceptions_test.c
 *
 * (raise value), (try thunk handler), (error-object? x), (error-value x),
 * (gensym [prefix]).
 */
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* try with no error — thunk's result returned unchanged. */
  r = lizard_test_eval(&e, "(try (lambda () (+ 1 2))"
                           "     (lambda (err) 'never-runs))");
  TEST_ASSERT(lizard_test_is_int(r, 3));

  /* try catches a user-raised error and the handler sees the payload. */
  r = lizard_test_eval(&e, "(try (lambda () (raise 'oops))"
                           "     (lambda (err) (error-value err)))");
  TEST_ASSERT(lizard_test_is_symbol(r, "oops"));

  /* The error payload can be any value. */
  r = lizard_test_eval(&e, "(try (lambda () (raise 42))"
                           "     (lambda (err) (error-value err)))");
  TEST_ASSERT(lizard_test_is_int(r, 42));

  r = lizard_test_eval(&e, "(try (lambda () (raise (list 'tag 1 2 3)))"
                           "     (lambda (err) (car (error-value err))))");
  TEST_ASSERT(lizard_test_is_symbol(r, "tag"));

  /* try catches system errors too (division by zero). */
  r = lizard_test_eval(&e, "(try (lambda () (/ 10 0))"
                           "     (lambda (err) 'caught))");
  TEST_ASSERT(lizard_test_is_symbol(r, "caught"));

  /* error-object? */
  r = lizard_test_eval(&e, "(try (lambda () (raise 'x))"
                           "     (lambda (err) (error-object? err)))");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(error-object? 42)");
  TEST_ASSERT(lizard_test_is_false(r));
  r = lizard_test_eval(&e, "(error-object? 'foo)");
  TEST_ASSERT(lizard_test_is_false(r));

  /* try where the handler ITSELF raises — that should also propagate. */
  r = lizard_test_eval(&e, "(try (lambda ()"
                           "       (try (lambda () (raise 'inner))"
                           "            (lambda (err) (raise 'outer))))"
                           "     (lambda (err) (error-value err)))");
  TEST_ASSERT(lizard_test_is_symbol(r, "outer"));

  /* gensym — two calls produce distinct symbols. */
  r = lizard_test_eval(&e, "(define g1 (gensym))"
                           "(define g2 (gensym))"
                           "(not (= g1 g2))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* gensym with a prefix. */
  r = lizard_test_eval(&e, "(define s (symbol->string (gensym \"counter-\")))"
                           "(string=? (substring s 0 8) \"counter-\")");
  TEST_ASSERT(lizard_test_is_true(r));

  /* Errors in try arguments. */
  r = lizard_test_eval(&e, "(try)");
  TEST_ASSERT(lizard_test_is_error(r));
  r = lizard_test_eval(&e, "(try (lambda () 1))");
  TEST_ASSERT(lizard_test_is_error(r));
  r = lizard_test_eval(&e, "(try 42 (lambda (e) e))"); /* thunk not a proc */
  TEST_ASSERT(lizard_test_is_error(r));

  /* raise with no args is an error. */
  r = lizard_test_eval(&e, "(raise)");
  TEST_ASSERT(lizard_test_is_error(r));

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
