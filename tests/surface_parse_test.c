/* tests/surface_parse_test.c
 *
 * Phase 2B: parser output can be wrapped as SurfaceTerm while preserving
 * source locations and parse diagnostics. Evaluator semantics remain unchanged.
 */

#include "core_term.h"
#include "surface_term.h"
#include "test_harness.h"
#include "test_helpers.h"

#include <string.h>

static lizard_surface_term_t *first_surface(lz_list_t *items) {
  lizard_surface_list_node_t *node;
  if (items == NULL || items->head == items->nil) {
    return NULL;
  }
  node = (lizard_surface_list_node_t *)items->head;
  return node->term;
}

int main(void) {
  lizard_test_env_t e;
  lz_list_t *items;
  lizard_surface_term_t *surface;
  lizard_core_term_t *core;
  lizard_source_span_t span;
  lizard_diagnostic_t diagnostic;

  lizard_test_env_init(&e);

  items = lizard_surface_parse_source(e.heap, "\n  (+ 1 2)\n", "surface.liz",
                                      &diagnostic);
  TEST_ASSERT(items != NULL);
  TEST_ASSERT_EQ(diagnostic.status, LIZARD_STATUS_OK);

  surface = first_surface(items);
  TEST_ASSERT(surface != NULL);
  TEST_ASSERT(lizard_surface_kind(surface) == LIZARD_SURFACE_DATUM);
  TEST_ASSERT(lizard_surface_ast(surface) != NULL);
  TEST_ASSERT(lizard_surface_span(surface, &span) == 1);
  TEST_ASSERT(span.filename != NULL);
  TEST_ASSERT(strcmp(span.filename, "surface.liz") == 0);
  TEST_ASSERT_EQ(span.start_line, 2);
  TEST_ASSERT_EQ(span.start_column, 3);

  core = lizard_core_from_surface(e.heap, surface);
  TEST_ASSERT(core != NULL);
  TEST_ASSERT(lizard_core_kind(core) == LIZARD_CORE_RUNTIME_AST);
  TEST_ASSERT(lizard_core_surface(core) == surface);
  TEST_ASSERT(lizard_core_runtime_ast(core) == lizard_surface_ast(surface));

  items = lizard_surface_parse_source(e.heap, "\"unterminated", "bad.liz",
                                      &diagnostic);
  TEST_ASSERT(items == NULL);
  TEST_ASSERT_EQ(diagnostic.status, LIZARD_STATUS_PARSE_ERROR);
  TEST_ASSERT(diagnostic.span.filename != NULL);
  TEST_ASSERT(strcmp(diagnostic.span.filename, "bad.liz") == 0);
  TEST_ASSERT_EQ(diagnostic.span.start_line, 1);
  TEST_ASSERT_EQ(diagnostic.span.start_column, 1);

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
