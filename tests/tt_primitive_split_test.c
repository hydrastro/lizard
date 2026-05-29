/* tests/tt_primitive_split_test.c
 *
 * Regression guard for Recoverable Core Phase 1J. HIT and truncation
 * primitives now live outside the monolithic primitives.c; this test
 * exercises the moved primitive entry points directly through normal
 * evaluation.
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;

  lizard_test_env_init(&e);
  lizard_tt_hit_registry_reset();
  lizard_logic_config_reset();

  r = lizard_test_eval(&e, "(HIT-constructor 'point (U 0))");
  TEST_ASSERT_STR(lizard_test_format(r), "(HIT-constructor point (U 0))");

  r = lizard_test_eval(&e, "(HIT-path 'loop 'point 'point)");
  TEST_ASSERT_STR(lizard_test_format(r), "(HIT-path loop point point)");

  lizard_test_eval(&e,
                   "(define circle (HIT 'Circle "
                   "  (HIT-constructor 'point) "
                   "  (HIT-path 'loop 'point 'point)))");
  r = lizard_test_eval(&e, "(HIT-lookup 'Circle)");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(HIT Circle (HIT-constructor point) (HIT-path loop point point))");

  lizard_test_eval(&e, "(set-logic 'truncations)");
  r = lizard_test_eval(&e, "(Trunc-level (Trunc 2 (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "2");
  r = lizard_test_eval(&e, "(Trunc-type (Trunc 2 (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");
  r = lizard_test_eval(&e, "(trunc-value (trunc 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "x");
  r = lizard_test_eval(&e, "(trunc-elim-prop (trunc-elim 'C 'h 'p 'v))");
  TEST_ASSERT_STR(lizard_test_format(r), "p");

  lizard_tt_hit_registry_reset();
  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
