/* tt_lattice.c
 *
 * Universe/couniverse lattice constructors, set operations, and ordering
 * checks. Split from tt_equality.c so the equality engine can focus on
 * normalization and judgmental equality.
 */

#include "tt_lattice.h"
#include "primitives.h"
#include "mem.h"

#include <string.h>

lizard_ast_node_t *lizard_tt_make_u_suc(lizard_heap_t *heap,
                                     lizard_ast_node_t *u) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_U_SUC;
  n->data.tt_u_suc.operand = u;
  return n;
}
lizard_ast_node_t *lizard_tt_make_u_max(lizard_heap_t *heap,
                                     lizard_ast_node_t *u,
                                     lizard_ast_node_t *v) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_U_MAX;
  n->data.tt_u_max.left = u;
  n->data.tt_u_max.right = v;
  return n;
}
lizard_ast_node_t *lizard_tt_make_u_min(lizard_heap_t *heap,
                                     lizard_ast_node_t *u,
                                     lizard_ast_node_t *v) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_U_MIN;
  n->data.tt_u_min.left = u;
  n->data.tt_u_min.right = v;
  return n;
}

/* Build a (U-set ...) node from a sorted, deduplicated dims array.
 * Phase L.2 helper. The caller must guarantee the array is sorted
 * and dedup'd; we don't re-sort here. count==0 is the empty set. */
lizard_ast_node_t *lizard_tt_make_universe_set(lizard_heap_t *heap,
                                            long *dims, long count) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_UNIVERSE_SET;
  n->data.tt_universe_set.dims = dims;
  n->data.tt_universe_set.count = count;
  return n;
}

/* Sorted-array set union (Phase L.2). Output is sorted, deduped. */
lizard_ast_node_t *lizard_tt_universe_set_union(lizard_heap_t *heap,
                                             lizard_ast_node_t *a,
                                             lizard_ast_node_t *b) {
  long ai, bi, oi, na, nb;
  long *out;
  na = a->data.tt_universe_set.count;
  nb = b->data.tt_universe_set.count;
  if (na + nb == 0) return lizard_tt_make_universe_set(heap, NULL, 0);
  out = lizard_heap_alloc(sizeof(long) * (size_t)(na + nb));
  ai = 0; bi = 0; oi = 0;
  while (ai < na && bi < nb) {
    long av = a->data.tt_universe_set.dims[ai];
    long bv = b->data.tt_universe_set.dims[bi];
    if (av < bv) { out[oi++] = av; ai++; }
    else if (av > bv) { out[oi++] = bv; bi++; }
    else { out[oi++] = av; ai++; bi++; }
  }
  while (ai < na) out[oi++] = a->data.tt_universe_set.dims[ai++];
  while (bi < nb) out[oi++] = b->data.tt_universe_set.dims[bi++];
  return lizard_tt_make_universe_set(heap, out, oi);
}

/* Sorted-array set intersection (Phase L.2). */
lizard_ast_node_t *lizard_tt_universe_set_intersect(lizard_heap_t *heap,
                                                 lizard_ast_node_t *a,
                                                 lizard_ast_node_t *b) {
  long ai, bi, oi, na, nb, smaller;
  long *out;
  na = a->data.tt_universe_set.count;
  nb = b->data.tt_universe_set.count;
  smaller = (na < nb) ? na : nb;
  if (smaller == 0) return lizard_tt_make_universe_set(heap, NULL, 0);
  out = lizard_heap_alloc(sizeof(long) * (size_t)smaller);
  ai = 0; bi = 0; oi = 0;
  while (ai < na && bi < nb) {
    long av = a->data.tt_universe_set.dims[ai];
    long bv = b->data.tt_universe_set.dims[bi];
    if (av < bv) ai++;
    else if (av > bv) bi++;
    else { out[oi++] = av; ai++; bi++; }
  }
  if (oi == 0) return lizard_tt_make_universe_set(heap, NULL, 0);
  return lizard_tt_make_universe_set(heap, out, oi);
}

/* Subset test: is A's dim set ⊆ B's? Both must be UNIVERSE_SET.
 * Returns 1 yes, 0 no. */
int lizard_tt_universe_set_subset(lizard_ast_node_t *a, lizard_ast_node_t *b) {
  long ai, bi, na, nb;
  na = a->data.tt_universe_set.count;
  nb = b->data.tt_universe_set.count;
  if (na > nb) return 0;
  ai = 0; bi = 0;
  while (ai < na && bi < nb) {
    long av = a->data.tt_universe_set.dims[ai];
    long bv = b->data.tt_universe_set.dims[bi];
    if (av == bv) { ai++; bi++; }
    else if (av > bv) { bi++; }
    else return 0;
  }
  return ai == na;
}

/* ===== Phase L.4: couniverse-set helpers =====
 *
 * Structurally identical to the universe-set helpers, but operating
 * on the tt_couniverse_set field. The couniverse lattice is its own
 * lattice — no automatic conversion to/from universe-set.
 */
lizard_ast_node_t *lizard_tt_make_couniverse_set(lizard_heap_t *heap,
                                              long *dims, long count) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_COUNIVERSE_SET;
  n->data.tt_couniverse_set.dims = dims;
  n->data.tt_couniverse_set.count = count;
  return n;
}
lizard_ast_node_t *lizard_tt_make_co_max(lizard_heap_t *heap,
                                      lizard_ast_node_t *u,
                                      lizard_ast_node_t *v) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_CO_MAX;
  n->data.tt_co_max.left = u;
  n->data.tt_co_max.right = v;
  return n;
}
lizard_ast_node_t *lizard_tt_make_co_min(lizard_heap_t *heap,
                                      lizard_ast_node_t *u,
                                      lizard_ast_node_t *v) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_CO_MIN;
  n->data.tt_co_min.left = u;
  n->data.tt_co_min.right = v;
  return n;
}
lizard_ast_node_t *lizard_tt_couniverse_set_union(lizard_heap_t *heap,
                                               lizard_ast_node_t *a,
                                               lizard_ast_node_t *b) {
  long ai, bi, oi, na, nb;
  long *out;
  na = a->data.tt_couniverse_set.count;
  nb = b->data.tt_couniverse_set.count;
  if (na + nb == 0) return lizard_tt_make_couniverse_set(heap, NULL, 0);
  out = lizard_heap_alloc(sizeof(long) * (size_t)(na + nb));
  ai = 0; bi = 0; oi = 0;
  while (ai < na && bi < nb) {
    long av = a->data.tt_couniverse_set.dims[ai];
    long bv = b->data.tt_couniverse_set.dims[bi];
    if (av < bv) { out[oi++] = av; ai++; }
    else if (av > bv) { out[oi++] = bv; bi++; }
    else { out[oi++] = av; ai++; bi++; }
  }
  while (ai < na) out[oi++] = a->data.tt_couniverse_set.dims[ai++];
  while (bi < nb) out[oi++] = b->data.tt_couniverse_set.dims[bi++];
  return lizard_tt_make_couniverse_set(heap, out, oi);
}
lizard_ast_node_t *lizard_tt_couniverse_set_intersect(lizard_heap_t *heap,
                                                   lizard_ast_node_t *a,
                                                   lizard_ast_node_t *b) {
  long ai, bi, oi, na, nb, smaller;
  long *out;
  na = a->data.tt_couniverse_set.count;
  nb = b->data.tt_couniverse_set.count;
  smaller = (na < nb) ? na : nb;
  if (smaller == 0) return lizard_tt_make_couniverse_set(heap, NULL, 0);
  out = lizard_heap_alloc(sizeof(long) * (size_t)smaller);
  ai = 0; bi = 0; oi = 0;
  while (ai < na && bi < nb) {
    long av = a->data.tt_couniverse_set.dims[ai];
    long bv = b->data.tt_couniverse_set.dims[bi];
    if (av < bv) ai++;
    else if (av > bv) bi++;
    else { out[oi++] = av; ai++; bi++; }
  }
  if (oi == 0) return lizard_tt_make_couniverse_set(heap, NULL, 0);
  return lizard_tt_make_couniverse_set(heap, out, oi);
}
int lizard_tt_couniverse_set_subset(lizard_ast_node_t *a, lizard_ast_node_t *b) {
  long ai, bi, na, nb;
  na = a->data.tt_couniverse_set.count;
  nb = b->data.tt_couniverse_set.count;
  if (na > nb) return 0;
  ai = 0; bi = 0;
  while (ai < na && bi < nb) {
    long av = a->data.tt_couniverse_set.dims[ai];
    long bv = b->data.tt_couniverse_set.dims[bi];
    if (av == bv) { ai++; bi++; }
    else if (av > bv) { bi++; }
    else return 0;
  }
  return ai == na;
}

/* NOTE: the leq/convertibility decision procedures
 * (lizard_tt_universe_leq, lizard_tt_universe_convertible,
 *  lizard_tt_couniverse_leq) are defined in the core type-theory module
 * and declared in primitives.h.  They were duplicated here during the
 * lattice extraction; the canonical definitions remain in the core, so
 * this module owns only the *-set constructors, union/intersect, and
 * subset operations. */
