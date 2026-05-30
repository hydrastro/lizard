/* tests/tt_modal_s5_lookup_and_dep_pi_test.c
 *
 * Tests for Phase M.5.7:
 *   (a) set-logic / current-logic round-trips through explicit names
 *       (the "remembered last-set bundle" mechanism)
 *   (b) box-app supports dependent Pi when t-axiom-enabled is on
 *
 * The remembered-name mechanism resolves the ambiguity where multiple
 * bundles match the same toggle state. Previously, set-logic 'S5 was
 * a no-op for current-logic because CoC matched the same toggles and
 * appeared earlier in the table. Now, the explicit name is preserved
 * while it still applies; manually toggling a rule that breaks the
 * match falls back to a table walk.
 *
 * Dependent Pi in box-app: previously rejected outright. Now, under
 * t-axiom-enabled, substitutes (unbox y a y) for the Pi binder in the
 * codomain. The unboxing is the T-axiom realized; K still rejects.
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);
  lizard_logic_config_reset();

  /* === Reverse-lookup with remembered name === */

  /* After reset: fallback to table walk. */
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "CoC");

  /* set-logic 'S5 now returns "S5" via the remembered-name mechanism. */
  lizard_test_eval(&e, "(set-logic 'S5)");
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "S5");

  /* set-logic switches preserve the new name. */
  lizard_test_eval(&e, "(set-logic 'S4)");
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "S4");
  lizard_test_eval(&e, "(set-logic 'S5)");
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "S5");

  /* If we manually toggle and break the match, the remembered name
   * stops applying and we fall back to the table walk. After M.5.9,
   * S5 requires both modal-5-axiom AND modal-symmetric. Disabling
   * just one makes the state a hybrid that doesn't match any bundle
   * — it's S5-with-asymmetric-Diamond, which lizard doesn't name.
   * The honest result is "custom". */
  lizard_test_eval(&e, "(set-logic 'S5)");
  lizard_test_eval(&e, "(logic-rule-disable 'modal-5-axiom)");
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "custom");

  /* To get S4 by manual toggle, disable both modal-5-axiom and
   * modal-symmetric. */
  lizard_test_eval(&e, "(set-logic 'S5)");
  lizard_test_eval(&e, "(logic-rule-disable 'modal-5-axiom)");
  lizard_test_eval(&e, "(logic-rule-disable 'modal-symmetric)");
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "S4");

  /* Reset clears the remembered name and the toggle state. */
  lizard_test_eval(&e, "(set-logic 'S5)");
  lizard_logic_config_reset();
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "CoC");

  /* lambda-P now resolves to lambda-P (previously aliased to LF). */
  lizard_test_eval(&e, "(set-logic 'lambda-P)");
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "lambda-P");

  /* === Dependent Pi in box-app === */

  /* Non-dependent Pi: regression check. */
  lizard_test_eval(&e, "(set-logic 'S4)");
  r = lizard_test_eval(&e,
    "(infer (context-extend "
    "         (context-extend (context) (variable 'f (Box (Pi '_ (U 0) (U 0))))) "
    "         (variable 'a (Box (U 0)))) "
    "       (box-app 'f 'a))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (U 0))");

  /* Dependent Pi under S4: works via T-axiom realization. The result
   * type contains an unbox term with a fresh binder name. We can't
   * predict the fresh name, so we verify by structure. */
  r = lizard_test_eval(&e,
    "(Box? (infer (context-extend "
    "         (context-extend (context) (variable 'f (Box (Pi 'x (U 0) 'x)))) "
    "         (variable 'a (Box (U 0)))) "
    "       (box-app 'f 'a)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  /* Dependent Pi under T: also works. */
  lizard_test_eval(&e, "(set-logic 'T)");
  r = lizard_test_eval(&e,
    "(Box? (infer (context-extend "
    "         (context-extend (context) (variable 'f (Box (Pi 'x (U 0) 'x)))) "
    "         (variable 'a (Box (U 0)))) "
    "       (box-app 'f 'a)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  /* Dependent Pi under K: rejected (no T-axiom). */
  lizard_test_eval(&e, "(set-logic 'K)");
  r = lizard_test_eval(&e,
    "(infer (context-extend "
    "         (context-extend (context) (variable 'f (Box (Pi 'x (U 0) 'x)))) "
    "         (variable 'a (Box (U 0)))) "
    "       (box-app 'f 'a))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* Non-dependent Pi works in K (no T-axiom needed). */
  r = lizard_test_eval(&e,
    "(infer (context-extend "
    "         (context-extend (context) (variable 'f (Box (Pi '_ (U 0) (U 0))))) "
    "         (variable 'a (Box (U 0)))) "
    "       (box-app 'f 'a))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (U 0))");

  /* Reduction of dependent box-app composes K-axiom with Pi-beta. */
  lizard_test_eval(&e, "(set-logic 'S4)");
  r = lizard_test_eval(&e,
    "(reduce (box-app (box (Lambda 'x 'x)) (box (U 7))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(box (U 7))");

  /* Direct toggle: disable t-axiom under S4 → dependent Pi rejected. */
  lizard_test_eval(&e, "(set-logic 'S4)");
  lizard_test_eval(&e, "(logic-rule-disable 't-axiom-enabled)");
  r = lizard_test_eval(&e,
    "(infer (context-extend "
    "         (context-extend (context) (variable 'f (Box (Pi 'x (U 0) 'x)))) "
    "         (variable 'a (Box (U 0)))) "
    "       (box-app 'f 'a))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  lizard_logic_config_reset();
  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
