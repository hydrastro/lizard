/* tests/runtime_expansion_trace_report_test.c
 *
 * Phase 2J: callers can retrieve structured expansion-trace events instead
 * of only the latest stage/detail strings.
 */

#include "lizard_api.h"
#include "test_harness.h"

#include <string.h>

int main(void) {
  lizard_runtime_t *runtime;
  lizard_context_t *context;
  lizard_value_t *value;
  lizard_expansion_trace_event_t event;
  char buffer[512];
  lizard_status_t status;

  runtime = lizard_runtime_create(NULL);
  TEST_ASSERT(runtime != NULL);
  context = lizard_context_create(runtime);
  TEST_ASSERT(context != NULL);

  value = NULL;
  status = lizard_context_eval_string(context,
      "(define-syntax double (syntax-rules () ((double x) (+ x x))))",
      &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);

  lizard_context_set_trace_expansion(context, 1);
  value = NULL;
  status = lizard_context_eval_string(context, "(double 21)", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT(value != NULL);
  TEST_ASSERT(!lizard_value_is_error(value));

  TEST_ASSERT(lizard_context_expansion_trace_count(context) == 1UL);
  TEST_ASSERT(lizard_context_expansion_trace_event(context, 0UL, &event));
  TEST_ASSERT(event.stage != NULL);
  TEST_ASSERT(event.detail != NULL);
  TEST_ASSERT(strcmp(event.stage, "macro-expand") == 0);
  TEST_ASSERT(strstr(event.detail, "existing-expander-adapter") != NULL);
  TEST_ASSERT(event.origin_filename != NULL);
  TEST_ASSERT(strcmp(event.origin_filename, "<string>") == 0);
  TEST_ASSERT(event.origin_line == 1);
  TEST_ASSERT(event.origin_column == 1);

  buffer[0] = '\0';
  TEST_ASSERT(lizard_context_expansion_trace_event_string(context, 0UL,
                                                          buffer,
                                                          sizeof(buffer)));
  TEST_ASSERT(strstr(buffer, "surface-trace-event") != NULL);
  TEST_ASSERT(strstr(buffer, "macro-expand") != NULL);
  TEST_ASSERT(strstr(buffer, "<string>") != NULL);

  TEST_ASSERT(!lizard_context_expansion_trace_event(context, 99UL, &event));
  TEST_ASSERT(event.stage == NULL);
  buffer[0] = 'x';
  TEST_ASSERT(!lizard_context_expansion_trace_event_string(context, 99UL,
                                                           buffer,
                                                           sizeof(buffer)));
  TEST_ASSERT(strstr(buffer, "null") != NULL);

  lizard_context_destroy(context);
  lizard_runtime_destroy(runtime);
  TEST_RETURN();
}
