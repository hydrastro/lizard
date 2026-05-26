/* tests/macros_test.c
 *
 * define-syntax + quasiquote + unquote + unquote-splicing.
 */
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* Trivial unquote inside quasiquote. */
  r = lizard_test_eval(&e, "`(a ,(+ 1 1) c)");
  TEST_ASSERT_STR(lizard_test_format(r), "(a 2 c)");

  /* Splice. */
  r = lizard_test_eval(&e, "(define xs '(b c d))"
                           "`(a ,@xs e)");
  TEST_ASSERT_STR(lizard_test_format(r), "(a b c d e)");

  /* Splice an empty list. */
  r = lizard_test_eval(&e, "`(a ,@'() b)");
  TEST_ASSERT_STR(lizard_test_format(r), "(a b)");

  /* Simple macro returning a literal. */
  r = lizard_test_eval(&e, "(define-syntax always-42 (lambda () 42))"
                           "(always-42)");
  TEST_ASSERT(lizard_test_is_int(r, 42));

  /* Macro that builds an application via quasiquote. */
  r = lizard_test_eval(&e, "(define-syntax double (lambda (x) `(* ,x 2)))"
                           "(double 21)");
  TEST_ASSERT(lizard_test_is_int(r, 42));

  /* Macro whose body is a special form (if). */
  r = lizard_test_eval(&e, "(define-syntax my-when"
                           "  (lambda (t b) `(if ,t ,b #f)))"
                           "(my-when #t (+ 100 23))");
  TEST_ASSERT(lizard_test_is_int(r, 123));

  r = lizard_test_eval(&e, "(my-when #f 99)");
  TEST_ASSERT(lizard_test_is_false(r));

  /* Macro expanding to a let. */
  r = lizard_test_eval(&e, "(define-syntax with"
                           "  (lambda (name value body)"
                           "    `(let ((,name ,value)) ,body)))"
                           "(with x 10 (* x x))");
  TEST_ASSERT(lizard_test_is_int(r, 100));

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
