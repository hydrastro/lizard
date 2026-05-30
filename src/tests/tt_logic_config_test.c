/* tests/tt_logic_config_test.c
 *
 * Tests for Phase M.1: logic-rule configuration registry.
 *
 * M.1 lands the infrastructure ONLY. No specific rule is pre-
 * registered and no kernel-typing site consults the registry.
 * Future phases (M.2 onwards) will register specific rules and
 * wire type-checking decisions to query the active configuration.
 *
 * Coverage:
 *   - Empty initial state
 *   - Registration adds entries; defaults to disabled
 *   - Enable / disable mutates state
 *   - Auto-registration on enable / disable
 *   - Unknown rules return 'unknown
 *   - Size and reset
 *   - Multiple rules tracked independently
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* Reset so we have a clean slate even if other tests ran first. */
  lizard_logic_config_reset();

  /* Empty initial state. */
  r = lizard_test_eval(&e, "(logic-config-size)");
  TEST_ASSERT_STR(lizard_test_format(r), "0");

  r = lizard_test_eval(&e, "(logic-config)");
  TEST_ASSERT_STR(lizard_test_format(r), "()");

  /* Register some rules. */
  lizard_test_eval(&e, "(logic-rule-register 'cube-x)");
  lizard_test_eval(&e, "(logic-rule-register 'cube-y)");

  r = lizard_test_eval(&e, "(logic-config-size)");
  TEST_ASSERT_STR(lizard_test_format(r), "2");

  /* Default: disabled. */
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'cube-x)");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'cube-y)");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");

  /* Enable changes state. */
  lizard_test_eval(&e, "(logic-rule-enable 'cube-x)");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'cube-x)");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'cube-y)");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");

  /* Disable. */
  lizard_test_eval(&e, "(logic-rule-disable 'cube-x)");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'cube-x)");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");

  /* Unknown rule. */
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'never-heard-of)");
  TEST_ASSERT_STR(lizard_test_format(r), "unknown");

  /* Auto-register on enable. */
  lizard_test_eval(&e, "(logic-rule-enable 'fresh-rule)");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'fresh-rule)");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e, "(logic-config-size)");
  TEST_ASSERT_STR(lizard_test_format(r), "3");

  /* Auto-register on disable. */
  lizard_test_eval(&e, "(logic-rule-disable 'another-fresh)");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'another-fresh)");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");
  r = lizard_test_eval(&e, "(logic-config-size)");
  TEST_ASSERT_STR(lizard_test_format(r), "4");

  /* Double-register is a no-op. */
  lizard_test_eval(&e, "(logic-rule-register 'cube-x)");
  r = lizard_test_eval(&e, "(logic-config-size)");
  TEST_ASSERT_STR(lizard_test_format(r), "4");

  /* Reset clears everything. */
  lizard_test_eval(&e, "(logic-config-reset)");
  r = lizard_test_eval(&e, "(logic-config-size)");
  TEST_ASSERT_STR(lizard_test_format(r), "0");
  r = lizard_test_eval(&e, "(logic-rule-enabled? 'cube-x)");
  TEST_ASSERT_STR(lizard_test_format(r), "unknown");

  /* Snapshot/restore at the C level (no Lisp wrapper for M.1). */
  lizard_test_eval(&e, "(logic-rule-enable 'a)");
  lizard_test_eval(&e, "(logic-rule-enable 'b)");
  {
    void *snap = lizard_logic_snapshot();
    /* Mutate the current config. */
    lizard_test_eval(&e, "(logic-rule-disable 'a)");
    r = lizard_test_eval(&e, "(logic-rule-enabled? 'a)");
    TEST_ASSERT_STR(lizard_test_format(r), "#f");
    /* Restore. */
    lizard_logic_restore(snap);
    r = lizard_test_eval(&e, "(logic-rule-enabled? 'a)");
    TEST_ASSERT_STR(lizard_test_format(r), "#t");
  }

  lizard_logic_config_reset();
  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
