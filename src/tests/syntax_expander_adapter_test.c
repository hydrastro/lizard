/* tests/syntax_expander_adapter_test.c
 *
 * Phase 2H: SurfaceTerm/ExpansionContext adapter around the existing macro
 * expander. The adapter must preserve old runtime AST behavior while adding
 * untrusted source/scope/property/trace metadata.
 */

#include "syntax_expander.h"
#include "primitives.h"
#include "test_harness.h"
#include "test_helpers.h"

#include <string.h>

int main(void) {
  lizard_test_env_t e;
  lizard_diagnostic_t diagnostic;
  lz_list_t *forms;
  lizard_surface_list_node_t *node;
  lizard_surface_term_t *input;
  lizard_surface_term_t *expanded_surface;
  lizard_expansion_context_t *context;
  lizard_ast_node_t *expanded_ast;
  lizard_ast_node_t *runtime_value;
  lizard_ast_node_t *old_runtime_value;
  lizard_ast_node_t *tag;
  lizard_source_span_t span;
  char *origin_debug;

  lizard_test_env_init(&e);

  runtime_value = lizard_test_eval(&e,
      "(define-syntax twice "
      "  (syntax-rules () ((twice x) (+ x x))))");
  TEST_ASSERT(runtime_value != NULL);

  forms = lizard_surface_parse_source(e.heap, "(twice 21)",
                                      "phase2h.liz", &diagnostic);
  TEST_ASSERT(forms != NULL);
  TEST_ASSERT(diagnostic.status == LIZARD_STATUS_OK);
  node = (lizard_surface_list_node_t *)forms->head;
  TEST_ASSERT(node != NULL);
  TEST_ASSERT(node->node.next != NULL);
  input = node->term;
  TEST_ASSERT(input != NULL);

  lizard_surface_set_phase(input, 4);
  lizard_surface_add_scope(input, 0x44UL);
  tag = lizard_test_eval(&e, "'origin-tag");
  TEST_ASSERT(tag != NULL);
  TEST_ASSERT(lizard_surface_property_set(e.heap, input, "origin", tag));

  context = lizard_expansion_context_new(e.heap, 4, "surface-adapter");
  TEST_ASSERT(context != NULL);

  expanded_surface = NULL;
  expanded_ast = NULL;
  TEST_ASSERT(lizard_syntax_expand_surface(context, e.env, input,
                                           &expanded_surface,
                                           &expanded_ast));
  TEST_ASSERT(expanded_surface != NULL);
  TEST_ASSERT(expanded_ast != NULL);

  runtime_value = lizard_eval(expanded_ast, e.env, e.heap,
                              lizard_identity_cont);
  TEST_ASSERT(lizard_test_is_int(runtime_value, 42));

  old_runtime_value = lizard_test_eval(&e, "(twice 21)");
  TEST_ASSERT(lizard_test_is_int(old_runtime_value, 42));

  TEST_ASSERT(lizard_surface_span(expanded_surface, &span));
  TEST_ASSERT(span.filename != NULL);
  TEST_ASSERT(strcmp(span.filename, "phase2h.liz") == 0);
  TEST_ASSERT(lizard_surface_phase(expanded_surface) == 4);
  TEST_ASSERT(lizard_surface_has_scope(expanded_surface, 0x44UL));
  TEST_ASSERT(lizard_test_is_symbol(
      lizard_surface_property_get(expanded_surface, "origin"),
      "origin-tag"));
  TEST_ASSERT(lizard_surface_trace_count(expanded_surface) == 1UL);
  TEST_ASSERT(strcmp(lizard_surface_trace_latest_stage(expanded_surface),
                     "macro-expand") == 0);
  TEST_ASSERT(strstr(lizard_surface_trace_latest_detail(expanded_surface),
                     "existing-expander-adapter") != NULL);

  origin_debug = lizard_surface_origin_chain_debug_string(e.heap,
                                                          expanded_surface);
  TEST_ASSERT(origin_debug != NULL);
  TEST_ASSERT(strstr(origin_debug, "traces=1") != NULL);
  TEST_ASSERT(strstr(origin_debug, "phase2h.liz") != NULL);

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
