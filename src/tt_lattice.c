/* tt_lattice.c
 *
 * Universe/couniverse lattice constructors, set operations, and
 * ordering checks. Split from tt_equality.c so the equality engine
 * can focus on normalization/judgmental equality.
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


int lizard_tt_universe_leq(lizard_ast_node_t *u, lizard_ast_node_t *v) {
  if (u == NULL || v == NULL) return -1;
  /* Both concrete integers */
  if (u->type == AST_TT_UNIVERSE && v->type == AST_TT_UNIVERSE) {
    return u->data.tt_universe.level <= v->data.tt_universe.level ? 1 : 0;
  }
  /* Phase L.2: (U-set S) ≤ (U-set T) iff S ⊆ T (subset is lattice order).
   * This is where the multi-dimensional lattice produces *incomparable*
   * elements — (U-set 0 1) and (U-set 0 2) are both ≤ (U-set 0 1 2)
   * but neither is ≤ the other. */
  if (u->type == AST_TT_UNIVERSE_SET && v->type == AST_TT_UNIVERSE_SET) {
    return lizard_tt_universe_set_subset(u, v) ? 1 : 0;
  }
  /* Mixed: lift (U n) to {n}-singleton for the comparison. */
  if (u->type == AST_TT_UNIVERSE && v->type == AST_TT_UNIVERSE_SET) {
    long *dim = lizard_heap_alloc(sizeof(long));
    lizard_ast_node_t lift;
    dim[0] = u->data.tt_universe.level;
    lift.type = AST_TT_UNIVERSE_SET;
    lift.data.tt_universe_set.dims = dim;
    lift.data.tt_universe_set.count = 1;
    return lizard_tt_universe_set_subset(&lift, v) ? 1 : 0;
  }
  if (u->type == AST_TT_UNIVERSE_SET && v->type == AST_TT_UNIVERSE) {
    long *dim = lizard_heap_alloc(sizeof(long));
    lizard_ast_node_t lift;
    dim[0] = v->data.tt_universe.level;
    lift.type = AST_TT_UNIVERSE_SET;
    lift.data.tt_universe_set.dims = dim;
    lift.data.tt_universe_set.count = 1;
    return lizard_tt_universe_set_subset(u, &lift) ? 1 : 0;
  }
  /* Variable identity */
  if (u->type == AST_TT_U_VAR && v->type == AST_TT_U_VAR) {
    if (strcmp(u->data.tt_u_var.name, v->data.tt_u_var.name) == 0) return 1;
    return -1;
  }
  /* (U-suc a) ≤ (U-suc b) iff a ≤ b */
  if (u->type == AST_TT_U_SUC && v->type == AST_TT_U_SUC) {
    return lizard_tt_universe_leq(u->data.tt_u_suc.operand,
                                  v->data.tt_u_suc.operand);
  }
  /* (U-suc a) ≤ (U n): need a ≤ (U n-1), false if n == 0 */
  if (u->type == AST_TT_U_SUC && v->type == AST_TT_UNIVERSE) {
    if (v->data.tt_universe.level == 0) return 0;
    {
      lizard_ast_node_t pred;
      pred.type = AST_TT_UNIVERSE;
      pred.data.tt_universe.level = v->data.tt_universe.level - 1;
      return lizard_tt_universe_leq(u->data.tt_u_suc.operand, &pred);
    }
  }
  /* (U n) ≤ (U-suc b): n=0 always ≤; else (U n-1) ≤ b */
  if (u->type == AST_TT_UNIVERSE && v->type == AST_TT_U_SUC) {
    if (u->data.tt_universe.level == 0) return 1;
    {
      lizard_ast_node_t pred;
      pred.type = AST_TT_UNIVERSE;
      pred.data.tt_universe.level = u->data.tt_universe.level - 1;
      return lizard_tt_universe_leq(&pred, v->data.tt_u_suc.operand);
    }
  }
  /* (U-max u1 u2) ≤ v iff u1 ≤ v AND u2 ≤ v */
  if (u->type == AST_TT_U_MAX) {
    int l = lizard_tt_universe_leq(u->data.tt_u_max.left, v);
    int r = lizard_tt_universe_leq(u->data.tt_u_max.right, v);
    if (l == 1 && r == 1) return 1;
    if (l == 0 || r == 0) return 0;
    return -1;
  }
  /* u ≤ (U-max v1 v2) — if u ≤ v1 or u ≤ v2, definitely yes.
   * Otherwise undecidable. */
  if (v->type == AST_TT_U_MAX) {
    int l = lizard_tt_universe_leq(u, v->data.tt_u_max.left);
    int r = lizard_tt_universe_leq(u, v->data.tt_u_max.right);
    if (l == 1 || r == 1) return 1;
    return -1;
  }
  /* (U-min u1 u2) ≤ v: meet is below either component, so if
   * either component is ≤ v, the meet is too. */
  if (u->type == AST_TT_U_MIN) {
    int l = lizard_tt_universe_leq(u->data.tt_u_min.left, v);
    int r = lizard_tt_universe_leq(u->data.tt_u_min.right, v);
    if (l == 1 || r == 1) return 1;
    return -1;
  }
  /* u ≤ (U-min v1 v2): u is below the meet iff u is below both. */
  if (v->type == AST_TT_U_MIN) {
    int l = lizard_tt_universe_leq(u, v->data.tt_u_min.left);
    int r = lizard_tt_universe_leq(u, v->data.tt_u_min.right);
    if (l == 1 && r == 1) return 1;
    if (l == 0 || r == 0) return 0;
    return -1;
  }
  return -1;
}

/* Cumulativity-aware type comparison. Two types T1 and T2 are
 * convertible-with-cumulativity if T1 ≡ T2 (alpha-equal), OR they're
 * both universes and the inferred level ≤ expected level. Used by
 * the type checker when checking a type against a universe. */
int lizard_tt_universe_convertible(lizard_ast_node_t *inferred,
                                   lizard_ast_node_t *expected) {
  if (lizard_tt_alpha_equal(inferred, expected)) return 1;
  /* Cumulativity only kicks in when both are universe-y. */
  return lizard_tt_universe_leq(inferred, expected) == 1;
}

/* Phase L.4: couniverse-leq decision procedure.
 *
 * Subset inclusion within the COUNIVERSE lattice. NEVER returns 1 when
 * one side is a universe (U-set) and the other a couniverse (Co-set) —
 * the lattices are kept separate. Returns -1 for that case (incomparable
 * sorts), 1 for subset, 0 for strict non-subset, -1 for undecidable.
 */
int lizard_tt_couniverse_leq(lizard_ast_node_t *u, lizard_ast_node_t *v) {
  if (u == NULL || v == NULL) return -1;
  if (u->type == AST_TT_COUNIVERSE_SET && v->type == AST_TT_COUNIVERSE_SET) {
    return lizard_tt_couniverse_set_subset(u, v) ? 1 : 0;
  }
  /* Mixing universe and couniverse is a sort error, not subset-false. */
  if ((u->type == AST_TT_COUNIVERSE_SET && v->type == AST_TT_UNIVERSE_SET) ||
      (u->type == AST_TT_UNIVERSE_SET && v->type == AST_TT_COUNIVERSE_SET)) {
    return -1;
  }
  /* Old single-nat couniverses (legacy). */
  if (u->type == AST_TT_COUNIVERSE && v->type == AST_TT_COUNIVERSE) {
    return u->data.tt_couniverse.level <= v->data.tt_couniverse.level ? 1 : 0;
  }
  return -1;
}

