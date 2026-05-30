/* tests/tt_cubical_test.c
 *
 * Tests for the cubical type theory layer:
 *   - Interval algebra: I-and, I-or, I-neg with their equations
 *   - Path-app beta: ((<i> body) @ j) → body[j/i]
 *   - Interval typing (i0, i1, I-var : I)
 *   - Path formation
 *   - path-abs checking against Path with endpoint verification
 *   - path-app typing
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* ===== Interval algebra ===== */

  /* I-and: zero absorbs */
  r = lizard_test_eval(&e, "(reduce (I-and (i0) (I-var 'i)))");
  TEST_ASSERT_STR(lizard_test_format(r), "i0");
  r = lizard_test_eval(&e, "(reduce (I-and (I-var 'i) (i0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "i0");

  /* I-and: one is identity */
  r = lizard_test_eval(&e, "(reduce (I-and (i1) (I-var 'i)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(I-var i)");
  r = lizard_test_eval(&e, "(reduce (I-and (I-var 'i) (i1)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(I-var i)");

  /* I-and: idempotence */
  r = lizard_test_eval(&e, "(reduce (I-and (I-var 'i) (I-var 'i)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(I-var i)");

  /* I-or: one absorbs */
  r = lizard_test_eval(&e, "(reduce (I-or (i1) (I-var 'i)))");
  TEST_ASSERT_STR(lizard_test_format(r), "i1");

  /* I-or: zero is identity */
  r = lizard_test_eval(&e, "(reduce (I-or (i0) (I-var 'i)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(I-var i)");

  /* I-neg: endpoints */
  r = lizard_test_eval(&e, "(reduce (I-neg (i0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "i1");
  r = lizard_test_eval(&e, "(reduce (I-neg (i1)))");
  TEST_ASSERT_STR(lizard_test_format(r), "i0");

  /* I-neg: involution */
  r = lizard_test_eval(&e, "(reduce (I-neg (I-neg (I-var 'i))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(I-var i)");

  /* ===== Path-app beta ===== */

  /* (<i> i) @ i0 = i0 */
  r = lizard_test_eval(&e, "(reduce (path-app (path-abs 'i (I-var 'i)) (i0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "i0");

  /* (<i> i) @ i1 = i1 */
  r = lizard_test_eval(&e, "(reduce (path-app (path-abs 'i (I-var 'i)) (i1)))");
  TEST_ASSERT_STR(lizard_test_format(r), "i1");

  /* (<i> body-with-no-i) @ j = body */
  r = lizard_test_eval(&e, "(reduce (path-app (path-abs 'i 'body) (I-var 'j)))");
  TEST_ASSERT_STR(lizard_test_format(r), "body");

  /* Compound: substitute and simplify */
  r = lizard_test_eval(&e,
                       "(reduce (path-app (path-abs 'i "
                       "                    (I-or (I-neg (I-var 'i)) (I-var 'i)))"
                       "                  (i0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "i1");

  /* ===== Cubical typing ===== */

  /* Interval pre-type and endpoints */
  r = lizard_test_eval(&e, "(infer (context) (I))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");
  r = lizard_test_eval(&e, "(infer (context) (i0))");
  TEST_ASSERT_STR(lizard_test_format(r), "I");
  r = lizard_test_eval(&e, "(infer (context) (i1))");
  TEST_ASSERT_STR(lizard_test_format(r), "I");

  /* I-and, I-or, I-neg type-check on intervals */
  r = lizard_test_eval(&e, "(infer (context) (I-and (i0) (i1)))");
  TEST_ASSERT_STR(lizard_test_format(r), "I");
  r = lizard_test_eval(&e, "(infer (context) (I-neg (i0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "I");

  /* Path formation */
  lizard_test_eval(&e,
                   "(define ctx_p (context (variable 'A (U 0))"
                   "                       (variable 'a 'A)))");
  r = lizard_test_eval(&e, "(infer ctx_p (Path 'A 'a 'a))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");

  /* The constant path: (<i> a) : (Path A a a) */
  r = lizard_test_eval(&e,
                       "(check ctx_p (path-abs 'i 'a) (Path 'A 'a 'a))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* Constant path does NOT check against Path A a b when a≠b */
  lizard_test_eval(&e,
                   "(define ctx_pab (context (variable 'A (U 0))"
                   "                         (variable 'a 'A)"
                   "                         (variable 'b 'A)))");
  r = lizard_test_eval(&e,
                       "(check ctx_pab (path-abs 'i 'a) (Path 'A 'a 'b))");
  TEST_ASSERT(lizard_test_is_false(r));

  /* Path application yields the path's domain */
  lizard_test_eval(&e,
                   "(define ctx_papp (context (variable 'A (U 0))"
                   "                          (variable 'a 'A)"
                   "                          (variable 'p (Path 'A 'a 'a))))");
  r = lizard_test_eval(&e, "(infer ctx_papp (path-app 'p (i0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "A");

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
