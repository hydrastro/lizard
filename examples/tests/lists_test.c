/* tests/lists_test.c
 *
 * cons / car / cdr / list / quote / null? / pair?.
 */
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* cons of two numbers — dotted pair. */
  r = lizard_test_eval(&e, "(cons 1 2)");
  TEST_ASSERT_STR(lizard_test_format(r), "(1 . 2)");

  /* Proper list via repeated cons. */
  r = lizard_test_eval(&e, "(cons 1 (cons 2 (cons 3 '())))");
  TEST_ASSERT_STR(lizard_test_format(r), "(1 2 3)");

  /* list shorthand. */
  r = lizard_test_eval(&e, "(list 1 2 3)");
  TEST_ASSERT_STR(lizard_test_format(r), "(1 2 3)");

  /* car / cdr. */
  r = lizard_test_eval(&e, "(car '(a b c))");
  TEST_ASSERT(lizard_test_is_symbol(r, "a"));
  r = lizard_test_eval(&e, "(cdr '(a b c))");
  TEST_ASSERT_STR(lizard_test_format(r), "(b c)");

  /* Predicates — pair? recognizes AST_PAIR (post-eval form). */
  r = lizard_test_eval(&e, "(pair? (cons 1 2))");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(pair? '(1 2))");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(pair? '())");
  TEST_ASSERT(lizard_test_is_false(r));
  r = lizard_test_eval(&e, "(null? '())");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(null? '(1))");
  TEST_ASSERT(lizard_test_is_false(r));

  /* Quote with nested list. */
  r = lizard_test_eval(&e, "'(a (b c) d)");
  TEST_ASSERT_STR(lizard_test_format(r), "(a (b c) d)");

  /* User-defined map. */
  r = lizard_test_eval(&e,
                       "(define (my-map f xs)"
                       "  (if (null? xs)"
                       "      '()"
                       "      (cons (f (car xs)) (my-map f (cdr xs)))))"
                       "(my-map (lambda (x) (* x x)) '(1 2 3 4 5))");
  TEST_ASSERT_STR(lizard_test_format(r), "(1 4 9 16 25)");

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
