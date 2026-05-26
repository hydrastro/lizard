/* tests/quasiquote_test.c
 *
 * Quasiquote / unquote / unquote-splicing — the hardest part of any
 * Scheme implementation to get right. These tests cover edge cases
 * around splicing into different positions, splicing empty/nil,
 * deep-nested substitution, and quasiquote inside macro bodies.
 */
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* Trivial literal — no unquote. */
  r = lizard_test_eval(&e, "`(a b c)");
  TEST_ASSERT_STR(lizard_test_format(r), "(a b c)");

  /* Single unquote. */
  r = lizard_test_eval(&e, "(define x 7) `(a ,x c)");
  TEST_ASSERT_STR(lizard_test_format(r), "(a 7 c)");

  /* Multiple unquotes. */
  r = lizard_test_eval(&e, "`(,x ,x ,x)");
  TEST_ASSERT_STR(lizard_test_format(r), "(7 7 7)");

  /* Unquote at front, middle, and end. */
  r = lizard_test_eval(&e, "`(,x b c)");
  TEST_ASSERT_STR(lizard_test_format(r), "(7 b c)");
  r = lizard_test_eval(&e, "`(a b ,x)");
  TEST_ASSERT_STR(lizard_test_format(r), "(a b 7)");

  /* Splice in middle. */
  r = lizard_test_eval(&e, "(define xs '(p q r)) `(a ,@xs b)");
  TEST_ASSERT_STR(lizard_test_format(r), "(a p q r b)");

  /* Splice at front. */
  r = lizard_test_eval(&e, "`(,@xs end)");
  TEST_ASSERT_STR(lizard_test_format(r), "(p q r end)");

  /* Splice at end. */
  r = lizard_test_eval(&e, "`(start ,@xs)");
  TEST_ASSERT_STR(lizard_test_format(r), "(start p q r)");

  /* Splice an empty list — should produce no elements. */
  r = lizard_test_eval(&e, "`(a ,@'() b)");
  TEST_ASSERT_STR(lizard_test_format(r), "(a b)");

  /* Splice the only element. */
  r = lizard_test_eval(&e, "`(,@xs)");
  TEST_ASSERT_STR(lizard_test_format(r), "(p q r)");

  /* Splice with computed list. */
  r = lizard_test_eval(&e, "`(a ,@(list 1 2 3) b)");
  TEST_ASSERT_STR(lizard_test_format(r), "(a 1 2 3 b)");

  /* Two splices in one quasiquote. */
  r = lizard_test_eval(&e, "(define ys '(1 2)) (define zs '(8 9))"
                           "`(,@ys mid ,@zs)");
  TEST_ASSERT_STR(lizard_test_format(r), "(1 2 mid 8 9)");

  /* Unquote of an expression, not just a variable. */
  r = lizard_test_eval(&e, "`(a ,(+ 10 5) c)");
  TEST_ASSERT_STR(lizard_test_format(r), "(a 15 c)");

  /* Unquote evaluating to a list — should appear as one element. */
  r = lizard_test_eval(&e, "`(a ,xs b)");
  TEST_ASSERT_STR(lizard_test_format(r), "(a (p q r) b)");

  /* Splice of the same list — appears as multiple elements. */
  r = lizard_test_eval(&e, "`(a ,@xs b)");
  TEST_ASSERT_STR(lizard_test_format(r), "(a p q r b)");

  /* Quasiquote with no unquotes inside a macro body. */
  r = lizard_test_eval(&e, "(define-syntax greet (lambda () '(hello world)))"
                           "(greet)");
  TEST_ASSERT_STR(lizard_test_format(r), "(hello world)");

  /* Macro using quasiquote that substitutes its argument. */
  r = lizard_test_eval(&e, "(define-syntax twice (lambda (x) `(* 2 ,x)))"
                           "(twice 21)");
  TEST_ASSERT(lizard_test_is_int(r, 42));

  /* Macro that builds a function call from a list-shaped argument. */
  r = lizard_test_eval(&e, "(define-syntax pair (lambda (a b) `(cons ,a ,b)))"
                           "(pair 1 2)");
  TEST_ASSERT_STR(lizard_test_format(r), "(1 . 2)");

  /* Macro expanding to an if. */
  r = lizard_test_eval(&e,
                       "(define-syntax unless (lambda (c b) `(if ,c #f ,b)))"
                       "(unless #f 'fire)");
  TEST_ASSERT(lizard_test_is_symbol(r, "fire"));

  /* Macro expanding to a lambda. */
  r = lizard_test_eval(&e, "(define-syntax make-doubler"
                           "  (lambda () `(lambda (x) (* x 2))))"
                           "((make-doubler) 21)");
  TEST_ASSERT(lizard_test_is_int(r, 42));

  /* The classic let-via-macro pattern. */
  r = lizard_test_eval(&e, "(define-syntax bind"
                           "  (lambda (name value body)"
                           "    `(let ((,name ,value)) ,body)))"
                           "(bind n 10 (* n n))");
  TEST_ASSERT(lizard_test_is_int(r, 100));

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
