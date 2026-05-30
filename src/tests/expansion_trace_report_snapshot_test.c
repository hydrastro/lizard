/* tests/expansion_trace_report_snapshot_test.c
 *
 * Phase 2K: owned trace reports must outlive later context evaluations and
 * remain independent snapshots, not borrowed views into last_expanded_surface.
 */

#include "lizard_api.h"
#include "test_harness.h"

#include <string.h>

int main(void) {
  lizard_runtime_t *runtime;
  lizard_context_t *context;
  lizard_value_t *value;
  lizard_expansion_trace_report_t *report;
  lizard_expansion_trace_event_t event;
  lizard_status_t status;
  char buffer[512];
  unsigned long report_count;

  runtime = lizard_runtime_create(NULL);
  TEST_ASSERT(runtime != NULL);
  context = lizard_context_create(runtime);
  TEST_ASSERT(context != NULL);

  lizard_context_set_trace_expansion(context, 1);
  value = NULL;
  status = lizard_context_eval_string(context, "(+ 1 2)", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT(value != NULL);
  TEST_ASSERT(lizard_context_expansion_trace_count(context) > 0UL);

  report = lizard_context_expansion_trace_report(context);
  TEST_ASSERT(report != NULL);
  report_count = lizard_expansion_trace_report_count(report);
  TEST_ASSERT(report_count > 0UL);

  TEST_ASSERT(lizard_expansion_trace_report_event(report, 0UL, &event));
  TEST_ASSERT(event.stage != NULL);
  TEST_ASSERT(strcmp(event.stage, "macro-expand") == 0);
  TEST_ASSERT(event.origin_filename != NULL);
  TEST_ASSERT(strcmp(event.origin_filename, "<string>") == 0);

  TEST_ASSERT(lizard_expansion_trace_report_event_string(report, 0UL, buffer,
                                                        sizeof(buffer)));
  TEST_ASSERT(strstr(buffer, "macro-expand") != NULL);

  /* Mutate the context's latest expansion; the report must remain a stable
   * snapshot and retain the original first event. */
  value = NULL;
  status = lizard_context_eval_string(context, "(+ 40 2)", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT_EQ(lizard_expansion_trace_report_count(report), report_count);
  TEST_ASSERT(lizard_expansion_trace_report_event(report, 0UL, &event));
  TEST_ASSERT(event.stage != NULL);
  TEST_ASSERT(strcmp(event.stage, "macro-expand") == 0);

  lizard_context_destroy(context);
  lizard_runtime_destroy(runtime);

  /* The report must also outlive the context/runtime. */
  TEST_ASSERT(lizard_expansion_trace_report_event(report, 0UL, &event));
  TEST_ASSERT(event.stage != NULL);
  TEST_ASSERT(strcmp(event.stage, "macro-expand") == 0);
  TEST_ASSERT(!lizard_expansion_trace_report_event(report, 999UL, &event));

  lizard_expansion_trace_report_destroy(report);
  TEST_RETURN();
}
