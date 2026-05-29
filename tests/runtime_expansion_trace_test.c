/* tests/runtime_expansion_trace_test.c
 *
 * Phase 2I: runtime API can optionally route evaluation through the
 * SurfaceTerm/macro-expansion adapter and expose untrusted trace metadata,
 * while default evaluation remains unchanged.
 */

#include "lizard_api.h"
#include "test_harness.h"

#include <string.h>

int main(void) {
  lizard_runtime_t *runtime;
  lizard_context_t *context;
  lizard_value_t *value;
  lizard_source_span_t span;
  lizard_status_t status;
  const char *stage;
  const char *detail;

  runtime = lizard_runtime_create(NULL);
  TEST_ASSERT(runtime != NULL);
  context = lizard_context_create(runtime);
  TEST_ASSERT(context != NULL);
  TEST_ASSERT(lizard_context_trace_expansion_enabled(context) == 0);
  TEST_ASSERT(lizard_context_last_expansion_trace_count(context) == 0UL);

  value = NULL;
  status = lizard_context_eval_string(context,
      "(define-syntax twice (syntax-rules () ((twice x) (+ x x))))",
      &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT(lizard_context_last_expansion_trace_count(context) == 0UL);

  value = NULL;
  status = lizard_context_eval_string(context, "(twice 21)", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT(value != NULL);
  TEST_ASSERT(!lizard_value_is_error(value));
  TEST_ASSERT(lizard_context_last_expansion_trace_count(context) == 0UL);

  lizard_context_set_trace_expansion(context, 1);
  TEST_ASSERT(lizard_context_trace_expansion_enabled(context) == 1);

  value = NULL;
  status = lizard_context_eval_string(context, "(twice 21)", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT(value != NULL);
  TEST_ASSERT(!lizard_value_is_error(value));
  TEST_ASSERT(lizard_context_last_expansion_trace_count(context) == 1UL);

  stage = lizard_context_last_expansion_stage(context);
  detail = lizard_context_last_expansion_detail(context);
  TEST_ASSERT(stage != NULL);
  TEST_ASSERT(detail != NULL);
  TEST_ASSERT(strcmp(stage, "macro-expand") == 0);
  TEST_ASSERT(strstr(detail, "existing-expander-adapter") != NULL);
  TEST_ASSERT(lizard_context_last_expansion_span(context, &span));
  TEST_ASSERT(span.filename != NULL);
  TEST_ASSERT(strcmp(span.filename, "<string>") == 0);

  lizard_context_set_trace_expansion(context, 0);
  TEST_ASSERT(lizard_context_trace_expansion_enabled(context) == 0);
  TEST_ASSERT(lizard_context_last_expansion_trace_count(context) == 0UL);

  lizard_context_destroy(context);
  lizard_runtime_destroy(runtime);
  TEST_RETURN();
}
