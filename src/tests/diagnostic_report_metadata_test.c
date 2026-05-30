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
  unsigned long count;
  FILE *fp;

  runtime = lizard_runtime_create(NULL);
  TEST_ASSERT(runtime != NULL);
  context = lizard_context_create(runtime);
  TEST_ASSERT(context != NULL);

  value = NULL;
  status = lizard_context_eval_string(context, "\"unterminated", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_PARSE_ERROR);

  report = lizard_context_diagnostic_report(context);
  TEST_ASSERT(report != NULL);
  count = lizard_diagnostic_report_count(report);
  TEST_ASSERT_EQ(count, 1);
  TEST_ASSERT(lizard_diagnostic_report_event(report, 0UL, &event));
  TEST_ASSERT_EQ(event.status, LIZARD_STATUS_PARSE_ERROR);
  TEST_ASSERT_EQ(event.severity, LIZARD_DIAGNOSTIC_SEVERITY_ERROR);
  TEST_ASSERT_EQ(event.category, LIZARD_DIAGNOSTIC_CATEGORY_TOKENIZER);
  TEST_ASSERT_STR(lizard_diagnostic_severity_name(event.severity), "error");
  TEST_ASSERT_STR(lizard_diagnostic_category_name(event.category),
                  "tokenizer");

  fp = fopen("build/tests/diagnostic_report_metadata.txt", "wb");
  TEST_ASSERT(fp != NULL);
  TEST_ASSERT(lizard_diagnostic_report_fprint(fp, report));
  fclose(fp);
  TEST_ASSERT(file_contains("build/tests/diagnostic_report_metadata.txt",
                            "lizard-diagnostic-report\tv=2"));
  TEST_ASSERT(file_contains("build/tests/diagnostic_report_metadata.txt",
                            "severity=error"));
  TEST_ASSERT(file_contains("build/tests/diagnostic_report_metadata.txt",
                            "category=tokenizer"));
  (void)remove("build/tests/diagnostic_report_metadata.txt");

  fp = fopen("build/tests/diagnostic_report_metadata.json", "wb");
  TEST_ASSERT(fp != NULL);
  TEST_ASSERT(lizard_diagnostic_report_fprint_json(fp, report));
  fclose(fp);
  TEST_ASSERT(file_contains("build/tests/diagnostic_report_metadata.json",
                            "\"version\":2"));
  TEST_ASSERT(file_contains("build/tests/diagnostic_report_metadata.json",
                            "\"severity\":\"error\""));
  TEST_ASSERT(file_contains("build/tests/diagnostic_report_metadata.json",
                            "\"category\":\"tokenizer\""));
  (void)remove("build/tests/diagnostic_report_metadata.json");

  lizard_diagnostic_report_destroy(report);
  lizard_context_destroy(context);
  lizard_runtime_destroy(runtime);
  TEST_RETURN();
}
