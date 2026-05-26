/* tests/tt_logic_cube_test.c
 *
 * Tests for Phase M.2: the three lambda-cube axes wired into the
 * Pi typing rule. This is the first phase where the logic-rule
 * registry actually affects what type-checks.
 *
 * Encoding: A Pi from (U n) to (U m) is classified by axis:
 *   - dom_level == 1, cod_level == 1: simple arrow (no axis) or
 *     type-depends-on-term if binder appears free in codomain
 *   - dom_level >= 2, cod_level == 1: term-depends-on-type (System F)
 *   - dom_level >= 2, cod_level >= 2: type-depends-on-type (Fω)
 *
 * Default: no rules registered → all Pi shapes allowed (backward-
 * compatible CoC behavior).
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);
  lizard_logic_config_reset();

  /* Baseline: no rules registered, all Pi shapes accepted. */
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 0) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 1) (U 1)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 2)");
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 1) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 2)");

  /* Disable type-depends-on-type. */
  lizard_test_eval(&e, "(logic-rule-disable 'type-depends-on-type)");

  /* Fω-style Pi rejected. */
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 1) (U 1)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* Simple arrow still accepted. */
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 0) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* System F Pi still accepted (its axis is term-depends-on-type,
   * which we haven't disabled). */
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 1) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 2)");

  /* Re-enable type-depends-on-type and disable term-depends-on-type. */
  lizard_test_eval(&e, "(logic-rule-enable 'type-depends-on-type)");
  lizard_test_eval(&e, "(logic-rule-disable 'term-depends-on-type)");

  /* System F Pi rejected. */
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 1) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* Fω Pi accepted. */
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 1) (U 1)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 2)");

  /* Simple arrow accepted (no axis required). */
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 0) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* Reset returns full freedom. */
  lizard_test_eval(&e, "(logic-config-reset)");
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 1) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 2)");
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 1) (U 1)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 2)");

  /* Unregistered rules: defaulted-on (not -off), preserving back-compat. */
  /* Verify that enabling all three explicitly gives same behavior. */
  lizard_test_eval(&e, "(logic-rule-enable 'term-depends-on-type)");
  lizard_test_eval(&e, "(logic-rule-enable 'type-depends-on-term)");
  lizard_test_eval(&e, "(logic-rule-enable 'type-depends-on-type)");
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 0) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 1) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 2)");
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 1) (U 1)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 2)");

  lizard_logic_config_reset();
  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
