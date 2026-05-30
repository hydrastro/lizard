/* tests/tt_modal_diamond_test.c
 *
 * Tests for Phase M.5.5 Turn 1: Diamond introduction and
 * elimination forms with beta reduction.
 *
 *   (diamond e)             — construct a Diamond value
 *   (let-diamond x b body)  — pattern-match a Diamond value
 *
 * Beta rule: (let-diamond x (diamond e) body) → body[e/x]
 *
 * M.5.5 Turn 1 uses placeholder typing without a specific modal
 * axiom commitment. Turn 2 adds the 5-axiom (◇A → □◇A) as a
 * typing rule with a modal-5-axiom toggle.
 *
 * Coverage:
 *   - Constructors produce well-formed AST
 *   - Predicates classify correctly
 *   - Beta rule fires when scrutinee is a literal (diamond e)
 *   - Non-beta let-diamond stays as-is
 *   - Typing: (diamond e) : (Diamond T) where e : T
 *   - Typing: (let-diamond x b body) : U where b : Diamond T, body : U
 *   - Cross-modal rejection: unbox of diamond rejects, etc.
 *   - Gate via modalities-enabled
 *   - Binder shadowing
 *   - Composition (Diamond of Box, Box of Diamond)
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);
  lizard_logic_config_reset();

  /* Construction. */
  r = lizard_test_eval(&e, "(diamond (U 0))");
  TEST_ASSERT_STR(lizard_test_format(r), "(diamond (U 0))");
  r = lizard_test_eval(&e, "(let-diamond 'x (diamond (U 0)) 'x)");
  TEST_ASSERT_STR(lizard_test_format(r), "(let-diamond x (diamond (U 0)) x)");

  /* Predicates. */
  r = lizard_test_eval(&e, "(diamond? (diamond (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e, "(diamond? (Diamond (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");
  r = lizard_test_eval(&e, "(diamond? (box (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");
  r = lizard_test_eval(&e, "(let-diamond? (let-diamond 'x (diamond (U 0)) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e, "(let-diamond? (unbox 'x (box (U 0)) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");

  /* Accessors. */
  r = lizard_test_eval(&e, "(diamond-body (diamond (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");
  r = lizard_test_eval(&e, "(let-diamond-binder (let-diamond 'x (diamond (U 0)) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "x");

  /* Beta rule. */
  r = lizard_test_eval(&e, "(reduce (let-diamond 'x (diamond (U 7)) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 7)");
  r = lizard_test_eval(&e, "(reduce (let-diamond 'x (diamond (U 5)) (U 1)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* Beta with substitution into a binding form. */
  r = lizard_test_eval(&e, "(reduce (let-diamond 'x (diamond (U 5)) (Pi 'y 'x 'y)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Pi (y (U 5)) y)");

  /* Non-beta let-diamond stays. */
  r = lizard_test_eval(&e, "(reduce (let-diamond 'x 'b 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "(let-diamond x b x)");

  /* Typing: diamond wraps the inferred type. */
  r = lizard_test_eval(&e, "(infer (context) (diamond (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Diamond (U 1))");

  /* Typing: let-diamond returns body's type with binder in scope. */
  r = lizard_test_eval(&e, "(infer (context) (let-diamond 'x (diamond (U 0)) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* Cross-modal rejection: unbox on Diamond fails. */
  r = lizard_test_eval(&e, "(infer (context) (unbox 'x (diamond (U 0)) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");
  /* And let-diamond on Box fails. */
  r = lizard_test_eval(&e, "(infer (context) (let-diamond 'x (box (U 0)) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* Binder shadowing in substitution. */
  r = lizard_test_eval(&e,
    "(reduce (let-diamond 'x (diamond (U 0)) (let-diamond 'x (diamond (U 7)) 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 7)");

  /* Composition: Diamond of Box. */
  r = lizard_test_eval(&e, "(infer (context) (diamond (box (U 0))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Diamond (Box (U 1)))");
  /* Box of Diamond. */
  r = lizard_test_eval(&e, "(infer (context) (box (diamond (U 0))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (Diamond (U 1)))");

  /* Gate. */
  lizard_test_eval(&e, "(logic-rule-disable 'modalities-enabled)");
  r = lizard_test_eval(&e, "(infer (context) (diamond (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");
  /* Beta still fires — only typing is gated. */
  r = lizard_test_eval(&e, "(reduce (let-diamond 'x (diamond (U 0)) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");

  lizard_logic_config_reset();
  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
