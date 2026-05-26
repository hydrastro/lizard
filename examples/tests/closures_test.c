/* tests/closures_test.c
 *
 * Closures with mutable state, currying, environment capture
 * across recursion, and the classic "make-counter / make-adder"
 * patterns. These exercise the closure->parent_env chain in ways
 * that simpler arithmetic tests don't.
 */
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* Counter generator — each call to make-counter produces a fresh
   * closure with its own private `n`. Two counters must not interfere. */
  r = lizard_test_eval(&e,
                       "(define (make-counter)"
                       "  (define n 0)"
                       "  (lambda ()"
                       "    (set! n (+ n 1))"
                       "    n))"
                       "(define c1 (make-counter))"
                       "(define c2 (make-counter))"
                       "(c1) (c1) (c1)"
                       "(c2)"
                       "(c1)");
  TEST_ASSERT(lizard_test_is_int(r, 4));

  r = lizard_test_eval(&e, "(c2)");
  TEST_ASSERT(lizard_test_is_int(r, 2));

  /* Currying: (((curry +) 3) 4) -> 7 */
  r = lizard_test_eval(&e,
                       "(define (curry f) (lambda (a) (lambda (b) (f a b))))"
                       "(((curry +) 3) 4)");
  TEST_ASSERT(lizard_test_is_int(r, 7));

  /* Partial application via lambda. */
  r = lizard_test_eval(&e,
                       "(define (make-adder n) (lambda (x) (+ x n)))"
                       "(define add100 (make-adder 100))"
                       "(define add-7 (make-adder -7))"
                       "(+ (add100 5) (add-7 50))");
  TEST_ASSERT(lizard_test_is_int(r, 148));

  /* Compose: (((compose f g) x) = (f (g x))). */
  r = lizard_test_eval(&e,
                       "(define (compose f g) (lambda (x) (f (g x))))"
                       "(define sq (lambda (x) (* x x)))"
                       "(define inc (lambda (x) (+ x 1)))"
                       "((compose sq inc) 4)");
  TEST_ASSERT(lizard_test_is_int(r, 25));   /* (4+1)^2 = 25 */

  r = lizard_test_eval(&e, "((compose inc sq) 4)");
  TEST_ASSERT(lizard_test_is_int(r, 17));   /* 4^2 + 1 = 17 */

  /* Closure captures by reference, not by value — set! in the enclosing
   * scope is visible from inside the closure even after definition. */
  r = lizard_test_eval(&e,
                       "(define x 10)"
                       "(define get-x (lambda () x))"
                       "(set! x 99)"
                       "(get-x)");
  TEST_ASSERT(lizard_test_is_int(r, 99));

  /* Accumulator: shared mutable state across two closures. */
  r = lizard_test_eval(&e,
                       "(define (make-account bal)"
                       "  (define (deposit amt) (set! bal (+ bal amt)) bal)"
                       "  (define (withdraw amt) (set! bal (- bal amt)) bal)"
                       "  (lambda (op amt)"
                       "    (if (= op 1) (deposit amt) (withdraw amt))))"
                       "(define acc (make-account 100))"
                       "(acc 1 50)"        /* +50 -> 150 */
                       "(acc 0 30)"        /* -30 -> 120 */
                       "(acc 1 5)");       /* + 5 -> 125 */
  TEST_ASSERT(lizard_test_is_int(r, 125));

  /* twice: apply f to itself.  twice(inc) 7 = 9 */
  r = lizard_test_eval(&e,
                       "(define (twice f) (lambda (x) (f (f x))))"
                       "((twice (lambda (n) (+ n 1))) 7)");
  TEST_ASSERT(lizard_test_is_int(r, 9));

  /* twice(twice(inc)) 7 = 11.  Composition of higher-order. */
  r = lizard_test_eval(&e,
                       "((twice (twice (lambda (n) (+ n 1)))) 7)");
  TEST_ASSERT(lizard_test_is_int(r, 11));

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
