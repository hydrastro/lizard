#include "test_harness.h"
#include <lizard_api.h>

int main(void) {
  lizard_runtime_t *runtime;
  lizard_context_t *context;
  lizard_value_t *value;
  lizard_source_span_t span;
  const lizard_diagnostic_t *diagnostic;
  lizard_status_t status;

  runtime = lizard_runtime_create(NULL);
  TEST_ASSERT(runtime != NULL);
  context = lizard_context_create(runtime);
  TEST_ASSERT(context != NULL);

  value = NULL;
  status = lizard_context_eval_string(context, "\n  42", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT(value != NULL);
  TEST_ASSERT(lizard_value_source_span(value, &span));
  TEST_ASSERT_EQ((int)span.start_line, 2);
  TEST_ASSERT_EQ((int)span.start_column, 3);

  diagnostic = lizard_context_last_diagnostic(context);
  TEST_ASSERT(diagnostic != NULL);
  TEST_ASSERT_EQ(diagnostic->status, LIZARD_STATUS_OK);

  status = lizard_context_eval_file(context, "tests/definitely-not-a-file.lisp", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_IO_ERROR);
  diagnostic = lizard_context_last_diagnostic(context);
  TEST_ASSERT(diagnostic != NULL);
  TEST_ASSERT_EQ(diagnostic->status, LIZARD_STATUS_IO_ERROR);

  lizard_context_destroy(context);
  lizard_runtime_destroy(runtime);
  return 0;
}
