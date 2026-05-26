/* tests/tt_check_test.c
 *
 * Tests for the bidirectional type checker for λΠ + Pi + one universe
 * with cumulativity.
 *
 * Coverage:
 *   - Universe inference: (U n) : (U n+1)
 *   - Pi formation
 *   - Lambda checking against Pi types
 *   - Variable lookup
 *   - Application using Pi types with substitution
 *   - Cumulativity: smaller universes check against larger
 *   - Rejection of ill-typed terms
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* Universe inference */
  r = lizard_test_eval(&e, "(infer (context) (U 0))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  r = lizard_test_eval(&e, "(infer (context) (U 7))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 8)");

  /* Variable lookup */
  lizard_test_eval(&e, "(define ctx (context (variable 'x (U 0))))");
  r = lizard_test_eval(&e, "(infer ctx 'x)");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");

  /* Pi formation: domain (U 0), codomain (U 0), so (Pi ...) : (U 0)
   * (after U-max-idem simplifies max((U 1), (U 1)) to (U 1)). */
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 0) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* Pi with higher codomain pushes universe up. */
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 0) (U 5)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 6)");

  /* Identity lambda checks against (Pi x (U 0) (U 0)) */
  r = lizard_test_eval(&e,
                       "(check (context) (Lambda 'x 'x) (Pi 'x (U 0) (U 0)))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* And against different binder names — alpha-equivalence: */
  r = lizard_test_eval(&e,
                       "(check (context) (Lambda 'y 'y) (Pi 'x (U 0) (U 0)))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* Unbound variable in a check — rejected */
  r = lizard_test_eval(&e, "(check (context) 'q (U 0))");
  TEST_ASSERT(lizard_test_is_false(r));

  /* Application: f : (Pi x A B), a : A, then (@ f a) : B[a/x] */
  lizard_test_eval(&e,
                   "(define ctx2 (context (variable 'f (Pi 'x (U 0) (U 0)))"
                   "                      (variable 'a (U 0))))");
  r = lizard_test_eval(&e, "(infer ctx2 (@ 'f 'a))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");

  /* Cumulativity: (U 0) : (U 1) by construction, but also : (U 5)
   * because (U 1) ≤ (U 5). */
  r = lizard_test_eval(&e, "(check (context) (U 0) (U 5))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* But NOT the reverse — (U 5) doesn't check against (U 0). */
  r = lizard_test_eval(&e, "(check (context) (U 5) (U 0))");
  TEST_ASSERT(lizard_test_is_false(r));

  /* Annotation: (annot t T) checks t against T and returns T */
  r = lizard_test_eval(&e,
                       "(infer (context) (annot (Lambda 'x 'x)"
                       "                         (Pi 'x (U 0) (U 0))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Pi (x (U 0)) (U 0))");

  /* Application of identity: ((Lambda x. x) v) where v has the right
   * type. We need a variable of type (U 0) in scope to feed it. */
  lizard_test_eval(&e,
                   "(define ctx_id (context (variable 't (U 0))))");
  r = lizard_test_eval(&e,
                       "(infer ctx_id "
                       "  (@ (annot (Lambda 'x 'x)"
                       "             (Pi 'x (U 0) (U 0)))"
                       "     't))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");

  /* And the negative case: passing (U 0) as argument fails because
   * (U 0) has type (U 1), not (U 0). */
  r = lizard_test_eval(&e,
                       "(check (context) "
                       "  (@ (annot (Lambda 'x 'x)"
                       "             (Pi 'x (U 0) (U 0)))"
                       "     (U 0))"
                       "  (U 0))");
  TEST_ASSERT(lizard_test_is_false(r));

  /* Type mismatch: Lambda checked against a non-Pi type */
  r = lizard_test_eval(&e,
                       "(check (context) (Lambda 'x 'x) (U 0))");
  TEST_ASSERT(lizard_test_is_false(r));

  /* ===== Id formation and refl checking ===== */

  /* Setup: ctx with A : (U 0), a : A, b : A */
  lizard_test_eval(&e,
                   "(define ctx_id (context (variable 'A (U 0))"
                   "                        (variable 'a 'A)"
                   "                        (variable 'b 'A)))");

  /* Id formation: (Id A a b) lives in A's universe */
  r = lizard_test_eval(&e, "(infer ctx_id (Id 'A 'a 'a))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");

  r = lizard_test_eval(&e, "(infer ctx_id (Id 'A 'a 'b))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");

  /* Id formation in (U 5) gives (U 5) by cumulativity */
  lizard_test_eval(&e,
                   "(define ctx_hi (context (variable 'T (U 5))"
                   "                        (variable 'x 'T)))");
  r = lizard_test_eval(&e, "(infer ctx_hi (Id 'T 'x 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 5)");

  /* refl: well-typed when point matches both endpoints */
  r = lizard_test_eval(&e,
                       "(check ctx_id (refl 'a) (Id 'A 'a 'a))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* refl: rejected when endpoints differ */
  r = lizard_test_eval(&e,
                       "(check ctx_id (refl 'a) (Id 'A 'a 'b))");
  TEST_ASSERT(lizard_test_is_false(r));

  /* refl: rejected when point doesn't match endpoints */
  r = lizard_test_eval(&e,
                       "(check ctx_id (refl 'b) (Id 'A 'a 'a))");
  TEST_ASSERT(lizard_test_is_false(r));

  /* refl: rejected against non-Id type */
  r = lizard_test_eval(&e,
                       "(check ctx_id (refl 'a) (U 0))");
  TEST_ASSERT(lizard_test_is_false(r));

  /* refl: rejected when the point isn't in scope */
  r = lizard_test_eval(&e,
                       "(check ctx_id (refl 'unbound) (Id 'A 'a 'a))");
  TEST_ASSERT(lizard_test_is_false(r));

  /* The HOTT-distinctive type: forall x : (U 0), Id (U 0) x x.
   * This is a real dependent type — for every type x, x equals x
   * in the type universe. It should be well-formed at level 1. */
  r = lizard_test_eval(&e,
                       "(infer (context) "
                       "  (Pi 'x (U 0) (Id (U 0) 'x 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* And the proof: (Lambda x. refl x) should check against it.
   * In the body, x : (U 0), so refl x : (Id (U 0) x x). */
  r = lizard_test_eval(&e,
                       "(check (context)"
                       "  (Lambda 'x (refl 'x))"
                       "  (Pi 'x (U 0) (Id (U 0) 'x 'x)))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* ===== Unit, tt, Bot ===== */
  r = lizard_test_eval(&e, "(infer (context) (Unit))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");
  r = lizard_test_eval(&e, "(infer (context) (tt))");
  TEST_ASSERT_STR(lizard_test_format(r), "Unit");
  r = lizard_test_eval(&e, "(check (context) (tt) (Unit))");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(infer (context) (Bot))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");

  /* ===== Sigma, Sum formation ===== */
  r = lizard_test_eval(&e, "(infer (context) (Sigma 'x (U 0) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");
  r = lizard_test_eval(&e, "(infer (context) (Sum (U 0) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* ===== Pair check ===== */
  lizard_test_eval(&e,
                   "(define ctx_AB2 (context (variable 'A (U 0))"
                   "                         (variable 'B (U 0))"
                   "                         (variable 'a 'A)"
                   "                         (variable 'b 'B)))");
  r = lizard_test_eval(&e,
                       "(check ctx_AB2 (Pair 'a 'b) (Sigma 'x 'A 'B))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* ===== inl, inr check ===== */
  r = lizard_test_eval(&e, "(check ctx_AB2 (inl 'a) (Sum 'A 'B))");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(check ctx_AB2 (inr 'b) (Sum 'A 'B))");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(check ctx_AB2 (inl 'a) (Sum 'B 'A))");
  TEST_ASSERT(lizard_test_is_false(r));

  /* ===== fst, snd inference ===== */
  lizard_test_eval(&e,
                   "(define ctx_p (context (variable 'A (U 0))"
                   "                       (variable 'B (U 0))"
                   "                       (variable 'p (Sigma 'x 'A 'B))))");
  r = lizard_test_eval(&e, "(infer ctx_p (fst 'p))");
  TEST_ASSERT_STR(lizard_test_format(r), "A");
  r = lizard_test_eval(&e, "(infer ctx_p (snd 'p))");
  TEST_ASSERT_STR(lizard_test_format(r), "B");

  /* ===== ap typing ===== */
  lizard_test_eval(&e,
                   "(define ctx_apf (context (variable 'A (U 0))"
                   "                         (variable 'B (U 0))"
                   "                         (variable 'f (Pi 'x 'A 'B))"
                   "                         (variable 'a1 'A)"
                   "                         (variable 'a2 'A)"
                   "                         (variable 'p (Id 'A 'a1 'a2))))");
  r = lizard_test_eval(&e, "(infer ctx_apf (ap 'f 'p))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Id B (@ f a1) (@ f a2))");

  /* ===== J typing — intricate ===== */
  lizard_test_eval(&e,
                   "(define ctx_J "
                   "  (context (variable 'A (U 0))"
                   "           (variable 'a 'A) (variable 'b 'A)"
                   "           (variable 'P (Pi 'x 'A (Pi 'p (Id 'A 'a 'x) (U 0))))"
                   "           (variable 'd (@ (@ 'P 'a) (refl 'a)))"
                   "           (variable 'q (Id 'A 'a 'b))))");
  r = lizard_test_eval(&e, "(infer ctx_J (J 'P 'd 'q))");
  TEST_ASSERT_STR(lizard_test_format(r), "(@ (@ P b) q)");

  /* ===== xport typing ===== */
  lizard_test_eval(&e,
                   "(define ctx_xp "
                   "  (context (variable 'A (U 0))"
                   "           (variable 'P (Pi 'x 'A (U 0)))"
                   "           (variable 'a 'A) (variable 'b 'A)"
                   "           (variable 'p (Id 'A 'a 'b))"
                   "           (variable 'v (@ 'P 'a))))");
  r = lizard_test_eval(&e, "(infer ctx_xp (xport 'P 'p 'v))");
  TEST_ASSERT_STR(lizard_test_format(r), "(@ P b)");

  /* ===== Case typing ===== */
  lizard_test_eval(&e,
                   "(define ctx_case "
                   "  (context (variable 'A (U 0)) (variable 'B (U 0))"
                   "           (variable 'C (U 0))"
                   "           (variable 's (Sum 'A 'B))"
                   "           (variable 'f (Pi 'a 'A 'C))"
                   "           (variable 'g (Pi 'b 'B 'C))))");
  r = lizard_test_eval(&e, "(infer ctx_case (Case 's 'f 'g))");
  TEST_ASSERT_STR(lizard_test_format(r), "C");

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
