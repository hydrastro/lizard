/* tests/kernel_primitive_split_test.c
 *
 * Regression guard for Recoverable Core Phase 1L's split of kernel,
 * proof, and tactic primitives out of primitives.c.
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
  status = lizard_context_eval_string(context, "(kernel-check 0 'Nat)", &value);
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
  status = lizard_context_eval_string(context, "(kernel-reduce '(succ 0))", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT(value != NULL);
  TEST_ASSERT(!lizard_value_is_error(value));

  lizard_context_destroy(context);
  lizard_runtime_destroy(runtime);
  TEST_RETURN();
}
