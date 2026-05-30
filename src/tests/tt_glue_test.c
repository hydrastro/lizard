/* tests/tt_glue_test.c
 *
 * Tests for Glue types, equivalences, and ua (Turns 9 & 10).
 *
 * Coverage:
 *   - Glue boundary rules (F1, F0)
 *   - unglue (glue-intro ...) extracts the underlying A-element
 *   - equiv-fun/inv of id-equiv act as identity
 *   - ua of identity equivalence gives constant path
 *   - Equiv, id-equiv, equiv-fun/inv typing
 *   - Glue typing
 *   - ua typing — converts equivalence to Path in U
 *     *** This is the form of computational univalence ***
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* ===== Glue boundary rules ===== */
  r = lizard_test_eval(&e, "(reduce (Glue 'A (F1) 'T 'eq))");
  TEST_ASSERT_STR(lizard_test_format(r), "T");

  r = lizard_test_eval(&e, "(reduce (Glue 'A (F0) 'T 'eq))");
  TEST_ASSERT_STR(lizard_test_format(r), "A");

  /* ===== unglue of glue-intro ===== */
  r = lizard_test_eval(&e,
                       "(reduce (unglue 'eq (glue-intro (F1) 't 'a)))");
  TEST_ASSERT_STR(lizard_test_format(r), "a");

  /* ===== id-equiv reductions ===== */
  r = lizard_test_eval(&e, "(reduce (@ (equiv-fun (id-equiv 'A)) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "x");

  r = lizard_test_eval(&e, "(reduce (@ (equiv-inv (id-equiv 'A)) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "x");

  /* ===== ua-endpoint rule (Turn 10's key reduction) ===== */
  r = lizard_test_eval(&e, "(reduce (path-app (ua (id-equiv 'A)) (i0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "A");

  r = lizard_test_eval(&e, "(reduce (path-app (ua (id-equiv 'A)) (i1)))");
  TEST_ASSERT_STR(lizard_test_format(r), "A");

  r = lizard_test_eval(&e,
                       "(reduce (path-app (ua (id-equiv 'A)) (I-var 'i)))");
  TEST_ASSERT_STR(lizard_test_format(r), "A");

  /* ===== Typing rules ===== */
  lizard_test_eval(&e,
                   "(define ctx_e (context (variable 'A (U 0))"
                   "                       (variable 'B (U 0))"
                   "                       (variable 'eq (Equiv 'A 'B))))");

  /* Equiv : Universe */
  r = lizard_test_eval(&e, "(infer ctx_e (Equiv 'A 'B))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");

  /* id-equiv : Equiv A A */
  r = lizard_test_eval(&e, "(infer ctx_e (id-equiv 'A))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Equiv A A)");

  /* equiv-fun : Pi _ A B */
  r = lizard_test_eval(&e, "(infer ctx_e (equiv-fun 'eq))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Pi (_ A) B)");

  /* equiv-inv : Pi _ B A */
  r = lizard_test_eval(&e, "(infer ctx_e (equiv-inv 'eq))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Pi (_ B) A)");

  /* Glue : Universe */
  r = lizard_test_eval(&e, "(infer ctx_e (Glue 'A (F0) 'B 'eq))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");

  /* unglue : the codomain of the equivalence */
  r = lizard_test_eval(&e,
                       "(infer (context (variable 'eq (Equiv 'A 'B))"
                       "                (variable 'g 'B))"
                       "       (unglue 'eq 'g))");
  TEST_ASSERT_STR(lizard_test_format(r), "B");

  /* *** UA: the headline rule. *** */
  r = lizard_test_eval(&e, "(infer ctx_e (ua 'eq))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Path (U 0) A B)");

  /* ua of identity gives a path A to A. */
  r = lizard_test_eval(&e, "(infer ctx_e (ua (id-equiv 'A)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Path (U 0) A A)");

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
