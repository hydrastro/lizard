/* tests/kernel_family_split_test.c
 *
 * Regression guard for Recoverable Core Phase 1N.  Kernel primitive
 * families now live in separate implementation files; this test exercises
 * one representative entry point from each family through the public runtime
 * API so future file moves cannot silently drop a registration or helper.
 */

#include "lizard_api.h"
#include "test_harness.h"

int main(void) {
  lizard_runtime_t *runtime;
  lizard_context_t *context;
  lizard_value_t *value;
  lizard_status_t status;

  runtime = lizard_runtime_create(NULL);
  TEST_ASSERT(runtime != NULL);
  context = lizard_context_create(runtime);
  TEST_ASSERT(context != NULL);

  value = NULL;
  status = lizard_context_eval_string(context, "(kernel-infer 0)", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT(value != NULL);
  TEST_ASSERT(!lizard_value_is_error(value));

  value = NULL;
  status = lizard_context_eval_string(context, "(kernel-equal? 0 0)", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT(value != NULL);
  TEST_ASSERT(!lizard_value_is_error(value));

  value = NULL;
  status = lizard_context_eval_string(
      context, "(begin-proof '(Id Nat 0 0)) (tactic-refl) (qed)", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT(value != NULL);
  TEST_ASSERT(!lizard_value_is_error(value));

  value = NULL;
  status = lizard_context_eval_string(context, "(kernel-hole 'Nat)", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT(value != NULL);
  TEST_ASSERT(!lizard_value_is_error(value));

  value = NULL;
  status = lizard_context_eval_string(
      context, "(kernel-define 'phase1n-zero 0 'Nat) (kernel-lookup 'phase1n-zero)",
      &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT(value != NULL);
  TEST_ASSERT(!lizard_value_is_error(value));

  lizard_context_destroy(context);
  lizard_runtime_destroy(runtime);
  TEST_RETURN();
}
