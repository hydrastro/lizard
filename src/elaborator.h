/* src/elaborator.h — bidirectional elaborator with holes (Phase 7, TT1/TT3).
 *
 * The elaborator type-checks a surface term that may contain holes (`?0`,
 * `?1`, ... — KT_META).  It pushes expected types downward (bidirectional
 * checking) so that a hole appearing in a checking position learns its *goal*
 * (the type it must have) and its local *context* (the hypotheses in scope).
 *
 * The elaborator is NOT part of the trusted core: once every hole is filled,
 * the finished term is handed to the kernel (`kt_check`) for the real verdict.
 * The elaborator only decides where the goals are.
 */
#ifndef LIZARD_ELABORATOR_H
#define LIZARD_ELABORATOR_H

#include "kernel.h"

#define ELAB_MAX_HOLES 16

typedef struct {
  int id;            /* the hole's metavariable id (?id) */
  kctx_t *ctx;       /* local context at the hole */
  kterm_t *goal;     /* the type the hole must inhabit (NULL = unconstrained) */
} elab_hole_t;

typedef struct {
  lizard_heap_t *heap;
  elab_hole_t holes[ELAB_MAX_HOLES];
  int n_holes;
  int ok;            /* 1 if the term elaborated (modulo holes), 0 on error */
} elab_state_t;

/* Elaborate `term` against `expected`, recording every hole's goal + context
 * in `out`.  Returns 1 if elaboration succeeded (the term is well-typed once
 * its holes are filled with terms of the recorded goals), 0 on a hard type
 * error.  A return of 1 with out->n_holes == 0 means the term is complete and
 * should be confirmed with kt_check. */
int elab_run(lizard_heap_t *heap, kctx_t *ctx, kterm_t *term,
             kterm_t *expected, elab_state_t *out);

#endif /* LIZARD_ELABORATOR_H */
