/* tests/syntax_object_scaffold_test.c
 *
 * Phase 2C: SurfaceTerm carries the minimum syntax-object metadata needed for
 * future hygienic macro expansion: phase, lexical scope marker, source span,
 * and a small property table. This does not change evaluator behavior.
 */

#include "surface_term.h"
#include "test_harness.h"
#include "test_helpers.h"

#include <string.h>

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *value;
  lizard_ast_node_t *property_value;
  lizard_surface_term_t *surface;
  lizard_source_span_t span;
  lizard_source_span_t got;

  lizard_test_env_init(&e);

  value = lizard_test_eval(&e, "(+ 10 32)");
  TEST_ASSERT(lizard_test_is_int(value, 42));

  span.filename = "syntax-object.liz";
  span.start_line = 3;
  span.start_column = 5;
  span.end_line = 3;
  span.end_column = 13;
  span.start_offset = 20;
  span.end_offset = 28;

  surface = lizard_surface_from_ast_span(e.heap, value, span);
  TEST_ASSERT(surface != NULL);
  TEST_ASSERT(lizard_surface_span(surface, &got) == 1);
  TEST_ASSERT(got.filename != NULL);
  TEST_ASSERT(strcmp(got.filename, "syntax-object.liz") == 0);
  TEST_ASSERT_EQ(got.start_line, 3);
  TEST_ASSERT_EQ(got.start_column, 5);

  TEST_ASSERT_EQ(lizard_surface_phase(surface), 0);
  lizard_surface_set_phase(surface, 2);
  TEST_ASSERT_EQ(lizard_surface_phase(surface), 2);

  TEST_ASSERT(lizard_surface_scope(surface) == 0UL);
  lizard_surface_add_scope(surface, 0x01UL);
  lizard_surface_add_scope(surface, 0x08UL);
  TEST_ASSERT(lizard_surface_scope(surface) == 0x09UL);
  lizard_surface_clear_scopes(surface);
  TEST_ASSERT(lizard_surface_scope(surface) == 0UL);

  property_value = lizard_test_eval(&e, "'macro-origin");
  TEST_ASSERT(property_value != NULL);
  TEST_ASSERT(lizard_surface_property_set(e.heap, surface, "origin",
                                         property_value) == 1);
  TEST_ASSERT(lizard_surface_property_has(surface, "origin") == 1);
  TEST_ASSERT(lizard_surface_property_get(surface, "origin") == property_value);

  TEST_ASSERT(lizard_surface_property_set(e.heap, surface, "origin", value) == 1);
  TEST_ASSERT(lizard_surface_property_get(surface, "origin") == value);
  TEST_ASSERT(lizard_surface_property_remove(surface, "origin") == 1);
  TEST_ASSERT(lizard_surface_property_has(surface, "origin") == 0);
  TEST_ASSERT(lizard_surface_property_get(surface, "origin") == NULL);

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
