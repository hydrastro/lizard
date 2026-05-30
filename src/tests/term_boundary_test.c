/* tests/term_boundary_test.c
 *
 * Phase 2A scaffold test: runtime AST, SurfaceTerm, CoreTerm, and KernelTerm
 * have distinct wrapper paths before the evaluator is routed through them.
 */

#include "core_term.h"
#include "surface_term.h"
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *runtime_value;
  lizard_surface_term_t *surface;
  lizard_core_term_t *core_runtime;
  lizard_core_term_t *core_kernel;
  lizard_core_term_t *hole;
  lizard_source_span_t span;
  lizard_source_span_t got;
  kterm_t *nat_type;

  lizard_test_env_init(&e);

  runtime_value = lizard_test_eval(&e, "(+ 1 2)");
  TEST_ASSERT(lizard_test_is_int(runtime_value, 3));

  span.filename = "term-boundary.lisp";
  span.start_line = 4;
  span.start_column = 2;
  span.end_line = 4;
  span.end_column = 9;
  span.start_offset = 17;
  span.end_offset = 24;

  surface = lizard_surface_from_ast_span(e.heap, runtime_value, span);
  TEST_ASSERT(surface != NULL);
  TEST_ASSERT(lizard_surface_kind(surface) == LIZARD_SURFACE_DATUM);
  TEST_ASSERT(lizard_surface_ast(surface) == runtime_value);
  TEST_ASSERT(lizard_surface_span(surface, &got) == 1);
  TEST_ASSERT(got.filename == span.filename);
  TEST_ASSERT_EQ(got.start_line, 4);
  TEST_ASSERT_EQ(got.start_column, 2);

  TEST_ASSERT_EQ(lizard_surface_phase(surface), 0);
  lizard_surface_set_phase(surface, 2);
  TEST_ASSERT_EQ(lizard_surface_phase(surface), 2);
  lizard_surface_add_scope(surface, 0x10UL);
  lizard_surface_add_scope(surface, 0x04UL);
  TEST_ASSERT(lizard_surface_scope(surface) == 0x14UL);

  core_runtime = lizard_core_from_surface_ast(e.heap, surface);
  TEST_ASSERT(core_runtime != NULL);
  TEST_ASSERT(lizard_core_kind(core_runtime) == LIZARD_CORE_RUNTIME_AST);
  TEST_ASSERT(lizard_core_surface(core_runtime) == surface);
  TEST_ASSERT(lizard_core_runtime_ast(core_runtime) == runtime_value);
  TEST_ASSERT(lizard_core_kernel_term(core_runtime) == NULL);

  nat_type = kt_nat(e.heap);
  core_kernel = lizard_core_from_kernel(e.heap, nat_type);
  TEST_ASSERT(core_kernel != NULL);
  TEST_ASSERT(lizard_core_kind(core_kernel) == LIZARD_CORE_KERNEL_TERM);
  TEST_ASSERT(lizard_core_runtime_ast(core_kernel) == NULL);
  TEST_ASSERT(lizard_core_kernel_term(core_kernel) == nat_type);

  hole = lizard_core_hole(e.heap, 7);
  TEST_ASSERT(hole != NULL);
  TEST_ASSERT(lizard_core_kind(hole) == LIZARD_CORE_HOLE);
  TEST_ASSERT_EQ(lizard_core_hole_id(hole), 7);

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
