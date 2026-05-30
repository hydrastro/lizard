/* tests/tt_optional_scaffold_split_test.c
 *
 * Regression guard for Recoverable Core Phase 1K. Optional S1/theory-
 * extension scaffolds and named logic bundle primitives now live outside
 * primitives.c; this test exercises the moved entry points through normal
 * evaluation.
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;

  lizard_test_env_init(&e);
  lizard_logic_config_reset();

  r = lizard_test_eval(&e, "(set-logic 'proof-scaffold)");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  r = lizard_test_eval(&e, "(S1)");
  TEST_ASSERT_STR(lizard_test_format(r), "S1");
  r = lizard_test_eval(&e, "(base)");
  TEST_ASSERT_STR(lizard_test_format(r), "base");
  r = lizard_test_eval(&e, "(loop)");
  TEST_ASSERT_STR(lizard_test_format(r), "loop");

  r = lizard_test_eval(&e, "(S1? (S1))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e, "(base? (base))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e, "(loop? (loop))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  r = lizard_test_eval(&e,
                       "(theory-extension 'my-rule (S1) (Trunc 0 (S1)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(theory-extension my-rule S1 (Trunc 0 S1))");
  r = lizard_test_eval(&e,
      "(theory-extension-name (theory-extension 'my-rule (S1)))");
  TEST_ASSERT_STR(lizard_test_format(r), "my-rule");
  r = lizard_test_eval(&e,
      "(theory-extension? (theory-extension 'my-rule (S1)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  r = lizard_test_eval(&e, "(current-logic)");
  TEST_ASSERT_STR(lizard_test_format(r), "proof-scaffold");
  r = lizard_test_eval(&e, "(list-logics)");
  TEST_ASSERT(r != NULL);

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
