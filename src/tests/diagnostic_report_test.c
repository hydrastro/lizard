/* tests/diagnostic_report_test.c
 *
 * Phase 2P: diagnostics can be snapshotted into owned reports for tooling.
 */

#include "lizard_api.h"
#include "test_harness.h"

#include <stdio.h>
#include <string.h>

static int file_contains(const char *path, const char *needle) {
  FILE *fp;
  char buffer[4096];
  size_t n;

  fp = fopen(path, "rb");
  if (fp == NULL) {
    return 0;
  }
  n = fread(buffer, 1U, sizeof(buffer) - 1U, fp);
  fclose(fp);
  buffer[n] = '\0';
  return strstr(buffer, needle) != NULL;
}

int main(void) {
  lizard_runtime_t *runtime;
  lizard_context_t *context;
  lizard_value_t *value;
  lizard_status_t status;
  lizard_diagnostic_report_t *report;
  lizard_diagnostic_event_t event;
  lizard_syntax_expansion_report_t *syntax_report;
  FILE *fp;
  char buffer[512];

  runtime = lizard_runtime_create(NULL);
  TEST_ASSERT(runtime != NULL);
  context = lizard_context_create(runtime);
  TEST_ASSERT(context != NULL);

  value = NULL;
  status = lizard_context_eval_string(context, "\"unterminated", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_PARSE_ERROR);

  report = lizard_context_diagnostic_report(context);
  TEST_ASSERT(report != NULL);
  TEST_ASSERT_EQ(lizard_diagnostic_report_count(report), 1UL);
  TEST_ASSERT(lizard_diagnostic_report_event(report, 0UL, &event));
  TEST_ASSERT_EQ(event.status, LIZARD_STATUS_PARSE_ERROR);
  TEST_ASSERT(event.message != NULL);
  TEST_ASSERT(strstr(event.message, "unterminated") != NULL);
  TEST_ASSERT(event.filename != NULL);
  TEST_ASSERT(strcmp(event.filename, "<string>") == 0);
  TEST_ASSERT_EQ(event.start_line, 1);
  TEST_ASSERT(lizard_diagnostic_report_event_string(report, 0UL, buffer,
                                                    sizeof(buffer)));
  TEST_ASSERT(strstr(buffer, "diagnostic-event") != NULL);

  fp = fopen("build/tests/diagnostic_report.txt", "wb");
  TEST_ASSERT(fp != NULL);
  TEST_ASSERT(lizard_diagnostic_report_fprint(fp, report));
  fclose(fp);
  TEST_ASSERT(file_contains("build/tests/diagnostic_report.txt",
                            "lizard-diagnostic-report"));
  TEST_ASSERT(file_contains("build/tests/diagnostic_report.txt",
                            "unterminated"));
  (void)remove("build/tests/diagnostic_report.txt");

  fp = fopen("build/tests/diagnostic_report.json", "wb");
  TEST_ASSERT(fp != NULL);
  TEST_ASSERT(lizard_diagnostic_report_fprint_json(fp, report));
  fclose(fp);
  TEST_ASSERT(file_contains("build/tests/diagnostic_report.json",
                            "\"type\":\"lizard-diagnostic-report\""));
  TEST_ASSERT(file_contains("build/tests/diagnostic_report.json",
                            "unterminated"));
  (void)remove("build/tests/diagnostic_report.json");
  lizard_diagnostic_report_destroy(report);

  syntax_report = lizard_context_syntax_expansion_report(
      context, "\"unterminated", "diagnostic-report.lisp");
  TEST_ASSERT(syntax_report != NULL);
  status = lizard_syntax_expansion_report_status(syntax_report);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_PARSE_ERROR);
  report = lizard_syntax_expansion_report_diagnostic_report(syntax_report);
  TEST_ASSERT(report != NULL);
  TEST_ASSERT_EQ(lizard_diagnostic_report_count(report), 1UL);
  TEST_ASSERT(lizard_diagnostic_report_event(report, 0UL, &event));
  TEST_ASSERT(event.filename != NULL);
  TEST_ASSERT(strcmp(event.filename, "diagnostic-report.lisp") == 0);
  lizard_diagnostic_report_destroy(report);
  lizard_syntax_expansion_report_destroy(syntax_report);

  value = NULL;
  status = lizard_context_eval_string(context, "(+ 1 2)", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  report = lizard_context_diagnostic_report(context);
  TEST_ASSERT(report != NULL);
  TEST_ASSERT_EQ(lizard_diagnostic_report_count(report), 0UL);
  lizard_diagnostic_report_destroy(report);

  lizard_context_destroy(context);
  lizard_runtime_destroy(runtime);
  TEST_RETURN();
}
