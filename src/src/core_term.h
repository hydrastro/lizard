#ifndef LIZARD_CORE_TERM_H
#define LIZARD_CORE_TERM_H

#include "kernel.h"
#include "surface_term.h"

/* Core terms are elaborator output.  They are still untrusted: the kernel must
 * re-check them after erasure/indexing.  Phase 2A keeps this as a scaffold so
 * we can introduce the boundary without perturbing the evaluator. */

typedef enum {
  LIZARD_CORE_RUNTIME_AST,
  LIZARD_CORE_KERNEL_TERM,
  LIZARD_CORE_HOLE,
  LIZARD_CORE_ERROR
} lizard_core_kind_t;

typedef struct lizard_core_term lizard_core_term_t;

lizard_core_term_t *lizard_core_from_surface(lizard_heap_t *heap,
                                             lizard_surface_term_t *surface);
lizard_core_term_t *lizard_core_from_surface_ast(lizard_heap_t *heap,
                                                 lizard_surface_term_t *surface);
lizard_core_term_t *lizard_core_from_kernel(lizard_heap_t *heap,
                                            kterm_t *kernel_term);
lizard_core_term_t *lizard_core_hole(lizard_heap_t *heap, int id);

lizard_core_kind_t lizard_core_kind(const lizard_core_term_t *term);
lizard_surface_term_t *lizard_core_surface(const lizard_core_term_t *term);
lizard_ast_node_t *lizard_core_runtime_ast(const lizard_core_term_t *term);
kterm_t *lizard_core_kernel_term(const lizard_core_term_t *term);
int lizard_core_hole_id(const lizard_core_term_t *term);

#endif /* LIZARD_CORE_TERM_H */
