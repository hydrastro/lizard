/* tests/expansion_context_scaffold_test.c
 *
 * Phase 2G: expansion-context scaffold. These APIs allocate fresh macro
 * scopes and attach trace metadata to SurfaceTerm values. They are untrusted
 * syntax-tooling metadata only; evaluator and macro expansion are unchanged.
 */

#include "expansion_context.h"
#include "surface_term.h"
#include "test_harness.h"
#include "test_helpers.h"

#include <string.h>

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *input_ast;
  lizard_ast_node_t *output_ast;
  lizard_surface_term_t *input;
  lizard_surface_term_t *output;
  lizard_expansion_context_t *context;
  lizard_source_span_t span;
  unsigned long first_scope;
  unsigned long second_scope;
  char *ctx_debug;
  char *trace_debug;
  char *origin_debug;

  lizard_test_env_init(&e);

  input_ast = lizard_test_eval(&e, "(+ 1 2)");
  output_ast = lizard_test_eval(&e, "(+ 3 4)");
  TEST_ASSERT(input_ast != NULL);
  TEST_ASSERT(output_ast != NULL);

  span.filename = "phase2g.liz";
  span.start_line = 4;
  span.start_column = 9;
  span.end_line = 4;
  span.end_column = 16;
  span.start_offset = 40;
  span.end_offset = 47;

  input = lizard_surface_from_ast_span(e.heap, input_ast, span);
  output = lizard_surface_from_ast_like(e.heap, output_ast, input);
  TEST_ASSERT(input != NULL);
  TEST_ASSERT(output != NULL);

  context = lizard_expansion_context_new(e.heap, 1, "macro-expand");
  TEST_ASSERT(context != NULL);
  TEST_ASSERT(lizard_expansion_context_phase(context) == 1);
  TEST_ASSERT(strcmp(lizard_expansion_context_name(context),
                     "macro-expand") == 0);

  first_scope = 0UL;
  TEST_ASSERT(lizard_expansion_context_introduce(
      context, output, input, "when", "introduce temporary", &first_scope));
  TEST_ASSERT(first_scope != 0UL);
  TEST_ASSERT(lizard_surface_has_scope(output, first_scope));
  TEST_ASSERT(lizard_surface_phase(output) == 1);
  TEST_ASSERT(lizard_surface_trace_count(output) == 1UL);
  TEST_ASSERT(strcmp(lizard_surface_trace_latest_stage(output),
                     "macro-introduce") == 0);
  TEST_ASSERT(strstr(lizard_surface_trace_latest_detail(output),
                     "when") != NULL);

  second_scope = lizard_expansion_context_fresh_scope(context);
  TEST_ASSERT(second_scope != 0UL);
  TEST_ASSERT(second_scope != first_scope);

  lizard_expansion_context_set_phase(context, 2);
  TEST_ASSERT(lizard_expansion_context_rewrite(context, output, input,
                                               "macro-rewrite",
                                               "lowered when"));
  TEST_ASSERT(lizard_surface_phase(output) == 2);
  TEST_ASSERT(lizard_surface_trace_count(output) == 2UL);
  TEST_ASSERT(strcmp(lizard_surface_trace_latest_stage(output),
                     "macro-rewrite") == 0);

  ctx_debug = lizard_expansion_context_debug_string(context);
  TEST_ASSERT(ctx_debug != NULL);
  TEST_ASSERT(strstr(ctx_debug, "macro-expand") != NULL);
  TEST_ASSERT(strstr(ctx_debug, "phase=2") != NULL);

  trace_debug = lizard_surface_trace_debug_string(e.heap, output);
  TEST_ASSERT(trace_debug != NULL);
  TEST_ASSERT(strstr(trace_debug, "count=2") != NULL);
  TEST_ASSERT(strstr(trace_debug, "macro-rewrite") != NULL);
  TEST_ASSERT(strstr(trace_debug, "phase2g.liz") != NULL);

  origin_debug = lizard_surface_origin_chain_debug_string(e.heap, output);
  TEST_ASSERT(origin_debug != NULL);
  TEST_ASSERT(strstr(origin_debug, "surface-origin-chain") != NULL);
  TEST_ASSERT(strstr(origin_debug, "traces=2") != NULL);

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
