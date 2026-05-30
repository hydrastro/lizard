/* src/core_term.c -- untrusted elaborated-term boundary.
 *
 * Phase 2A scaffolding: CoreTerm distinguishes future elaborator output from
 * both runtime AST values and trusted KernelTerm values.  It intentionally does
 * not change evaluation yet.
 */

#include "core_term.h"
#include "mem.h"

struct lizard_core_term {
  lizard_core_kind_t kind;
  lizard_surface_term_t *surface;
  lizard_ast_node_t *runtime_ast;
  kterm_t *kernel_term;
  int hole_id;
};

static lizard_core_term_t *core_alloc(lizard_heap_t *heap,
                                      lizard_core_kind_t kind) {
  lizard_core_term_t *term;
  term = (lizard_core_term_t *)lizard_heap_alloc(sizeof(*term));
  term->kind = kind;
  term->surface = NULL;
  term->runtime_ast = NULL;
  term->kernel_term = NULL;
  term->hole_id = -1;
  return term;
}

lizard_core_term_t *lizard_core_from_surface(lizard_heap_t *heap,
                                             lizard_surface_term_t *surface) {
  return lizard_core_from_surface_ast(heap, surface);
}

lizard_core_term_t *lizard_core_from_surface_ast(lizard_heap_t *heap,
                                                 lizard_surface_term_t *surface) {
  lizard_core_term_t *term;
  if (surface == NULL || lizard_surface_ast(surface) == NULL) {
    return core_alloc(heap, LIZARD_CORE_ERROR);
  }
  term = core_alloc(heap, LIZARD_CORE_RUNTIME_AST);
  term->surface = surface;
  term->runtime_ast = lizard_surface_ast(surface);
  return term;
}

lizard_core_term_t *lizard_core_from_kernel(lizard_heap_t *heap,
                                            kterm_t *kernel_term) {
  lizard_core_term_t *term;
  if (kernel_term == NULL) {
    return core_alloc(heap, LIZARD_CORE_ERROR);
  }
  term = core_alloc(heap, LIZARD_CORE_KERNEL_TERM);
  term->kernel_term = kernel_term;
  return term;
}

lizard_core_term_t *lizard_core_hole(lizard_heap_t *heap, int id) {
  lizard_core_term_t *term;
  term = core_alloc(heap, LIZARD_CORE_HOLE);
  term->hole_id = id;
  return term;
}

lizard_core_kind_t lizard_core_kind(const lizard_core_term_t *term) {
  return term != NULL ? term->kind : LIZARD_CORE_ERROR;
}

lizard_surface_term_t *lizard_core_surface(const lizard_core_term_t *term) {
  return term != NULL ? term->surface : NULL;
}

lizard_ast_node_t *lizard_core_runtime_ast(const lizard_core_term_t *term) {
  return term != NULL ? term->runtime_ast : NULL;
}

kterm_t *lizard_core_kernel_term(const lizard_core_term_t *term) {
  return term != NULL ? term->kernel_term : NULL;
}

int lizard_core_hole_id(const lizard_core_term_t *term) {
  return term != NULL ? term->hole_id : -1;
}
