/* tests/control_test.c
 *
 * if / cond / begin / and / or / not / let — the bread and butter.
 */
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* if both branches. */
  r = lizard_test_eval(&e, "(if #t 1 2)");
  TEST_ASSERT(lizard_test_is_int(r, 1));
  r = lizard_test_eval(&e, "(if #f 1 2)");
  TEST_ASSERT(lizard_test_is_int(r, 2));
  r = lizard_test_eval(&e, "(if (= 1 1) 'a 'b)");
  TEST_ASSERT(lizard_test_is_symbol(r, "a"));

  /* cond — was a missing eval case before the fix. */
  r = lizard_test_eval(&e, "(cond (#t 1))");
  TEST_ASSERT(lizard_test_is_int(r, 1));
  r = lizard_test_eval(&e, "(cond (#f 1) (#t 2))");
  TEST_ASSERT(lizard_test_is_int(r, 2));
  r = lizard_test_eval(&e, "(cond (#f 1) (#f 2) (else 99))");
  TEST_ASSERT(lizard_test_is_int(r, 99));
  /* No matching clause -> nil. */
  r = lizard_test_eval(&e, "(cond (#f 1))");
  TEST_ASSERT(r && r->type == AST_NIL);

  /* begin — must run every expr once, return last. */
  r = lizard_test_eval(&e,
                       "(define c 0)"
                       "(begin (set! c (+ c 1))"
                       "       (set! c (+ c 1))"
                       "       (set! c (+ c 1)))"
                       "c");
  TEST_ASSERT(lizard_test_is_int(r, 3));

  /* and/or short-circuit, return the deciding value. */
  r = lizard_test_eval(&e, "(and 1 2 3)");
  TEST_ASSERT(lizard_test_is_int(r, 3));
  r = lizard_test_eval(&e, "(and 1 #f 3)");
  TEST_ASSERT(lizard_test_is_false(r));
  r = lizard_test_eval(&e, "(or #f #f 7)");
  TEST_ASSERT(lizard_test_is_int(r, 7));
  r = lizard_test_eval(&e, "(or #f #f #f)");
  TEST_ASSERT(lizard_test_is_false(r));

  /* not. */
  r = lizard_test_eval(&e, "(not #t)");
  TEST_ASSERT(lizard_test_is_false(r));
  r = lizard_test_eval(&e, "(not #f)");
  TEST_ASSERT(lizard_test_is_true(r));

  /* let. */
  r = lizard_test_eval(&e, "(let ((a 3) (b 4)) (+ (* a a) (* b b)))");
  TEST_ASSERT(lizard_test_is_int(r, 25));

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
