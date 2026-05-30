/* tests/tt_modal_5_axiom_test.c
 *
 * Tests for Phase M.5.5 Turn 2: the 5-axiom (◇A → □◇A) as a
 * type-checking rule that distinguishes S5 from S4.
 *
 * Encoding: under S5, `let-diamond` extends the VALID context
 * instead of the truth context. The unboxed Diamond content
 * becomes "necessarily-possibly true" — it survives entry into
 * future boxes, which is exactly the 5-axiom's content.
 *
 * Toggle: modal-5-axiom (default-on per default-allow convention).
 *   K, T, S4   → 0 (no 5-axiom)
 *   S5         → 1 (5-axiom)
 *   modal-STLC → 0 (S4-flavored)
 *
 * Coverage:
 *   - S4 vs S5 distinguishable via let-diamond context placement
 *   - Headline case: (let-diamond x (diamond e) (box (diamond x)))
 *     accepts under S5 (returns Box (Diamond _)), rejects under S4
 *   - K and T also reject (no 5-axiom)
 *   - Direct toggle control works
 *   - 4-axiom still works under both S4 and S5
 *   - Bundle-driven settings are correct
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);
  lizard_logic_config_reset();

  /* S4 disables 5-axiom. */
  lizard_test_eval(&e, "(set-logic 'S4)");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'modal-5-axiom)");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");

  /* Under S4: let-diamond binder lands in TRUTH, dropped inside box. */
  r = lizard_test_eval(&e,
    "(infer (context) (let-diamond 'x (diamond (U 0)) (box 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* The 5-axiom headline case fails under S4. */
  r = lizard_test_eval(&e,
    "(infer (context) (let-diamond 'x (diamond (U 0)) (box (diamond 'x))))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* S5 enables 5-axiom. */
  lizard_test_eval(&e, "(set-logic 'S5)");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'modal-5-axiom)");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  /* Under S5: let-diamond binder lands in VALID, survives inside box. */
  r = lizard_test_eval(&e,
    "(infer (context) (let-diamond 'x (diamond (U 0)) (box 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (U 1))");

  /* The 5-axiom headline case: ◇A → □◇A. */
  r = lizard_test_eval(&e,
    "(infer (context) (let-diamond 'x (diamond (U 0)) (box (diamond 'x))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (Diamond (U 1)))");

  /* K, T also lack 5-axiom. */
  lizard_test_eval(&e, "(set-logic 'K)");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'modal-5-axiom)");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");
  r = lizard_test_eval(&e,
    "(infer (context) (let-diamond 'x (diamond (U 0)) (box 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  lizard_test_eval(&e, "(set-logic 'T)");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'modal-5-axiom)");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");
  r = lizard_test_eval(&e,
    "(infer (context) (let-diamond 'x (diamond (U 0)) (box 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* modal-STLC is S4-flavored — no 5-axiom. */
  lizard_test_eval(&e, "(set-logic 'modal-STLC)");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'modal-5-axiom)");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");

  /* Direct toggle control: turn 5-axiom on manually. */
  lizard_test_eval(&e, "(set-logic 'S4)");
  lizard_test_eval(&e, "(logic-rule-enable 'modal-5-axiom)");
  r = lizard_test_eval(&e,
    "(infer (context) (let-diamond 'x (diamond (U 0)) (box 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (U 1))");

  /* Turn it off again. */
  lizard_test_eval(&e, "(logic-rule-disable 'modal-5-axiom)");
  r = lizard_test_eval(&e,
    "(infer (context) (let-diamond 'x (diamond (U 0)) (box 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* 4-axiom interaction: still works in S5. */
  lizard_test_eval(&e, "(set-logic 'S5)");
  r = lizard_test_eval(&e,
    "(infer-modal (context-extend (context) (variable 'x (U 0))) "
    "             (context) "
    "             (box (box 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (Box (U 0)))");

  /* Cross-modal rejection unaffected by 5-axiom. */
  r = lizard_test_eval(&e,
    "(infer (context) (let-diamond 'x (box (U 0)) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* Beta rule unchanged. */
  r = lizard_test_eval(&e, "(reduce (let-diamond 'x (diamond (U 7)) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 7)");

  lizard_logic_config_reset();
  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
