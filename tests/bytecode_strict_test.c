#include "lizard_api.h"
#include "test_harness.h"

int main(void) {
  lizard_runtime_t *runtime;
  lizard_context_t *context;
  lizard_value_t *value;
  const lizard_diagnostic_t *diagnostic;
  lizard_status_t status;

  runtime = lizard_runtime_create(NULL);
  TEST_ASSERT(runtime != NULL);
  context = lizard_context_create(runtime);
  TEST_ASSERT(context != NULL);

  value = NULL;
  status = lizard_context_eval_string(context, "(vm-eval '(+ 1 2))", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT(value != NULL);
  TEST_ASSERT(!lizard_value_is_error(value));

  value = NULL;
  status = lizard_context_eval_string(context,
      "(vm-eval (Pi 'x (U 0) (U 0)))", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_EVAL_ERROR);
  TEST_ASSERT(value != NULL);
  TEST_ASSERT(lizard_value_is_error(value));
  diagnostic = lizard_context_last_diagnostic(context);
  TEST_ASSERT(diagnostic != NULL);
  TEST_ASSERT_EQ(diagnostic->status, LIZARD_STATUS_EVAL_ERROR);

  value = NULL;
  status = lizard_context_eval_string(context, "(+ 4 5)", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT(value != NULL);
  TEST_ASSERT(!lizard_value_is_error(value));

  lizard_context_destroy(context);
  lizard_runtime_destroy(runtime);
  TEST_RETURN();
}
