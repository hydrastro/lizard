/* tests/api_report_types_test.c
 *
 * Regression for Phase 2W: public report/syntax-object boundary types must be
 * visible from lizard_api.h.  This prevents internal headers such as
 * surface_term.h from exposing prototypes that mention undefined public types.
 */

#include "lizard_api.h"
#include "test_harness.h"

#include <string.h>

int main(void) {
  lizard_expansion_trace_event_t trace_event;
  lizard_diagnostic_event_t diagnostic_event;
  lizard_expansion_trace_report_t *trace_report;
  lizard_diagnostic_report_t *diagnostic_report;
  lizard_syntax_expansion_report_t *syntax_report;

  trace_report = NULL;
  diagnostic_report = NULL;
  syntax_report = NULL;

  trace_event.stage = "macro-expand";
  trace_event.detail = "public-api-boundary";
  trace_event.origin_filename = "api-report-types.lisp";
  trace_event.origin_line = 1;
  trace_event.origin_column = 2;
  trace_event.origin_phase = 0;
  trace_event.origin_scope_summary = 0UL;

  TEST_ASSERT(strcmp(trace_event.stage, "macro-expand") == 0);
  TEST_ASSERT(strcmp(trace_event.detail, "public-api-boundary") == 0);
  TEST_ASSERT(trace_event.origin_line == 1);
  TEST_ASSERT(trace_event.origin_column == 2);
  TEST_ASSERT(trace_event.origin_phase == 0);
  TEST_ASSERT(trace_event.origin_scope_summary == 0UL);

  diagnostic_event.status = LIZARD_STATUS_PARSE_ERROR;
  diagnostic_event.severity = LIZARD_DIAGNOSTIC_SEVERITY_ERROR;
  diagnostic_event.category = LIZARD_DIAGNOSTIC_CATEGORY_PARSER;
  diagnostic_event.message = "parse failed";
  diagnostic_event.filename = "api-report-types.lisp";
  diagnostic_event.start_line = 1;
  diagnostic_event.start_column = 1;
  diagnostic_event.end_line = 1;
  diagnostic_event.end_column = 2;
  diagnostic_event.start_offset = 0;
  diagnostic_event.end_offset = 1;

  TEST_ASSERT(diagnostic_event.status == LIZARD_STATUS_PARSE_ERROR);
  TEST_ASSERT(diagnostic_event.severity == LIZARD_DIAGNOSTIC_SEVERITY_ERROR);
  TEST_ASSERT(diagnostic_event.category == LIZARD_DIAGNOSTIC_CATEGORY_PARSER);
  TEST_ASSERT(strcmp(diagnostic_event.message, "parse failed") == 0);

  TEST_ASSERT(trace_report == NULL);
  TEST_ASSERT(diagnostic_report == NULL);
  TEST_ASSERT(syntax_report == NULL);

  TEST_RETURN();
}
