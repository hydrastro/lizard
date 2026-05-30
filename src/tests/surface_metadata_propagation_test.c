/* tests/surface_metadata_propagation_test.c
 *
 * Phase 2E: metadata propagation helpers for syntax-object rewrites.
 * These helpers are scaffolding only; evaluator/macro behavior is unchanged.
 */

#include "surface_term.h"
#include "test_harness.h"
#include "test_helpers.h"

#include <string.h>

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *value;
  lizard_ast_node_t *quoted;
  lizard_ast_node_t *property_value;
  lizard_surface_term_t *origin;
  lizard_surface_term_t *derived;
  lizard_surface_term_t *without_props;
  lizard_surface_term_t *from_quoted;
  lizard_source_span_t span;
  lizard_source_span_t got;
  char *debug;

  lizard_test_env_init(&e);

  value = lizard_test_eval(&e, "(+ 20 22)");
  TEST_ASSERT(lizard_test_is_int(value, 42));

  span.filename = "phase2e.liz";
  span.start_line = 9;
  span.start_column = 4;
  span.end_line = 9;
  span.end_column = 12;
  span.start_offset = 80;
  span.end_offset = 88;

  origin = lizard_surface_from_ast_span(e.heap, value, span);
  TEST_ASSERT(origin != NULL);
  lizard_surface_set_phase(origin, 3);
  lizard_surface_add_scope(origin, 0x10UL);
  lizard_surface_add_scope(origin, 0x40UL);

  property_value = lizard_test_eval(&e, "'phase2e-origin");
  TEST_ASSERT(property_value != NULL);
  TEST_ASSERT(lizard_surface_property_set(e.heap, origin, "origin",
                                          property_value) == 1);

  derived = lizard_surface_from_ast_like(e.heap, value, origin);
  TEST_ASSERT(derived != NULL);
  TEST_ASSERT(lizard_surface_span(derived, &got) == 1);
  TEST_ASSERT(got.filename != NULL);
  TEST_ASSERT(strcmp(got.filename, "phase2e.liz") == 0);
  TEST_ASSERT_EQ(got.start_line, 9);
  TEST_ASSERT_EQ(lizard_surface_phase(derived), 3);
  TEST_ASSERT(lizard_surface_same_scopes(origin, derived) == 1);
  TEST_ASSERT(lizard_surface_property_get(derived, "origin") == property_value);

  without_props = lizard_surface_from_ast(e.heap, value);
  TEST_ASSERT(lizard_surface_copy_metadata(e.heap, without_props, origin, 0) == 1);
  TEST_ASSERT_EQ(lizard_surface_phase(without_props), 3);
  TEST_ASSERT(lizard_surface_same_scopes(origin, without_props) == 1);
  TEST_ASSERT(lizard_surface_property_has(without_props, "origin") == 0);

  quoted = lizard_test_eval(&e, "'(+ 1 2)");
  TEST_ASSERT(quoted != NULL);
  from_quoted = lizard_surface_from_quoted_datum(e.heap, quoted, origin);
  TEST_ASSERT(from_quoted != NULL);
  TEST_ASSERT(lizard_surface_ast(from_quoted) != NULL);
  TEST_ASSERT(lizard_surface_ast(from_quoted)->type == AST_APPLICATION);
  TEST_ASSERT_EQ(lizard_surface_phase(from_quoted), 3);
  TEST_ASSERT(lizard_surface_same_scopes(origin, from_quoted) == 1);
  TEST_ASSERT(lizard_surface_property_get(from_quoted, "origin") == property_value);

  debug = lizard_surface_debug_string(e.heap, from_quoted);
  TEST_ASSERT(debug != NULL);
  TEST_ASSERT(strstr(debug, "phase=3") != NULL);
  TEST_ASSERT(strstr(debug, "phase2e.liz") != NULL);
  TEST_ASSERT(strstr(debug, "scopes=2") != NULL);

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
