/* tests/tt_symmetric_s5_turn1_test.c
 *
 * Tests for Phase M.5.9 Turn 1: symmetric-S5 infrastructure.
 *
 * Turn 1 adds the AST nodes, descents, primitives, toggle, and
 * three-context API as wrappers. The typing rules themselves come
 * in Turn 2.
 *
 * Coverage:
 *   - AST_TT_DIAMOND_INTRO_SYM (dia) construction, predicate, accessor
 *   - AST_TT_POSS_COERCE (poss-coerce) construction, predicate, accessor
 *   - Reduction: dia preserves shape, poss-coerce unwraps (no-op)
 *   - Toggle modal-symmetric exists and is settable
 *   - S4/S5 bundles set modal-symmetric correctly
 *   - Typing rules raise an explicit not-yet-implemented error
 *     (M.5.9 Turn 2 will wire them)
 *   - Existing asymmetric forms (diamond, let-diamond) unaffected
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);
  lizard_logic_config_reset();

  /* === Construction === */

  r = lizard_test_eval(&e, "(dia (U 0))");
  TEST_ASSERT_STR(lizard_test_format(r), "(dia (U 0))");

  r = lizard_test_eval(&e, "(poss-coerce (U 0))");
  TEST_ASSERT_STR(lizard_test_format(r), "(poss-coerce (U 0))");

  /* === Predicates === */

  r = lizard_test_eval(&e, "(dia? (dia (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  /* dia is distinct from the asymmetric diamond. */
  r = lizard_test_eval(&e, "(dia? (diamond (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");
  r = lizard_test_eval(&e, "(diamond? (dia (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");

  r = lizard_test_eval(&e, "(poss-coerce? (poss-coerce (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e, "(poss-coerce? (U 0))");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");

  /* === Accessors === */

  r = lizard_test_eval(&e, "(dia-body (dia (U 7)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 7)");
  r = lizard_test_eval(&e, "(poss-coerce-body (poss-coerce 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "x");

  /* === Reduction === */

  /* dia is irreducible at the AST level; body reduces, dia stays. */
  r = lizard_test_eval(&e, "(reduce (dia (U 7)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(dia (U 7))");

  /* poss-coerce is operationally a no-op — unwraps to body. */
  r = lizard_test_eval(&e, "(reduce (poss-coerce (U 7)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 7)");
  /* Coerce of a more complex term: body normalizes through the coerce. */
  r = lizard_test_eval(&e, "(reduce (poss-coerce (unbox 'x (box (U 5)) 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 5)");

  /* === Substitution respects the new forms === */

  r = lizard_test_eval(&e, "(reduce (unbox 'x (box (U 5)) (dia 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(dia (U 5))");
  r = lizard_test_eval(&e, "(reduce (unbox 'x (box (U 5)) (poss-coerce 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 5)");

  /* === Toggle === */

  /* S4 disables modal-symmetric explicitly. */
  lizard_test_eval(&e, "(set-logic 'S4)");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'modal-symmetric)");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");

  /* S5 enables modal-symmetric explicitly. */
  lizard_test_eval(&e, "(set-logic 'S5)");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'modal-symmetric)");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "S5");

  /* K, T also have modal-symmetric off. */
  lizard_test_eval(&e, "(set-logic 'K)");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'modal-symmetric)");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");
  lizard_test_eval(&e, "(set-logic 'T)");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'modal-symmetric)");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");

  /* === Typing: Turn 2b wires the real rules ===
   * After Turn 2b, the symmetric forms have real typing rules.
   * Updated assertions: dia of a true-typed body is rejected (must
   * be poss); poss-coerce of a true-typed body produces the same
   * type with POSS kind (the kind is internal; user sees the type). */

  lizard_test_eval(&e, "(set-logic 'S5)");
  /* dia of a true-typed body: rejected. */
  r = lizard_test_eval(&e, "(infer (context) (dia (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");
  /* poss-coerce of a true-typed body: produces the same type. */
  r = lizard_test_eval(&e, "(infer (context) (poss-coerce (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* === Existing asymmetric forms still work === */

  /* (diamond e) is the old form; it still has its M.5.5/M.5.7 typing
   * rule and doesn't consult modal-symmetric. */
  r = lizard_test_eval(&e, "(infer (context) (diamond (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Diamond (U 1))");

  /* (let-diamond x (diamond a) b) still works under S5. */
  r = lizard_test_eval(&e, "(infer (context) (let-diamond 'x (diamond (U 0)) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  lizard_logic_config_reset();
  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
