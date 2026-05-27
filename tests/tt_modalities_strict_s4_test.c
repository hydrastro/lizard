/* tests/tt_modalities_strict_s4_test.c
 *
 * Tests for Phase M.5.2 Turn 2: strict S4 modal typing rules
 * using the dual-context infrastructure.
 *
 * Box-intro rule (strict S4):
 *   Δ; · ⊢ e : T
 *   ─────────────────────
 *   Δ; Γ ⊢ box e : Box T
 *
 * Unbox rule:
 *   Δ; Γ ⊢ b : Box T    Δ, x:T; Γ ⊢ body : U
 *   ────────────────────────────────────────
 *   Δ; Γ ⊢ unbox x b body : U
 *
 * The truth context Γ is dropped when checking a box body. Only
 * the valid context Δ is visible. The unbox rule extends Δ (not Γ)
 * with the unboxed variable.
 *
 * Toggle: modal-strict-typing (default on). When off, both rules
 * fall back to Turn 1 loose typing.
 *
 * Coverage:
 *   - Backward compat: empty contexts behave like Turn 1
 *   - Truth-context variable rejected inside box
 *   - Valid-context variable accepted inside box
 *   - Unbox places binder in valid context
 *   - Round-trip: unbox-then-box reuses valid hypothesis
 *   - Toggle off reverts to Turn 1
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);
  lizard_logic_config_reset();

  /* Backward compat: empty valid + empty truth, plain (box (U 0)). */
  r = lizard_test_eval(&e, "(infer-modal (context) (context) (box (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (U 1))");

  /* Truth-only variable IS visible outside box. */
  r = lizard_test_eval(&e,
    "(infer-modal (context) "
    "             (context-extend (context) (variable 'y (U 0))) "
    "             'y)");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");

  /* Truth-only variable is NOT visible inside box (strict S4 reject). */
  r = lizard_test_eval(&e,
    "(infer-modal (context) "
    "             (context-extend (context) (variable 'y (U 0))) "
    "             (box 'y))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* Valid-context variable IS visible inside box. */
  r = lizard_test_eval(&e,
    "(infer-modal (context-extend (context) (variable 'y (U 0))) "
    "             (context) "
    "             (box 'y))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (U 0))");

  /* Valid-context variable visible outside box too (lookup checks
   * both contexts). */
  r = lizard_test_eval(&e,
    "(infer-modal (context-extend (context) (variable 'y (U 0))) "
    "             (context) "
    "             'y)");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");

  /* Unbox places binder in valid context — survives subsequent box. */
  r = lizard_test_eval(&e,
    "(infer (context) (unbox 'x (box (U 0)) (box 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (U 1))");

  /* Toggle off: strict rules revert to loose Turn 1 behavior. */
  lizard_test_eval(&e, "(logic-rule-disable 'modal-strict-typing)");
  r = lizard_test_eval(&e,
    "(infer-modal (context) "
    "             (context-extend (context) (variable 'y (U 0))) "
    "             (box 'y))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (U 0))");
  lizard_test_eval(&e, "(logic-rule-enable 'modal-strict-typing)");

  /* Strict rules restored. */
  r = lizard_test_eval(&e,
    "(infer-modal (context) "
    "             (context-extend (context) (variable 'y (U 0))) "
    "             (box 'y))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* Modalities-enabled gate still works (M.5.1). */
  lizard_test_eval(&e, "(logic-rule-disable 'modalities-enabled)");
  r = lizard_test_eval(&e, "(infer (context) (box (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");
  lizard_test_eval(&e, "(logic-rule-enable 'modalities-enabled)");

  /* Beta rule unchanged. */
  r = lizard_test_eval(&e, "(reduce (unbox 'x (box (U 7)) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 7)");

  lizard_logic_config_reset();
  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
