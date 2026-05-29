/* tests/diagnostic_category_paths_test.c
 *
 * Phase 2Z: tokenizer/parser/IO diagnostics all flow through the canonical
 * diagnostic construction path and preserve categories.
 */

#include "lizard_api.h"
#include "test_harness.h"

#include <string.h>

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
  status = lizard_context_eval_string(context, "\"unterminated", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_PARSE_ERROR);
  diagnostic = lizard_context_last_diagnostic(context);
  TEST_ASSERT(diagnostic != NULL);
  TEST_ASSERT_EQ(diagnostic->category, LIZARD_DIAGNOSTIC_CATEGORY_TOKENIZER);
  TEST_ASSERT_EQ(diagnostic->severity, LIZARD_DIAGNOSTIC_SEVERITY_ERROR);
  TEST_ASSERT(strstr(diagnostic->message, "unterminated") != NULL);

  value = NULL;
  status = lizard_context_eval_string(context, "(", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_PARSE_ERROR);
  diagnostic = lizard_context_last_diagnostic(context);
  TEST_ASSERT(diagnostic != NULL);
  TEST_ASSERT_EQ(diagnostic->category, LIZARD_DIAGNOSTIC_CATEGORY_PARSER);
  TEST_ASSERT_EQ(diagnostic->severity, LIZARD_DIAGNOSTIC_SEVERITY_ERROR);
  TEST_ASSERT(diagnostic->message[0] != '\0');

  value = NULL;
  status = lizard_context_eval_file(context,
                                    "build/tests/definitely-missing.lisp",
                                    &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_IO_ERROR);
  diagnostic = lizard_context_last_diagnostic(context);
  TEST_ASSERT(diagnostic != NULL);
  TEST_ASSERT_EQ(diagnostic->category, LIZARD_DIAGNOSTIC_CATEGORY_IO);
  TEST_ASSERT_EQ(diagnostic->severity, LIZARD_DIAGNOSTIC_SEVERITY_ERROR);

  lizard_context_destroy(context);
  lizard_runtime_destroy(runtime);
  TEST_RETURN();
}
