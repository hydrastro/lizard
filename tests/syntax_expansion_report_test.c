/* tests/syntax_expansion_report_test.c
 *
 * Phase 2N: expansion reports inspect macro expansion without evaluating.
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
  lizard_syntax_expansion_report_t *report;
  lizard_expansion_trace_event_t event;
  lizard_source_span_t span;
  const char *summary;
  FILE *json_file;
  lizard_status_t report_status;

  runtime = lizard_runtime_create(NULL);
  TEST_ASSERT(runtime != NULL);
  context = lizard_context_create(runtime);
  TEST_ASSERT(context != NULL);

  report = lizard_context_syntax_expansion_report(
      context, "(+ 1 2)", "report-test.lisp");
  TEST_ASSERT(report != NULL);
  report_status = lizard_syntax_expansion_report_status(report);
  TEST_ASSERT_EQ(report_status, LIZARD_STATUS_OK);
  TEST_ASSERT_EQ(lizard_syntax_expansion_report_form_count(report), 1UL);
  summary = lizard_syntax_expansion_report_expanded_ast_summary(report);
  TEST_ASSERT(summary != NULL);
  TEST_ASSERT(strstr(summary, "+") != NULL);
  TEST_ASSERT(lizard_syntax_expansion_report_span(report, &span));
  TEST_ASSERT(span.filename != NULL);
  TEST_ASSERT(strcmp(span.filename, "report-test.lisp") == 0);
  TEST_ASSERT_EQ(span.start_line, 1);
  TEST_ASSERT_EQ(lizard_syntax_expansion_report_phase(report), 0);
  TEST_ASSERT(lizard_syntax_expansion_report_trace_count(report) > 0UL);
  TEST_ASSERT(lizard_syntax_expansion_report_trace_event(report, 0UL, &event));
  TEST_ASSERT(event.stage != NULL);
  TEST_ASSERT(strcmp(event.stage, "macro-expand") == 0);
  json_file = fopen("build/tests/syntax_expansion_report.json", "wb");
  TEST_ASSERT(json_file != NULL);
  TEST_ASSERT(lizard_syntax_expansion_report_fprint_json(json_file, report));
  fclose(json_file);
  TEST_ASSERT(file_contains("build/tests/syntax_expansion_report.json",
                                        "\"type\":\"lizard-syntax-expansion\""));
  TEST_ASSERT(file_contains("build/tests/syntax_expansion_report.json",
                                        "\"trace\":"));
  (void)remove("build/tests/syntax_expansion_report.json");
  lizard_syntax_expansion_report_destroy(report);

  report = lizard_context_syntax_expansion_report(
      context, "\"unterminated", "bad-report.lisp");
  TEST_ASSERT(report != NULL);
  report_status = lizard_syntax_expansion_report_status(report);
  TEST_ASSERT_EQ(report_status, LIZARD_STATUS_PARSE_ERROR);
  TEST_ASSERT(lizard_syntax_expansion_report_diagnostic(report) != NULL);
  lizard_syntax_expansion_report_destroy(report);

  lizard_context_destroy(context);
  lizard_runtime_destroy(runtime);
  TEST_RETURN();
}
