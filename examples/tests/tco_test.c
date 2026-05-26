/* tests/tco_test.c
 *
 * Verifies tail-call optimization. Without TCO, every Scheme call
 * eats a C stack frame and a self-recursive function blows the
 * stack at depth ~10^4 (with default 8MB stack). With TCO, the
 * lambda body trampoline reuses one C frame, so the depth is
 * bounded by the heap only.
 *
 * This test counts down from 100_000 — comfortably past the
 * C-stack threshold but small enough to run in <1s. If TCO
 * regresses, this test segfaults; if anything else breaks, it
 * returns the wrong answer.
 */
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* Pure tail recursion — no bignum growth, only TCO matters. */
  r = lizard_test_eval(&e,
                       "(define (count n)"
                       "  (if (= n 0) 'done (count (- n 1))))"
                       "(count 100000)");
  TEST_ASSERT(lizard_test_is_symbol(r, "done"));

  /* Mutual tail recursion across two functions. */
  r = lizard_test_eval(&e,
                       "(define (even? n) (if (= n 0) #t (odd?  (- n 1))))"
                       "(define (odd?  n) (if (= n 0) #f (even? (- n 1))))"
                       "(even? 50000)");
  TEST_ASSERT(lizard_test_is_true(r));

  /* Tail call out of a cond clause. */
  r = lizard_test_eval(&e,
                       "(define (down n)"
                       "  (cond ((= n 0) 0)"
                       "        (else (down (- n 1)))))"
                       "(down 50000)");
  TEST_ASSERT(lizard_test_is_int(r, 0));

  /* Tail call as the last expression of a (begin ...) body. */
  r = lizard_test_eval(&e,
                       "(define (loop n)"
                       "  (begin"
                       "    (= n n)"
                       "    (if (= n 0) 'k (loop (- n 1)))))"
                       "(loop 20000)");
  TEST_ASSERT(lizard_test_is_symbol(r, "k"));

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
