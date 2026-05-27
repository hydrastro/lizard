/* tests/tt_truncation_test.c
 *
 * Tests for Phase H.2 — propositional truncation, promoted from
 * scaffold to checked typing rules with a primary computation rule.
 *
 * What this turn ships (on the merged AST shape):
 *   1. (Trunc level A) type former at the universe of A.
 *   2. (trunc e) constructor; result type is (Trunc <level> A) with
 *      level left NULL (inference doesn't synthesize a level from
 *      bare trunc). Prints as (Trunc A).
 *   3. (trunc-elim C h e) eliminator:
 *        - C must be a type
 *        - e must be Trunc-typed (Trunc _ A) for some A
 *        - h must be of type (Pi _ A C)
 *        - Result type: C
 *   4. Primary computation rule (deterministic):
 *        (trunc-elim C h (trunc x))  ⟶  (@ h x)
 *      No overlap with any other reduction.
 *
 * Honest gap: propositionality obligation on the motive C is not
 * structurally enforced (see docs/CLAIMS_MATRIX.md).
 *
 * All operations gated on the `truncations-enabled` logic rule (via
 * (set-logic 'truncations) or its bundle).
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);
  lizard_logic_config_reset();

  /* Enable truncations. */
  lizard_test_eval(&e, "(set-logic 'truncations)");

  /* === Type former === */

  r = lizard_test_eval(&e, "(infer (context) (Trunc 0 (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  r = lizard_test_eval(&e, "(infer (context) (Trunc -1 (U 5)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 6)");

  /* Type former preserves universe regardless of level value. */
  r = lizard_test_eval(&e, "(infer (context) (Trunc 7 (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* === Constructor === */

  /* (trunc value): infers underlying type from value, leaves level NULL. */
  lizard_test_eval(&e,
    "(define ctxv (context-extend (context) (variable 'x (U 0))))");
  r = lizard_test_eval(&e, "(infer ctxv (trunc 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Trunc (U 0))");

  /* === Eliminator === */

  /* Canonical setup: A, C, h : Pi _ A C, v : Trunc 0 A */
  lizard_test_eval(&e,
    "(define ctxe "
    "  (context-extend "
    "    (context-extend "
    "      (context-extend "
    "        (context-extend (context) "
    "          (variable 'A (U 0))) "
    "        (variable 'C (U 0))) "
    "      (variable 'h (Pi '_ 'A 'C))) "
    "    (variable 'v (Trunc 0 'A))))");

  r = lizard_test_eval(&e, "(infer ctxe (trunc-elim 'C 'h 'v))");
  TEST_ASSERT_STR(lizard_test_format(r), "C");

  /* === Primary computation rule (beta) === */

  r = lizard_test_eval(&e,
    "(reduce (trunc-elim 'C 'h (trunc 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(@ h x)");

  /* === Determinism === */

  /* Variable scrutinee: no LHS pattern matches, recursor stays. */
  r = lizard_test_eval(&e, "(reduce (trunc-elim 'C 'h 'v))");
  TEST_ASSERT_STR(lizard_test_format(r), "(trunc-elim C h v)");

  /* trunc-elim scrutinee (not a trunc): no match. */
  r = lizard_test_eval(&e,
    "(reduce (trunc-elim 'C 'h (trunc-elim 'D 'h2 'v)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(trunc-elim C h (trunc-elim D h2 v))");

  /* Application as scrutinee: no match. */
  r = lizard_test_eval(&e, "(reduce (trunc-elim 'C 'h (@ 'f 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(trunc-elim C h (@ f x))");

  /* Beta fires inside compound terms: outer fires, inner stays. */
  r = lizard_test_eval(&e,
    "(reduce (trunc-elim 'C 'h (trunc (trunc 'x))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(@ h (trunc x))");

  /* === Error cases === */

  /* Scrutinee not Trunc-typed: rejected. */
  lizard_test_eval(&e,
    "(define ctxBad (context-extend (context) (variable 'w (U 0))))");
  r = lizard_test_eval(&e, "(infer ctxBad (trunc-elim (U 0) 'h 'w))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* === Gating: truncations-enabled disabled rejects === */

  lizard_test_eval(&e, "(logic-rule-disable 'truncations-enabled)");
  /* When disabled, the constructor (Trunc ...) itself fails at the
   * Lisp primitive layer, not at the typing layer. The error is
   * raised from the primitive. */
  r = lizard_test_eval(&e, "(Trunc 0 (U 0))");
  TEST_ASSERT_STR(lizard_test_format(r), "Error: user-raised");
  lizard_test_eval(&e, "(set-logic 'truncations)");

  /* === Predicates and accessors === */

  r = lizard_test_eval(&e, "(Trunc? (Trunc 0 (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e, "(Trunc? (U 0))");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");

  r = lizard_test_eval(&e, "(trunc? (trunc 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e, "(trunc-elim? (trunc-elim 'C 'h 'v))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  /* Trunc-level and Trunc-type accessors. */
  r = lizard_test_eval(&e, "(Trunc-level (Trunc 7 (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "7");
  r = lizard_test_eval(&e, "(Trunc-type (Trunc 7 (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");

  r = lizard_test_eval(&e, "(trunc-value (trunc 'foo))");
  TEST_ASSERT_STR(lizard_test_format(r), "foo");

  r = lizard_test_eval(&e,
    "(trunc-elim-motive (trunc-elim 'mot 'hh 'vv))");
  TEST_ASSERT_STR(lizard_test_format(r), "mot");
  r = lizard_test_eval(&e,
    "(trunc-elim-handler (trunc-elim 'mot 'hh 'vv))");
  TEST_ASSERT_STR(lizard_test_format(r), "hh");
  r = lizard_test_eval(&e,
    "(trunc-elim-value (trunc-elim 'mot 'hh 'vv))");
  TEST_ASSERT_STR(lizard_test_format(r), "vv");

  /* === Turn 3: Propositionality coherence (4-arg form) === */

  /* Build context with prop witness:
   *   A : (U 0), C : (U 0), h : Pi _ A C,
   *   p : Pi x C (Pi y C (Path C x y)),
   *   v : Trunc 0 A  */
  lizard_test_eval(&e,
    "(define ctxP "
    "  (context-extend "
    "    (context-extend "
    "      (context-extend "
    "        (context-extend "
    "          (context-extend (context) "
    "            (variable 'A (U 0))) "
    "          (variable 'C (U 0))) "
    "        (variable 'h (Pi '_ 'A 'C))) "
    "      (variable 'p (Pi 'x 'C (Pi 'y 'C (Path 'C 'x 'y))))) "
    "    (variable 'v (Trunc 0 'A))))");

  /* 4-arg form typechecks when prop is well-typed. */
  r = lizard_test_eval(&e, "(infer ctxP (trunc-elim 'C 'h 'p 'v))");
  TEST_ASSERT_STR(lizard_test_format(r), "C");

  /* 4-arg form beta: same as 3-arg. */
  r = lizard_test_eval(&e,
    "(reduce (trunc-elim 'C 'h 'p (trunc 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(@ h x)");

  /* 4-arg form: prop with wrong outer Pi domain rejected. */
  lizard_test_eval(&e,
    "(define ctxBadP "
    "  (context-extend "
    "    (context-extend "
    "      (context-extend "
    "        (context-extend "
    "          (context-extend (context) "
    "            (variable 'A (U 0))) "
    "          (variable 'C (U 0))) "
    "        (variable 'h (Pi '_ 'A 'C))) "
    "      (variable 'badp (Pi 'x 'A (Pi 'y 'C (Path 'C 'x 'y))))) "
    "    (variable 'v (Trunc 0 'A))))");
  r = lizard_test_eval(&e, "(infer ctxBadP (trunc-elim 'C 'h 'badp 'v))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* Prop accessor on 4-arg form returns the prop witness. */
  r = lizard_test_eval(&e, "(trunc-elim-prop (trunc-elim 'C 'h 'p 'v))");
  TEST_ASSERT_STR(lizard_test_format(r), "p");

  /* Prop accessor on 3-arg form returns '(). */
  r = lizard_test_eval(&e, "(trunc-elim-prop (trunc-elim 'C 'h 'v))");
  TEST_ASSERT_STR(lizard_test_format(r), "()");

  lizard_logic_config_reset();
  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
