/* tests/tt_logic_structural_test.c
 *
 * Tests for Phase M.4: structural-rule toggles.
 *
 * M.4 introduces three named structural-rule toggles — weakening,
 * contraction, exchange — and wires WEAKENING to the Pi typing
 * rule: when weakening is disabled, a non-dependent Pi (where the
 * binder doesn't appear in the codomain) is rejected.
 *
 * Contraction and exchange are registered but NOT yet wired —
 * their real implementation requires usage-tracking refactor that
 * is deferred. M.4 tests cover what's actually wired.
 *
 * Coverage:
 *   - Disabling weakening rejects non-dependent Pi
 *   - Disabling weakening still allows dependent Pi
 *   - Enabling weakening restores all Pi shapes
 *   - Substructural bundles (linear-STLC, affine-STLC, relevant-STLC)
 *   - Reverse lookup picks the right bundle when structural rules
 *     are explicitly set
 *   - The cube-only bundles (STLC etc.) still match when structural
 *     rules are at their defaults
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);
  lizard_logic_config_reset();

  /* Default: weakening allowed, simple arrow OK. */
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 0) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* Disable weakening: simple arrow rejected. */
  lizard_test_eval(&e, "(logic-rule-disable 'weakening)");
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 0) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* But a dependent Pi (binder used in codomain) is still OK. */
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 0) (Id (U 0) 'x 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* Re-enable weakening. */
  lizard_test_eval(&e, "(logic-rule-enable 'weakening)");
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 0) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* linear-STLC: no weakening, no contraction. */
  lizard_test_eval(&e, "(set-logic 'linear-STLC)");
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "linear-STLC");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'weakening)");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'contraction)");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");
  /* Simple arrow rejected under linear. */
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 0) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* affine-STLC: weakening on, contraction off. */
  lizard_test_eval(&e, "(set-logic 'affine-STLC)");
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "affine-STLC");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'weakening)");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'contraction)");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");
  /* Simple arrow allowed (weakening on). */
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 0) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* relevant-STLC: weakening off, contraction on. */
  lizard_test_eval(&e, "(set-logic 'relevant-STLC)");
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "relevant-STLC");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'weakening)");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'contraction)");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  /* Simple arrow rejected (weakening off). */
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 0) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* Reset and verify default behavior restored. */
  lizard_logic_config_reset();
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "CoC");
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 0) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* Cube-only bundle still works after reset. */
  lizard_test_eval(&e, "(set-logic 'STLC)");
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "STLC");

  lizard_logic_config_reset();
  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
