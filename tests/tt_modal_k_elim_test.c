/* tests/tt_modal_k_elim_test.c
 *
 * Tests for Phase M.5.6: K's distinguished elimination via
 *   (a) box-app (the K-axiom □(A→B) → □A → □B as a term)
 *   (b) the t-axiom-enabled toggle gating unbox's extraction behavior
 *
 * Under K (t-axiom-enabled off), unbox cannot extract a value — the
 * body of unbox must itself be Box-typed. This forces unbox to be
 * used for structural manipulation only, not extraction. The K-axiom
 * lives in box-app, available in all modal logics.
 *
 * Bundle settings:
 *   K          → t_axiom = 0  (no extraction; only box-app + structural unbox)
 *   T, S4, S5  → t_axiom = 1  (extraction works)
 *
 * Coverage:
 *   - box-app construction, predicates, accessors, printer
 *   - K-axiom reduction (box-app of two boxes → box of app)
 *   - box-app typing (non-dependent Pi), rejection cases
 *   - K disables extraction via unbox
 *   - T/S4/S5 keep extraction
 *   - Direct toggle control of t-axiom-enabled
 *   - K can still do structural manipulation: unbox where body returns Box
 *   - Bundle reverse-lookup distinguishes K from T (was previously aliased)
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);
  lizard_logic_config_reset();

  /* box-app construction. */
  r = lizard_test_eval(&e, "(box-app (box 'f) (box 'a))");
  TEST_ASSERT_STR(lizard_test_format(r), "(box-app (box f) (box a))");

  /* Predicate. */
  r = lizard_test_eval(&e, "(box-app? (box-app (box 'f) (box 'a)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e, "(box-app? (box 'f))");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");

  /* Accessors. */
  r = lizard_test_eval(&e, "(box-app-fun (box-app (box 'f) (box 'a)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(box f)");
  r = lizard_test_eval(&e, "(box-app-arg (box-app (box 'f) (box 'a)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(box a)");

  /* K-axiom reduction. */
  r = lizard_test_eval(&e, "(reduce (box-app (box 'f) (box 'a)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(box (@ f a))");

  /* box-app typing: non-dependent Pi. */
  r = lizard_test_eval(&e,
    "(infer (context-extend "
    "         (context-extend (context) (variable 'f (Box (Pi '_ (U 0) (U 0))))) "
    "         (variable 'a (Box (U 0)))) "
    "       (box-app 'f 'a))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (U 0))");

  /* box-app typing: argument type mismatch. */
  r = lizard_test_eval(&e,
    "(infer (context-extend "
    "         (context-extend (context) (variable 'f (Box (Pi '_ (U 0) (U 0))))) "
    "         (variable 'a (Box (U 1)))) "
    "       (box-app 'f 'a))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* box-app typing: fun not Box-typed. */
  r = lizard_test_eval(&e,
    "(infer (context-extend (context) (variable 'a (Box (U 0)))) "
    "       (box-app (Pi '_ (U 0) (U 0)) 'a))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* box-app on dependent Pi: now SUPPORTED under T+ (M.5.7).
   * The result substitutes (unbox y a y) for x in B. The fresh-name
   * generator produces "__boxapp_<dim>" where <dim> is the next
   * dim counter value. We can't predict the exact dim number across
   * test runs, so we test the K case (still rejected) instead. */
  lizard_test_eval(&e, "(set-logic 'K)");
  r = lizard_test_eval(&e,
    "(infer (context-extend "
    "         (context-extend (context) (variable 'f (Box (Pi 'x (U 0) 'x)))) "
    "         (variable 'a (Box (U 0)))) "
    "       (box-app 'f 'a))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");
  /* Same expression under S4: now works. */
  lizard_test_eval(&e, "(set-logic 'S4)");
  /* The result has form (Box (unbox __boxapp_N a __boxapp_N)) for
   * some N. Just check the result is a Box. */
  r = lizard_test_eval(&e,
    "(Box? (infer (context-extend "
    "         (context-extend (context) (variable 'f (Box (Pi 'x (U 0) 'x)))) "
    "         (variable 'a (Box (U 0)))) "
    "       (box-app 'f 'a)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  lizard_logic_config_reset();

  /* box-app gated on modalities-enabled. */
  lizard_test_eval(&e, "(logic-rule-disable 'modalities-enabled)");
  r = lizard_test_eval(&e,
    "(infer (context-extend "
    "         (context-extend (context) (variable 'f (Box (Pi '_ (U 0) (U 0))))) "
    "         (variable 'a (Box (U 0)))) "
    "       (box-app 'f 'a))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");
  lizard_test_eval(&e, "(logic-rule-enable 'modalities-enabled)");

  /* === K's distinguished elim === */

  /* Set K: t-axiom is off. */
  lizard_test_eval(&e, "(set-logic 'K)");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 't-axiom-enabled)");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");

  /* K rejects extraction via unbox. */
  r = lizard_test_eval(&e, "(infer (context) (unbox 'x (box (U 0)) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* But K allows structural unbox where body remains Box-typed. */
  r = lizard_test_eval(&e, "(infer (context) (unbox 'x (box (U 0)) (box 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (U 1))");

  /* K supports box-app. */
  r = lizard_test_eval(&e,
    "(infer (context-extend "
    "         (context-extend (context) (variable 'f (Box (Pi '_ (U 0) (U 0))))) "
    "         (variable 'a (Box (U 0)))) "
    "       (box-app 'f 'a))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (U 0))");

  /* === T re-enables extraction === */

  lizard_test_eval(&e, "(set-logic 'T)");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 't-axiom-enabled)");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e, "(infer (context) (unbox 'x (box (U 0)) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* S4 and S5 too. */
  lizard_test_eval(&e, "(set-logic 'S4)");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 't-axiom-enabled)");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  lizard_test_eval(&e, "(set-logic 'S5)");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 't-axiom-enabled)");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  /* === Direct toggle === */

  lizard_test_eval(&e, "(set-logic 'S4)");
  lizard_test_eval(&e, "(logic-rule-disable 't-axiom-enabled)");
  /* Extraction now rejected even in S4. */
  r = lizard_test_eval(&e, "(infer (context) (unbox 'x (box (U 0)) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");
  lizard_test_eval(&e, "(logic-rule-enable 't-axiom-enabled)");

  /* === Bundle reverse-lookup distinguishes K from T === */

  lizard_test_eval(&e, "(set-logic 'K)");
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "K");

  lizard_test_eval(&e, "(set-logic 'T)");
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "T");

  lizard_test_eval(&e, "(set-logic 'S4)");
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "S4");

  lizard_logic_config_reset();
  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
