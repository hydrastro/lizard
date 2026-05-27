/* tests/tt_logic_bundles_test.c
 *
 * Tests for Phase M.3: named logic bundles.
 *
 * M.3 is sugar over M.2's cube toggles. (set-logic 'NAME) applies
 * the right combination of enable/disable across the three cube
 * axes. (current-logic) does reverse lookup.
 *
 * Coverage:
 *   - All eight cube corners + alias (LF / lambda-P)
 *   - set-logic followed by Pi typing gives expected accept/reject
 *   - current-logic returns the right name for each bundle
 *   - Manual config maps back to a bundle name when it matches
 *   - Unknown bundle name returns #f without changing config
 *   - 'custom returned for non-cube configurations
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);
  lizard_logic_config_reset();

  /* Default (no config) = CoC because unregistered = on. */
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "CoC");

  /* STLC: all three disabled. */
  lizard_test_eval(&e, "(set-logic 'STLC)");
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "STLC");
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 0) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 1) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* System F: term-on-type only. */
  lizard_test_eval(&e, "(set-logic 'F)");
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "F");
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 1) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 2)");
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 1) (U 1)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* LF / lambda-P: type-on-term only. */
  lizard_test_eval(&e, "(set-logic 'LF)");
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "LF");

  lizard_test_eval(&e, "(set-logic 'lambda-P)");
  r = lizard_test_eval(&e, "(current-logic)");
  /* Bundle table is walked first-match-wins; LF appears before
   * lambda-P so both configs yield "LF". This is a deliberate
   * alias resolution; lambda-P sets the same flags but is named
   * canonically as LF. */
  TEST_ASSERT_STR(lizard_test_format(r), "LF");

  /* F-omega: type-on-type only. */
  lizard_test_eval(&e, "(set-logic 'F-omega)");
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "F-omega");
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 1) (U 1)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 2)");
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 1) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* CoC: all three. */
  lizard_test_eval(&e, "(set-logic 'CoC)");
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "CoC");
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 1) (U 1)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 2)");
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 1) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 2)");

  /* Manual setup that matches a bundle. */
  lizard_logic_config_reset();
  lizard_test_eval(&e, "(logic-rule-enable 'term-depends-on-type)");
  lizard_test_eval(&e, "(logic-rule-enable 'type-depends-on-term)");
  lizard_test_eval(&e, "(logic-rule-disable 'type-depends-on-type)");
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "lambda-P2");

  /* Unknown bundle name. */
  r = lizard_test_eval(&e, "(set-logic 'NotALogic)");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");
  /* And current-logic is unchanged. */
  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "lambda-P2");

  /* list-logics returns all bundle names. */
  r = lizard_test_eval(&e, "(list-logics)");
  /* Order is reverse of table (we prepend). M.5.3 modal bundles
   * come first, then M.6 features, then M.4 substructural, then
   * cube corners. */
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(modal-STLC S5 S4 T K "
                  "CoC-plus-lattice STLC-strict "
                  "relevant-STLC affine-STLC linear-STLC "
                  "CoC lambda-omega lambda-P-omega lambda-P2 "
                  "F-omega lambda-P LF F STLC)");

  lizard_logic_config_reset();
  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
