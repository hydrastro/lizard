#include <lizard.h>
#include "test_harness.h"

int main(void) {
  lizard_runtime_options_t options;
  lizard_expansion_trace_event_t trace_event;
  lizard_runtime_options_default(&options);
  TEST_ASSERT(options.initial_heap_size != 0U);
  TEST_ASSERT(options.max_segment_size != 0U);
  trace_event.stage = "public-header";
  trace_event.detail = "trace-event-visible";
  trace_event.origin_filename = "public_header_test";
  trace_event.origin_line = 1;
  trace_event.origin_column = 1;
  trace_event.origin_phase = 0;
  trace_event.origin_scope_summary = 0UL;
  TEST_ASSERT(trace_event.stage != NULL);
  return 0;
}
