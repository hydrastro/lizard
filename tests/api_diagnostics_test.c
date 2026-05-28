#include "lizard_api.h"
#include "test_harness.h"

#include <stdio.h>
#include <string.h>

int main(void) {
  lizard_runtime_t *runtime;
  lizard_context_t *context;
  lizard_value_t *value;
  const lizard_diagnostic_t *diagnostic;
  lizard_status_t status;
  FILE *fp;
  const char *bad_file;

  runtime = lizard_runtime_create(NULL);
  TEST_ASSERT(runtime != NULL);
  context = lizard_context_create(runtime);
  TEST_ASSERT(context != NULL);

  value = NULL;
  status = lizard_context_eval_string(context, "(define x", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_PARSE_ERROR);
  TEST_ASSERT(value == NULL);
  diagnostic = lizard_context_last_diagnostic(context);
  TEST_ASSERT(diagnostic != NULL);
  TEST_ASSERT_EQ(diagnostic->status, LIZARD_STATUS_PARSE_ERROR);
  TEST_ASSERT(diagnostic->message[0] != '\0');
  TEST_ASSERT(diagnostic->span.start_line > 0);

  /* The same context must remain usable after a parse failure. */
  value = NULL;
  status = lizard_context_eval_string(context, "(+ 1 2)", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT(value != NULL);
  diagnostic = lizard_context_last_diagnostic(context);
  TEST_ASSERT(diagnostic != NULL);
  TEST_ASSERT_EQ(diagnostic->status, LIZARD_STATUS_OK);

  value = NULL;
  status = lizard_context_eval_string(context, "missing-symbol", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_EVAL_ERROR);
  TEST_ASSERT(lizard_value_is_error(value));
  diagnostic = lizard_context_last_diagnostic(context);
  TEST_ASSERT(diagnostic != NULL);
  TEST_ASSERT_EQ(diagnostic->status, LIZARD_STATUS_EVAL_ERROR);

  value = NULL;
  status = lizard_context_eval_string(context, "\"unterminated", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_PARSE_ERROR);
  TEST_ASSERT(value == NULL);
  diagnostic = lizard_context_last_diagnostic(context);
  TEST_ASSERT(diagnostic != NULL);
  TEST_ASSERT_EQ(diagnostic->status, LIZARD_STATUS_PARSE_ERROR);
  TEST_ASSERT(diagnostic->message[0] != '\0');
  TEST_ASSERT(diagnostic->span.filename != NULL);
  TEST_ASSERT_EQ(diagnostic->span.start_line, 1);
  TEST_ASSERT_EQ(diagnostic->span.start_column, 1);

  bad_file = "build/tests/api_bad_input.lisp";
  fp = fopen(bad_file, "wb");
  TEST_ASSERT(fp != NULL);
  fputs("\n  \"unterminated", fp);
  fclose(fp);

  value = NULL;
  status = lizard_context_eval_file(context, bad_file, &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_PARSE_ERROR);
  diagnostic = lizard_context_last_diagnostic(context);
  TEST_ASSERT(diagnostic != NULL);
  TEST_ASSERT_EQ(diagnostic->status, LIZARD_STATUS_PARSE_ERROR);
  TEST_ASSERT(diagnostic->span.filename != NULL);
  TEST_ASSERT(strcmp(diagnostic->span.filename, bad_file) == 0);
  TEST_ASSERT_EQ(diagnostic->span.start_line, 2);
  TEST_ASSERT_EQ(diagnostic->span.start_column, 3);

  lizard_context_destroy(context);
  lizard_runtime_destroy(runtime);
  TEST_RETURN();
}
