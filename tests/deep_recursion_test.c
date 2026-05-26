/* tests/deep_recursion_test.c
 *
 * Stress-test tail-call optimisation at extreme depth and through
 * all the forms that should preserve tail position: lambda body
 * tail, if tail, cond tail, begin tail, let body tail, mutual
 * recursion across both branches of an if.
 *
 * Pre-TCO any of these would smash the C stack at ~10^4 depth;
 * post-TCO they should complete in O(1) C frames.
 */
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* 500,000-iteration plain tail recursion. */
  r = lizard_test_eval(&e,
                       "(define (count n)"
                       "  (if (= n 0) n (count (- n 1))))"
                       "(count 500000)");
  TEST_ASSERT(lizard_test_is_int(r, 0));

  /* Tail through cond/else. */
  r = lizard_test_eval(&e,
                       "(define (down n)"
                       "  (cond ((= n 0) 'k)"
                       "        (else (down (- n 1)))))"
                       "(down 200000)");
  TEST_ASSERT(lizard_test_is_symbol(r, "k"));

  /* Tail through multi-clause cond. */
  r = lizard_test_eval(&e,
                       "(define (down n)"
                       "  (cond ((< n 0) 'never)"
                       "        ((= n 0) 'done)"
                       "        ((> n 0) (down (- n 1)))))"
                       "(down 100000)");
  TEST_ASSERT(lizard_test_is_symbol(r, "done"));

  /* Tail as last expr of begin. */
  r = lizard_test_eval(&e,
                       "(define (loop n)"
                       "  (begin (= n n)"
                       "         (= n n)"
                       "         (if (= n 0) 'k (loop (- n 1)))))"
                       "(loop 50000)");
  TEST_ASSERT(lizard_test_is_symbol(r, "k"));

  /* Tail through let body. */
  r = lizard_test_eval(&e,
                       "(define (loop n)"
                       "  (let ((m (- n 1)))"
                       "    (if (< n 1) 'k (loop m))))"
                       "(loop 50000)");
  TEST_ASSERT(lizard_test_is_symbol(r, "k"));

  /* Mutual recursion across both branches. */
  r = lizard_test_eval(&e,
                       "(define (even? n) (if (= n 0) #t (odd?  (- n 1))))"
                       "(define (odd?  n) (if (= n 0) #f (even? (- n 1))))"
                       "(even? 200000)");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(odd? 200001)");
  TEST_ASSERT(lizard_test_is_true(r));

  /* Accumulator-style — fast iterative factorial via TCO loop.
   * Computes 1000! and just checks it's a real (big) number. */
  r = lizard_test_eval(&e,
                       "(define (fact-iter n acc)"
                       "  (if (= n 0) acc (fact-iter (- n 1) (* acc n))))"
                       "(define f (fact-iter 1000 1))"
                       "(number? f)");
  TEST_ASSERT(lizard_test_is_true(r));

  /* Verify the iterative factorial's first/last digits match the
   * recursive one. 1000! has a known form so we just compare two
   * tail-recursive variants. */
  r = lizard_test_eval(&e,
                       "(define (fact-iter2 n acc)"
                       "  (if (= n 1) acc (fact-iter2 (- n 1) (* acc n))))"
                       "(- (fact-iter 100 1) (fact-iter2 100 1))");
  TEST_ASSERT(lizard_test_is_int(r, 0));

  /* A tail-recursive sum that goes through let, if, and recursive
   * call all in tail position. sum-iter 1 10000 = 50005000. */
  r = lizard_test_eval(&e,
                       "(define (sum-iter k n acc)"
                       "  (if (> k n) acc"
                       "      (let ((k+1 (+ k 1)))"
                       "        (sum-iter k+1 n (+ acc k)))))"
                       "(sum-iter 1 10000 0)");
  TEST_ASSERT(lizard_test_is_int(r, 50005000));

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
