/* tests/tt_modalities_intro_elim_test.c
 *
 * Tests for Phase M.5.2 Turn 1: Box introduction and elimination
 * forms with the beta reduction rule.
 *
 * (box e)             introduces a Box value
 * (unbox x b body)    eliminates a Box value, binding x to the
 *                     unboxed contents within body
 *
 * Beta rule: (unbox x (box e) body) → body[e/x]
 *
 * M.5.2 Turn 1 uses placeholder typing (the loose form without
 * dual-context discipline). Turn 2 will tighten the rules to
 * enforce S4-style context separation.
 *
 * Coverage:
 *   - Constructors produce well-formed AST
 *   - Predicates classify correctly
 *   - Beta rule fires when scrutinee is a literal (box e)
 *   - Non-beta unbox stays as-is (only descends into subterms)
 *   - Typing: (box e) : (Box T) where e : T
 *   - Typing: (unbox x b body) : U where b : Box T, body : U
 *   - Gate via modalities-enabled
 *   - Substitution handles binder shadowing correctly
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);
  lizard_logic_config_reset();

  /* Construction. */
  r = lizard_test_eval(&e, "(box (U 0))");
  TEST_ASSERT_STR(lizard_test_format(r), "(box (U 0))");
  r = lizard_test_eval(&e, "(unbox 'x (box (U 0)) 'x)");
  TEST_ASSERT_STR(lizard_test_format(r), "(unbox x (box (U 0)) x)");

  /* Predicates. */
  r = lizard_test_eval(&e, "(box? (box (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e, "(box? (Box (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");
  r = lizard_test_eval(&e, "(unbox? (unbox 'x (box (U 0)) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e, "(unbox? (box (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");

  /* Accessors. */
  r = lizard_test_eval(&e, "(box-body (box (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");
  r = lizard_test_eval(&e, "(unbox-binder (unbox 'x (box (U 0)) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "x");
  r = lizard_test_eval(&e, "(unbox-scrutinee (unbox 'x (box (U 0)) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "(box (U 0))");

  /* Beta rule. */
  r = lizard_test_eval(&e, "(reduce (unbox 'x (box (U 0)) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");
  r = lizard_test_eval(&e, "(reduce (unbox 'x (box (U 5)) (U 1)))");
  /* Body doesn't use x, but beta still fires; result is body unchanged. */
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* Beta with substitution into a binding form. */
  r = lizard_test_eval(&e, "(reduce (unbox 'x (box (U 5)) (Pi 'y 'x 'y)))");
  /* The Pi printer elides the explicit binder annotation when it's
   * derivable, so (Pi 'y (U 5) 'y) prints as "(Pi (y (U 5)) y)". */
  TEST_ASSERT_STR(lizard_test_format(r), "(Pi (y (U 5)) y)");

  /* Non-beta unbox stays. */
  r = lizard_test_eval(&e, "(reduce (unbox 'x 'b 'x))");
  /* The scrutinee 'b is a free symbol, not a (box ...), so no beta. */
  TEST_ASSERT_STR(lizard_test_format(r), "(unbox x b x)");

  /* Typing: box wraps the inferred type. */
  r = lizard_test_eval(&e, "(infer (context) (box (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (U 1))");
  r = lizard_test_eval(&e, "(infer (context) (box (Pi 'x (U 0) (U 0))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (U 1))");

  /* Typing: unbox returns body's type with binder in scope. */
  r = lizard_test_eval(&e, "(infer (context) (unbox 'x (box (U 0)) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* Typing rejects when scrutinee isn't Box-typed. */
  r = lizard_test_eval(&e, "(infer (context) (unbox 'x (U 0) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* Binder shadowing in substitution: outer x doesn't leak into body
   * if box-elim's binder is also x. */
  r = lizard_test_eval(&e,
    "(reduce (unbox 'x (box (U 0)) (unbox 'x (box (U 7)) 'x)))");
  /* Inner unbox shadows outer; inner beta gives (U 7). Outer beta
   * then would substitute (U 0) for 'x in (U 7), which has no free x,
   * so result stays (U 7). */
  TEST_ASSERT_STR(lizard_test_format(r), "(U 7)");

  /* Gate. */
  lizard_test_eval(&e, "(logic-rule-disable 'modalities-enabled)");
  r = lizard_test_eval(&e, "(infer (context) (box (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");
  r = lizard_test_eval(&e, "(infer (context) (unbox 'x (box (U 0)) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");
  /* Beta reduction still fires on the AST level — only typing is gated. */
  r = lizard_test_eval(&e, "(reduce (unbox 'x (box (U 0)) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");

  lizard_logic_config_reset();
  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
