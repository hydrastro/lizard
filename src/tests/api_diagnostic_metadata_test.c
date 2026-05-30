#include "lizard_api.h"
#include "test_harness.h"

#include <string.h>

int main(void) {
  lizard_runtime_t *runtime;
  lizard_context_t *context;
  lizard_value_t *value;
  const lizard_diagnostic_t *diagnostic;
  lizard_status_t status;
  lizard_diagnostic_severity_t severity;
  lizard_diagnostic_category_t category;

  severity = lizard_status_default_severity(LIZARD_STATUS_PARSE_ERROR);
  TEST_ASSERT_EQ(severity, LIZARD_DIAGNOSTIC_SEVERITY_ERROR);
  category = lizard_status_default_category(LIZARD_STATUS_PARSE_ERROR);
  TEST_ASSERT_EQ(category, LIZARD_DIAGNOSTIC_CATEGORY_PARSER);
  TEST_ASSERT_STR(lizard_diagnostic_severity_name(severity), "error");
  TEST_ASSERT_STR(lizard_diagnostic_category_name(category), "parser");

  runtime = lizard_runtime_create(NULL);
  TEST_ASSERT(runtime != NULL);
  context = lizard_context_create(runtime);
  TEST_ASSERT(context != NULL);

  value = NULL;
  status = lizard_context_eval_string(context, "\"unterminated", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_PARSE_ERROR);

  diagnostic = lizard_context_last_diagnostic(context);
  TEST_ASSERT(diagnostic != NULL);
  TEST_ASSERT_EQ(diagnostic->status, LIZARD_STATUS_PARSE_ERROR);
  TEST_ASSERT_EQ(diagnostic->severity, LIZARD_DIAGNOSTIC_SEVERITY_ERROR);
  TEST_ASSERT_EQ(diagnostic->category, LIZARD_DIAGNOSTIC_CATEGORY_TOKENIZER);
  TEST_ASSERT(strstr(diagnostic->message, "unterminated") != NULL);

  value = NULL;
  status = lizard_context_eval_string(context, "(+ 1 2)", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  diagnostic = lizard_context_last_diagnostic(context);
  TEST_ASSERT(diagnostic != NULL);
  TEST_ASSERT_EQ(diagnostic->status, LIZARD_STATUS_OK);
  TEST_ASSERT_EQ(diagnostic->severity, LIZARD_DIAGNOSTIC_SEVERITY_INFO);
  TEST_ASSERT_EQ(diagnostic->category, LIZARD_DIAGNOSTIC_CATEGORY_UNKNOWN);

  lizard_context_destroy(context);
  lizard_runtime_destroy(runtime);
  TEST_RETURN();
}
