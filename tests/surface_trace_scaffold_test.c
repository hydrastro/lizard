/* tests/surface_trace_scaffold_test.c
 *
 * Phase 2F: syntax-object transformation tracing scaffold.  These records are
 * debugging metadata only; evaluator and macro semantics do not consult them.
 */

#include "surface_term.h"
#include "test_harness.h"
#include "test_helpers.h"

#include <string.h>

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *value;
  lizard_ast_node_t *rewritten_value;
  lizard_surface_term_t *origin;
  lizard_surface_term_t *rewritten;
  lizard_surface_term_t *copied;
  lizard_source_span_t span;
  char *debug;
  char *surface_debug;

  lizard_test_env_init(&e);

  value = lizard_test_eval(&e, "(+ 1 2)");
  rewritten_value = lizard_test_eval(&e, "(+ 3 4)");
  TEST_ASSERT(value != NULL);
  TEST_ASSERT(rewritten_value != NULL);

  span.filename = "phase2f.liz";
  span.start_line = 12;
  span.start_column = 7;
  span.end_line = 12;
  span.end_column = 14;
  span.start_offset = 120;
  span.end_offset = 127;

  origin = lizard_surface_from_ast_span(e.heap, value, span);
  TEST_ASSERT(origin != NULL);
  lizard_surface_set_phase(origin, 2);
  lizard_surface_add_scope(origin, 0x21UL);

  TEST_ASSERT(lizard_surface_trace_count(origin) == 0UL);
  TEST_ASSERT(lizard_surface_trace_add(e.heap, origin, "reader",
                                       "parsed datum", origin) == 1);
  TEST_ASSERT(lizard_surface_trace_count(origin) == 1UL);
  TEST_ASSERT(strcmp(lizard_surface_trace_latest_stage(origin), "reader") == 0);
  TEST_ASSERT(strcmp(lizard_surface_trace_latest_detail(origin),
                     "parsed datum") == 0);

  rewritten = lizard_surface_from_ast_like(e.heap, rewritten_value, origin);
  TEST_ASSERT(rewritten != NULL);
  TEST_ASSERT(lizard_surface_trace_count(rewritten) == 1UL);
  TEST_ASSERT(lizard_surface_trace_add(e.heap, rewritten, "rewrite",
                                       "constant folded", origin) == 1);
  TEST_ASSERT(lizard_surface_trace_count(rewritten) == 2UL);
  TEST_ASSERT(strcmp(lizard_surface_trace_latest_stage(rewritten),
                     "rewrite") == 0);
  TEST_ASSERT(strcmp(lizard_surface_trace_latest_detail(rewritten),
                     "constant folded") == 0);

  copied = lizard_surface_from_ast(e.heap, rewritten_value);
  TEST_ASSERT(copied != NULL);
  TEST_ASSERT(lizard_surface_trace_copy(e.heap, copied, rewritten) == 1);
  TEST_ASSERT(lizard_surface_trace_count(copied) == 2UL);
  TEST_ASSERT(strcmp(lizard_surface_trace_latest_stage(copied), "rewrite") == 0);

  debug = lizard_surface_trace_debug_string(e.heap, copied);
  TEST_ASSERT(debug != NULL);
  TEST_ASSERT(strstr(debug, "count=2") != NULL);
  TEST_ASSERT(strstr(debug, "latest=rewrite") != NULL);
  TEST_ASSERT(strstr(debug, "constant folded") != NULL);
  TEST_ASSERT(strstr(debug, "phase2f.liz") != NULL);

  surface_debug = lizard_surface_debug_string(e.heap, copied);
  TEST_ASSERT(surface_debug != NULL);
  TEST_ASSERT(strstr(surface_debug, "traces=2") != NULL);

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
