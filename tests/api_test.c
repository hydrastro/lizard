#include "lizard_api.h"
#include "test_harness.h"

#include <stdio.h>

int main(void) {
  lizard_runtime_options_t options;
  lizard_runtime_t *runtime;
  lizard_context_t *context;
  lizard_value_t *value;
  lizard_status_t status;
  lizard_value_type_t value_type;

  lizard_runtime_options_default(&options);
  runtime = lizard_runtime_create(&options);
  TEST_ASSERT(runtime != NULL);

  context = lizard_context_create(runtime);
  TEST_ASSERT(context != NULL);

  value = NULL;
  status = lizard_context_eval_string(context, "(+ 40 2)", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT(value != NULL);
  value_type = lizard_value_type(value);
  TEST_ASSERT_EQ(value_type, LIZARD_VALUE_NUMBER);
  TEST_ASSERT_STR(lizard_value_type_name(value_type), "number");

  value = NULL;
  status = lizard_context_eval_string(context, "missing-symbol", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_ERROR);
  TEST_ASSERT(lizard_value_is_error(value));
  TEST_ASSERT(lizard_value_error_code(value) != 0);
  TEST_ASSERT(lizard_context_last_error(context) != NULL);

  lizard_context_destroy(context);
  lizard_runtime_destroy(runtime);
  TEST_RETURN();
}
