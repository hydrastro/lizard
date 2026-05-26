/* tests/lambda_test.c
 *
 * Lambdas, recursion, multi-body defines, mutual recursion, bignums.
 */
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* Plain lambda + immediate application. */
  r = lizard_test_eval(&e, "((lambda (x) (* x x)) 7)");
  TEST_ASSERT(lizard_test_is_int(r, 49));

  /* define + recursion. */
  r = lizard_test_eval(&e, "(define (fact n)"
                           "  (if (= n 0) 1 (* n (fact (- n 1)))))"
                           "(fact 10)");
  TEST_ASSERT(lizard_test_is_int(r, 3628800));

  /* Multi-body define — used to fail with 'missing closing paren'. */
  r = lizard_test_eval(&e, "(define c 0)"
                           "(define (bump)"
                           "  (set! c (+ c 1))"
                           "  (set! c (+ c 10))"
                           "  c)"
                           "(bump)");
  TEST_ASSERT(lizard_test_is_int(r, 11));

  /* Mutual recursion. */
  r = lizard_test_eval(&e, "(define (even? n) (if (= n 0) #t (odd?  (- n 1))))"
                           "(define (odd?  n) (if (= n 0) #f (even? (- n 1))))"
                           "(even? 100)");
  TEST_ASSERT(lizard_test_is_true(r));

  /* Bignum — 20! is well past 64-bit. */
  r = lizard_test_eval(&e, "(define (fact n)"
                           "  (if (= n 0) 1 (* n (fact (- n 1)))))"
                           "(fact 20)");
  TEST_ASSERT_STR(lizard_test_format(r), "2432902008176640000");

  /* fact(30) — definitely a bignum. */
  r = lizard_test_eval(&e, "(fact 30)");
  TEST_ASSERT_STR(lizard_test_format(r), "265252859812191058636308480000000");

  /* Closures capture their environment. */
  r = lizard_test_eval(&e, "(define (make-adder n) (lambda (x) (+ x n)))"
                           "(define add5 (make-adder 5))"
                           "(add5 10)");
  TEST_ASSERT(lizard_test_is_int(r, 15));

  /* Unbound symbol -> error (NOT 'attempt to apply a non-function'). */
  r = lizard_test_eval(&e, "(undefined-function-name 1 2)");
  TEST_ASSERT(lizard_test_is_error(r));

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
