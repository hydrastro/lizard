/* tests/tt_faces_test.c
 *
 * Tests for the face algebra (Turn 7).
 *
 * Coverage:
 *   - Face concrete reduction (F-eq on endpoints)
 *   - Boolean face algebra (F-and, F-or with F0/F1)
 *   - Connection-on-face equations (De Morgan-style)
 *   - Face entailment decision procedure
 *   - Partial and Sub typing
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* ===== F-eq concrete reduction ===== */
  r = lizard_test_eval(&e, "(reduce (F-eq (i0) (i0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "F1");
  r = lizard_test_eval(&e, "(reduce (F-eq (i1) (i1)))");
  TEST_ASSERT_STR(lizard_test_format(r), "F1");
  r = lizard_test_eval(&e, "(reduce (F-eq (i0) (i1)))");
  TEST_ASSERT_STR(lizard_test_format(r), "F0");
  r = lizard_test_eval(&e, "(reduce (F-eq (i1) (i0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "F0");

  /* F-eq reflexivity */
  r = lizard_test_eval(&e, "(reduce (F-eq (I-var 'i) (I-var 'i)))");
  TEST_ASSERT_STR(lizard_test_format(r), "F1");

  /* ===== Boolean face algebra ===== */
  r = lizard_test_eval(&e, "(reduce (F-and (F1) (F-eq (I-var 'i) (i0))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(F-eq (I-var i) i0)");
  r = lizard_test_eval(&e, "(reduce (F-and (F0) (F-eq (I-var 'i) (i0))))");
  TEST_ASSERT_STR(lizard_test_format(r), "F0");
  r = lizard_test_eval(&e,
                       "(reduce (F-and (F-eq (I-var 'i) (i0))"
                       "               (F-eq (I-var 'i) (i0))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(F-eq (I-var i) i0)");

  r = lizard_test_eval(&e, "(reduce (F-or (F1) (F-eq (I-var 'i) (i0))))");
  TEST_ASSERT_STR(lizard_test_format(r), "F1");
  r = lizard_test_eval(&e, "(reduce (F-or (F0) (F-eq (I-var 'i) (i0))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(F-eq (I-var i) i0)");

  /* ===== Connection-on-face equations ===== */

  /* (F-eq (i ∧ j) i0) --> (F-or (F-eq i i0) (F-eq j i0)) */
  r = lizard_test_eval(&e,
                       "(reduce (F-eq (I-and (I-var 'i) (I-var 'j)) (i0)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(F-or (F-eq (I-var i) i0) (F-eq (I-var j) i0))");

  /* (F-eq (i ∧ j) i1) --> (F-and (F-eq i i1) (F-eq j i1)) */
  r = lizard_test_eval(&e,
                       "(reduce (F-eq (I-and (I-var 'i) (I-var 'j)) (i1)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(F-and (F-eq (I-var i) i1) (F-eq (I-var j) i1))");

  /* (F-eq (i ∨ j) i0) --> (F-and (F-eq i i0) (F-eq j i0)) */
  r = lizard_test_eval(&e,
                       "(reduce (F-eq (I-or (I-var 'i) (I-var 'j)) (i0)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(F-and (F-eq (I-var i) i0) (F-eq (I-var j) i0))");

  /* (F-eq (~ i) i0) --> (F-eq i i1) */
  r = lizard_test_eval(&e, "(reduce (F-eq (I-neg (I-var 'i)) (i0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(F-eq (I-var i) i1)");

  /* (F-eq (~ i) i1) --> (F-eq i i0) */
  r = lizard_test_eval(&e, "(reduce (F-eq (I-neg (I-var 'i)) (i1)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(F-eq (I-var i) i0)");

  /* ===== Face entailment ===== */
  r = lizard_test_eval(&e, "(face-entails? (F0) (F-eq (I-var 'i) (i0)))");
  TEST_ASSERT(lizard_test_is_true(r));

  r = lizard_test_eval(&e, "(face-entails? (F-eq (I-var 'i) (i0)) (F1))");
  TEST_ASSERT(lizard_test_is_true(r));

  r = lizard_test_eval(&e,
                       "(face-entails? (F-eq (I-var 'i) (i0))"
                       "               (F-eq (I-var 'i) (i0)))");
  TEST_ASSERT(lizard_test_is_true(r));

  r = lizard_test_eval(&e,
                       "(face-entails? "
                       "  (F-and (F-eq (I-var 'i) (i0)) (F-eq (I-var 'j) (i1)))"
                       "  (F-eq (I-var 'i) (i0)))");
  TEST_ASSERT(lizard_test_is_true(r));

  r = lizard_test_eval(&e,
                       "(face-entails? "
                       "  (F-eq (I-var 'i) (i0))"
                       "  (F-or (F-eq (I-var 'i) (i0)) (F-eq (I-var 'j) (i1))))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* ===== Face typing ===== */
  r = lizard_test_eval(&e, "(infer (context) (F0))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");

  r = lizard_test_eval(&e, "(infer (context) (F1))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");

  r = lizard_test_eval(&e, "(infer (context) (F-eq (i0) (i1)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");

  r = lizard_test_eval(&e, "(infer (context) (F-and (F0) (F1)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");

  /* ===== Partial typing ===== */
  lizard_test_eval(&e, "(define ctx_p (context (variable 'A (U 0))))");
  r = lizard_test_eval(&e, "(infer ctx_p (Partial (F1) 'A))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
