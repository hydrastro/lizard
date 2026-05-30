#ifndef LIZARD_EXPANSION_CONTEXT_H
#define LIZARD_EXPANSION_CONTEXT_H

#include "surface_term.h"

/* Expansion contexts are untrusted syntax-tooling state. They allocate fresh
 * macro scopes and attach trace metadata to SurfaceTerm objects, but neither
 * the evaluator nor the kernel consults them. */

typedef struct lizard_expansion_context lizard_expansion_context_t;

lizard_expansion_context_t *lizard_expansion_context_new(
    lizard_heap_t *heap, int phase, const char *name);

lizard_heap_t *lizard_expansion_context_heap(
    const lizard_expansion_context_t *context);

int lizard_expansion_context_phase(
    const lizard_expansion_context_t *context);

void lizard_expansion_context_set_phase(lizard_expansion_context_t *context,
                                        int phase);

const char *lizard_expansion_context_name(
    const lizard_expansion_context_t *context);

unsigned long lizard_expansion_context_fresh_scope(
    lizard_expansion_context_t *context);

int lizard_expansion_context_introduce(
    lizard_expansion_context_t *context, lizard_surface_term_t *term,
    const lizard_surface_term_t *origin, const char *macro_name,
    const char *detail, unsigned long *out_scope);

int lizard_expansion_context_rewrite(
    lizard_expansion_context_t *context, lizard_surface_term_t *term,
    const lizard_surface_term_t *origin, const char *stage,
    const char *detail);

char *lizard_expansion_context_debug_string(
    lizard_expansion_context_t *context);

#endif /* LIZARD_EXPANSION_CONTEXT_H */
