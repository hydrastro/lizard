/* tests/tt_modal_4_axiom_test.c
 *
 * Tests for Phase M.5.4: 4-axiom toggle distinguishing T from S4.
 *
 * The 4-axiom (□A → □□A) says necessity is idempotent: if A is
 * necessarily true, then it is necessarily-necessarily true.
 *
 * In Pfenning-Davies style S4, the 4-axiom is built into the box-
 * introduction rule: when entering a box, the valid context Δ is
 * PRESERVED as valid. A valid hypothesis remains valid across
 * nested box-introductions.
 *
 * In T (which lacks the 4-axiom), entering a box CONSUMES the valid
 * context: it's promoted to truth (or equivalently, valid is reset
 * to empty). A valid hypothesis is visible one box deep but
 * disappears at the next nesting.
 *
 * Lizard's `modal-4-axiom` toggle (default-on per default-allow
 * convention) controls this. Bundle settings:
 *
 *   K, T:     modal-4-axiom = 0 (off)
 *   S4, S5:   modal-4-axiom = 1 (on)
 *   modal-STLC: modal-4-axiom = 1 (S4-flavored by default)
 *
 * Coverage:
 *   - Single box of valid var works in both T and S4
 *   - Nested box of valid var works in S4 only
 *   - Triple-nested in S4
 *   - Truth-var rejection unaffected by 4-axiom toggle
 *   - Toggle direct control works
 *   - Round-trip (unbox-then-box) works in both, but nested-on-RHS
 *     works only in S4
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);
  lizard_logic_config_reset();

  /* Default state: 4-axiom defaults on (S4 behavior). */
  r = lizard_test_eval(&e,
    "(infer-modal (context-extend (context) (variable 'x (U 0))) "
    "             (context) "
    "             (box (box 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (Box (U 0)))");

  /* Triple-nested works under S4. */
  r = lizard_test_eval(&e,
    "(infer-modal (context-extend (context) (variable 'x (U 0))) "
    "             (context) "
    "             (box (box (box 'x))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (Box (Box (U 0))))");

  /* Set T: 4-axiom should be off. */
  lizard_test_eval(&e, "(set-logic 'T)");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'modal-4-axiom)");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");

  /* Single box still works under T. */
  r = lizard_test_eval(&e,
    "(infer-modal (context-extend (context) (variable 'x (U 0))) "
    "             (context) "
    "             (box 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (U 0))");

  /* Nested box FAILS under T — no 4-axiom. */
  r = lizard_test_eval(&e,
    "(infer-modal (context-extend (context) (variable 'x (U 0))) "
    "             (context) "
    "             (box (box 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* Set S4: nested works again. */
  lizard_test_eval(&e, "(set-logic 'S4)");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'modal-4-axiom)");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e,
    "(infer-modal (context-extend (context) (variable 'x (U 0))) "
    "             (context) "
    "             (box (box 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (Box (U 0)))");

  /* K is like T — no 4-axiom. */
  lizard_test_eval(&e, "(set-logic 'K)");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'modal-4-axiom)");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");
  r = lizard_test_eval(&e,
    "(infer-modal (context-extend (context) (variable 'x (U 0))) "
    "             (context) "
    "             (box (box 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* S5 is like S4 — has 4-axiom. */
  lizard_test_eval(&e, "(set-logic 'S5)");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'modal-4-axiom)");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  /* Truth-var rejection unaffected by 4-axiom — works in both T and S4. */
  lizard_test_eval(&e, "(set-logic 'T)");
  r = lizard_test_eval(&e,
    "(infer-modal (context) "
    "             (context-extend (context) (variable 'y (U 0))) "
    "             (box 'y))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* Round-trip (unbox-then-box) works in T. */
  r = lizard_test_eval(&e,
    "(infer (context) (unbox 'x (box (U 0)) (box 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (U 1))");

  /* Round-trip with NESTED box on RHS — works in S4, not T. */
  r = lizard_test_eval(&e,
    "(infer (context) (unbox 'x (box (U 0)) (box (box 'x))))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");
  /* Switch to S4: same expression now works. */
  lizard_test_eval(&e, "(set-logic 'S4)");
  r = lizard_test_eval(&e,
    "(infer (context) (unbox 'x (box (U 0)) (box (box 'x))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (Box (U 1)))");

  /* Direct toggle control. */
  lizard_test_eval(&e, "(logic-rule-disable 'modal-4-axiom)");
  r = lizard_test_eval(&e,
    "(infer-modal (context-extend (context) (variable 'x (U 0))) "
    "             (context) "
    "             (box (box 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");
  lizard_test_eval(&e, "(logic-rule-enable 'modal-4-axiom)");
  r = lizard_test_eval(&e,
    "(infer-modal (context-extend (context) (variable 'x (U 0))) "
    "             (context) "
    "             (box (box 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (Box (U 0)))");

  lizard_logic_config_reset();
  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
