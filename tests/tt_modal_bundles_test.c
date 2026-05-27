/* tests/tt_modal_bundles_test.c
 *
 * Tests for Phase M.5.3: modal-logic bundles.
 *
 * M.5.3 adds five named bundles to the table:
 *   K, T, S4, S5         — all with CoC cube base + modalities on + strict
 *   modal-STLC           — STLC base + modalities on + strict
 *
 * Important honest scope note: at lizard's current depth, K/T/S4/S5
 * are operationally INDISTINGUISHABLE. The strict box-intro and unbox
 * rules from M.5.2 Turn 2 are the same across these logics. What
 * differs between them mathematically are AXIOMS (T-axiom, 4-axiom,
 * 5-axiom) which are not yet wired as type-checkable rules.
 *
 * Consequence: `set-logic 'S4` works (applies the toggles), but
 * `current-logic` returns the cube-base name (CoC) because the modal
 * fields are at the default state when the matcher reaches CoC first.
 * The modal-bundle names are forward-only at this phase.
 *
 * Coverage:
 *   - set-logic 'K/T/S4/S5 succeeds (returns #t)
 *   - After set-logic, modalities-enabled and modal-strict-typing
 *     are both on
 *   - Box typechecking works under these bundles
 *   - Strict modal discipline (M.5.2 Turn 2) is active
 *   - modal-STLC retains STLC cube restrictions
 *   - list-logics includes the new bundles
 *   - Unknown modal-bundle name returns #f
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);
  lizard_logic_config_reset();

  /* set-logic 'S4 succeeds. */
  r = lizard_test_eval(&e, "(set-logic 'S4)");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  /* Both modal toggles are on. */
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'modalities-enabled)");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'modal-strict-typing)");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  /* Box typechecking works under S4. */
  r = lizard_test_eval(&e, "(infer (context) (box (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (U 1))");

  /* Strict S4 discipline is active: truth var rejected inside box. */
  r = lizard_test_eval(&e,
    "(infer-modal (context) "
    "             (context-extend (context) (variable 'y (U 0))) "
    "             (box 'y))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* K, T, S5 all succeed at set-logic. */
  r = lizard_test_eval(&e, "(set-logic 'K)");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e, "(set-logic 'T)");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e, "(set-logic 'S5)");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  /* modal-STLC: STLC cube base + modalities on. */
  lizard_test_eval(&e, "(set-logic 'modal-STLC)");
  /* Box still works. */
  r = lizard_test_eval(&e, "(infer (context) (box (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (U 1))");
  /* STLC restriction: System F Pi rejected. */
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 1) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* M.5.5 update: as of Turn 2 (5-axiom), the modal bundles are
   * genuinely distinguishable. set-logic 'S4 sets modal_4_axiom=1
   * and modal_5_axiom=0, which doesn't match CoC's don't-care
   * fields under default-allow (those require both modal axioms
   * at default-on). So current-logic now returns the right name. */
  lizard_test_eval(&e, "(set-logic 'S4)");
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "S4");

  lizard_test_eval(&e, "(set-logic 'modal-STLC)");
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "modal-STLC");

  /* When we disable modalities explicitly under CoC, CoC's matcher
   * (which has don't-care for modal fields, requiring default state)
   * sees modalities_en != 1 and doesn't match. Result: custom. This
   * is a quirk of the current matcher's interpretation of -1 as
   * "wildcard for default state" rather than "wildcard for any state."
   * Future M.5.x work could refine this. */
  lizard_test_eval(&e, "(set-logic 'CoC)");
  lizard_test_eval(&e, "(logic-rule-disable 'modalities-enabled)");
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "custom");

  /* Unknown modal bundle name returns #f. */
  r = lizard_test_eval(&e, "(set-logic 'S6)");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");

  /* list-logics includes the new bundles. */
  r = lizard_test_eval(&e, "(list-logics)");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(proof-scaffold truncations cubical-S1 "
                  "modal-STLC S5 S4 T K "
                  "CoC-plus-lattice STLC-strict "
                  "relevant-STLC affine-STLC linear-STLC "
                  "CoC lambda-omega lambda-P-omega lambda-P2 "
                  "F-omega lambda-P LF F STLC)");

  /* Reset returns full freedom. */
  lizard_logic_config_reset();
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "CoC");

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
