/* tests/higher_order_test.c
 *
 * Higher-order function composition. These build a small functional
 * core (map, filter, fold-left, fold-right, range) and verify their
 * compositions agree with mathematical identities.
 */
#include "test_harness.h"
#include "test_helpers.h"

/* Set up shared helpers in the test environment once. Each later
 * eval can reference them by name since the env persists. */
static void install_prelude(lizard_test_env_t *e) {
  lizard_test_eval(
      e, "(define (map f xs)"
         "  (if (null? xs) '() (cons (f (car xs)) (map f (cdr xs)))))");
  lizard_test_eval(e,
                   "(define (filter p xs)"
                   "  (cond ((null? xs) '())"
                   "        ((p (car xs)) (cons (car xs) (filter p (cdr xs))))"
                   "        (else (filter p (cdr xs)))))");
  lizard_test_eval(
      e, "(define (fold-left f acc xs)"
         "  (if (null? xs) acc (fold-left f (f acc (car xs)) (cdr xs))))");
  lizard_test_eval(
      e, "(define (fold-right f acc xs)"
         "  (if (null? xs) acc (f (car xs) (fold-right f acc (cdr xs)))))");
  lizard_test_eval(e, "(define (range lo hi)"
                      "  (if (>= lo hi) '() (cons lo (range (+ lo 1) hi))))");
  lizard_test_eval(
      e, "(define (length xs) (if (null? xs) 0 (+ 1 (length (cdr xs)))))");
  lizard_test_eval(e, "(define (compose f g) (lambda (x) (f (g x))))");
  lizard_test_eval(e, "(define (id x) x)");
  lizard_test_eval(e, "(define (sum xs) (fold-left + 0 xs))");
  lizard_test_eval(e, "(define (product xs) (fold-left * 1 xs))");
}

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);
  install_prelude(&e);

  /* map preserves length. */
  r = lizard_test_eval(&e, "(length (map (lambda (x) (* x x)) (range 0 100)))");
  TEST_ASSERT(lizard_test_is_int(r, 100));

  /* sum 1..100 = 5050. */
  r = lizard_test_eval(&e, "(sum (range 1 101))");
  TEST_ASSERT(lizard_test_is_int(r, 5050));

  /* sum of first n squares = n(n+1)(2n+1)/6 — formula check for n=10. */
  r = lizard_test_eval(&e, "(sum (map (lambda (x) (* x x)) (range 1 11)))");
  TEST_ASSERT(lizard_test_is_int(r, 385));

  /* product of 1..10 = 10!. */
  r = lizard_test_eval(&e, "(product (range 1 11))");
  TEST_ASSERT(lizard_test_is_int(r, 3628800));

  /* filter even -> 50 evens between 0..99. */
  r = lizard_test_eval(&e, "(length (filter (lambda (x) (= (% x 2) 0))"
                           "                (range 0 100)))");
  TEST_ASSERT(lizard_test_is_int(r, 50));

  /* fold-left and fold-right agree for associative + commutative ops. */
  r = lizard_test_eval(&e, "(- (fold-left  + 0 (range 1 50))"
                           "   (fold-right + 0 (range 1 50)))");
  TEST_ASSERT(lizard_test_is_int(r, 0));

  /* But disagree for non-commutative (subtraction): */
  r = lizard_test_eval(&e, "(fold-left - 0 (list 1 2 3))");
  TEST_ASSERT(lizard_test_is_int(r, -6));
  r = lizard_test_eval(&e, "(fold-right - 0 (list 1 2 3))");
  TEST_ASSERT(lizard_test_is_int(r, 2));

  /* (compose f id) x = f x. */
  r = lizard_test_eval(&e, "(define inc (lambda (n) (+ n 1)))"
                           "((compose inc id) 41)");
  TEST_ASSERT(lizard_test_is_int(r, 42));

  /* Pipeline: filter odd, square, sum. */
  r = lizard_test_eval(&e, "(sum (map (lambda (x) (* x x))"
                           "          (filter (lambda (x) (= (% x 2) 1))"
                           "                  (range 1 11))))");
  TEST_ASSERT(lizard_test_is_int(r, 165)); /* 1+9+25+49+81 */

  /* map twice = compose. */
  r = lizard_test_eval(&e, "(define xs (range 0 50))"
                           "(- (sum (map inc (map inc xs)))"
                           "   (sum (map (compose inc inc) xs)))");
  TEST_ASSERT(lizard_test_is_int(r, 0));

  /* Functor law: map id = id (sums agree). */
  r = lizard_test_eval(&e, "(- (sum (map id xs)) (sum xs))");
  TEST_ASSERT(lizard_test_is_int(r, 0));

  /* Function passed as data through a list. */
  r = lizard_test_eval(&e, "(define fs (list (lambda (x) (+ x 1))"
                           "                 (lambda (x) (* x 2))"
                           "                 (lambda (x) (- x 3))))"
                           "(fold-left (lambda (v f) (f v)) 10 fs)");
  /* ((10+1)*2)-3 = 19 */
  TEST_ASSERT(lizard_test_is_int(r, 19));

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
