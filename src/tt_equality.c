/* tt_equality.c
 *
 * A judgmental equality engine for the identity-type fragment of
 * lizard's type-theory notation layer.
 *
 * What this provides:
 *
 *   (reduce t)        Normalize a term under the built-in
 *                     reduction rules for the identity-type
 *                     fragment. Returns the normal form. The
 *                     rules:
 *
 *                       sym(refl_a)         -->  refl_a
 *                       sym(sym(p))         -->  p
 *                       trans(refl_a, p)    -->  p
 *                       trans(p, refl_a)    -->  p
 *                       trans(trans(p,q),r) -->  trans(p, trans(q,r))
 *                       transport(refl_a,x) -->  x
 *                       ap(f, refl_a)       -->  refl_{f(a)}  (where supported)
 *
 *                     Plus structural recursion: reduce inside
 *                     symmetry, transitivity, transport, before
 *                     applying head reductions.
 *
 *   (equal? a b)      Decide judgmental equality. Returns #t if
 *                     reduce(a) and reduce(b) are structurally
 *                     identical, #f otherwise. NOTE: this is alpha-
 *                     structural for now — no alpha-equivalence
 *                     under binders. Bound-variable identity is
 *                     pointwise equality of the binder's symbol.
 *
 *   (reduce-trace t)  Returns a list of intermediate terms produced
 *                     during reduction. Useful for debugging and for
 *                     understanding what the rules do. The list
 *                     starts with t and ends with reduce(t); each
 *                     element is one rewrite step.
 *
 * What this does NOT provide:
 *
 *   - Conversion checking for any type former other than Id.
 *     There are no beta-reduction rules for Pi here, no eta rules,
 *     no Sigma projections being simplified. Pi and Sigma terms
 *     pass through reduce unchanged.
 *   - Alpha-equivalence. Two terms with bound variables of
 *     different names but the same structure are NOT equal here.
 *     A proper implementation requires de Bruijn levels or
 *     hash-cons-with-rename.
 *   - Anything about universes or couniverses. (U 0) and (U 0)
 *     are equal because they're structurally equal; (U 0) and
 *     (U 1) are not. No cumulativity, no lattice operations.
 *   - Confluence guarantees beyond what the rule set provides.
 *     The rules above form a confluent system on the identity
 *     fragment (checkable by hand: critical pairs all close).
 *     Adding rules requires re-checking this.
 *
 * Why this is interesting:
 *
 *   This is the smallest piece of a type theory that contains
 *   "computational identity" in any real sense — the property
 *   that judgmental equality on identity proofs is decided by
 *   running rewrite rules, not by structural equality of AST nodes.
 *   With this in place, (Id-sym (Id-sym (refl 'x))) is judgmentally
 *   equal to (refl 'x), in the sense that (equal? <former> <latter>)
 *   returns #t. Without this, only structural equality holds, and
 *   the two AST nodes are distinct values.
 *
 *   This module is intentionally small and self-contained. Adding
 *   new reduction rules — for Pi-beta, for Sigma-pairs, for
 *   universe-level lattice operations, for couniverse stratification
 *   — happens here, by adding cases to lizard_tt_step. The flag
 *   system lets each rule be turned off independently, so
 *   experimental rule sets can be tried without recompiling.
 *
 * Pluggable flags:
 *
 *   Each reduction rule consults a flag before firing. A flag is
 *   a (symbol . boolean) entry in a global hash. Default values
 *   are all #t. Flag operations are exposed via:
 *
 *     (flag-set! 'name #t)   Enable a rule
 *     (flag-set! 'name #f)   Disable a rule
 *     (flag-get 'name)       Query
 *     (flag-list)            All flag names
 *
 *   The flags defined here:
 *
 *     reduce-sym-refl        sym(refl) -> refl
 *     reduce-sym-involutive  sym(sym(p)) -> p
 *     reduce-trans-refl-l    trans(refl, p) -> p
 *     reduce-trans-refl-r    trans(p, refl) -> p
 *     reduce-trans-assoc     trans(trans(p,q),r) -> trans(p, trans(q,r))
 *     reduce-transport-refl  transport(refl, x) -> x
 *     reduce-structural      apply rules inside subterms
 *
 *   Turning off `reduce-structural` reduces only the head. Turning
 *   off the head rules but leaving `reduce-structural` on gives a
 *   strict structural reducer that descends but doesn't simplify.
 */

#include "primitives.h"
#include "env.h"
#include "errors.h"
#include "lizard.h"
#include "mem.h"
#include <string.h>
#include <stdlib.h>  /* malloc/free for H.1 HIT registry */

/* ----- Flag system --------------------------------------------------- */

/* We use a simple linked-list of (name, value) pairs rather than the
 * hash table because the heap is GC-managed in a way that makes
 * holding pointers to mutable hashes across calls awkward. The flag
 * list is small (a handful of entries) so linear scan is fine. */

typedef struct lizard_tt_flag {
  const char *name;
  int value;
  struct lizard_tt_flag *next;
} lizard_tt_flag_t;

static lizard_tt_flag_t *flag_list = NULL;

static lizard_tt_flag_t *flag_find_or_create(const char *name) {
  lizard_tt_flag_t *f;
  for (f = flag_list; f != NULL; f = f->next) {
    if (strcmp(f->name, name) == 0) return f;
  }
  /* Not found; create with default value 1. Persisted across calls;
   * this is fine because flag entries are tiny and finite. */
  f = (lizard_tt_flag_t *)malloc(sizeof(lizard_tt_flag_t));
  f->name = strdup(name);
  f->value = 1;
  f->next = flag_list;
  flag_list = f;
  return f;
}

int lizard_tt_flag_get(const char *name) {
  return flag_find_or_create(name)->value;
}

void lizard_tt_flag_set(const char *name, int value) {
  flag_find_or_create(name)->value = value ? 1 : 0;
}

/* Initialise the standard flag set. Called once at startup. */
void lizard_tt_flags_init(void) {
  flag_find_or_create("reduce-sym-refl")->value = 1;
  flag_find_or_create("reduce-sym-involutive")->value = 1;
  flag_find_or_create("reduce-trans-refl-l")->value = 1;
  flag_find_or_create("reduce-trans-refl-r")->value = 1;
  flag_find_or_create("reduce-trans-inverse")->value = 1;
  flag_find_or_create("reduce-trans-assoc")->value = 1;
  flag_find_or_create("reduce-transport-refl")->value = 1;
  flag_find_or_create("reduce-pi-beta")->value = 1;
  /* HOTT-fragment rules — identity computes structurally on type
   * formers and along functions. */
  flag_find_or_create("reduce-ap-refl")->value = 1;
  flag_find_or_create("reduce-ap-sym")->value = 1;
  flag_find_or_create("reduce-ap-trans")->value = 1;
  flag_find_or_create("reduce-id-pi")->value = 1;
  flag_find_or_create("reduce-id-sigma")->value = 1;
  flag_find_or_create("reduce-id-sum")->value = 1;
  flag_find_or_create("reduce-id-unit")->value = 1;
  flag_find_or_create("reduce-fst-beta")->value = 1;
  flag_find_or_create("reduce-snd-beta")->value = 1;
  flag_find_or_create("reduce-case-beta")->value = 1;
  flag_find_or_create("reduce-j-refl")->value = 1;
  /* HOTT-fragment: per-type-former transport rules. The motive
   * (Lambda 'x T) tells the engine which rule to apply based on T. */
  flag_find_or_create("reduce-xport-refl")->value = 1;
  flag_find_or_create("reduce-xport-unit")->value = 1;
  flag_find_or_create("reduce-xport-sum")->value = 1;
  flag_find_or_create("reduce-xport-sigma")->value = 1;
  flag_find_or_create("reduce-xport-pi")->value = 1;
  /* Universe-expression simplification. */
  flag_find_or_create("reduce-u-suc-concrete")->value = 1;
  flag_find_or_create("reduce-u-max-concrete")->value = 1;
  flag_find_or_create("reduce-u-max-idem")->value = 1;
  flag_find_or_create("reduce-u-min-concrete")->value = 1;
  flag_find_or_create("reduce-u-min-idem")->value = 1;
  flag_find_or_create("reduce-u-lattice-absorb")->value = 1;
  /* Phase L.2: set-based universe operations. */
  flag_find_or_create("reduce-u-set-concrete")->value = 1;
  /* Phase L.4: couniverse-lattice operations. */
  flag_find_or_create("reduce-co-set-concrete")->value = 1;
  flag_find_or_create("reduce-co-max-idem")->value = 1;
  flag_find_or_create("reduce-co-min-idem")->value = 1;
  flag_find_or_create("reduce-co-lattice-absorb")->value = 1;
  /* Cubical layer simplification. */
  flag_find_or_create("reduce-i-and")->value = 1;
  flag_find_or_create("reduce-i-or")->value = 1;
  flag_find_or_create("reduce-i-neg")->value = 1;
  flag_find_or_create("reduce-path-beta")->value = 1;
  /* Face algebra simplification (Turn 7). */
  flag_find_or_create("reduce-f-and")->value = 1;
  flag_find_or_create("reduce-f-or")->value = 1;
  flag_find_or_create("reduce-f-eq-concrete")->value = 1;
  flag_find_or_create("reduce-f-conn-eq")->value = 1;
  /* Kan composition rules (Turn 8). */
  flag_find_or_create("reduce-comp-f1")->value = 1;      /* φ=F1 boundary */
  flag_find_or_create("reduce-comp-pi")->value = 1;
  flag_find_or_create("reduce-comp-sigma")->value = 1;
  flag_find_or_create("reduce-comp-path")->value = 1;
  flag_find_or_create("reduce-comp-unit")->value = 1;
  flag_find_or_create("reduce-hcomp-f1")->value = 1;
  flag_find_or_create("reduce-fill-to-comp")->value = 1;
  /* Glue and ua (Turns 9 & 10). */
  flag_find_or_create("reduce-glue-f1")->value = 1;     /* Glue with F1 face */
  flag_find_or_create("reduce-glue-f0")->value = 1;     /* Glue with F0 face */
  flag_find_or_create("reduce-unglue-intro")->value = 1; /* unglue(glue) */
  flag_find_or_create("reduce-equiv-fun-id")->value = 1; /* equiv-fun(id-equiv) */
  flag_find_or_create("reduce-equiv-inv-id")->value = 1; /* equiv-inv(id-equiv) */
  flag_find_or_create("reduce-ua-endpoints")->value = 1; /* (ua e) @ i0/i1 */
  /* Systems and comp Glue (Turn 11). */
  flag_find_or_create("reduce-system-nil-comp")->value = 1; /* system-nil → use base */
  flag_find_or_create("reduce-system-true-clause")->value = 1; /* F1 clause selects */
  flag_find_or_create("reduce-comp-glue")->value = 1; /* comp over Glue */
  flag_find_or_create("reduce-ua-comp")->value = 1; /* (ua e) @ i computes via Glue */
  flag_find_or_create("reduce-structural")->value = 1;
}

/* Forward declarations for the small TT-node constructors. They're
 * defined later in the file but used earlier by substitution. */
static lizard_ast_node_t *make_refl(lizard_heap_t *heap,
                                    lizard_ast_node_t *value);
static lizard_ast_node_t *make_sym(lizard_heap_t *heap,
                                   lizard_ast_node_t *path);
static lizard_ast_node_t *make_trans(lizard_heap_t *heap,
                                     lizard_ast_node_t *p,
                                     lizard_ast_node_t *q);
static lizard_ast_node_t *make_transport(lizard_heap_t *heap,
                                         lizard_ast_node_t *path,
                                         lizard_ast_node_t *value);
static lizard_ast_node_t *make_ap(lizard_heap_t *heap,
                                  lizard_ast_node_t *fn,
                                  lizard_ast_node_t *path);
static lizard_ast_node_t *make_app(lizard_heap_t *heap,
                                   lizard_ast_node_t *fn,
                                   lizard_ast_node_t *arg);
static lizard_ast_node_t *make_pi(lizard_heap_t *heap,
                                  lizard_ast_node_t *binder,
                                  lizard_ast_node_t *domain,
                                  lizard_ast_node_t *codomain);
static lizard_ast_node_t *make_id(lizard_heap_t *heap,
                                  lizard_ast_node_t *domain,
                                  lizard_ast_node_t *a,
                                  lizard_ast_node_t *b);
static lizard_ast_node_t *make_pair(lizard_heap_t *heap,
                                    lizard_ast_node_t *fst,
                                    lizard_ast_node_t *snd);
static lizard_ast_node_t *make_fst(lizard_heap_t *heap, lizard_ast_node_t *t);
static lizard_ast_node_t *make_snd(lizard_heap_t *heap, lizard_ast_node_t *t);
static lizard_ast_node_t *make_inl(lizard_heap_t *heap, lizard_ast_node_t *v);
static lizard_ast_node_t *make_inr(lizard_heap_t *heap, lizard_ast_node_t *v);
static lizard_ast_node_t *make_case(lizard_heap_t *heap,
                                    lizard_ast_node_t *s,
                                    lizard_ast_node_t *l,
                                    lizard_ast_node_t *r);
static lizard_ast_node_t *make_j(lizard_heap_t *heap,
                                 lizard_ast_node_t *motive,
                                 lizard_ast_node_t *refl_case,
                                 lizard_ast_node_t *path);
static lizard_ast_node_t *make_unit(lizard_heap_t *heap);
static lizard_ast_node_t *make_bot(lizard_heap_t *heap);
static lizard_ast_node_t *make_xport(lizard_heap_t *heap,
                                     lizard_ast_node_t *motive,
                                     lizard_ast_node_t *path,
                                     lizard_ast_node_t *value);
static lizard_ast_node_t *make_universe(lizard_heap_t *heap, long level);
static lizard_ast_node_t *make_u_suc(lizard_heap_t *heap, lizard_ast_node_t *u);
static lizard_ast_node_t *make_u_max(lizard_heap_t *heap,
                                     lizard_ast_node_t *u,
                                     lizard_ast_node_t *v);
static lizard_ast_node_t *make_u_min(lizard_heap_t *heap,
                                     lizard_ast_node_t *u,
                                     lizard_ast_node_t *v);
/* Phase L.4 — couniverse helpers. */
static lizard_ast_node_t *make_co_max(lizard_heap_t *heap,
                                      lizard_ast_node_t *u,
                                      lizard_ast_node_t *v);
static lizard_ast_node_t *make_co_min(lizard_heap_t *heap,
                                      lizard_ast_node_t *u,
                                      lizard_ast_node_t *v);
static lizard_ast_node_t *make_couniverse_set(lizard_heap_t *heap,
                                              long *dims, long count);
/* Cubical-layer helpers. */
static lizard_ast_node_t *make_i0(lizard_heap_t *heap);
static lizard_ast_node_t *make_i1(lizard_heap_t *heap);
static lizard_ast_node_t *make_i_and(lizard_heap_t *heap,
                                     lizard_ast_node_t *a,
                                     lizard_ast_node_t *b);
static lizard_ast_node_t *make_i_or(lizard_heap_t *heap,
                                    lizard_ast_node_t *a,
                                    lizard_ast_node_t *b);
static lizard_ast_node_t *make_i_neg(lizard_heap_t *heap,
                                     lizard_ast_node_t *a);
static lizard_ast_node_t *make_path(lizard_heap_t *heap,
                                    lizard_ast_node_t *A,
                                    lizard_ast_node_t *a,
                                    lizard_ast_node_t *b);
static lizard_ast_node_t *make_path_abs(lizard_heap_t *heap,
                                        lizard_ast_node_t *binder,
                                        lizard_ast_node_t *body);
static lizard_ast_node_t *make_path_app(lizard_heap_t *heap,
                                        lizard_ast_node_t *p,
                                        lizard_ast_node_t *i);
/* Face and partial-element helpers (Turn 7). */
static lizard_ast_node_t *make_f0(lizard_heap_t *heap);
static lizard_ast_node_t *make_f1(lizard_heap_t *heap);
static lizard_ast_node_t *make_f_eq(lizard_heap_t *heap,
                                    lizard_ast_node_t *l,
                                    lizard_ast_node_t *r);
static lizard_ast_node_t *make_f_and(lizard_heap_t *heap,
                                     lizard_ast_node_t *l,
                                     lizard_ast_node_t *r);
static lizard_ast_node_t *make_f_or(lizard_heap_t *heap,
                                    lizard_ast_node_t *l,
                                    lizard_ast_node_t *r);
static lizard_ast_node_t *make_partial(lizard_heap_t *heap,
                                       lizard_ast_node_t *face,
                                       lizard_ast_node_t *type);
static lizard_ast_node_t *make_sub(lizard_heap_t *heap,
                                   lizard_ast_node_t *type,
                                   lizard_ast_node_t *face,
                                   lizard_ast_node_t *partial);
/* Kan composition helpers (Turn 8). */
static lizard_ast_node_t *make_comp(lizard_heap_t *heap,
                                    int kind, /* AST_TT_COMP/HCOMP/FILL */
                                    lizard_ast_node_t *type_family,
                                    lizard_ast_node_t *face,
                                    lizard_ast_node_t *partial,
                                    lizard_ast_node_t *base);
/* Equivalence, Glue, ua helpers (Turns 9 & 10). */
static lizard_ast_node_t *make_equiv_type(lizard_heap_t *heap,
                                          lizard_ast_node_t *A,
                                          lizard_ast_node_t *B);
static lizard_ast_node_t *make_id_equiv(lizard_heap_t *heap,
                                        lizard_ast_node_t *A);
static lizard_ast_node_t *make_equiv_fun(lizard_heap_t *heap,
                                         lizard_ast_node_t *e);
static lizard_ast_node_t *make_equiv_inv(lizard_heap_t *heap,
                                         lizard_ast_node_t *e);
static lizard_ast_node_t *make_glue(lizard_heap_t *heap,
                                    lizard_ast_node_t *A,
                                    lizard_ast_node_t *face,
                                    lizard_ast_node_t *T,
                                    lizard_ast_node_t *e);
static lizard_ast_node_t *make_glue_intro(lizard_heap_t *heap,
                                          lizard_ast_node_t *face,
                                          lizard_ast_node_t *t,
                                          lizard_ast_node_t *a);
static lizard_ast_node_t *make_unglue(lizard_heap_t *heap,
                                      lizard_ast_node_t *e,
                                      lizard_ast_node_t *g);
static lizard_ast_node_t *make_ua(lizard_heap_t *heap,
                                  lizard_ast_node_t *e);
/* System helpers (Turn 11). */
static lizard_ast_node_t *make_system_nil(lizard_heap_t *heap);
static lizard_ast_node_t *make_system_cons(lizard_heap_t *heap,
                                           lizard_ast_node_t *face,
                                           lizard_ast_node_t *value,
                                           lizard_ast_node_t *next);

/* ----- Structural equality (no alpha) -------------------------------- */

/* Compare two AST nodes for structural identity. Used by equal? after
 * reducing both sides. This does NOT do alpha-equivalence — bound
 * variables must have the same names. For a proper system we'd use
 * de Bruijn levels; for now we accept the limitation. */
/* ----- Capture-avoiding substitution ---------------------------------
 *
 * `subst(t, x, v)` returns `t[v/x]` — every free occurrence of `x` in
 * `t` is replaced by `v`. This is used to implement Pi-beta:
 *   ((lambda (x) b) a)  -->  b[a/x]
 *
 * Capture avoidance: when we go under a binder `(Pi 'y A B)`, if y
 * shadows x, we stop substituting in the body. If y is a free
 * variable of v, naive substitution would capture it; we'd need to
 * alpha-rename y first. For the fragment we handle (where v is
 * typically a normalized argument and binders rarely clash), we
 * detect the dangerous case and use a gensym-style fresh name.
 *
 * Implementation note: this is the simplest correct algorithm I can
 * write without de Bruijn. It's O(|t| × |v|) in the dangerous case
 * because we walk v to check for the binder name. For the common
 * case (no capture risk) it's O(|t|). For a production type checker
 * you'd switch to de Bruijn here; for the playground this suffices. */

static int contains_free_var(lizard_ast_node_t *t, const char *name) {
  if (t == NULL || name == NULL) return 0;
  switch (t->type) {
  case AST_SYMBOL:
    return strcmp(t->data.variable, name) == 0;
  case AST_PAIR:
    return contains_free_var(t->data.pair.car, name) ||
           contains_free_var(t->data.pair.cdr, name);
  case AST_TT_PI: {
    if (t->data.tt_pi.binder && t->data.tt_pi.binder->type == AST_SYMBOL &&
        strcmp(t->data.tt_pi.binder->data.variable, name) == 0) {
      /* Binder shadows; only domain is in scope of search. */
      return contains_free_var(t->data.tt_pi.domain, name);
    }
    return contains_free_var(t->data.tt_pi.domain, name) ||
           contains_free_var(t->data.tt_pi.codomain, name);
  }
  case AST_TT_SIGMA: {
    if (t->data.tt_sigma.binder &&
        t->data.tt_sigma.binder->type == AST_SYMBOL &&
        strcmp(t->data.tt_sigma.binder->data.variable, name) == 0) {
      return contains_free_var(t->data.tt_sigma.domain, name);
    }
    return contains_free_var(t->data.tt_sigma.domain, name) ||
           contains_free_var(t->data.tt_sigma.codomain, name);
  }
  case AST_TT_PI_FRESH: {
    if (t->data.tt_pi_fresh.binder &&
        t->data.tt_pi_fresh.binder->type == AST_SYMBOL &&
        strcmp(t->data.tt_pi_fresh.binder->data.variable, name) == 0) {
      return contains_free_var(t->data.tt_pi_fresh.domain, name);
    }
    return contains_free_var(t->data.tt_pi_fresh.domain, name) ||
           contains_free_var(t->data.tt_pi_fresh.codomain, name);
  }
  case AST_TT_SIGMA_FRESH: {
    if (t->data.tt_sigma_fresh.binder &&
        t->data.tt_sigma_fresh.binder->type == AST_SYMBOL &&
        strcmp(t->data.tt_sigma_fresh.binder->data.variable, name) == 0) {
      return contains_free_var(t->data.tt_sigma_fresh.domain, name);
    }
    return contains_free_var(t->data.tt_sigma_fresh.domain, name) ||
           contains_free_var(t->data.tt_sigma_fresh.codomain, name);
  }
  case AST_TT_CO_PI_FRESH: {
    if (t->data.tt_co_pi_fresh.binder &&
        t->data.tt_co_pi_fresh.binder->type == AST_SYMBOL &&
        strcmp(t->data.tt_co_pi_fresh.binder->data.variable, name) == 0) {
      return contains_free_var(t->data.tt_co_pi_fresh.domain, name);
    }
    return contains_free_var(t->data.tt_co_pi_fresh.domain, name) ||
           contains_free_var(t->data.tt_co_pi_fresh.codomain, name);
  }
  case AST_TT_CO_SIGMA_FRESH: {
    if (t->data.tt_co_sigma_fresh.binder &&
        t->data.tt_co_sigma_fresh.binder->type == AST_SYMBOL &&
        strcmp(t->data.tt_co_sigma_fresh.binder->data.variable, name) == 0) {
      return contains_free_var(t->data.tt_co_sigma_fresh.domain, name);
    }
    return contains_free_var(t->data.tt_co_sigma_fresh.domain, name) ||
           contains_free_var(t->data.tt_co_sigma_fresh.codomain, name);
  }
  case AST_TT_BOX:
    return contains_free_var(t->data.tt_box.argument, name);
  case AST_TT_DIAMOND:
    return contains_free_var(t->data.tt_diamond.argument, name);
  case AST_TT_BOX_INTRO:
    return contains_free_var(t->data.tt_box_intro.body, name);
  case AST_TT_BOX_ELIM: {
    /* The binder shadows in body but not in scrutinee. */
    if (contains_free_var(t->data.tt_box_elim.scrutinee, name)) return 1;
    if (t->data.tt_box_elim.binder &&
        t->data.tt_box_elim.binder->type == AST_SYMBOL &&
        strcmp(t->data.tt_box_elim.binder->data.variable, name) == 0) {
      /* Shadowed in body. */
      return 0;
    }
    return contains_free_var(t->data.tt_box_elim.body, name);
  }
  case AST_TT_APP:
    return contains_free_var(t->data.tt_app.fun, name) ||
           contains_free_var(t->data.tt_app.arg, name);
  case AST_TT_SUM:
    return contains_free_var(t->data.tt_sum.left, name) ||
           contains_free_var(t->data.tt_sum.right, name);
  case AST_TT_ID:
    return contains_free_var(t->data.tt_id.domain, name) ||
           contains_free_var(t->data.tt_id.a, name) ||
           contains_free_var(t->data.tt_id.b, name);
  case AST_TT_REFL:
    return contains_free_var(t->data.tt_refl.value, name);
  case AST_TT_ID_SYM:
    return contains_free_var(t->data.tt_id_sym.path, name);
  case AST_TT_ID_TRANS:
    return contains_free_var(t->data.tt_id_trans.p, name) ||
           contains_free_var(t->data.tt_id_trans.q, name);
  case AST_TT_TRANSPORT:
    return contains_free_var(t->data.tt_transport.path, name) ||
           contains_free_var(t->data.tt_transport.value, name);
  case AST_TT_LAMBDA: {
    if (t->data.tt_lambda.binder &&
        t->data.tt_lambda.binder->type == AST_SYMBOL &&
        strcmp(t->data.tt_lambda.binder->data.variable, name) == 0) {
      return 0;   /* binder shadows the name */
    }
    return contains_free_var(t->data.tt_lambda.body, name);
  }
  case AST_TT_AP:
    return contains_free_var(t->data.tt_ap.fn, name) ||
           contains_free_var(t->data.tt_ap.path, name);
  case AST_TT_PAIR:
    return contains_free_var(t->data.tt_pair.fst, name) ||
           contains_free_var(t->data.tt_pair.snd, name);
  case AST_TT_FST:
  case AST_TT_SND:
    return contains_free_var(t->data.tt_proj.target, name);
  case AST_TT_INL:
  case AST_TT_INR:
    return contains_free_var(t->data.tt_inj.value, name);
  case AST_TT_CASE:
    return contains_free_var(t->data.tt_case.scrutinee, name) ||
           contains_free_var(t->data.tt_case.left_branch, name) ||
           contains_free_var(t->data.tt_case.right_branch, name);
  case AST_TT_J:
    return contains_free_var(t->data.tt_j.motive, name) ||
           contains_free_var(t->data.tt_j.refl_case, name) ||
           contains_free_var(t->data.tt_j.path, name);
  case AST_TT_XPORT:
    return contains_free_var(t->data.tt_xport.motive, name) ||
           contains_free_var(t->data.tt_xport.path, name) ||
           contains_free_var(t->data.tt_xport.value, name);
  case AST_TT_U_VAR:
    /* Universe variables stand in a separate namespace, but for the
     * conservative free-var test we say "no" — they don't clash with
     * term-level variables. */
    return 0;
  case AST_TT_U_SUC:
    return contains_free_var(t->data.tt_u_suc.operand, name);
  case AST_TT_U_MAX:
    return contains_free_var(t->data.tt_u_max.left, name) ||
           contains_free_var(t->data.tt_u_max.right, name);
  case AST_TT_U_MIN:
    return contains_free_var(t->data.tt_u_min.left, name) ||
           contains_free_var(t->data.tt_u_min.right, name);
  /* Cubical: interval terms have their own variable namespace but we
   * conservatively say they don't bind term-level names. */
  case AST_TT_INTERVAL:
  case AST_TT_I0:
  case AST_TT_I1:
  case AST_TT_I_VAR:
    return 0;
  case AST_TT_I_AND:
  case AST_TT_I_OR:
    return contains_free_var(t->data.tt_i_binop.left, name) ||
           contains_free_var(t->data.tt_i_binop.right, name);
  case AST_TT_I_NEG:
    return contains_free_var(t->data.tt_i_neg.operand, name);
  case AST_TT_PATH:
    return contains_free_var(t->data.tt_path.domain, name) ||
           contains_free_var(t->data.tt_path.a, name) ||
           contains_free_var(t->data.tt_path.b, name);
  case AST_TT_PATH_ABS:
    /* The interval binder doesn't shadow term-level vars, so descend. */
    return contains_free_var(t->data.tt_path_abs.body, name);
  case AST_TT_PATH_APP:
    return contains_free_var(t->data.tt_path_app.path, name) ||
           contains_free_var(t->data.tt_path_app.point, name);
  case AST_TT_F0:
  case AST_TT_F1:
    return 0;
  case AST_TT_F_EQ:
    return contains_free_var(t->data.tt_f_eq.left, name) ||
           contains_free_var(t->data.tt_f_eq.right, name);
  case AST_TT_F_AND:
  case AST_TT_F_OR:
    return contains_free_var(t->data.tt_f_binop.left, name) ||
           contains_free_var(t->data.tt_f_binop.right, name);
  case AST_TT_PARTIAL:
    return contains_free_var(t->data.tt_partial.face, name) ||
           contains_free_var(t->data.tt_partial.type, name);
  case AST_TT_SUB:
    return contains_free_var(t->data.tt_sub.type, name) ||
           contains_free_var(t->data.tt_sub.face, name) ||
           contains_free_var(t->data.tt_sub.partial, name);
  case AST_TT_COMP:
  case AST_TT_HCOMP:
  case AST_TT_FILL:
    return contains_free_var(t->data.tt_comp.type_family, name) ||
           contains_free_var(t->data.tt_comp.face, name) ||
           contains_free_var(t->data.tt_comp.partial, name) ||
           contains_free_var(t->data.tt_comp.base, name);
  case AST_TT_EQUIV_TYPE:
    return contains_free_var(t->data.tt_equiv_type.domain, name) ||
           contains_free_var(t->data.tt_equiv_type.codomain, name);
  case AST_TT_ID_EQUIV:
  case AST_TT_EQUIV_FUN:
  case AST_TT_EQUIV_INV:
    return contains_free_var(t->data.tt_equiv_op.operand, name);
  case AST_TT_GLUE:
    return contains_free_var(t->data.tt_glue.base, name) ||
           contains_free_var(t->data.tt_glue.face, name) ||
           contains_free_var(t->data.tt_glue.t, name) ||
           contains_free_var(t->data.tt_glue.equiv, name);
  case AST_TT_GLUE_INTRO:
    return contains_free_var(t->data.tt_glue_intro.face, name) ||
           contains_free_var(t->data.tt_glue_intro.t, name) ||
           contains_free_var(t->data.tt_glue_intro.a, name);
  case AST_TT_UNGLUE:
    return contains_free_var(t->data.tt_unglue.equiv, name) ||
           contains_free_var(t->data.tt_unglue.target, name);
  case AST_TT_UA:
    return contains_free_var(t->data.tt_ua.equiv, name);
  case AST_TT_SYSTEM_NIL:
    return 0;
  case AST_TT_SYSTEM_CONS:
    return contains_free_var(t->data.tt_system_cons.face, name) ||
           contains_free_var(t->data.tt_system_cons.value, name) ||
           contains_free_var(t->data.tt_system_cons.next, name);
  case AST_TT_UNIT:
  case AST_TT_TT:
  case AST_TT_BOT:
    return 0;
  /* Phase H.1: HIT structures. We treat the declaration itself as a
   * top-level form that doesn't capture variables — it's metadata —
   * but HIT_APP, HIT_PATH, and HIT_CONSTRUCTOR can have variables in
   * their arguments / endpoints / arg-types. */
  case AST_TT_HIT_REF:
    return 0;
  case AST_TT_HIT_DECL: {
    lz_list_node_t *p;
    for (p = t->data.tt_hit_decl.constructors->head;
         p != t->data.tt_hit_decl.constructors->nil; p = p->next) {
      if (contains_free_var(((lizard_ast_list_node_t *)p)->ast, name)) return 1;
    }
    for (p = t->data.tt_hit_decl.paths->head;
         p != t->data.tt_hit_decl.paths->nil; p = p->next) {
      if (contains_free_var(((lizard_ast_list_node_t *)p)->ast, name)) return 1;
    }
    return 0;
  }
  case AST_TT_HIT_CONSTRUCTOR: {
    lz_list_node_t *p;
    for (p = t->data.tt_hit_constructor.arg_types->head;
         p != t->data.tt_hit_constructor.arg_types->nil; p = p->next) {
      if (contains_free_var(((lizard_ast_list_node_t *)p)->ast, name)) return 1;
    }
    return 0;
  }
  case AST_TT_HIT_PATH:
    return contains_free_var(t->data.tt_hit_path.source, name) ||
           contains_free_var(t->data.tt_hit_path.target, name);
  case AST_TT_HIT_APP: {
    lz_list_node_t *p;
    for (p = t->data.tt_hit_app.args->head;
         p != t->data.tt_hit_app.args->nil; p = p->next) {
      if (contains_free_var(((lizard_ast_list_node_t *)p)->ast, name)) return 1;
    }
    return 0;
  }
  default:
    return 0;
  }
}

/* Generate a fresh symbol name that doesn't clash with `name` or with
 * any free variable of `t` and `v`. Used to alpha-rename binders
 * when capture would occur. */
static lizard_ast_node_t *fresh_symbol(const char *base,
                                       lizard_ast_node_t *t,
                                       lizard_ast_node_t *v,
                                       lizard_heap_t *heap) {
  static unsigned long counter = 0;
  char *buf;
  lizard_ast_node_t *sym;
  size_t baselen = base ? strlen(base) : 4;
  while (1) {
    counter++;
    buf = lizard_heap_alloc(baselen + 32);
    sprintf(buf, "%s_%lu", base ? base : "fresh", counter);
    if (!contains_free_var(t, buf) && !contains_free_var(v, buf)) {
      sym = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      sym->type = AST_SYMBOL;
      sym->data.variable = buf;
      return sym;
    }
  }
}

static lizard_ast_node_t *subst_rec(lizard_ast_node_t *t,
                                    const char *x,
                                    lizard_ast_node_t *v,
                                    lizard_heap_t *heap);

/* Substitute under a binder. Handles shadowing and capture. */
static lizard_ast_node_t *subst_under_binder(
    lizard_ast_node_t *binder,
    lizard_ast_node_t *body,
    const char *x,
    lizard_ast_node_t *v,
    lizard_heap_t *heap,
    lizard_ast_node_t **out_binder) {
  const char *bname;
  if (binder == NULL || binder->type != AST_SYMBOL) {
    /* No real binder — just substitute in body. */
    *out_binder = binder;
    return subst_rec(body, x, v, heap);
  }
  bname = binder->data.variable;
  if (strcmp(bname, x) == 0) {
    /* Binder shadows x; don't substitute in body. */
    *out_binder = binder;
    return body;
  }
  if (contains_free_var(v, bname)) {
    /* Binder would capture a free variable of v. Alpha-rename. */
    lizard_ast_node_t *fresh = fresh_symbol(bname, body, v, heap);
    lizard_ast_node_t *renamed_body = subst_rec(body, bname, fresh, heap);
    *out_binder = fresh;
    return subst_rec(renamed_body, x, v, heap);
  }
  *out_binder = binder;
  return subst_rec(body, x, v, heap);
}

static lizard_ast_node_t *subst_rec(lizard_ast_node_t *t,
                                    const char *x,
                                    lizard_ast_node_t *v,
                                    lizard_heap_t *heap) {
  if (t == NULL) return NULL;
  switch (t->type) {
  case AST_SYMBOL:
    if (strcmp(t->data.variable, x) == 0) return v;
    return t;
  case AST_TT_PI: {
    lizard_ast_node_t *new_dom = subst_rec(t->data.tt_pi.domain, x, v, heap);
    lizard_ast_node_t *new_binder;
    lizard_ast_node_t *new_cod = subst_under_binder(
        t->data.tt_pi.binder, t->data.tt_pi.codomain, x, v, heap, &new_binder);
    if (new_dom == t->data.tt_pi.domain && new_cod == t->data.tt_pi.codomain &&
        new_binder == t->data.tt_pi.binder)
      return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_PI;
      n->data.tt_pi.binder = new_binder;
      n->data.tt_pi.domain = new_dom;
      n->data.tt_pi.codomain = new_cod;
      return n;
    }
  }
  case AST_TT_SIGMA: {
    lizard_ast_node_t *new_dom = subst_rec(t->data.tt_sigma.domain, x, v, heap);
    lizard_ast_node_t *new_binder;
    lizard_ast_node_t *new_cod = subst_under_binder(
        t->data.tt_sigma.binder, t->data.tt_sigma.codomain, x, v, heap,
        &new_binder);
    if (new_dom == t->data.tt_sigma.domain &&
        new_cod == t->data.tt_sigma.codomain &&
        new_binder == t->data.tt_sigma.binder)
      return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_SIGMA;
      n->data.tt_sigma.binder = new_binder;
      n->data.tt_sigma.domain = new_dom;
      n->data.tt_sigma.codomain = new_cod;
      return n;
    }
  }
  case AST_TT_PI_FRESH: {
    lizard_ast_node_t *new_dom = subst_rec(t->data.tt_pi_fresh.domain, x, v, heap);
    lizard_ast_node_t *new_binder;
    lizard_ast_node_t *new_cod = subst_under_binder(
        t->data.tt_pi_fresh.binder, t->data.tt_pi_fresh.codomain, x, v, heap,
        &new_binder);
    if (new_dom == t->data.tt_pi_fresh.domain &&
        new_cod == t->data.tt_pi_fresh.codomain &&
        new_binder == t->data.tt_pi_fresh.binder)
      return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_PI_FRESH;
      n->data.tt_pi_fresh.binder = new_binder;
      n->data.tt_pi_fresh.domain = new_dom;
      n->data.tt_pi_fresh.codomain = new_cod;
      return n;
    }
  }
  case AST_TT_SIGMA_FRESH: {
    lizard_ast_node_t *new_dom = subst_rec(t->data.tt_sigma_fresh.domain, x, v, heap);
    lizard_ast_node_t *new_binder;
    lizard_ast_node_t *new_cod = subst_under_binder(
        t->data.tt_sigma_fresh.binder, t->data.tt_sigma_fresh.codomain, x, v, heap,
        &new_binder);
    if (new_dom == t->data.tt_sigma_fresh.domain &&
        new_cod == t->data.tt_sigma_fresh.codomain &&
        new_binder == t->data.tt_sigma_fresh.binder)
      return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_SIGMA_FRESH;
      n->data.tt_sigma_fresh.binder = new_binder;
      n->data.tt_sigma_fresh.domain = new_dom;
      n->data.tt_sigma_fresh.codomain = new_cod;
      return n;
    }
  }
  case AST_TT_CO_PI_FRESH: {
    lizard_ast_node_t *new_dom = subst_rec(t->data.tt_co_pi_fresh.domain, x, v, heap);
    lizard_ast_node_t *new_binder;
    lizard_ast_node_t *new_cod = subst_under_binder(
        t->data.tt_co_pi_fresh.binder, t->data.tt_co_pi_fresh.codomain, x, v, heap,
        &new_binder);
    if (new_dom == t->data.tt_co_pi_fresh.domain &&
        new_cod == t->data.tt_co_pi_fresh.codomain &&
        new_binder == t->data.tt_co_pi_fresh.binder)
      return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_CO_PI_FRESH;
      n->data.tt_co_pi_fresh.binder = new_binder;
      n->data.tt_co_pi_fresh.domain = new_dom;
      n->data.tt_co_pi_fresh.codomain = new_cod;
      return n;
    }
  }
  case AST_TT_CO_SIGMA_FRESH: {
    lizard_ast_node_t *new_dom = subst_rec(t->data.tt_co_sigma_fresh.domain, x, v, heap);
    lizard_ast_node_t *new_binder;
    lizard_ast_node_t *new_cod = subst_under_binder(
        t->data.tt_co_sigma_fresh.binder, t->data.tt_co_sigma_fresh.codomain, x, v, heap,
        &new_binder);
    if (new_dom == t->data.tt_co_sigma_fresh.domain &&
        new_cod == t->data.tt_co_sigma_fresh.codomain &&
        new_binder == t->data.tt_co_sigma_fresh.binder)
      return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_CO_SIGMA_FRESH;
      n->data.tt_co_sigma_fresh.binder = new_binder;
      n->data.tt_co_sigma_fresh.domain = new_dom;
      n->data.tt_co_sigma_fresh.codomain = new_cod;
      return n;
    }
  }
  case AST_TT_BOX: {
    lizard_ast_node_t *new_arg = subst_rec(t->data.tt_box.argument, x, v, heap);
    if (new_arg == t->data.tt_box.argument) return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_BOX;
      n->data.tt_box.argument = new_arg;
      return n;
    }
  }
  case AST_TT_DIAMOND: {
    lizard_ast_node_t *new_arg = subst_rec(t->data.tt_diamond.argument, x, v, heap);
    if (new_arg == t->data.tt_diamond.argument) return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_DIAMOND;
      n->data.tt_diamond.argument = new_arg;
      return n;
    }
  }
  case AST_TT_BOX_INTRO: {
    lizard_ast_node_t *new_body = subst_rec(t->data.tt_box_intro.body, x, v, heap);
    if (new_body == t->data.tt_box_intro.body) return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_BOX_INTRO;
      n->data.tt_box_intro.body = new_body;
      return n;
    }
  }
  case AST_TT_BOX_ELIM: {
    /* Substitute in scrutinee always; in body only if binder doesn't
     * shadow x. */
    lizard_ast_node_t *new_scrut = subst_rec(t->data.tt_box_elim.scrutinee, x, v, heap);
    lizard_ast_node_t *new_binder;
    lizard_ast_node_t *new_body = subst_under_binder(
        t->data.tt_box_elim.binder, t->data.tt_box_elim.body, x, v, heap,
        &new_binder);
    if (new_scrut == t->data.tt_box_elim.scrutinee &&
        new_body == t->data.tt_box_elim.body &&
        new_binder == t->data.tt_box_elim.binder) {
      return t;
    }
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_BOX_ELIM;
      n->data.tt_box_elim.binder = new_binder;
      n->data.tt_box_elim.scrutinee = new_scrut;
      n->data.tt_box_elim.body = new_body;
      return n;
    }
  }
  case AST_TT_APP: {
    lizard_ast_node_t *fn = subst_rec(t->data.tt_app.fun, x, v, heap);
    lizard_ast_node_t *ar = subst_rec(t->data.tt_app.arg, x, v, heap);
    if (fn == t->data.tt_app.fun && ar == t->data.tt_app.arg) return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_APP;
      n->data.tt_app.fun = fn;
      n->data.tt_app.arg = ar;
      return n;
    }
  }
  case AST_TT_SUM: {
    lizard_ast_node_t *l = subst_rec(t->data.tt_sum.left, x, v, heap);
    lizard_ast_node_t *r = subst_rec(t->data.tt_sum.right, x, v, heap);
    if (l == t->data.tt_sum.left && r == t->data.tt_sum.right) return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_SUM;
      n->data.tt_sum.left = l;
      n->data.tt_sum.right = r;
      return n;
    }
  }
  case AST_TT_ID: {
    lizard_ast_node_t *dom = subst_rec(t->data.tt_id.domain, x, v, heap);
    lizard_ast_node_t *a = subst_rec(t->data.tt_id.a, x, v, heap);
    lizard_ast_node_t *b = subst_rec(t->data.tt_id.b, x, v, heap);
    if (dom == t->data.tt_id.domain && a == t->data.tt_id.a &&
        b == t->data.tt_id.b)
      return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_ID;
      n->data.tt_id.domain = dom;
      n->data.tt_id.a = a;
      n->data.tt_id.b = b;
      return n;
    }
  }
  case AST_TT_REFL: {
    lizard_ast_node_t *val = subst_rec(t->data.tt_refl.value, x, v, heap);
    if (val == t->data.tt_refl.value) return t;
    return make_refl(heap, val);
  }
  case AST_TT_ID_SYM: {
    lizard_ast_node_t *p = subst_rec(t->data.tt_id_sym.path, x, v, heap);
    if (p == t->data.tt_id_sym.path) return t;
    return make_sym(heap, p);
  }
  case AST_TT_ID_TRANS: {
    lizard_ast_node_t *p = subst_rec(t->data.tt_id_trans.p, x, v, heap);
    lizard_ast_node_t *q = subst_rec(t->data.tt_id_trans.q, x, v, heap);
    if (p == t->data.tt_id_trans.p && q == t->data.tt_id_trans.q) return t;
    return make_trans(heap, p, q);
  }
  case AST_TT_TRANSPORT: {
    lizard_ast_node_t *p = subst_rec(t->data.tt_transport.path, x, v, heap);
    lizard_ast_node_t *vv = subst_rec(t->data.tt_transport.value, x, v, heap);
    if (p == t->data.tt_transport.path && vv == t->data.tt_transport.value)
      return t;
    return make_transport(heap, p, vv);
  }
  case AST_TT_LAMBDA: {
    lizard_ast_node_t *new_binder;
    lizard_ast_node_t *new_body = subst_under_binder(
        t->data.tt_lambda.binder, t->data.tt_lambda.body, x, v, heap,
        &new_binder);
    if (new_body == t->data.tt_lambda.body && new_binder == t->data.tt_lambda.binder)
      return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_LAMBDA;
      n->data.tt_lambda.binder = new_binder;
      n->data.tt_lambda.body = new_body;
      return n;
    }
  }
  case AST_TT_AP: {
    lizard_ast_node_t *fn = subst_rec(t->data.tt_ap.fn, x, v, heap);
    lizard_ast_node_t *path = subst_rec(t->data.tt_ap.path, x, v, heap);
    if (fn == t->data.tt_ap.fn && path == t->data.tt_ap.path) return t;
    return make_ap(heap, fn, path);
  }
  case AST_TT_PAIR: {
    lizard_ast_node_t *f = subst_rec(t->data.tt_pair.fst, x, v, heap);
    lizard_ast_node_t *s = subst_rec(t->data.tt_pair.snd, x, v, heap);
    if (f == t->data.tt_pair.fst && s == t->data.tt_pair.snd) return t;
    return make_pair(heap, f, s);
  }
  case AST_TT_FST: {
    lizard_ast_node_t *p = subst_rec(t->data.tt_proj.target, x, v, heap);
    if (p == t->data.tt_proj.target) return t;
    return make_fst(heap, p);
  }
  case AST_TT_SND: {
    lizard_ast_node_t *p = subst_rec(t->data.tt_proj.target, x, v, heap);
    if (p == t->data.tt_proj.target) return t;
    return make_snd(heap, p);
  }
  case AST_TT_INL: {
    lizard_ast_node_t *vv = subst_rec(t->data.tt_inj.value, x, v, heap);
    if (vv == t->data.tt_inj.value) return t;
    return make_inl(heap, vv);
  }
  case AST_TT_INR: {
    lizard_ast_node_t *vv = subst_rec(t->data.tt_inj.value, x, v, heap);
    if (vv == t->data.tt_inj.value) return t;
    return make_inr(heap, vv);
  }
  case AST_TT_CASE: {
    lizard_ast_node_t *s = subst_rec(t->data.tt_case.scrutinee, x, v, heap);
    lizard_ast_node_t *l = subst_rec(t->data.tt_case.left_branch, x, v, heap);
    lizard_ast_node_t *r = subst_rec(t->data.tt_case.right_branch, x, v, heap);
    if (s == t->data.tt_case.scrutinee &&
        l == t->data.tt_case.left_branch &&
        r == t->data.tt_case.right_branch) return t;
    return make_case(heap, s, l, r);
  }
  case AST_TT_J: {
    lizard_ast_node_t *m = subst_rec(t->data.tt_j.motive, x, v, heap);
    lizard_ast_node_t *d = subst_rec(t->data.tt_j.refl_case, x, v, heap);
    lizard_ast_node_t *p = subst_rec(t->data.tt_j.path, x, v, heap);
    if (m == t->data.tt_j.motive &&
        d == t->data.tt_j.refl_case &&
        p == t->data.tt_j.path) return t;
    return make_j(heap, m, d, p);
  }
  case AST_TT_XPORT: {
    lizard_ast_node_t *m = subst_rec(t->data.tt_xport.motive, x, v, heap);
    lizard_ast_node_t *p = subst_rec(t->data.tt_xport.path, x, v, heap);
    lizard_ast_node_t *val = subst_rec(t->data.tt_xport.value, x, v, heap);
    if (m == t->data.tt_xport.motive &&
        p == t->data.tt_xport.path &&
        val == t->data.tt_xport.value) return t;
    return make_xport(heap, m, p, val);
  }
  case AST_TT_U_VAR:
    /* Universe variables live in a separate namespace and aren't
     * substituted by term-level subst. */
    return t;
  case AST_TT_U_SUC: {
    lizard_ast_node_t *u = subst_rec(t->data.tt_u_suc.operand, x, v, heap);
    if (u == t->data.tt_u_suc.operand) return t;
    return make_u_suc(heap, u);
  }
  case AST_TT_U_MAX: {
    lizard_ast_node_t *l = subst_rec(t->data.tt_u_max.left, x, v, heap);
    lizard_ast_node_t *r = subst_rec(t->data.tt_u_max.right, x, v, heap);
    if (l == t->data.tt_u_max.left && r == t->data.tt_u_max.right) return t;
    return make_u_max(heap, l, r);
  }
  case AST_TT_U_MIN: {
    lizard_ast_node_t *l = subst_rec(t->data.tt_u_min.left, x, v, heap);
    lizard_ast_node_t *r = subst_rec(t->data.tt_u_min.right, x, v, heap);
    if (l == t->data.tt_u_min.left && r == t->data.tt_u_min.right) return t;
    return make_u_min(heap, l, r);
  }
  /* Cubical nodes. Interval vars are a separate namespace from term
   * vars, so term-level substitution doesn't affect them. */
  case AST_TT_INTERVAL:
  case AST_TT_I0:
  case AST_TT_I1:
  case AST_TT_I_VAR:
    return t;
  case AST_TT_I_AND: {
    lizard_ast_node_t *l = subst_rec(t->data.tt_i_binop.left, x, v, heap);
    lizard_ast_node_t *r = subst_rec(t->data.tt_i_binop.right, x, v, heap);
    if (l == t->data.tt_i_binop.left && r == t->data.tt_i_binop.right) return t;
    return make_i_and(heap, l, r);
  }
  case AST_TT_I_OR: {
    lizard_ast_node_t *l = subst_rec(t->data.tt_i_binop.left, x, v, heap);
    lizard_ast_node_t *r = subst_rec(t->data.tt_i_binop.right, x, v, heap);
    if (l == t->data.tt_i_binop.left && r == t->data.tt_i_binop.right) return t;
    return make_i_or(heap, l, r);
  }
  case AST_TT_I_NEG: {
    lizard_ast_node_t *o = subst_rec(t->data.tt_i_neg.operand, x, v, heap);
    if (o == t->data.tt_i_neg.operand) return t;
    return make_i_neg(heap, o);
  }
  case AST_TT_PATH: {
    lizard_ast_node_t *A = subst_rec(t->data.tt_path.domain, x, v, heap);
    lizard_ast_node_t *pa = subst_rec(t->data.tt_path.a, x, v, heap);
    lizard_ast_node_t *pb = subst_rec(t->data.tt_path.b, x, v, heap);
    if (A == t->data.tt_path.domain && pa == t->data.tt_path.a &&
        pb == t->data.tt_path.b) return t;
    return make_path(heap, A, pa, pb);
  }
  case AST_TT_PATH_ABS: {
    /* Path-abs binds an interval var, which is a separate namespace
     * from term vars. So term-level subst descends into the body
     * without shadowing concerns. */
    lizard_ast_node_t *body = subst_rec(t->data.tt_path_abs.body, x, v, heap);
    if (body == t->data.tt_path_abs.body) return t;
    return make_path_abs(heap, t->data.tt_path_abs.binder, body);
  }
  case AST_TT_PATH_APP: {
    lizard_ast_node_t *p = subst_rec(t->data.tt_path_app.path, x, v, heap);
    lizard_ast_node_t *i = subst_rec(t->data.tt_path_app.point, x, v, heap);
    if (p == t->data.tt_path_app.path && i == t->data.tt_path_app.point) return t;
    return make_path_app(heap, p, i);
  }
  case AST_TT_F0:
  case AST_TT_F1:
    return t;
  case AST_TT_F_EQ: {
    lizard_ast_node_t *l = subst_rec(t->data.tt_f_eq.left, x, v, heap);
    lizard_ast_node_t *r = subst_rec(t->data.tt_f_eq.right, x, v, heap);
    if (l == t->data.tt_f_eq.left && r == t->data.tt_f_eq.right) return t;
    return make_f_eq(heap, l, r);
  }
  case AST_TT_F_AND: {
    lizard_ast_node_t *l = subst_rec(t->data.tt_f_binop.left, x, v, heap);
    lizard_ast_node_t *r = subst_rec(t->data.tt_f_binop.right, x, v, heap);
    if (l == t->data.tt_f_binop.left && r == t->data.tt_f_binop.right) return t;
    return make_f_and(heap, l, r);
  }
  case AST_TT_F_OR: {
    lizard_ast_node_t *l = subst_rec(t->data.tt_f_binop.left, x, v, heap);
    lizard_ast_node_t *r = subst_rec(t->data.tt_f_binop.right, x, v, heap);
    if (l == t->data.tt_f_binop.left && r == t->data.tt_f_binop.right) return t;
    return make_f_or(heap, l, r);
  }
  case AST_TT_PARTIAL: {
    lizard_ast_node_t *f = subst_rec(t->data.tt_partial.face, x, v, heap);
    lizard_ast_node_t *T = subst_rec(t->data.tt_partial.type, x, v, heap);
    if (f == t->data.tt_partial.face && T == t->data.tt_partial.type) return t;
    return make_partial(heap, f, T);
  }
  case AST_TT_SUB: {
    lizard_ast_node_t *T = subst_rec(t->data.tt_sub.type, x, v, heap);
    lizard_ast_node_t *f = subst_rec(t->data.tt_sub.face, x, v, heap);
    lizard_ast_node_t *p = subst_rec(t->data.tt_sub.partial, x, v, heap);
    if (T == t->data.tt_sub.type && f == t->data.tt_sub.face &&
        p == t->data.tt_sub.partial) return t;
    return make_sub(heap, T, f, p);
  }
  case AST_TT_COMP:
  case AST_TT_HCOMP:
  case AST_TT_FILL: {
    lizard_ast_node_t *tf = subst_rec(t->data.tt_comp.type_family, x, v, heap);
    lizard_ast_node_t *f  = subst_rec(t->data.tt_comp.face, x, v, heap);
    lizard_ast_node_t *p  = subst_rec(t->data.tt_comp.partial, x, v, heap);
    lizard_ast_node_t *b  = subst_rec(t->data.tt_comp.base, x, v, heap);
    if (tf == t->data.tt_comp.type_family && f == t->data.tt_comp.face &&
        p == t->data.tt_comp.partial && b == t->data.tt_comp.base) return t;
    return make_comp(heap, t->type, tf, f, p, b);
  }
  case AST_TT_EQUIV_TYPE: {
    lizard_ast_node_t *A = subst_rec(t->data.tt_equiv_type.domain, x, v, heap);
    lizard_ast_node_t *B = subst_rec(t->data.tt_equiv_type.codomain, x, v, heap);
    if (A == t->data.tt_equiv_type.domain && B == t->data.tt_equiv_type.codomain) return t;
    return make_equiv_type(heap, A, B);
  }
  case AST_TT_ID_EQUIV: {
    lizard_ast_node_t *o = subst_rec(t->data.tt_equiv_op.operand, x, v, heap);
    if (o == t->data.tt_equiv_op.operand) return t;
    return make_id_equiv(heap, o);
  }
  case AST_TT_EQUIV_FUN: {
    lizard_ast_node_t *o = subst_rec(t->data.tt_equiv_op.operand, x, v, heap);
    if (o == t->data.tt_equiv_op.operand) return t;
    return make_equiv_fun(heap, o);
  }
  case AST_TT_EQUIV_INV: {
    lizard_ast_node_t *o = subst_rec(t->data.tt_equiv_op.operand, x, v, heap);
    if (o == t->data.tt_equiv_op.operand) return t;
    return make_equiv_inv(heap, o);
  }
  case AST_TT_GLUE: {
    lizard_ast_node_t *A = subst_rec(t->data.tt_glue.base, x, v, heap);
    lizard_ast_node_t *f = subst_rec(t->data.tt_glue.face, x, v, heap);
    lizard_ast_node_t *T = subst_rec(t->data.tt_glue.t, x, v, heap);
    lizard_ast_node_t *e = subst_rec(t->data.tt_glue.equiv, x, v, heap);
    if (A == t->data.tt_glue.base && f == t->data.tt_glue.face &&
        T == t->data.tt_glue.t && e == t->data.tt_glue.equiv) return t;
    return make_glue(heap, A, f, T, e);
  }
  case AST_TT_GLUE_INTRO: {
    lizard_ast_node_t *f = subst_rec(t->data.tt_glue_intro.face, x, v, heap);
    lizard_ast_node_t *tval = subst_rec(t->data.tt_glue_intro.t, x, v, heap);
    lizard_ast_node_t *aval = subst_rec(t->data.tt_glue_intro.a, x, v, heap);
    if (f == t->data.tt_glue_intro.face && tval == t->data.tt_glue_intro.t &&
        aval == t->data.tt_glue_intro.a) return t;
    return make_glue_intro(heap, f, tval, aval);
  }
  case AST_TT_UNGLUE: {
    lizard_ast_node_t *e = subst_rec(t->data.tt_unglue.equiv, x, v, heap);
    lizard_ast_node_t *g = subst_rec(t->data.tt_unglue.target, x, v, heap);
    if (e == t->data.tt_unglue.equiv && g == t->data.tt_unglue.target) return t;
    return make_unglue(heap, e, g);
  }
  case AST_TT_UA: {
    lizard_ast_node_t *e = subst_rec(t->data.tt_ua.equiv, x, v, heap);
    if (e == t->data.tt_ua.equiv) return t;
    return make_ua(heap, e);
  }
  case AST_TT_SYSTEM_NIL:
    return t;
  case AST_TT_SYSTEM_CONS: {
    lizard_ast_node_t *f = subst_rec(t->data.tt_system_cons.face, x, v, heap);
    lizard_ast_node_t *val = subst_rec(t->data.tt_system_cons.value, x, v, heap);
    lizard_ast_node_t *nx = subst_rec(t->data.tt_system_cons.next, x, v, heap);
    if (f == t->data.tt_system_cons.face && val == t->data.tt_system_cons.value &&
        nx == t->data.tt_system_cons.next) return t;
    return make_system_cons(heap, f, val, nx);
  }
  case AST_TT_UNIT:
  case AST_TT_TT:
  case AST_TT_BOT:
    return t;
  /* Phase H.1: HIT structures. */
  case AST_TT_HIT_REF:
    return t;  /* name reference, no subterms */
  case AST_TT_HIT_DECL: {
    /* Substitute through each constructor and path. If nothing changed,
     * return t unchanged; otherwise build a fresh decl node. */
    lz_list_node_t *p;
    lz_list_t *new_ctors, *new_paths;
    int changed = 0;
    new_ctors = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
    new_paths = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
    for (p = t->data.tt_hit_decl.constructors->head;
         p != t->data.tt_hit_decl.constructors->nil; p = p->next) {
      lizard_ast_node_t *old = ((lizard_ast_list_node_t *)p)->ast;
      lizard_ast_node_t *new_ast = subst_rec(old, x, v, heap);
      lizard_ast_list_node_t *cell = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
      cell->ast = new_ast;
      if (new_ast != old) changed = 1;
      list_append(new_ctors, &cell->node);
    }
    for (p = t->data.tt_hit_decl.paths->head;
         p != t->data.tt_hit_decl.paths->nil; p = p->next) {
      lizard_ast_node_t *old = ((lizard_ast_list_node_t *)p)->ast;
      lizard_ast_node_t *new_ast = subst_rec(old, x, v, heap);
      lizard_ast_list_node_t *cell = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
      cell->ast = new_ast;
      if (new_ast != old) changed = 1;
      list_append(new_paths, &cell->node);
    }
    if (!changed) return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_HIT_DECL;
      n->data.tt_hit_decl.name = t->data.tt_hit_decl.name;
      n->data.tt_hit_decl.constructors = new_ctors;
      n->data.tt_hit_decl.paths = new_paths;
      return n;
    }
  }
  case AST_TT_HIT_CONSTRUCTOR: {
    lz_list_node_t *p;
    lz_list_t *new_types;
    int changed = 0;
    new_types = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
    for (p = t->data.tt_hit_constructor.arg_types->head;
         p != t->data.tt_hit_constructor.arg_types->nil; p = p->next) {
      lizard_ast_node_t *old = ((lizard_ast_list_node_t *)p)->ast;
      lizard_ast_node_t *new_ast = subst_rec(old, x, v, heap);
      lizard_ast_list_node_t *cell = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
      cell->ast = new_ast;
      if (new_ast != old) changed = 1;
      list_append(new_types, &cell->node);
    }
    if (!changed) return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_HIT_CONSTRUCTOR;
      n->data.tt_hit_constructor.name = t->data.tt_hit_constructor.name;
      n->data.tt_hit_constructor.arg_types = new_types;
      return n;
    }
  }
  case AST_TT_HIT_PATH: {
    lizard_ast_node_t *src = subst_rec(t->data.tt_hit_path.source, x, v, heap);
    lizard_ast_node_t *tgt = subst_rec(t->data.tt_hit_path.target, x, v, heap);
    if (src == t->data.tt_hit_path.source && tgt == t->data.tt_hit_path.target) return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_HIT_PATH;
      n->data.tt_hit_path.name = t->data.tt_hit_path.name;
      n->data.tt_hit_path.source = src;
      n->data.tt_hit_path.target = tgt;
      return n;
    }
  }
  case AST_TT_HIT_APP: {
    lz_list_node_t *p;
    lz_list_t *new_args;
    int changed = 0;
    new_args = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
    for (p = t->data.tt_hit_app.args->head;
         p != t->data.tt_hit_app.args->nil; p = p->next) {
      lizard_ast_node_t *old = ((lizard_ast_list_node_t *)p)->ast;
      lizard_ast_node_t *new_ast = subst_rec(old, x, v, heap);
      lizard_ast_list_node_t *cell = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
      cell->ast = new_ast;
      if (new_ast != old) changed = 1;
      list_append(new_args, &cell->node);
    }
    if (!changed) return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_HIT_APP;
      n->data.tt_hit_app.name = t->data.tt_hit_app.name;
      n->data.tt_hit_app.args = new_args;
      return n;
    }
  }
  default:
    return t;
  }
}

lizard_ast_node_t *lizard_tt_subst(lizard_ast_node_t *t, const char *x,
                                   lizard_ast_node_t *v,
                                   lizard_heap_t *heap) {
  return subst_rec(t, x, v, heap);
}

/* Interval-level substitution. Forward declaration so it's visible
 * to the rewrite rules below. */
static lizard_ast_node_t *subst_interval(lizard_ast_node_t *t,
                                         const char *x,
                                         lizard_ast_node_t *v,
                                         lizard_heap_t *heap);

static lizard_ast_node_t *subst_interval(lizard_ast_node_t *t,
                                         const char *x,
                                         lizard_ast_node_t *v,
                                         lizard_heap_t *heap) {
  if (t == NULL) return NULL;
  switch (t->type) {
  case AST_TT_I_VAR:
    if (strcmp(t->data.tt_i_var.name, x) == 0) return v;
    return t;
  case AST_TT_INTERVAL:
  case AST_TT_I0:
  case AST_TT_I1:
  case AST_SYMBOL:
  case AST_NUMBER:
  case AST_BOOL:
  case AST_NIL:
  case AST_STRING:
  case AST_TT_UNIVERSE:
  case AST_TT_UNIVERSE_SET:
  case AST_TT_COUNIVERSE_SET:
  case AST_TT_U_VAR:
  case AST_TT_UNIT:
  case AST_TT_TT:
  case AST_TT_BOT:
    return t;
  case AST_TT_I_AND: {
    lizard_ast_node_t *l = subst_interval(t->data.tt_i_binop.left, x, v, heap);
    lizard_ast_node_t *r = subst_interval(t->data.tt_i_binop.right, x, v, heap);
    if (l == t->data.tt_i_binop.left && r == t->data.tt_i_binop.right) return t;
    return make_i_and(heap, l, r);
  }
  case AST_TT_I_OR: {
    lizard_ast_node_t *l = subst_interval(t->data.tt_i_binop.left, x, v, heap);
    lizard_ast_node_t *r = subst_interval(t->data.tt_i_binop.right, x, v, heap);
    if (l == t->data.tt_i_binop.left && r == t->data.tt_i_binop.right) return t;
    return make_i_or(heap, l, r);
  }
  case AST_TT_I_NEG: {
    lizard_ast_node_t *o = subst_interval(t->data.tt_i_neg.operand, x, v, heap);
    if (o == t->data.tt_i_neg.operand) return t;
    return make_i_neg(heap, o);
  }
  case AST_TT_PATH: {
    lizard_ast_node_t *A = subst_interval(t->data.tt_path.domain, x, v, heap);
    lizard_ast_node_t *pa = subst_interval(t->data.tt_path.a, x, v, heap);
    lizard_ast_node_t *pb = subst_interval(t->data.tt_path.b, x, v, heap);
    if (A == t->data.tt_path.domain && pa == t->data.tt_path.a &&
        pb == t->data.tt_path.b) return t;
    return make_path(heap, A, pa, pb);
  }
  case AST_TT_PATH_ABS:
    /* Shadow check: if this path-abs binds the same interval var, stop. */
    if (t->data.tt_path_abs.binder &&
        t->data.tt_path_abs.binder->type == AST_SYMBOL &&
        strcmp(t->data.tt_path_abs.binder->data.variable, x) == 0) {
      return t;
    }
    {
      lizard_ast_node_t *body = subst_interval(t->data.tt_path_abs.body, x, v, heap);
      if (body == t->data.tt_path_abs.body) return t;
      return make_path_abs(heap, t->data.tt_path_abs.binder, body);
    }
  case AST_TT_PATH_APP: {
    lizard_ast_node_t *p = subst_interval(t->data.tt_path_app.path, x, v, heap);
    lizard_ast_node_t *i = subst_interval(t->data.tt_path_app.point, x, v, heap);
    if (p == t->data.tt_path_app.path && i == t->data.tt_path_app.point) return t;
    return make_path_app(heap, p, i);
  }
  /* Faces and partials: interval-subst penetrates into them. */
  case AST_TT_F0:
  case AST_TT_F1:
    return t;
  case AST_TT_F_EQ: {
    lizard_ast_node_t *l = subst_interval(t->data.tt_f_eq.left, x, v, heap);
    lizard_ast_node_t *r = subst_interval(t->data.tt_f_eq.right, x, v, heap);
    if (l == t->data.tt_f_eq.left && r == t->data.tt_f_eq.right) return t;
    return make_f_eq(heap, l, r);
  }
  case AST_TT_F_AND: {
    lizard_ast_node_t *l = subst_interval(t->data.tt_f_binop.left, x, v, heap);
    lizard_ast_node_t *r = subst_interval(t->data.tt_f_binop.right, x, v, heap);
    if (l == t->data.tt_f_binop.left && r == t->data.tt_f_binop.right) return t;
    return make_f_and(heap, l, r);
  }
  case AST_TT_F_OR: {
    lizard_ast_node_t *l = subst_interval(t->data.tt_f_binop.left, x, v, heap);
    lizard_ast_node_t *r = subst_interval(t->data.tt_f_binop.right, x, v, heap);
    if (l == t->data.tt_f_binop.left && r == t->data.tt_f_binop.right) return t;
    return make_f_or(heap, l, r);
  }
  case AST_TT_PARTIAL: {
    lizard_ast_node_t *f = subst_interval(t->data.tt_partial.face, x, v, heap);
    lizard_ast_node_t *T = subst_interval(t->data.tt_partial.type, x, v, heap);
    if (f == t->data.tt_partial.face && T == t->data.tt_partial.type) return t;
    return make_partial(heap, f, T);
  }
  case AST_TT_SUB: {
    lizard_ast_node_t *T = subst_interval(t->data.tt_sub.type, x, v, heap);
    lizard_ast_node_t *f = subst_interval(t->data.tt_sub.face, x, v, heap);
    lizard_ast_node_t *p = subst_interval(t->data.tt_sub.partial, x, v, heap);
    if (T == t->data.tt_sub.type && f == t->data.tt_sub.face &&
        p == t->data.tt_sub.partial) return t;
    return make_sub(heap, T, f, p);
  }
  case AST_TT_COMP:
  case AST_TT_HCOMP:
  case AST_TT_FILL: {
    lizard_ast_node_t *tf = subst_interval(t->data.tt_comp.type_family, x, v, heap);
    lizard_ast_node_t *f  = subst_interval(t->data.tt_comp.face, x, v, heap);
    lizard_ast_node_t *p  = subst_interval(t->data.tt_comp.partial, x, v, heap);
    lizard_ast_node_t *b  = subst_interval(t->data.tt_comp.base, x, v, heap);
    if (tf == t->data.tt_comp.type_family && f == t->data.tt_comp.face &&
        p == t->data.tt_comp.partial && b == t->data.tt_comp.base) return t;
    return make_comp(heap, t->type, tf, f, p, b);
  }
  case AST_TT_EQUIV_TYPE: {
    lizard_ast_node_t *A = subst_interval(t->data.tt_equiv_type.domain, x, v, heap);
    lizard_ast_node_t *B = subst_interval(t->data.tt_equiv_type.codomain, x, v, heap);
    if (A == t->data.tt_equiv_type.domain && B == t->data.tt_equiv_type.codomain) return t;
    return make_equiv_type(heap, A, B);
  }
  case AST_TT_ID_EQUIV: {
    lizard_ast_node_t *o = subst_interval(t->data.tt_equiv_op.operand, x, v, heap);
    if (o == t->data.tt_equiv_op.operand) return t;
    return make_id_equiv(heap, o);
  }
  case AST_TT_EQUIV_FUN: {
    lizard_ast_node_t *o = subst_interval(t->data.tt_equiv_op.operand, x, v, heap);
    if (o == t->data.tt_equiv_op.operand) return t;
    return make_equiv_fun(heap, o);
  }
  case AST_TT_EQUIV_INV: {
    lizard_ast_node_t *o = subst_interval(t->data.tt_equiv_op.operand, x, v, heap);
    if (o == t->data.tt_equiv_op.operand) return t;
    return make_equiv_inv(heap, o);
  }
  case AST_TT_GLUE: {
    lizard_ast_node_t *A = subst_interval(t->data.tt_glue.base, x, v, heap);
    lizard_ast_node_t *f = subst_interval(t->data.tt_glue.face, x, v, heap);
    lizard_ast_node_t *T = subst_interval(t->data.tt_glue.t, x, v, heap);
    lizard_ast_node_t *e = subst_interval(t->data.tt_glue.equiv, x, v, heap);
    if (A == t->data.tt_glue.base && f == t->data.tt_glue.face &&
        T == t->data.tt_glue.t && e == t->data.tt_glue.equiv) return t;
    return make_glue(heap, A, f, T, e);
  }
  case AST_TT_GLUE_INTRO: {
    lizard_ast_node_t *f = subst_interval(t->data.tt_glue_intro.face, x, v, heap);
    lizard_ast_node_t *tval = subst_interval(t->data.tt_glue_intro.t, x, v, heap);
    lizard_ast_node_t *aval = subst_interval(t->data.tt_glue_intro.a, x, v, heap);
    if (f == t->data.tt_glue_intro.face && tval == t->data.tt_glue_intro.t &&
        aval == t->data.tt_glue_intro.a) return t;
    return make_glue_intro(heap, f, tval, aval);
  }
  case AST_TT_UNGLUE: {
    lizard_ast_node_t *e = subst_interval(t->data.tt_unglue.equiv, x, v, heap);
    lizard_ast_node_t *g = subst_interval(t->data.tt_unglue.target, x, v, heap);
    if (e == t->data.tt_unglue.equiv && g == t->data.tt_unglue.target) return t;
    return make_unglue(heap, e, g);
  }
  case AST_TT_UA: {
    lizard_ast_node_t *e = subst_interval(t->data.tt_ua.equiv, x, v, heap);
    if (e == t->data.tt_ua.equiv) return t;
    return make_ua(heap, e);
  }
  case AST_TT_SYSTEM_NIL:
    return t;
  case AST_TT_SYSTEM_CONS: {
    lizard_ast_node_t *f = subst_interval(t->data.tt_system_cons.face, x, v, heap);
    lizard_ast_node_t *val = subst_interval(t->data.tt_system_cons.value, x, v, heap);
    lizard_ast_node_t *nx = subst_interval(t->data.tt_system_cons.next, x, v, heap);
    if (f == t->data.tt_system_cons.face && val == t->data.tt_system_cons.value &&
        nx == t->data.tt_system_cons.next) return t;
    return make_system_cons(heap, f, val, nx);
  }
  case AST_TT_LAMBDA: {
    lizard_ast_node_t *body = subst_interval(t->data.tt_lambda.body, x, v, heap);
    if (body == t->data.tt_lambda.body) return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_LAMBDA;
      n->data.tt_lambda.binder = t->data.tt_lambda.binder;
      n->data.tt_lambda.body = body;
      return n;
    }
  }
  case AST_TT_APP: {
    lizard_ast_node_t *f = subst_interval(t->data.tt_app.fun, x, v, heap);
    lizard_ast_node_t *a = subst_interval(t->data.tt_app.arg, x, v, heap);
    if (f == t->data.tt_app.fun && a == t->data.tt_app.arg) return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_APP;
      n->data.tt_app.fun = f;
      n->data.tt_app.arg = a;
      return n;
    }
  }
  case AST_TT_PI: {
    lizard_ast_node_t *dom = subst_interval(t->data.tt_pi.domain, x, v, heap);
    lizard_ast_node_t *cod = subst_interval(t->data.tt_pi.codomain, x, v, heap);
    if (dom == t->data.tt_pi.domain && cod == t->data.tt_pi.codomain) return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_PI;
      n->data.tt_pi.binder = t->data.tt_pi.binder;
      n->data.tt_pi.domain = dom;
      n->data.tt_pi.codomain = cod;
      return n;
    }
  }
  case AST_TT_PI_FRESH: {
    lizard_ast_node_t *dom = subst_interval(t->data.tt_pi_fresh.domain, x, v, heap);
    lizard_ast_node_t *cod = subst_interval(t->data.tt_pi_fresh.codomain, x, v, heap);
    if (dom == t->data.tt_pi_fresh.domain && cod == t->data.tt_pi_fresh.codomain) return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_PI_FRESH;
      n->data.tt_pi_fresh.binder = t->data.tt_pi_fresh.binder;
      n->data.tt_pi_fresh.domain = dom;
      n->data.tt_pi_fresh.codomain = cod;
      return n;
    }
  }
  case AST_TT_SIGMA_FRESH: {
    lizard_ast_node_t *dom = subst_interval(t->data.tt_sigma_fresh.domain, x, v, heap);
    lizard_ast_node_t *cod = subst_interval(t->data.tt_sigma_fresh.codomain, x, v, heap);
    if (dom == t->data.tt_sigma_fresh.domain && cod == t->data.tt_sigma_fresh.codomain) return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_SIGMA_FRESH;
      n->data.tt_sigma_fresh.binder = t->data.tt_sigma_fresh.binder;
      n->data.tt_sigma_fresh.domain = dom;
      n->data.tt_sigma_fresh.codomain = cod;
      return n;
    }
  }
  case AST_TT_CO_PI_FRESH: {
    lizard_ast_node_t *dom = subst_interval(t->data.tt_co_pi_fresh.domain, x, v, heap);
    lizard_ast_node_t *cod = subst_interval(t->data.tt_co_pi_fresh.codomain, x, v, heap);
    if (dom == t->data.tt_co_pi_fresh.domain && cod == t->data.tt_co_pi_fresh.codomain) return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_CO_PI_FRESH;
      n->data.tt_co_pi_fresh.binder = t->data.tt_co_pi_fresh.binder;
      n->data.tt_co_pi_fresh.domain = dom;
      n->data.tt_co_pi_fresh.codomain = cod;
      return n;
    }
  }
  case AST_TT_CO_SIGMA_FRESH: {
    lizard_ast_node_t *dom = subst_interval(t->data.tt_co_sigma_fresh.domain, x, v, heap);
    lizard_ast_node_t *cod = subst_interval(t->data.tt_co_sigma_fresh.codomain, x, v, heap);
    if (dom == t->data.tt_co_sigma_fresh.domain && cod == t->data.tt_co_sigma_fresh.codomain) return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_CO_SIGMA_FRESH;
      n->data.tt_co_sigma_fresh.binder = t->data.tt_co_sigma_fresh.binder;
      n->data.tt_co_sigma_fresh.domain = dom;
      n->data.tt_co_sigma_fresh.codomain = cod;
      return n;
    }
  }
  case AST_TT_BOX: {
    lizard_ast_node_t *arg = subst_interval(t->data.tt_box.argument, x, v, heap);
    if (arg == t->data.tt_box.argument) return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_BOX;
      n->data.tt_box.argument = arg;
      return n;
    }
  }
  case AST_TT_DIAMOND: {
    lizard_ast_node_t *arg = subst_interval(t->data.tt_diamond.argument, x, v, heap);
    if (arg == t->data.tt_diamond.argument) return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_DIAMOND;
      n->data.tt_diamond.argument = arg;
      return n;
    }
  }
  case AST_TT_BOX_INTRO: {
    lizard_ast_node_t *body = subst_interval(t->data.tt_box_intro.body, x, v, heap);
    if (body == t->data.tt_box_intro.body) return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_BOX_INTRO;
      n->data.tt_box_intro.body = body;
      return n;
    }
  }
  case AST_TT_BOX_ELIM: {
    lizard_ast_node_t *scrut = subst_interval(t->data.tt_box_elim.scrutinee, x, v, heap);
    lizard_ast_node_t *body = subst_interval(t->data.tt_box_elim.body, x, v, heap);
    if (scrut == t->data.tt_box_elim.scrutinee &&
        body == t->data.tt_box_elim.body) return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_BOX_ELIM;
      n->data.tt_box_elim.binder = t->data.tt_box_elim.binder;
      n->data.tt_box_elim.scrutinee = scrut;
      n->data.tt_box_elim.body = body;
      return n;
    }
  }
  /* Phase H.1: HIT structures may carry interval-dependent subterms
   * in path endpoints and HIT_APP arguments. Recurse and rebuild. */
  case AST_TT_HIT_REF:
    return t;
  case AST_TT_HIT_PATH: {
    lizard_ast_node_t *src = subst_interval(t->data.tt_hit_path.source, x, v, heap);
    lizard_ast_node_t *tgt = subst_interval(t->data.tt_hit_path.target, x, v, heap);
    if (src == t->data.tt_hit_path.source && tgt == t->data.tt_hit_path.target) return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_HIT_PATH;
      n->data.tt_hit_path.name = t->data.tt_hit_path.name;
      n->data.tt_hit_path.source = src;
      n->data.tt_hit_path.target = tgt;
      return n;
    }
  }
  case AST_TT_HIT_APP: {
    lz_list_node_t *p;
    lz_list_t *new_args;
    int changed = 0;
    new_args = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
    for (p = t->data.tt_hit_app.args->head;
         p != t->data.tt_hit_app.args->nil; p = p->next) {
      lizard_ast_node_t *old = ((lizard_ast_list_node_t *)p)->ast;
      lizard_ast_node_t *new_ast = subst_interval(old, x, v, heap);
      lizard_ast_list_node_t *cell = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
      cell->ast = new_ast;
      if (new_ast != old) changed = 1;
      list_append(new_args, &cell->node);
    }
    if (!changed) return t;
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_HIT_APP;
      n->data.tt_hit_app.name = t->data.tt_hit_app.name;
      n->data.tt_hit_app.args = new_args;
      return n;
    }
  }
  default:
    return t;
  }
}

/* ----- Alpha-equivalence-aware structural equality ----------------- *
 *
 * Two terms are alpha-equivalent if they're structurally equal under
 * a consistent renaming of bound variables. (Pi 'x A B) and
 * (Pi 'y A B[x->y]) are equal under alpha. The implementation:
 *
 *   - Maintain two parallel binder environments, one per term.
 *   - At each AST_SYMBOL, look up the name in the environment:
 *     - If found at depth d in both, equal iff depths match.
 *     - If found in one but not the other, not equal.
 *     - If found in neither, compare names (it's a free variable).
 *   - At each binder (Pi/Sigma), push the binder names onto the two
 *     environments and recurse.
 *
 * The environment is a linked list of names; depth is the position
 * from the head. Searches are O(depth) which is fine for practical
 * binder nesting (rarely > 10).
 */

typedef struct binder_env {
  const char *name;
  struct binder_env *next;
} binder_env_t;

/* Look up name in env. Returns 0 if not found, else 1-based depth
 * (counted from innermost binder). */
static int binder_lookup(binder_env_t *env, const char *name) {
  int depth = 1;
  for (; env != NULL; env = env->next, depth++) {
    if (env->name && name && strcmp(env->name, name) == 0) return depth;
  }
  return 0;
}

static int alpha_equal_rec(lizard_ast_node_t *a, lizard_ast_node_t *b,
                           binder_env_t *ea, binder_env_t *eb);

static int alpha_equal_symbol(lizard_ast_node_t *a, lizard_ast_node_t *b,
                              binder_env_t *ea, binder_env_t *eb) {
  int da, db;
  if (a->type != AST_SYMBOL || b->type != AST_SYMBOL) return 0;
  da = binder_lookup(ea, a->data.variable);
  db = binder_lookup(eb, b->data.variable);
  if (da != 0 || db != 0) {
    /* At least one side has the name bound. They must match in
     * binding status and depth. */
    return da == db;
  }
  /* Both are free; compare names. */
  return strcmp(a->data.variable, b->data.variable) == 0;
}

static int alpha_equal_rec(lizard_ast_node_t *a, lizard_ast_node_t *b,
                           binder_env_t *ea, binder_env_t *eb) {
  if (a == b && ea == NULL && eb == NULL) return 1;
  if (a == NULL || b == NULL) return 0;
  if (a->type != b->type) return 0;
  switch (a->type) {
  case AST_NUMBER:
    return mpz_cmp(a->data.number, b->data.number) == 0;
  case AST_STRING:
    return strcmp(a->data.string, b->data.string) == 0;
  case AST_SYMBOL:
    return alpha_equal_symbol(a, b, ea, eb);
  case AST_BOOL:
    return a->data.boolean == b->data.boolean;
  case AST_NIL:
    return 1;
  case AST_PAIR:
    return alpha_equal_rec(a->data.pair.car, b->data.pair.car, ea, eb) &&
           alpha_equal_rec(a->data.pair.cdr, b->data.pair.cdr, ea, eb);
  case AST_TT_PI: {
    binder_env_t na, nb;
    if (!alpha_equal_rec(a->data.tt_pi.domain, b->data.tt_pi.domain, ea, eb))
      return 0;
    na.name = (a->data.tt_pi.binder && a->data.tt_pi.binder->type == AST_SYMBOL)
                  ? a->data.tt_pi.binder->data.variable : NULL;
    na.next = ea;
    nb.name = (b->data.tt_pi.binder && b->data.tt_pi.binder->type == AST_SYMBOL)
                  ? b->data.tt_pi.binder->data.variable : NULL;
    nb.next = eb;
    return alpha_equal_rec(a->data.tt_pi.codomain, b->data.tt_pi.codomain,
                           &na, &nb);
  }
  case AST_TT_SIGMA: {
    binder_env_t na, nb;
    if (!alpha_equal_rec(a->data.tt_sigma.domain, b->data.tt_sigma.domain,
                         ea, eb))
      return 0;
    na.name = (a->data.tt_sigma.binder &&
               a->data.tt_sigma.binder->type == AST_SYMBOL)
                  ? a->data.tt_sigma.binder->data.variable : NULL;
    na.next = ea;
    nb.name = (b->data.tt_sigma.binder &&
               b->data.tt_sigma.binder->type == AST_SYMBOL)
                  ? b->data.tt_sigma.binder->data.variable : NULL;
    nb.next = eb;
    return alpha_equal_rec(a->data.tt_sigma.codomain, b->data.tt_sigma.codomain,
                           &na, &nb);
  }
  case AST_TT_PI_FRESH: {
    binder_env_t na, nb;
    if (!alpha_equal_rec(a->data.tt_pi_fresh.domain, b->data.tt_pi_fresh.domain, ea, eb))
      return 0;
    na.name = (a->data.tt_pi_fresh.binder && a->data.tt_pi_fresh.binder->type == AST_SYMBOL)
                  ? a->data.tt_pi_fresh.binder->data.variable : NULL;
    na.next = ea;
    nb.name = (b->data.tt_pi_fresh.binder && b->data.tt_pi_fresh.binder->type == AST_SYMBOL)
                  ? b->data.tt_pi_fresh.binder->data.variable : NULL;
    nb.next = eb;
    return alpha_equal_rec(a->data.tt_pi_fresh.codomain, b->data.tt_pi_fresh.codomain,
                           &na, &nb);
  }
  case AST_TT_SIGMA_FRESH: {
    binder_env_t na, nb;
    if (!alpha_equal_rec(a->data.tt_sigma_fresh.domain, b->data.tt_sigma_fresh.domain, ea, eb))
      return 0;
    na.name = (a->data.tt_sigma_fresh.binder &&
               a->data.tt_sigma_fresh.binder->type == AST_SYMBOL)
                  ? a->data.tt_sigma_fresh.binder->data.variable : NULL;
    na.next = ea;
    nb.name = (b->data.tt_sigma_fresh.binder &&
               b->data.tt_sigma_fresh.binder->type == AST_SYMBOL)
                  ? b->data.tt_sigma_fresh.binder->data.variable : NULL;
    nb.next = eb;
    return alpha_equal_rec(a->data.tt_sigma_fresh.codomain, b->data.tt_sigma_fresh.codomain,
                           &na, &nb);
  }
  case AST_TT_CO_PI_FRESH: {
    binder_env_t na, nb;
    if (!alpha_equal_rec(a->data.tt_co_pi_fresh.domain, b->data.tt_co_pi_fresh.domain, ea, eb))
      return 0;
    na.name = (a->data.tt_co_pi_fresh.binder &&
               a->data.tt_co_pi_fresh.binder->type == AST_SYMBOL)
                  ? a->data.tt_co_pi_fresh.binder->data.variable : NULL;
    na.next = ea;
    nb.name = (b->data.tt_co_pi_fresh.binder &&
               b->data.tt_co_pi_fresh.binder->type == AST_SYMBOL)
                  ? b->data.tt_co_pi_fresh.binder->data.variable : NULL;
    nb.next = eb;
    return alpha_equal_rec(a->data.tt_co_pi_fresh.codomain, b->data.tt_co_pi_fresh.codomain,
                           &na, &nb);
  }
  case AST_TT_CO_SIGMA_FRESH: {
    binder_env_t na, nb;
    if (!alpha_equal_rec(a->data.tt_co_sigma_fresh.domain, b->data.tt_co_sigma_fresh.domain, ea, eb))
      return 0;
    na.name = (a->data.tt_co_sigma_fresh.binder &&
               a->data.tt_co_sigma_fresh.binder->type == AST_SYMBOL)
                  ? a->data.tt_co_sigma_fresh.binder->data.variable : NULL;
    na.next = ea;
    nb.name = (b->data.tt_co_sigma_fresh.binder &&
               b->data.tt_co_sigma_fresh.binder->type == AST_SYMBOL)
                  ? b->data.tt_co_sigma_fresh.binder->data.variable : NULL;
    nb.next = eb;
    return alpha_equal_rec(a->data.tt_co_sigma_fresh.codomain, b->data.tt_co_sigma_fresh.codomain,
                           &na, &nb);
  }
  case AST_TT_BOX:
    return alpha_equal_rec(a->data.tt_box.argument, b->data.tt_box.argument, ea, eb);
  case AST_TT_DIAMOND:
    return alpha_equal_rec(a->data.tt_diamond.argument, b->data.tt_diamond.argument, ea, eb);
  case AST_TT_BOX_INTRO:
    return alpha_equal_rec(a->data.tt_box_intro.body, b->data.tt_box_intro.body, ea, eb);
  case AST_TT_BOX_ELIM: {
    binder_env_t na, nb;
    if (!alpha_equal_rec(a->data.tt_box_elim.scrutinee,
                         b->data.tt_box_elim.scrutinee, ea, eb)) return 0;
    na.name = (a->data.tt_box_elim.binder &&
               a->data.tt_box_elim.binder->type == AST_SYMBOL)
                  ? a->data.tt_box_elim.binder->data.variable : NULL;
    na.next = ea;
    nb.name = (b->data.tt_box_elim.binder &&
               b->data.tt_box_elim.binder->type == AST_SYMBOL)
                  ? b->data.tt_box_elim.binder->data.variable : NULL;
    nb.next = eb;
    return alpha_equal_rec(a->data.tt_box_elim.body, b->data.tt_box_elim.body,
                           &na, &nb);
  }
  case AST_TT_APP:
    return alpha_equal_rec(a->data.tt_app.fun, b->data.tt_app.fun, ea, eb) &&
           alpha_equal_rec(a->data.tt_app.arg, b->data.tt_app.arg, ea, eb);
  case AST_TT_SUM:
    return alpha_equal_rec(a->data.tt_sum.left, b->data.tt_sum.left, ea, eb) &&
           alpha_equal_rec(a->data.tt_sum.right, b->data.tt_sum.right, ea, eb);
  case AST_TT_UNIVERSE:
    return a->data.tt_universe.level == b->data.tt_universe.level;
  case AST_TT_COUNIVERSE:
    return a->data.tt_couniverse.level == b->data.tt_couniverse.level;
  case AST_TT_UNIVERSE_SET: {
    long i;
    if (a->data.tt_universe_set.count != b->data.tt_universe_set.count)
      return 0;
    for (i = 0; i < a->data.tt_universe_set.count; i++) {
      if (a->data.tt_universe_set.dims[i] != b->data.tt_universe_set.dims[i])
        return 0;
    }
    return 1;
  }
  case AST_TT_COUNIVERSE_SET: {
    long i;
    if (a->data.tt_couniverse_set.count != b->data.tt_couniverse_set.count)
      return 0;
    for (i = 0; i < a->data.tt_couniverse_set.count; i++) {
      if (a->data.tt_couniverse_set.dims[i] != b->data.tt_couniverse_set.dims[i])
        return 0;
    }
    return 1;
  }
  case AST_TT_CO_MAX:
    return alpha_equal_rec(a->data.tt_co_max.left,
                           b->data.tt_co_max.left, ea, eb) &&
           alpha_equal_rec(a->data.tt_co_max.right,
                           b->data.tt_co_max.right, ea, eb);
  case AST_TT_CO_MIN:
    return alpha_equal_rec(a->data.tt_co_min.left,
                           b->data.tt_co_min.left, ea, eb) &&
           alpha_equal_rec(a->data.tt_co_min.right,
                           b->data.tt_co_min.right, ea, eb);
  case AST_TT_ID:
    return alpha_equal_rec(a->data.tt_id.domain, b->data.tt_id.domain, ea, eb) &&
           alpha_equal_rec(a->data.tt_id.a, b->data.tt_id.a, ea, eb) &&
           alpha_equal_rec(a->data.tt_id.b, b->data.tt_id.b, ea, eb);
  case AST_TT_REFL:
    return alpha_equal_rec(a->data.tt_refl.value, b->data.tt_refl.value, ea, eb);
  case AST_TT_ID_SYM:
    return alpha_equal_rec(a->data.tt_id_sym.path, b->data.tt_id_sym.path,
                           ea, eb);
  case AST_TT_ID_TRANS:
    return alpha_equal_rec(a->data.tt_id_trans.p, b->data.tt_id_trans.p, ea, eb) &&
           alpha_equal_rec(a->data.tt_id_trans.q, b->data.tt_id_trans.q, ea, eb);
  case AST_TT_TRANSPORT:
    return alpha_equal_rec(a->data.tt_transport.path,
                           b->data.tt_transport.path, ea, eb) &&
           alpha_equal_rec(a->data.tt_transport.value,
                           b->data.tt_transport.value, ea, eb);
  case AST_TT_EQUIV:
    return alpha_equal_rec(a->data.tt_equiv.left, b->data.tt_equiv.left, ea, eb) &&
           alpha_equal_rec(a->data.tt_equiv.right, b->data.tt_equiv.right, ea, eb) &&
           alpha_equal_rec(a->data.tt_equiv.fwd, b->data.tt_equiv.fwd, ea, eb) &&
           alpha_equal_rec(a->data.tt_equiv.bwd, b->data.tt_equiv.bwd, ea, eb);
  case AST_TT_LAMBDA: {
    binder_env_t na, nb;
    na.name = (a->data.tt_lambda.binder &&
               a->data.tt_lambda.binder->type == AST_SYMBOL)
                  ? a->data.tt_lambda.binder->data.variable : NULL;
    na.next = ea;
    nb.name = (b->data.tt_lambda.binder &&
               b->data.tt_lambda.binder->type == AST_SYMBOL)
                  ? b->data.tt_lambda.binder->data.variable : NULL;
    nb.next = eb;
    return alpha_equal_rec(a->data.tt_lambda.body, b->data.tt_lambda.body,
                           &na, &nb);
  }
  case AST_TT_AP:
    return alpha_equal_rec(a->data.tt_ap.fn, b->data.tt_ap.fn, ea, eb) &&
           alpha_equal_rec(a->data.tt_ap.path, b->data.tt_ap.path, ea, eb);
  case AST_TT_PAIR:
    return alpha_equal_rec(a->data.tt_pair.fst, b->data.tt_pair.fst, ea, eb) &&
           alpha_equal_rec(a->data.tt_pair.snd, b->data.tt_pair.snd, ea, eb);
  case AST_TT_FST:
  case AST_TT_SND:
    return alpha_equal_rec(a->data.tt_proj.target,
                           b->data.tt_proj.target, ea, eb);
  case AST_TT_INL:
  case AST_TT_INR:
    return alpha_equal_rec(a->data.tt_inj.value,
                           b->data.tt_inj.value, ea, eb);
  case AST_TT_CASE:
    return alpha_equal_rec(a->data.tt_case.scrutinee,
                           b->data.tt_case.scrutinee, ea, eb) &&
           alpha_equal_rec(a->data.tt_case.left_branch,
                           b->data.tt_case.left_branch, ea, eb) &&
           alpha_equal_rec(a->data.tt_case.right_branch,
                           b->data.tt_case.right_branch, ea, eb);
  case AST_TT_J:
    return alpha_equal_rec(a->data.tt_j.motive, b->data.tt_j.motive, ea, eb) &&
           alpha_equal_rec(a->data.tt_j.refl_case, b->data.tt_j.refl_case, ea, eb) &&
           alpha_equal_rec(a->data.tt_j.path, b->data.tt_j.path, ea, eb);
  case AST_TT_XPORT:
    return alpha_equal_rec(a->data.tt_xport.motive,
                           b->data.tt_xport.motive, ea, eb) &&
           alpha_equal_rec(a->data.tt_xport.path,
                           b->data.tt_xport.path, ea, eb) &&
           alpha_equal_rec(a->data.tt_xport.value,
                           b->data.tt_xport.value, ea, eb);
  case AST_TT_U_VAR:
    return strcmp(a->data.tt_u_var.name, b->data.tt_u_var.name) == 0;
  case AST_TT_U_SUC:
    return alpha_equal_rec(a->data.tt_u_suc.operand,
                           b->data.tt_u_suc.operand, ea, eb);
  case AST_TT_U_MAX:
    return alpha_equal_rec(a->data.tt_u_max.left,
                           b->data.tt_u_max.left, ea, eb) &&
           alpha_equal_rec(a->data.tt_u_max.right,
                           b->data.tt_u_max.right, ea, eb);
  case AST_TT_U_MIN:
    return alpha_equal_rec(a->data.tt_u_min.left,
                           b->data.tt_u_min.left, ea, eb) &&
           alpha_equal_rec(a->data.tt_u_min.right,
                           b->data.tt_u_min.right, ea, eb);
  /* Cubical nodes. */
  case AST_TT_INTERVAL:
  case AST_TT_I0:
  case AST_TT_I1:
    return 1;
  case AST_TT_I_VAR:
    return strcmp(a->data.tt_i_var.name, b->data.tt_i_var.name) == 0;
  case AST_TT_I_AND:
  case AST_TT_I_OR:
    return alpha_equal_rec(a->data.tt_i_binop.left,
                           b->data.tt_i_binop.left, ea, eb) &&
           alpha_equal_rec(a->data.tt_i_binop.right,
                           b->data.tt_i_binop.right, ea, eb);
  case AST_TT_I_NEG:
    return alpha_equal_rec(a->data.tt_i_neg.operand,
                           b->data.tt_i_neg.operand, ea, eb);
  case AST_TT_PATH:
    return alpha_equal_rec(a->data.tt_path.domain,
                           b->data.tt_path.domain, ea, eb) &&
           alpha_equal_rec(a->data.tt_path.a, b->data.tt_path.a, ea, eb) &&
           alpha_equal_rec(a->data.tt_path.b, b->data.tt_path.b, ea, eb);
  case AST_TT_PATH_ABS:
    /* Conservative alpha-equality on path abstractions: require same
     * binder name. This gives false negatives (two abstractions with
     * different binder names but equal modulo renaming compare unequal)
     * but no false positives, so it's sound. Full interval-level
     * alpha-equivalence requires extending the binder-env machinery
     * to track interval variables separately; deferred. */
    {
      const char *na = (a->data.tt_path_abs.binder &&
                        a->data.tt_path_abs.binder->type == AST_SYMBOL)
                       ? a->data.tt_path_abs.binder->data.variable : NULL;
      const char *nb = (b->data.tt_path_abs.binder &&
                        b->data.tt_path_abs.binder->type == AST_SYMBOL)
                       ? b->data.tt_path_abs.binder->data.variable : NULL;
      if (na && nb && strcmp(na, nb) == 0) {
        return alpha_equal_rec(a->data.tt_path_abs.body,
                               b->data.tt_path_abs.body, ea, eb);
      }
      return 0;
    }
  case AST_TT_PATH_APP:
    return alpha_equal_rec(a->data.tt_path_app.path,
                           b->data.tt_path_app.path, ea, eb) &&
           alpha_equal_rec(a->data.tt_path_app.point,
                           b->data.tt_path_app.point, ea, eb);
  case AST_TT_F0:
  case AST_TT_F1:
    return 1;
  case AST_TT_F_EQ:
    return alpha_equal_rec(a->data.tt_f_eq.left, b->data.tt_f_eq.left, ea, eb) &&
           alpha_equal_rec(a->data.tt_f_eq.right, b->data.tt_f_eq.right, ea, eb);
  case AST_TT_F_AND:
  case AST_TT_F_OR:
    return alpha_equal_rec(a->data.tt_f_binop.left,
                           b->data.tt_f_binop.left, ea, eb) &&
           alpha_equal_rec(a->data.tt_f_binop.right,
                           b->data.tt_f_binop.right, ea, eb);
  case AST_TT_PARTIAL:
    return alpha_equal_rec(a->data.tt_partial.face, b->data.tt_partial.face, ea, eb) &&
           alpha_equal_rec(a->data.tt_partial.type, b->data.tt_partial.type, ea, eb);
  case AST_TT_SUB:
    return alpha_equal_rec(a->data.tt_sub.type, b->data.tt_sub.type, ea, eb) &&
           alpha_equal_rec(a->data.tt_sub.face, b->data.tt_sub.face, ea, eb) &&
           alpha_equal_rec(a->data.tt_sub.partial, b->data.tt_sub.partial, ea, eb);
  case AST_TT_COMP:
  case AST_TT_HCOMP:
  case AST_TT_FILL:
    return alpha_equal_rec(a->data.tt_comp.type_family,
                           b->data.tt_comp.type_family, ea, eb) &&
           alpha_equal_rec(a->data.tt_comp.face, b->data.tt_comp.face, ea, eb) &&
           alpha_equal_rec(a->data.tt_comp.partial, b->data.tt_comp.partial, ea, eb) &&
           alpha_equal_rec(a->data.tt_comp.base, b->data.tt_comp.base, ea, eb);
  case AST_TT_EQUIV_TYPE:
    return alpha_equal_rec(a->data.tt_equiv_type.domain,
                           b->data.tt_equiv_type.domain, ea, eb) &&
           alpha_equal_rec(a->data.tt_equiv_type.codomain,
                           b->data.tt_equiv_type.codomain, ea, eb);
  case AST_TT_ID_EQUIV:
  case AST_TT_EQUIV_FUN:
  case AST_TT_EQUIV_INV:
    return alpha_equal_rec(a->data.tt_equiv_op.operand,
                           b->data.tt_equiv_op.operand, ea, eb);
  case AST_TT_GLUE:
    return alpha_equal_rec(a->data.tt_glue.base, b->data.tt_glue.base, ea, eb) &&
           alpha_equal_rec(a->data.tt_glue.face, b->data.tt_glue.face, ea, eb) &&
           alpha_equal_rec(a->data.tt_glue.t, b->data.tt_glue.t, ea, eb) &&
           alpha_equal_rec(a->data.tt_glue.equiv, b->data.tt_glue.equiv, ea, eb);
  case AST_TT_GLUE_INTRO:
    return alpha_equal_rec(a->data.tt_glue_intro.face,
                           b->data.tt_glue_intro.face, ea, eb) &&
           alpha_equal_rec(a->data.tt_glue_intro.t, b->data.tt_glue_intro.t, ea, eb) &&
           alpha_equal_rec(a->data.tt_glue_intro.a, b->data.tt_glue_intro.a, ea, eb);
  case AST_TT_UNGLUE:
    return alpha_equal_rec(a->data.tt_unglue.equiv,
                           b->data.tt_unglue.equiv, ea, eb) &&
           alpha_equal_rec(a->data.tt_unglue.target,
                           b->data.tt_unglue.target, ea, eb);
  case AST_TT_UA:
    return alpha_equal_rec(a->data.tt_ua.equiv, b->data.tt_ua.equiv, ea, eb);
  case AST_TT_SYSTEM_NIL:
    return 1;
  case AST_TT_SYSTEM_CONS:
    return alpha_equal_rec(a->data.tt_system_cons.face,
                           b->data.tt_system_cons.face, ea, eb) &&
           alpha_equal_rec(a->data.tt_system_cons.value,
                           b->data.tt_system_cons.value, ea, eb) &&
           alpha_equal_rec(a->data.tt_system_cons.next,
                           b->data.tt_system_cons.next, ea, eb);
  case AST_TT_UNIT:
  case AST_TT_TT:
  case AST_TT_BOT:
    return 1;       /* nullary — same type means equal */
  /* Phase H.1: HIT structural equality. */
  case AST_TT_HIT_REF:
    return alpha_equal_rec(a->data.tt_hit_ref.name,
                           b->data.tt_hit_ref.name, ea, eb);
  case AST_TT_HIT_DECL: {
    lz_list_node_t *pa, *pb;
    if (!alpha_equal_rec(a->data.tt_hit_decl.name,
                         b->data.tt_hit_decl.name, ea, eb)) return 0;
    pa = a->data.tt_hit_decl.constructors->head;
    pb = b->data.tt_hit_decl.constructors->head;
    while (pa != a->data.tt_hit_decl.constructors->nil &&
           pb != b->data.tt_hit_decl.constructors->nil) {
      if (!alpha_equal_rec(((lizard_ast_list_node_t *)pa)->ast,
                           ((lizard_ast_list_node_t *)pb)->ast, ea, eb)) return 0;
      pa = pa->next; pb = pb->next;
    }
    if (pa != a->data.tt_hit_decl.constructors->nil ||
        pb != b->data.tt_hit_decl.constructors->nil) return 0;
    pa = a->data.tt_hit_decl.paths->head;
    pb = b->data.tt_hit_decl.paths->head;
    while (pa != a->data.tt_hit_decl.paths->nil &&
           pb != b->data.tt_hit_decl.paths->nil) {
      if (!alpha_equal_rec(((lizard_ast_list_node_t *)pa)->ast,
                           ((lizard_ast_list_node_t *)pb)->ast, ea, eb)) return 0;
      pa = pa->next; pb = pb->next;
    }
    return pa == a->data.tt_hit_decl.paths->nil &&
           pb == b->data.tt_hit_decl.paths->nil;
  }
  case AST_TT_HIT_CONSTRUCTOR: {
    lz_list_node_t *pa, *pb;
    if (!alpha_equal_rec(a->data.tt_hit_constructor.name,
                         b->data.tt_hit_constructor.name, ea, eb)) return 0;
    pa = a->data.tt_hit_constructor.arg_types->head;
    pb = b->data.tt_hit_constructor.arg_types->head;
    while (pa != a->data.tt_hit_constructor.arg_types->nil &&
           pb != b->data.tt_hit_constructor.arg_types->nil) {
      if (!alpha_equal_rec(((lizard_ast_list_node_t *)pa)->ast,
                           ((lizard_ast_list_node_t *)pb)->ast, ea, eb)) return 0;
      pa = pa->next; pb = pb->next;
    }
    return pa == a->data.tt_hit_constructor.arg_types->nil &&
           pb == b->data.tt_hit_constructor.arg_types->nil;
  }
  case AST_TT_HIT_PATH:
    return alpha_equal_rec(a->data.tt_hit_path.name,
                           b->data.tt_hit_path.name, ea, eb) &&
           alpha_equal_rec(a->data.tt_hit_path.source,
                           b->data.tt_hit_path.source, ea, eb) &&
           alpha_equal_rec(a->data.tt_hit_path.target,
                           b->data.tt_hit_path.target, ea, eb);
  case AST_TT_HIT_APP: {
    lz_list_node_t *pa, *pb;
    if (!alpha_equal_rec(a->data.tt_hit_app.name,
                         b->data.tt_hit_app.name, ea, eb)) return 0;
    pa = a->data.tt_hit_app.args->head;
    pb = b->data.tt_hit_app.args->head;
    while (pa != a->data.tt_hit_app.args->nil &&
           pb != b->data.tt_hit_app.args->nil) {
      if (!alpha_equal_rec(((lizard_ast_list_node_t *)pa)->ast,
                           ((lizard_ast_list_node_t *)pb)->ast, ea, eb)) return 0;
      pa = pa->next; pb = pb->next;
    }
    return pa == a->data.tt_hit_app.args->nil &&
           pb == b->data.tt_hit_app.args->nil;
  }
  default:
    return 0;
  }
}

int lizard_tt_alpha_equal(lizard_ast_node_t *a, lizard_ast_node_t *b) {
  return alpha_equal_rec(a, b, NULL, NULL);
}

/* ----- Structural equality (no alpha — preserved for callers that
 * want pointer-/name-strict comparison) ------------------------------ */
int lizard_tt_structurally_equal(lizard_ast_node_t *a, lizard_ast_node_t *b) {
  if (a == b) return 1;
  if (a == NULL || b == NULL) return 0;
  if (a->type != b->type) return 0;
  switch (a->type) {
  case AST_NUMBER:
    return mpz_cmp(a->data.number, b->data.number) == 0;
  case AST_STRING:
    return strcmp(a->data.string, b->data.string) == 0;
  case AST_SYMBOL:
    return strcmp(a->data.variable, b->data.variable) == 0;
  case AST_BOOL:
    return a->data.boolean == b->data.boolean;
  case AST_NIL:
    return 1;
  case AST_PAIR:
    return lizard_tt_structurally_equal(a->data.pair.car, b->data.pair.car) &&
           lizard_tt_structurally_equal(a->data.pair.cdr, b->data.pair.cdr);
  case AST_TT_PI:
    return lizard_tt_structurally_equal(a->data.tt_pi.binder, b->data.tt_pi.binder) &&
           lizard_tt_structurally_equal(a->data.tt_pi.domain, b->data.tt_pi.domain) &&
           lizard_tt_structurally_equal(a->data.tt_pi.codomain, b->data.tt_pi.codomain);
  case AST_TT_SIGMA:
    return lizard_tt_structurally_equal(a->data.tt_sigma.binder, b->data.tt_sigma.binder) &&
           lizard_tt_structurally_equal(a->data.tt_sigma.domain, b->data.tt_sigma.domain) &&
           lizard_tt_structurally_equal(a->data.tt_sigma.codomain, b->data.tt_sigma.codomain);
  case AST_TT_PI_FRESH:
    return lizard_tt_structurally_equal(a->data.tt_pi_fresh.binder, b->data.tt_pi_fresh.binder) &&
           lizard_tt_structurally_equal(a->data.tt_pi_fresh.domain, b->data.tt_pi_fresh.domain) &&
           lizard_tt_structurally_equal(a->data.tt_pi_fresh.codomain, b->data.tt_pi_fresh.codomain);
  case AST_TT_SIGMA_FRESH:
    return lizard_tt_structurally_equal(a->data.tt_sigma_fresh.binder, b->data.tt_sigma_fresh.binder) &&
           lizard_tt_structurally_equal(a->data.tt_sigma_fresh.domain, b->data.tt_sigma_fresh.domain) &&
           lizard_tt_structurally_equal(a->data.tt_sigma_fresh.codomain, b->data.tt_sigma_fresh.codomain);
  case AST_TT_CO_PI_FRESH:
    return lizard_tt_structurally_equal(a->data.tt_co_pi_fresh.binder, b->data.tt_co_pi_fresh.binder) &&
           lizard_tt_structurally_equal(a->data.tt_co_pi_fresh.domain, b->data.tt_co_pi_fresh.domain) &&
           lizard_tt_structurally_equal(a->data.tt_co_pi_fresh.codomain, b->data.tt_co_pi_fresh.codomain);
  case AST_TT_CO_SIGMA_FRESH:
    return lizard_tt_structurally_equal(a->data.tt_co_sigma_fresh.binder, b->data.tt_co_sigma_fresh.binder) &&
           lizard_tt_structurally_equal(a->data.tt_co_sigma_fresh.domain, b->data.tt_co_sigma_fresh.domain) &&
           lizard_tt_structurally_equal(a->data.tt_co_sigma_fresh.codomain, b->data.tt_co_sigma_fresh.codomain);
  case AST_TT_BOX:
    return lizard_tt_structurally_equal(a->data.tt_box.argument, b->data.tt_box.argument);
  case AST_TT_DIAMOND:
    return lizard_tt_structurally_equal(a->data.tt_diamond.argument, b->data.tt_diamond.argument);
  case AST_TT_BOX_INTRO:
    return lizard_tt_structurally_equal(a->data.tt_box_intro.body, b->data.tt_box_intro.body);
  case AST_TT_BOX_ELIM:
    return lizard_tt_structurally_equal(a->data.tt_box_elim.binder, b->data.tt_box_elim.binder) &&
           lizard_tt_structurally_equal(a->data.tt_box_elim.scrutinee, b->data.tt_box_elim.scrutinee) &&
           lizard_tt_structurally_equal(a->data.tt_box_elim.body, b->data.tt_box_elim.body);
  case AST_TT_APP:
    return lizard_tt_structurally_equal(a->data.tt_app.fun, b->data.tt_app.fun) &&
           lizard_tt_structurally_equal(a->data.tt_app.arg, b->data.tt_app.arg);
  case AST_TT_SUM:
    return lizard_tt_structurally_equal(a->data.tt_sum.left, b->data.tt_sum.left) &&
           lizard_tt_structurally_equal(a->data.tt_sum.right, b->data.tt_sum.right);
  case AST_TT_UNIVERSE:
    return a->data.tt_universe.level == b->data.tt_universe.level;
  case AST_TT_COUNIVERSE:
    return a->data.tt_couniverse.level == b->data.tt_couniverse.level;
  case AST_TT_UNIVERSE_SET: {
    long i;
    if (a->data.tt_universe_set.count != b->data.tt_universe_set.count)
      return 0;
    for (i = 0; i < a->data.tt_universe_set.count; i++) {
      if (a->data.tt_universe_set.dims[i] != b->data.tt_universe_set.dims[i])
        return 0;
    }
    return 1;
  }
  case AST_TT_COUNIVERSE_SET: {
    long i;
    if (a->data.tt_couniverse_set.count != b->data.tt_couniverse_set.count)
      return 0;
    for (i = 0; i < a->data.tt_couniverse_set.count; i++) {
      if (a->data.tt_couniverse_set.dims[i] != b->data.tt_couniverse_set.dims[i])
        return 0;
    }
    return 1;
  }
  case AST_TT_CO_MAX:
    return lizard_tt_structurally_equal(a->data.tt_co_max.left,
                                        b->data.tt_co_max.left) &&
           lizard_tt_structurally_equal(a->data.tt_co_max.right,
                                        b->data.tt_co_max.right);
  case AST_TT_CO_MIN:
    return lizard_tt_structurally_equal(a->data.tt_co_min.left,
                                        b->data.tt_co_min.left) &&
           lizard_tt_structurally_equal(a->data.tt_co_min.right,
                                        b->data.tt_co_min.right);
  case AST_TT_ID:
    return lizard_tt_structurally_equal(a->data.tt_id.domain, b->data.tt_id.domain) &&
           lizard_tt_structurally_equal(a->data.tt_id.a, b->data.tt_id.a) &&
           lizard_tt_structurally_equal(a->data.tt_id.b, b->data.tt_id.b);
  case AST_TT_REFL:
    return lizard_tt_structurally_equal(a->data.tt_refl.value, b->data.tt_refl.value);
  case AST_TT_ID_SYM:
    return lizard_tt_structurally_equal(a->data.tt_id_sym.path, b->data.tt_id_sym.path);
  case AST_TT_ID_TRANS:
    return lizard_tt_structurally_equal(a->data.tt_id_trans.p, b->data.tt_id_trans.p) &&
           lizard_tt_structurally_equal(a->data.tt_id_trans.q, b->data.tt_id_trans.q);
  case AST_TT_TRANSPORT:
    return lizard_tt_structurally_equal(a->data.tt_transport.path,  b->data.tt_transport.path) &&
           lizard_tt_structurally_equal(a->data.tt_transport.value, b->data.tt_transport.value);
  case AST_TT_EQUIV:
    return lizard_tt_structurally_equal(a->data.tt_equiv.left,  b->data.tt_equiv.left) &&
           lizard_tt_structurally_equal(a->data.tt_equiv.right, b->data.tt_equiv.right) &&
           lizard_tt_structurally_equal(a->data.tt_equiv.fwd,   b->data.tt_equiv.fwd) &&
           lizard_tt_structurally_equal(a->data.tt_equiv.bwd,   b->data.tt_equiv.bwd);
  case AST_TT_AP:
    return lizard_tt_structurally_equal(a->data.tt_ap.fn, b->data.tt_ap.fn) &&
           lizard_tt_structurally_equal(a->data.tt_ap.path, b->data.tt_ap.path);
  case AST_TT_LAMBDA:
    return lizard_tt_structurally_equal(a->data.tt_lambda.binder,
                                        b->data.tt_lambda.binder) &&
           lizard_tt_structurally_equal(a->data.tt_lambda.body,
                                        b->data.tt_lambda.body);
  case AST_TT_PAIR:
    return lizard_tt_structurally_equal(a->data.tt_pair.fst, b->data.tt_pair.fst) &&
           lizard_tt_structurally_equal(a->data.tt_pair.snd, b->data.tt_pair.snd);
  case AST_TT_FST:
  case AST_TT_SND:
    return lizard_tt_structurally_equal(a->data.tt_proj.target,
                                        b->data.tt_proj.target);
  case AST_TT_INL:
  case AST_TT_INR:
    return lizard_tt_structurally_equal(a->data.tt_inj.value,
                                        b->data.tt_inj.value);
  case AST_TT_CASE:
    return lizard_tt_structurally_equal(a->data.tt_case.scrutinee,
                                        b->data.tt_case.scrutinee) &&
           lizard_tt_structurally_equal(a->data.tt_case.left_branch,
                                        b->data.tt_case.left_branch) &&
           lizard_tt_structurally_equal(a->data.tt_case.right_branch,
                                        b->data.tt_case.right_branch);
  case AST_TT_J:
    return lizard_tt_structurally_equal(a->data.tt_j.motive, b->data.tt_j.motive) &&
           lizard_tt_structurally_equal(a->data.tt_j.refl_case, b->data.tt_j.refl_case) &&
           lizard_tt_structurally_equal(a->data.tt_j.path, b->data.tt_j.path);
  case AST_TT_XPORT:
    return lizard_tt_structurally_equal(a->data.tt_xport.motive,
                                        b->data.tt_xport.motive) &&
           lizard_tt_structurally_equal(a->data.tt_xport.path,
                                        b->data.tt_xport.path) &&
           lizard_tt_structurally_equal(a->data.tt_xport.value,
                                        b->data.tt_xport.value);
  case AST_TT_U_VAR:
    return strcmp(a->data.tt_u_var.name, b->data.tt_u_var.name) == 0;
  case AST_TT_U_SUC:
    return lizard_tt_structurally_equal(a->data.tt_u_suc.operand,
                                        b->data.tt_u_suc.operand);
  case AST_TT_U_MAX:
    return lizard_tt_structurally_equal(a->data.tt_u_max.left,
                                        b->data.tt_u_max.left) &&
           lizard_tt_structurally_equal(a->data.tt_u_max.right,
                                        b->data.tt_u_max.right);
  case AST_TT_U_MIN:
    return lizard_tt_structurally_equal(a->data.tt_u_min.left,
                                        b->data.tt_u_min.left) &&
           lizard_tt_structurally_equal(a->data.tt_u_min.right,
                                        b->data.tt_u_min.right);
  case AST_TT_INTERVAL:
  case AST_TT_I0:
  case AST_TT_I1:
    return 1;
  case AST_TT_I_VAR:
    return strcmp(a->data.tt_i_var.name, b->data.tt_i_var.name) == 0;
  case AST_TT_I_AND:
  case AST_TT_I_OR:
    return lizard_tt_structurally_equal(a->data.tt_i_binop.left,
                                        b->data.tt_i_binop.left) &&
           lizard_tt_structurally_equal(a->data.tt_i_binop.right,
                                        b->data.tt_i_binop.right);
  case AST_TT_I_NEG:
    return lizard_tt_structurally_equal(a->data.tt_i_neg.operand,
                                        b->data.tt_i_neg.operand);
  case AST_TT_PATH:
    return lizard_tt_structurally_equal(a->data.tt_path.domain,
                                        b->data.tt_path.domain) &&
           lizard_tt_structurally_equal(a->data.tt_path.a, b->data.tt_path.a) &&
           lizard_tt_structurally_equal(a->data.tt_path.b, b->data.tt_path.b);
  case AST_TT_PATH_ABS:
    return lizard_tt_structurally_equal(a->data.tt_path_abs.binder,
                                        b->data.tt_path_abs.binder) &&
           lizard_tt_structurally_equal(a->data.tt_path_abs.body,
                                        b->data.tt_path_abs.body);
  case AST_TT_PATH_APP:
    return lizard_tt_structurally_equal(a->data.tt_path_app.path,
                                        b->data.tt_path_app.path) &&
           lizard_tt_structurally_equal(a->data.tt_path_app.point,
                                        b->data.tt_path_app.point);
  case AST_TT_F0:
  case AST_TT_F1:
    return 1;
  case AST_TT_F_EQ:
    return lizard_tt_structurally_equal(a->data.tt_f_eq.left,
                                        b->data.tt_f_eq.left) &&
           lizard_tt_structurally_equal(a->data.tt_f_eq.right,
                                        b->data.tt_f_eq.right);
  case AST_TT_F_AND:
  case AST_TT_F_OR:
    return lizard_tt_structurally_equal(a->data.tt_f_binop.left,
                                        b->data.tt_f_binop.left) &&
           lizard_tt_structurally_equal(a->data.tt_f_binop.right,
                                        b->data.tt_f_binop.right);
  case AST_TT_PARTIAL:
    return lizard_tt_structurally_equal(a->data.tt_partial.face,
                                        b->data.tt_partial.face) &&
           lizard_tt_structurally_equal(a->data.tt_partial.type,
                                        b->data.tt_partial.type);
  case AST_TT_SUB:
    return lizard_tt_structurally_equal(a->data.tt_sub.type,
                                        b->data.tt_sub.type) &&
           lizard_tt_structurally_equal(a->data.tt_sub.face,
                                        b->data.tt_sub.face) &&
           lizard_tt_structurally_equal(a->data.tt_sub.partial,
                                        b->data.tt_sub.partial);
  case AST_TT_COMP:
  case AST_TT_HCOMP:
  case AST_TT_FILL:
    return lizard_tt_structurally_equal(a->data.tt_comp.type_family,
                                        b->data.tt_comp.type_family) &&
           lizard_tt_structurally_equal(a->data.tt_comp.face,
                                        b->data.tt_comp.face) &&
           lizard_tt_structurally_equal(a->data.tt_comp.partial,
                                        b->data.tt_comp.partial) &&
           lizard_tt_structurally_equal(a->data.tt_comp.base,
                                        b->data.tt_comp.base);
  case AST_TT_EQUIV_TYPE:
    return lizard_tt_structurally_equal(a->data.tt_equiv_type.domain,
                                        b->data.tt_equiv_type.domain) &&
           lizard_tt_structurally_equal(a->data.tt_equiv_type.codomain,
                                        b->data.tt_equiv_type.codomain);
  case AST_TT_ID_EQUIV:
  case AST_TT_EQUIV_FUN:
  case AST_TT_EQUIV_INV:
    return lizard_tt_structurally_equal(a->data.tt_equiv_op.operand,
                                        b->data.tt_equiv_op.operand);
  case AST_TT_GLUE:
    return lizard_tt_structurally_equal(a->data.tt_glue.base, b->data.tt_glue.base) &&
           lizard_tt_structurally_equal(a->data.tt_glue.face, b->data.tt_glue.face) &&
           lizard_tt_structurally_equal(a->data.tt_glue.t, b->data.tt_glue.t) &&
           lizard_tt_structurally_equal(a->data.tt_glue.equiv, b->data.tt_glue.equiv);
  case AST_TT_GLUE_INTRO:
    return lizard_tt_structurally_equal(a->data.tt_glue_intro.face,
                                        b->data.tt_glue_intro.face) &&
           lizard_tt_structurally_equal(a->data.tt_glue_intro.t,
                                        b->data.tt_glue_intro.t) &&
           lizard_tt_structurally_equal(a->data.tt_glue_intro.a,
                                        b->data.tt_glue_intro.a);
  case AST_TT_UNGLUE:
    return lizard_tt_structurally_equal(a->data.tt_unglue.equiv,
                                        b->data.tt_unglue.equiv) &&
           lizard_tt_structurally_equal(a->data.tt_unglue.target,
                                        b->data.tt_unglue.target);
  case AST_TT_UA:
    return lizard_tt_structurally_equal(a->data.tt_ua.equiv, b->data.tt_ua.equiv);
  case AST_TT_SYSTEM_NIL:
    return 1;
  case AST_TT_SYSTEM_CONS:
    return lizard_tt_structurally_equal(a->data.tt_system_cons.face,
                                        b->data.tt_system_cons.face) &&
           lizard_tt_structurally_equal(a->data.tt_system_cons.value,
                                        b->data.tt_system_cons.value) &&
           lizard_tt_structurally_equal(a->data.tt_system_cons.next,
                                        b->data.tt_system_cons.next);
  case AST_TT_UNIT:
  case AST_TT_TT:
  case AST_TT_BOT:
    return 1;
  /* Phase H.1: HIT structural equality (same logic as alpha — these
   * structures have no binders to track). */
  case AST_TT_HIT_REF:
    return lizard_tt_structurally_equal(a->data.tt_hit_ref.name,
                                        b->data.tt_hit_ref.name);
  case AST_TT_HIT_DECL: {
    lz_list_node_t *pa, *pb;
    if (!lizard_tt_structurally_equal(a->data.tt_hit_decl.name,
                                      b->data.tt_hit_decl.name)) return 0;
    pa = a->data.tt_hit_decl.constructors->head;
    pb = b->data.tt_hit_decl.constructors->head;
    while (pa != a->data.tt_hit_decl.constructors->nil &&
           pb != b->data.tt_hit_decl.constructors->nil) {
      if (!lizard_tt_structurally_equal(((lizard_ast_list_node_t *)pa)->ast,
                                        ((lizard_ast_list_node_t *)pb)->ast)) return 0;
      pa = pa->next; pb = pb->next;
    }
    if (pa != a->data.tt_hit_decl.constructors->nil ||
        pb != b->data.tt_hit_decl.constructors->nil) return 0;
    pa = a->data.tt_hit_decl.paths->head;
    pb = b->data.tt_hit_decl.paths->head;
    while (pa != a->data.tt_hit_decl.paths->nil &&
           pb != b->data.tt_hit_decl.paths->nil) {
      if (!lizard_tt_structurally_equal(((lizard_ast_list_node_t *)pa)->ast,
                                        ((lizard_ast_list_node_t *)pb)->ast)) return 0;
      pa = pa->next; pb = pb->next;
    }
    return pa == a->data.tt_hit_decl.paths->nil &&
           pb == b->data.tt_hit_decl.paths->nil;
  }
  case AST_TT_HIT_CONSTRUCTOR: {
    lz_list_node_t *pa, *pb;
    if (!lizard_tt_structurally_equal(a->data.tt_hit_constructor.name,
                                      b->data.tt_hit_constructor.name)) return 0;
    pa = a->data.tt_hit_constructor.arg_types->head;
    pb = b->data.tt_hit_constructor.arg_types->head;
    while (pa != a->data.tt_hit_constructor.arg_types->nil &&
           pb != b->data.tt_hit_constructor.arg_types->nil) {
      if (!lizard_tt_structurally_equal(((lizard_ast_list_node_t *)pa)->ast,
                                        ((lizard_ast_list_node_t *)pb)->ast)) return 0;
      pa = pa->next; pb = pb->next;
    }
    return pa == a->data.tt_hit_constructor.arg_types->nil &&
           pb == b->data.tt_hit_constructor.arg_types->nil;
  }
  case AST_TT_HIT_PATH:
    return lizard_tt_structurally_equal(a->data.tt_hit_path.name,
                                        b->data.tt_hit_path.name) &&
           lizard_tt_structurally_equal(a->data.tt_hit_path.source,
                                        b->data.tt_hit_path.source) &&
           lizard_tt_structurally_equal(a->data.tt_hit_path.target,
                                        b->data.tt_hit_path.target);
  case AST_TT_HIT_APP: {
    lz_list_node_t *pa, *pb;
    if (!lizard_tt_structurally_equal(a->data.tt_hit_app.name,
                                      b->data.tt_hit_app.name)) return 0;
    pa = a->data.tt_hit_app.args->head;
    pb = b->data.tt_hit_app.args->head;
    while (pa != a->data.tt_hit_app.args->nil &&
           pb != b->data.tt_hit_app.args->nil) {
      if (!lizard_tt_structurally_equal(((lizard_ast_list_node_t *)pa)->ast,
                                        ((lizard_ast_list_node_t *)pb)->ast)) return 0;
      pa = pa->next; pb = pb->next;
    }
    return pa == a->data.tt_hit_app.args->nil &&
           pb == b->data.tt_hit_app.args->nil;
  }
  default:
    /* For types we don't specifically handle, fall back to pointer
     * equality. This means two structurally-equal-but-distinctly-
     * allocated unhandled nodes won't compare equal. That's a
     * limitation we accept; the alternative is to recurse into
     * every field, which we will when we extend the engine. */
    return 0;
  }
}

/* ----- Term constructors used during reduction ----------------------- */

static lizard_ast_node_t *make_refl(lizard_heap_t *heap,
                                    lizard_ast_node_t *value) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_REFL;
  n->data.tt_refl.value = value;
  return n;
}

static lizard_ast_node_t *make_sym(lizard_heap_t *heap,
                                   lizard_ast_node_t *path) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_ID_SYM;
  n->data.tt_id_sym.path = path;
  return n;
}

static lizard_ast_node_t *make_trans(lizard_heap_t *heap,
                                     lizard_ast_node_t *p,
                                     lizard_ast_node_t *q) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_ID_TRANS;
  n->data.tt_id_trans.p = p;
  n->data.tt_id_trans.q = q;
  return n;
}

static lizard_ast_node_t *make_transport(lizard_heap_t *heap,
                                         lizard_ast_node_t *path,
                                         lizard_ast_node_t *value) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_TRANSPORT;
  n->data.tt_transport.path = path;
  n->data.tt_transport.value = value;
  return n;
}

static lizard_ast_node_t *make_ap(lizard_heap_t *heap,
                                  lizard_ast_node_t *fn,
                                  lizard_ast_node_t *path) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_AP;
  n->data.tt_ap.fn = fn;
  n->data.tt_ap.path = path;
  return n;
}

static lizard_ast_node_t *make_app(lizard_heap_t *heap,
                                   lizard_ast_node_t *fn,
                                   lizard_ast_node_t *arg) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_APP;
  n->data.tt_app.fun = fn;
  n->data.tt_app.arg = arg;
  return n;
}

static lizard_ast_node_t *make_pi(lizard_heap_t *heap,
                                  lizard_ast_node_t *binder,
                                  lizard_ast_node_t *domain,
                                  lizard_ast_node_t *codomain) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_PI;
  n->data.tt_pi.binder = binder;
  n->data.tt_pi.domain = domain;
  n->data.tt_pi.codomain = codomain;
  return n;
}

static lizard_ast_node_t *make_id(lizard_heap_t *heap,
                                  lizard_ast_node_t *domain,
                                  lizard_ast_node_t *a,
                                  lizard_ast_node_t *b) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_ID;
  n->data.tt_id.domain = domain;
  n->data.tt_id.a = a;
  n->data.tt_id.b = b;
  return n;
}

static lizard_ast_node_t *make_pair(lizard_heap_t *heap,
                                    lizard_ast_node_t *fst,
                                    lizard_ast_node_t *snd) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_PAIR;
  n->data.tt_pair.fst = fst;
  n->data.tt_pair.snd = snd;
  return n;
}
static lizard_ast_node_t *make_fst(lizard_heap_t *heap, lizard_ast_node_t *t) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_FST;
  n->data.tt_proj.target = t;
  return n;
}
static lizard_ast_node_t *make_snd(lizard_heap_t *heap, lizard_ast_node_t *t) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_SND;
  n->data.tt_proj.target = t;
  return n;
}
static lizard_ast_node_t *make_inl(lizard_heap_t *heap, lizard_ast_node_t *v) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_INL;
  n->data.tt_inj.value = v;
  return n;
}
static lizard_ast_node_t *make_inr(lizard_heap_t *heap, lizard_ast_node_t *v) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_INR;
  n->data.tt_inj.value = v;
  return n;
}
static lizard_ast_node_t *make_case(lizard_heap_t *heap,
                                    lizard_ast_node_t *s,
                                    lizard_ast_node_t *l,
                                    lizard_ast_node_t *r) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_CASE;
  n->data.tt_case.scrutinee = s;
  n->data.tt_case.left_branch = l;
  n->data.tt_case.right_branch = r;
  return n;
}
static lizard_ast_node_t *make_j(lizard_heap_t *heap,
                                 lizard_ast_node_t *motive,
                                 lizard_ast_node_t *refl_case,
                                 lizard_ast_node_t *path) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_J;
  n->data.tt_j.motive = motive;
  n->data.tt_j.refl_case = refl_case;
  n->data.tt_j.path = path;
  return n;
}
static lizard_ast_node_t *make_unit(lizard_heap_t *heap) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_UNIT;
  return n;
}
static lizard_ast_node_t *make_bot(lizard_heap_t *heap) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_BOT;
  return n;
}
static lizard_ast_node_t *make_xport(lizard_heap_t *heap,
                                     lizard_ast_node_t *motive,
                                     lizard_ast_node_t *path,
                                     lizard_ast_node_t *value) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_XPORT;
  n->data.tt_xport.motive = motive;
  n->data.tt_xport.path = path;
  n->data.tt_xport.value = value;
  return n;
}
static lizard_ast_node_t *make_universe(lizard_heap_t *heap, long level) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_UNIVERSE;
  n->data.tt_universe.level = level;
  return n;
}

/* Public wrapper for use by tt_check.c. */
lizard_ast_node_t *lizard_tt_make_universe(lizard_heap_t *heap, long level) {
  return make_universe(heap, level);
}
static lizard_ast_node_t *make_u_suc(lizard_heap_t *heap,
                                     lizard_ast_node_t *u) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_U_SUC;
  n->data.tt_u_suc.operand = u;
  return n;
}
static lizard_ast_node_t *make_u_max(lizard_heap_t *heap,
                                     lizard_ast_node_t *u,
                                     lizard_ast_node_t *v) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_U_MAX;
  n->data.tt_u_max.left = u;
  n->data.tt_u_max.right = v;
  return n;
}
static lizard_ast_node_t *make_u_min(lizard_heap_t *heap,
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
static lizard_ast_node_t *make_universe_set(lizard_heap_t *heap,
                                            long *dims, long count) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_UNIVERSE_SET;
  n->data.tt_universe_set.dims = dims;
  n->data.tt_universe_set.count = count;
  return n;
}

/* Sorted-array set union (Phase L.2). Output is sorted, deduped. */
static lizard_ast_node_t *universe_set_union(lizard_heap_t *heap,
                                             lizard_ast_node_t *a,
                                             lizard_ast_node_t *b) {
  long ai, bi, oi, na, nb;
  long *out;
  na = a->data.tt_universe_set.count;
  nb = b->data.tt_universe_set.count;
  if (na + nb == 0) return make_universe_set(heap, NULL, 0);
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
  return make_universe_set(heap, out, oi);
}

/* Sorted-array set intersection (Phase L.2). */
static lizard_ast_node_t *universe_set_intersect(lizard_heap_t *heap,
                                                 lizard_ast_node_t *a,
                                                 lizard_ast_node_t *b) {
  long ai, bi, oi, na, nb, smaller;
  long *out;
  na = a->data.tt_universe_set.count;
  nb = b->data.tt_universe_set.count;
  smaller = (na < nb) ? na : nb;
  if (smaller == 0) return make_universe_set(heap, NULL, 0);
  out = lizard_heap_alloc(sizeof(long) * (size_t)smaller);
  ai = 0; bi = 0; oi = 0;
  while (ai < na && bi < nb) {
    long av = a->data.tt_universe_set.dims[ai];
    long bv = b->data.tt_universe_set.dims[bi];
    if (av < bv) ai++;
    else if (av > bv) bi++;
    else { out[oi++] = av; ai++; bi++; }
  }
  if (oi == 0) return make_universe_set(heap, NULL, 0);
  return make_universe_set(heap, out, oi);
}

/* Subset test: is A's dim set ⊆ B's? Both must be UNIVERSE_SET.
 * Returns 1 yes, 0 no. */
static int universe_set_subset(lizard_ast_node_t *a, lizard_ast_node_t *b) {
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
static lizard_ast_node_t *make_couniverse_set(lizard_heap_t *heap,
                                              long *dims, long count) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_COUNIVERSE_SET;
  n->data.tt_couniverse_set.dims = dims;
  n->data.tt_couniverse_set.count = count;
  return n;
}
static lizard_ast_node_t *make_co_max(lizard_heap_t *heap,
                                      lizard_ast_node_t *u,
                                      lizard_ast_node_t *v) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_CO_MAX;
  n->data.tt_co_max.left = u;
  n->data.tt_co_max.right = v;
  return n;
}
static lizard_ast_node_t *make_co_min(lizard_heap_t *heap,
                                      lizard_ast_node_t *u,
                                      lizard_ast_node_t *v) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_CO_MIN;
  n->data.tt_co_min.left = u;
  n->data.tt_co_min.right = v;
  return n;
}
static lizard_ast_node_t *couniverse_set_union(lizard_heap_t *heap,
                                               lizard_ast_node_t *a,
                                               lizard_ast_node_t *b) {
  long ai, bi, oi, na, nb;
  long *out;
  na = a->data.tt_couniverse_set.count;
  nb = b->data.tt_couniverse_set.count;
  if (na + nb == 0) return make_couniverse_set(heap, NULL, 0);
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
  return make_couniverse_set(heap, out, oi);
}
static lizard_ast_node_t *couniverse_set_intersect(lizard_heap_t *heap,
                                                   lizard_ast_node_t *a,
                                                   lizard_ast_node_t *b) {
  long ai, bi, oi, na, nb, smaller;
  long *out;
  na = a->data.tt_couniverse_set.count;
  nb = b->data.tt_couniverse_set.count;
  smaller = (na < nb) ? na : nb;
  if (smaller == 0) return make_couniverse_set(heap, NULL, 0);
  out = lizard_heap_alloc(sizeof(long) * (size_t)smaller);
  ai = 0; bi = 0; oi = 0;
  while (ai < na && bi < nb) {
    long av = a->data.tt_couniverse_set.dims[ai];
    long bv = b->data.tt_couniverse_set.dims[bi];
    if (av < bv) ai++;
    else if (av > bv) bi++;
    else { out[oi++] = av; ai++; bi++; }
  }
  if (oi == 0) return make_couniverse_set(heap, NULL, 0);
  return make_couniverse_set(heap, out, oi);
}
static int couniverse_set_subset(lizard_ast_node_t *a, lizard_ast_node_t *b) {
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

/* Cubical-layer constructors. */
static lizard_ast_node_t *make_i0(lizard_heap_t *heap) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_I0;
  return n;
}
static lizard_ast_node_t *make_i1(lizard_heap_t *heap) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_I1;
  return n;
}
static lizard_ast_node_t *make_i_and(lizard_heap_t *heap,
                                     lizard_ast_node_t *a,
                                     lizard_ast_node_t *b) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_I_AND;
  n->data.tt_i_binop.left = a;
  n->data.tt_i_binop.right = b;
  return n;
}
static lizard_ast_node_t *make_i_or(lizard_heap_t *heap,
                                    lizard_ast_node_t *a,
                                    lizard_ast_node_t *b) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_I_OR;
  n->data.tt_i_binop.left = a;
  n->data.tt_i_binop.right = b;
  return n;
}
static lizard_ast_node_t *make_i_neg(lizard_heap_t *heap,
                                     lizard_ast_node_t *a) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_I_NEG;
  n->data.tt_i_neg.operand = a;
  return n;
}
static lizard_ast_node_t *make_path(lizard_heap_t *heap,
                                    lizard_ast_node_t *A,
                                    lizard_ast_node_t *a,
                                    lizard_ast_node_t *b) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_PATH;
  n->data.tt_path.domain = A;
  n->data.tt_path.a = a;
  n->data.tt_path.b = b;
  return n;
}
static lizard_ast_node_t *make_path_abs(lizard_heap_t *heap,
                                        lizard_ast_node_t *binder,
                                        lizard_ast_node_t *body) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_PATH_ABS;
  n->data.tt_path_abs.binder = binder;
  n->data.tt_path_abs.body = body;
  return n;
}
static lizard_ast_node_t *make_path_app(lizard_heap_t *heap,
                                        lizard_ast_node_t *p,
                                        lizard_ast_node_t *i) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_PATH_APP;
  n->data.tt_path_app.path = p;
  n->data.tt_path_app.point = i;
  return n;
}

static lizard_ast_node_t *make_f0(lizard_heap_t *heap) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_F0;
  return n;
}
static lizard_ast_node_t *make_f1(lizard_heap_t *heap) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_F1;
  return n;
}
static lizard_ast_node_t *make_f_eq(lizard_heap_t *heap,
                                    lizard_ast_node_t *l,
                                    lizard_ast_node_t *r) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_F_EQ;
  n->data.tt_f_eq.left = l;
  n->data.tt_f_eq.right = r;
  return n;
}
static lizard_ast_node_t *make_f_and(lizard_heap_t *heap,
                                     lizard_ast_node_t *l,
                                     lizard_ast_node_t *r) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_F_AND;
  n->data.tt_f_binop.left = l;
  n->data.tt_f_binop.right = r;
  return n;
}
static lizard_ast_node_t *make_f_or(lizard_heap_t *heap,
                                    lizard_ast_node_t *l,
                                    lizard_ast_node_t *r) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_F_OR;
  n->data.tt_f_binop.left = l;
  n->data.tt_f_binop.right = r;
  return n;
}
static lizard_ast_node_t *make_partial(lizard_heap_t *heap,
                                       lizard_ast_node_t *face,
                                       lizard_ast_node_t *type) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_PARTIAL;
  n->data.tt_partial.face = face;
  n->data.tt_partial.type = type;
  return n;
}
static lizard_ast_node_t *make_sub(lizard_heap_t *heap,
                                   lizard_ast_node_t *type,
                                   lizard_ast_node_t *face,
                                   lizard_ast_node_t *partial) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_SUB;
  n->data.tt_sub.type = type;
  n->data.tt_sub.face = face;
  n->data.tt_sub.partial = partial;
  return n;
}
static lizard_ast_node_t *make_comp(lizard_heap_t *heap,
                                    int kind,
                                    lizard_ast_node_t *type_family,
                                    lizard_ast_node_t *face,
                                    lizard_ast_node_t *partial,
                                    lizard_ast_node_t *base) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = (lizard_ast_node_type_t)kind;
  n->data.tt_comp.type_family = type_family;
  n->data.tt_comp.face = face;
  n->data.tt_comp.partial = partial;
  n->data.tt_comp.base = base;
  return n;
}

static lizard_ast_node_t *make_equiv_type(lizard_heap_t *heap,
                                          lizard_ast_node_t *A,
                                          lizard_ast_node_t *B) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_EQUIV_TYPE;
  n->data.tt_equiv_type.domain = A;
  n->data.tt_equiv_type.codomain = B;
  return n;
}
static lizard_ast_node_t *make_id_equiv(lizard_heap_t *heap,
                                        lizard_ast_node_t *A) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_ID_EQUIV;
  n->data.tt_equiv_op.operand = A;
  return n;
}
static lizard_ast_node_t *make_equiv_fun(lizard_heap_t *heap,
                                         lizard_ast_node_t *e) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_EQUIV_FUN;
  n->data.tt_equiv_op.operand = e;
  return n;
}
static lizard_ast_node_t *make_equiv_inv(lizard_heap_t *heap,
                                         lizard_ast_node_t *e) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_EQUIV_INV;
  n->data.tt_equiv_op.operand = e;
  return n;
}
static lizard_ast_node_t *make_glue(lizard_heap_t *heap,
                                    lizard_ast_node_t *A,
                                    lizard_ast_node_t *face,
                                    lizard_ast_node_t *T,
                                    lizard_ast_node_t *e) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_GLUE;
  n->data.tt_glue.base = A;
  n->data.tt_glue.face = face;
  n->data.tt_glue.t = T;
  n->data.tt_glue.equiv = e;
  return n;
}
static lizard_ast_node_t *make_glue_intro(lizard_heap_t *heap,
                                          lizard_ast_node_t *face,
                                          lizard_ast_node_t *t,
                                          lizard_ast_node_t *a) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_GLUE_INTRO;
  n->data.tt_glue_intro.face = face;
  n->data.tt_glue_intro.t = t;
  n->data.tt_glue_intro.a = a;
  return n;
}
static lizard_ast_node_t *make_unglue(lizard_heap_t *heap,
                                      lizard_ast_node_t *e,
                                      lizard_ast_node_t *g) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_UNGLUE;
  n->data.tt_unglue.equiv = e;
  n->data.tt_unglue.target = g;
  return n;
}
static lizard_ast_node_t *make_ua(lizard_heap_t *heap,
                                  lizard_ast_node_t *e) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_UA;
  n->data.tt_ua.equiv = e;
  return n;
}

static lizard_ast_node_t *make_system_nil(lizard_heap_t *heap) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_SYSTEM_NIL;
  return n;
}
static lizard_ast_node_t *make_system_cons(lizard_heap_t *heap,
                                           lizard_ast_node_t *face,
                                           lizard_ast_node_t *value,
                                           lizard_ast_node_t *next) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_SYSTEM_CONS;
  n->data.tt_system_cons.face = face;
  n->data.tt_system_cons.value = value;
  n->data.tt_system_cons.next = next;
  return n;
}

/* ----- Memoization table -------------------------------------------- */

/* Map from AST node pointer to its normal form. The table is per-call
 * (cleared at the top-level reduce entry); within a call, structurally-
 * equal subterms that share a pointer get cached.
 *
 * Open-addressed table with linear probing, doubled at load > 0.5. */

typedef struct memo_entry {
  lizard_ast_node_t *key;
  lizard_ast_node_t *val;
} memo_entry_t;

typedef struct memo_table {
  memo_entry_t *entries;
  size_t cap;
  size_t count;
} memo_table_t;

static size_t memo_hash(lizard_ast_node_t *p) {
  /* Pointer hash, C89-compatible. We shift right by 4 to discard the
   * low bits which are usually 0 due to allocator alignment, then
   * mix with multiplication. The constant is chosen for good
   * avalanche on 32-bit values. */
  unsigned long x = (unsigned long)p;
  x >>= 4;
  x *= 2654435761UL;  /* Knuth's multiplicative constant */
  return (size_t)x;
}

static void memo_grow(memo_table_t *m) {
  size_t old_cap = m->cap;
  memo_entry_t *old_entries = m->entries;
  size_t new_cap = old_cap == 0 ? 64 : old_cap * 2;
  size_t i;
  m->entries = (memo_entry_t *)calloc(new_cap, sizeof(memo_entry_t));
  m->cap = new_cap;
  m->count = 0;
  for (i = 0; i < old_cap; i++) {
    if (old_entries[i].key) {
      size_t h = memo_hash(old_entries[i].key) % m->cap;
      while (m->entries[h].key) h = (h + 1) % m->cap;
      m->entries[h] = old_entries[i];
      m->count++;
    }
  }
  free(old_entries);
}

static lizard_ast_node_t *memo_get(memo_table_t *m, lizard_ast_node_t *key) {
  size_t h;
  if (m->cap == 0) return NULL;
  h = memo_hash(key) % m->cap;
  while (m->entries[h].key) {
    if (m->entries[h].key == key) return m->entries[h].val;
    h = (h + 1) % m->cap;
  }
  return NULL;
}

static void memo_put(memo_table_t *m, lizard_ast_node_t *key,
                     lizard_ast_node_t *val) {
  size_t h;
  if (m->cap == 0 || m->count * 2 >= m->cap) memo_grow(m);
  h = memo_hash(key) % m->cap;
  while (m->entries[h].key) {
    if (m->entries[h].key == key) {
      m->entries[h].val = val;
      return;
    }
    h = (h + 1) % m->cap;
  }
  m->entries[h].key = key;
  m->entries[h].val = val;
  m->count++;
}

static void memo_init(memo_table_t *m) {
  m->entries = NULL;
  m->cap = 0;
  m->count = 0;
}
static void memo_destroy(memo_table_t *m) {
  free(m->entries);
  m->entries = NULL;
  m->cap = 0;
  m->count = 0;
}

/* ----- Bottom-up normalization ---------------------------------------
 *
 * The strategy:
 *
 *   normalize(t) =
 *     1. If t is in memo, return memo[t].
 *     2. Recursively normalize all subterms of t.
 *     3. Build t' with the normalized subterms.
 *     4. Apply head rewrites on t' (each may produce a t'' whose
 *        subterms are NOT yet normalized; if so, recursively
 *        normalize t'').
 *     5. Memoize: memo[t] = t''.
 *     6. Return t''.
 *
 * Why this is fast: each subterm is normalized exactly once. The
 * total work is O(node count × rewrite chain depth at each node),
 * which is linear in the input size for typical workloads.
 *
 * Termination: same lex measure as before. Every rewrite either
 * (a) strictly decreases the number of sym/trans/transport nodes,
 * or (b) re-associates with a strict decrease in left-weight.
 * Recursive normalize calls only happen on rewritten terms, which
 * are strictly smaller in the lex measure. */

static lizard_ast_node_t *try_head_rewrites(lizard_ast_node_t *t,
                                            lizard_heap_t *heap,
                                            int *changed);

static lizard_ast_node_t *normalize_rec(lizard_ast_node_t *t,
                                        lizard_heap_t *heap,
                                        memo_table_t *memo) {
  lizard_ast_node_t *cached, *result;
  int changed;
  if (t == NULL) return NULL;
  cached = memo_get(memo, t);
  if (cached) return cached;

  /* First, normalize subterms — produces a new node with normalized
   * children, structurally identical to t at the head. */
  switch (t->type) {
  case AST_TT_REFL: {
    lizard_ast_node_t *v = normalize_rec(t->data.tt_refl.value, heap, memo);
    if (v == t->data.tt_refl.value) result = t;
    else result = make_refl(heap, v);
    break;
  }
  case AST_TT_ID_SYM: {
    lizard_ast_node_t *p = normalize_rec(t->data.tt_id_sym.path, heap, memo);
    if (p == t->data.tt_id_sym.path) result = t;
    else result = make_sym(heap, p);
    break;
  }
  case AST_TT_ID_TRANS: {
    lizard_ast_node_t *p = normalize_rec(t->data.tt_id_trans.p, heap, memo);
    lizard_ast_node_t *q = normalize_rec(t->data.tt_id_trans.q, heap, memo);
    if (p == t->data.tt_id_trans.p && q == t->data.tt_id_trans.q) result = t;
    else result = make_trans(heap, p, q);
    break;
  }
  case AST_TT_TRANSPORT: {
    lizard_ast_node_t *p = normalize_rec(t->data.tt_transport.path, heap, memo);
    lizard_ast_node_t *v = normalize_rec(t->data.tt_transport.value, heap, memo);
    if (p == t->data.tt_transport.path && v == t->data.tt_transport.value)
      result = t;
    else result = make_transport(heap, p, v);
    break;
  }
  case AST_TT_PI: {
    lizard_ast_node_t *dom = normalize_rec(t->data.tt_pi.domain, heap, memo);
    lizard_ast_node_t *cod = normalize_rec(t->data.tt_pi.codomain, heap, memo);
    if (dom == t->data.tt_pi.domain && cod == t->data.tt_pi.codomain)
      result = t;
    else {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_PI;
      n->data.tt_pi.binder = t->data.tt_pi.binder;
      n->data.tt_pi.domain = dom;
      n->data.tt_pi.codomain = cod;
      result = n;
    }
    break;
  }
  case AST_TT_SIGMA: {
    lizard_ast_node_t *dom = normalize_rec(t->data.tt_sigma.domain, heap, memo);
    lizard_ast_node_t *cod = normalize_rec(t->data.tt_sigma.codomain, heap, memo);
    if (dom == t->data.tt_sigma.domain && cod == t->data.tt_sigma.codomain)
      result = t;
    else {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_SIGMA;
      n->data.tt_sigma.binder = t->data.tt_sigma.binder;
      n->data.tt_sigma.domain = dom;
      n->data.tt_sigma.codomain = cod;
      result = n;
    }
    break;
  }
  case AST_TT_PI_FRESH: {
    lizard_ast_node_t *dom = normalize_rec(t->data.tt_pi_fresh.domain, heap, memo);
    lizard_ast_node_t *cod = normalize_rec(t->data.tt_pi_fresh.codomain, heap, memo);
    if (dom == t->data.tt_pi_fresh.domain && cod == t->data.tt_pi_fresh.codomain)
      result = t;
    else {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_PI_FRESH;
      n->data.tt_pi_fresh.binder = t->data.tt_pi_fresh.binder;
      n->data.tt_pi_fresh.domain = dom;
      n->data.tt_pi_fresh.codomain = cod;
      result = n;
    }
    break;
  }
  case AST_TT_SIGMA_FRESH: {
    lizard_ast_node_t *dom = normalize_rec(t->data.tt_sigma_fresh.domain, heap, memo);
    lizard_ast_node_t *cod = normalize_rec(t->data.tt_sigma_fresh.codomain, heap, memo);
    if (dom == t->data.tt_sigma_fresh.domain && cod == t->data.tt_sigma_fresh.codomain)
      result = t;
    else {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_SIGMA_FRESH;
      n->data.tt_sigma_fresh.binder = t->data.tt_sigma_fresh.binder;
      n->data.tt_sigma_fresh.domain = dom;
      n->data.tt_sigma_fresh.codomain = cod;
      result = n;
    }
    break;
  }
  case AST_TT_CO_PI_FRESH: {
    lizard_ast_node_t *dom = normalize_rec(t->data.tt_co_pi_fresh.domain, heap, memo);
    lizard_ast_node_t *cod = normalize_rec(t->data.tt_co_pi_fresh.codomain, heap, memo);
    if (dom == t->data.tt_co_pi_fresh.domain && cod == t->data.tt_co_pi_fresh.codomain)
      result = t;
    else {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_CO_PI_FRESH;
      n->data.tt_co_pi_fresh.binder = t->data.tt_co_pi_fresh.binder;
      n->data.tt_co_pi_fresh.domain = dom;
      n->data.tt_co_pi_fresh.codomain = cod;
      result = n;
    }
    break;
  }
  case AST_TT_CO_SIGMA_FRESH: {
    lizard_ast_node_t *dom = normalize_rec(t->data.tt_co_sigma_fresh.domain, heap, memo);
    lizard_ast_node_t *cod = normalize_rec(t->data.tt_co_sigma_fresh.codomain, heap, memo);
    if (dom == t->data.tt_co_sigma_fresh.domain && cod == t->data.tt_co_sigma_fresh.codomain)
      result = t;
    else {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_CO_SIGMA_FRESH;
      n->data.tt_co_sigma_fresh.binder = t->data.tt_co_sigma_fresh.binder;
      n->data.tt_co_sigma_fresh.domain = dom;
      n->data.tt_co_sigma_fresh.codomain = cod;
      result = n;
    }
    break;
  }
  case AST_TT_BOX: {
    /* M.5.1: no reduction rules for Box yet. Just descend into the
     * argument; the Box wrapper is preserved. */
    lizard_ast_node_t *arg = normalize_rec(t->data.tt_box.argument, heap, memo);
    if (arg == t->data.tt_box.argument) {
      result = t;
    } else {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_BOX;
      n->data.tt_box.argument = arg;
      result = n;
    }
    break;
  }
  case AST_TT_DIAMOND: {
    lizard_ast_node_t *arg = normalize_rec(t->data.tt_diamond.argument, heap, memo);
    if (arg == t->data.tt_diamond.argument) {
      result = t;
    } else {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_DIAMOND;
      n->data.tt_diamond.argument = arg;
      result = n;
    }
    break;
  }
  case AST_TT_BOX_INTRO: {
    lizard_ast_node_t *body = normalize_rec(t->data.tt_box_intro.body, heap, memo);
    if (body == t->data.tt_box_intro.body) {
      result = t;
    } else {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_BOX_INTRO;
      n->data.tt_box_intro.body = body;
      result = n;
    }
    break;
  }
  case AST_TT_BOX_ELIM: {
    /* Phase M.5.2 beta rule:
     *   (unbox x (box e) body) → body[e/x]
     * Otherwise, normalize subterms. */
    lizard_ast_node_t *scrut = normalize_rec(t->data.tt_box_elim.scrutinee, heap, memo);
    if (scrut->type == AST_TT_BOX_INTRO &&
        t->data.tt_box_elim.binder &&
        t->data.tt_box_elim.binder->type == AST_SYMBOL) {
      lizard_ast_node_t *substituted = subst_rec(
          t->data.tt_box_elim.body,
          t->data.tt_box_elim.binder->data.variable,
          scrut->data.tt_box_intro.body,
          heap);
      result = normalize_rec(substituted, heap, memo);
    } else {
      lizard_ast_node_t *body = normalize_rec(t->data.tt_box_elim.body, heap, memo);
      if (scrut == t->data.tt_box_elim.scrutinee &&
          body == t->data.tt_box_elim.body) {
        result = t;
      } else {
        lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
        n->type = AST_TT_BOX_ELIM;
        n->data.tt_box_elim.binder = t->data.tt_box_elim.binder;
        n->data.tt_box_elim.scrutinee = scrut;
        n->data.tt_box_elim.body = body;
        result = n;
      }
    }
    break;
  }
  case AST_TT_ID: {
    lizard_ast_node_t *dom = normalize_rec(t->data.tt_id.domain, heap, memo);
    lizard_ast_node_t *a = normalize_rec(t->data.tt_id.a, heap, memo);
    lizard_ast_node_t *b = normalize_rec(t->data.tt_id.b, heap, memo);
    if (dom == t->data.tt_id.domain && a == t->data.tt_id.a && b == t->data.tt_id.b)
      result = t;
    else {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_ID;
      n->data.tt_id.domain = dom;
      n->data.tt_id.a = a;
      n->data.tt_id.b = b;
      result = n;
    }
    break;
  }
  case AST_TT_APP: {
    lizard_ast_node_t *fn = normalize_rec(t->data.tt_app.fun, heap, memo);
    lizard_ast_node_t *ar = normalize_rec(t->data.tt_app.arg, heap, memo);
    if (fn == t->data.tt_app.fun && ar == t->data.tt_app.arg) result = t;
    else {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_APP;
      n->data.tt_app.fun = fn;
      n->data.tt_app.arg = ar;
      result = n;
    }
    break;
  }
  case AST_TT_SUM: {
    lizard_ast_node_t *l = normalize_rec(t->data.tt_sum.left, heap, memo);
    lizard_ast_node_t *r = normalize_rec(t->data.tt_sum.right, heap, memo);
    if (l == t->data.tt_sum.left && r == t->data.tt_sum.right) result = t;
    else {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_SUM;
      n->data.tt_sum.left = l;
      n->data.tt_sum.right = r;
      result = n;
    }
    break;
  }
  case AST_TT_LAMBDA: {
    lizard_ast_node_t *body = normalize_rec(t->data.tt_lambda.body, heap, memo);
    if (body == t->data.tt_lambda.body) result = t;
    else {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_LAMBDA;
      n->data.tt_lambda.binder = t->data.tt_lambda.binder;
      n->data.tt_lambda.body = body;
      result = n;
    }
    break;
  }
  case AST_TT_AP: {
    lizard_ast_node_t *fn = normalize_rec(t->data.tt_ap.fn, heap, memo);
    lizard_ast_node_t *p = normalize_rec(t->data.tt_ap.path, heap, memo);
    if (fn == t->data.tt_ap.fn && p == t->data.tt_ap.path) result = t;
    else result = make_ap(heap, fn, p);
    break;
  }
  case AST_TT_PAIR: {
    lizard_ast_node_t *f = normalize_rec(t->data.tt_pair.fst, heap, memo);
    lizard_ast_node_t *s = normalize_rec(t->data.tt_pair.snd, heap, memo);
    if (f == t->data.tt_pair.fst && s == t->data.tt_pair.snd) result = t;
    else result = make_pair(heap, f, s);
    break;
  }
  case AST_TT_FST: {
    lizard_ast_node_t *p = normalize_rec(t->data.tt_proj.target, heap, memo);
    if (p == t->data.tt_proj.target) result = t;
    else result = make_fst(heap, p);
    break;
  }
  case AST_TT_SND: {
    lizard_ast_node_t *p = normalize_rec(t->data.tt_proj.target, heap, memo);
    if (p == t->data.tt_proj.target) result = t;
    else result = make_snd(heap, p);
    break;
  }
  case AST_TT_INL: {
    lizard_ast_node_t *v = normalize_rec(t->data.tt_inj.value, heap, memo);
    if (v == t->data.tt_inj.value) result = t;
    else result = make_inl(heap, v);
    break;
  }
  case AST_TT_INR: {
    lizard_ast_node_t *v = normalize_rec(t->data.tt_inj.value, heap, memo);
    if (v == t->data.tt_inj.value) result = t;
    else result = make_inr(heap, v);
    break;
  }
  case AST_TT_CASE: {
    lizard_ast_node_t *s = normalize_rec(t->data.tt_case.scrutinee, heap, memo);
    lizard_ast_node_t *l = normalize_rec(t->data.tt_case.left_branch, heap, memo);
    lizard_ast_node_t *r = normalize_rec(t->data.tt_case.right_branch, heap, memo);
    if (s == t->data.tt_case.scrutinee &&
        l == t->data.tt_case.left_branch &&
        r == t->data.tt_case.right_branch) result = t;
    else result = make_case(heap, s, l, r);
    break;
  }
  case AST_TT_J: {
    lizard_ast_node_t *m = normalize_rec(t->data.tt_j.motive, heap, memo);
    lizard_ast_node_t *d = normalize_rec(t->data.tt_j.refl_case, heap, memo);
    lizard_ast_node_t *p = normalize_rec(t->data.tt_j.path, heap, memo);
    if (m == t->data.tt_j.motive &&
        d == t->data.tt_j.refl_case &&
        p == t->data.tt_j.path) result = t;
    else result = make_j(heap, m, d, p);
    break;
  }
  case AST_TT_XPORT: {
    lizard_ast_node_t *m = normalize_rec(t->data.tt_xport.motive, heap, memo);
    lizard_ast_node_t *p = normalize_rec(t->data.tt_xport.path, heap, memo);
    lizard_ast_node_t *v = normalize_rec(t->data.tt_xport.value, heap, memo);
    if (m == t->data.tt_xport.motive &&
        p == t->data.tt_xport.path &&
        v == t->data.tt_xport.value) result = t;
    else result = make_xport(heap, m, p, v);
    break;
  }
  case AST_TT_U_VAR:
    result = t;
    break;
  case AST_TT_U_SUC: {
    lizard_ast_node_t *u = normalize_rec(t->data.tt_u_suc.operand, heap, memo);
    if (u == t->data.tt_u_suc.operand) result = t;
    else result = make_u_suc(heap, u);
    break;
  }
  case AST_TT_U_MAX: {
    lizard_ast_node_t *l = normalize_rec(t->data.tt_u_max.left, heap, memo);
    lizard_ast_node_t *r = normalize_rec(t->data.tt_u_max.right, heap, memo);
    if (l == t->data.tt_u_max.left && r == t->data.tt_u_max.right) result = t;
    else result = make_u_max(heap, l, r);
    break;
  }
  case AST_TT_U_MIN: {
    lizard_ast_node_t *l = normalize_rec(t->data.tt_u_min.left, heap, memo);
    lizard_ast_node_t *r = normalize_rec(t->data.tt_u_min.right, heap, memo);
    if (l == t->data.tt_u_min.left && r == t->data.tt_u_min.right) result = t;
    else result = make_u_min(heap, l, r);
    break;
  }
  case AST_TT_CO_MAX: {
    lizard_ast_node_t *l = normalize_rec(t->data.tt_co_max.left, heap, memo);
    lizard_ast_node_t *r = normalize_rec(t->data.tt_co_max.right, heap, memo);
    if (l == t->data.tt_co_max.left && r == t->data.tt_co_max.right) result = t;
    else result = make_co_max(heap, l, r);
    break;
  }
  case AST_TT_CO_MIN: {
    lizard_ast_node_t *l = normalize_rec(t->data.tt_co_min.left, heap, memo);
    lizard_ast_node_t *r = normalize_rec(t->data.tt_co_min.right, heap, memo);
    if (l == t->data.tt_co_min.left && r == t->data.tt_co_min.right) result = t;
    else result = make_co_min(heap, l, r);
    break;
  }
  case AST_TT_INTERVAL:
  case AST_TT_I0:
  case AST_TT_I1:
  case AST_TT_I_VAR:
    result = t;
    break;
  case AST_TT_I_AND: {
    lizard_ast_node_t *l = normalize_rec(t->data.tt_i_binop.left, heap, memo);
    lizard_ast_node_t *r = normalize_rec(t->data.tt_i_binop.right, heap, memo);
    if (l == t->data.tt_i_binop.left && r == t->data.tt_i_binop.right) result = t;
    else result = make_i_and(heap, l, r);
    break;
  }
  case AST_TT_I_OR: {
    lizard_ast_node_t *l = normalize_rec(t->data.tt_i_binop.left, heap, memo);
    lizard_ast_node_t *r = normalize_rec(t->data.tt_i_binop.right, heap, memo);
    if (l == t->data.tt_i_binop.left && r == t->data.tt_i_binop.right) result = t;
    else result = make_i_or(heap, l, r);
    break;
  }
  case AST_TT_I_NEG: {
    lizard_ast_node_t *o = normalize_rec(t->data.tt_i_neg.operand, heap, memo);
    if (o == t->data.tt_i_neg.operand) result = t;
    else result = make_i_neg(heap, o);
    break;
  }
  case AST_TT_PATH: {
    lizard_ast_node_t *A = normalize_rec(t->data.tt_path.domain, heap, memo);
    lizard_ast_node_t *pa = normalize_rec(t->data.tt_path.a, heap, memo);
    lizard_ast_node_t *pb = normalize_rec(t->data.tt_path.b, heap, memo);
    if (A == t->data.tt_path.domain && pa == t->data.tt_path.a &&
        pb == t->data.tt_path.b) result = t;
    else result = make_path(heap, A, pa, pb);
    break;
  }
  case AST_TT_PATH_ABS: {
    lizard_ast_node_t *body = normalize_rec(t->data.tt_path_abs.body, heap, memo);
    if (body == t->data.tt_path_abs.body) result = t;
    else result = make_path_abs(heap, t->data.tt_path_abs.binder, body);
    break;
  }
  case AST_TT_PATH_APP: {
    lizard_ast_node_t *p = normalize_rec(t->data.tt_path_app.path, heap, memo);
    lizard_ast_node_t *i = normalize_rec(t->data.tt_path_app.point, heap, memo);
    if (p == t->data.tt_path_app.path && i == t->data.tt_path_app.point) result = t;
    else result = make_path_app(heap, p, i);
    break;
  }
  case AST_TT_F0:
  case AST_TT_F1:
    result = t;
    break;
  case AST_TT_F_EQ: {
    lizard_ast_node_t *l = normalize_rec(t->data.tt_f_eq.left, heap, memo);
    lizard_ast_node_t *r = normalize_rec(t->data.tt_f_eq.right, heap, memo);
    if (l == t->data.tt_f_eq.left && r == t->data.tt_f_eq.right) result = t;
    else result = make_f_eq(heap, l, r);
    break;
  }
  case AST_TT_F_AND: {
    lizard_ast_node_t *l = normalize_rec(t->data.tt_f_binop.left, heap, memo);
    lizard_ast_node_t *r = normalize_rec(t->data.tt_f_binop.right, heap, memo);
    if (l == t->data.tt_f_binop.left && r == t->data.tt_f_binop.right) result = t;
    else result = make_f_and(heap, l, r);
    break;
  }
  case AST_TT_F_OR: {
    lizard_ast_node_t *l = normalize_rec(t->data.tt_f_binop.left, heap, memo);
    lizard_ast_node_t *r = normalize_rec(t->data.tt_f_binop.right, heap, memo);
    if (l == t->data.tt_f_binop.left && r == t->data.tt_f_binop.right) result = t;
    else result = make_f_or(heap, l, r);
    break;
  }
  case AST_TT_PARTIAL: {
    lizard_ast_node_t *f = normalize_rec(t->data.tt_partial.face, heap, memo);
    lizard_ast_node_t *T = normalize_rec(t->data.tt_partial.type, heap, memo);
    if (f == t->data.tt_partial.face && T == t->data.tt_partial.type) result = t;
    else result = make_partial(heap, f, T);
    break;
  }
  case AST_TT_SUB: {
    lizard_ast_node_t *T = normalize_rec(t->data.tt_sub.type, heap, memo);
    lizard_ast_node_t *f = normalize_rec(t->data.tt_sub.face, heap, memo);
    lizard_ast_node_t *p = normalize_rec(t->data.tt_sub.partial, heap, memo);
    if (T == t->data.tt_sub.type && f == t->data.tt_sub.face &&
        p == t->data.tt_sub.partial) result = t;
    else result = make_sub(heap, T, f, p);
    break;
  }
  case AST_TT_COMP:
  case AST_TT_HCOMP:
  case AST_TT_FILL: {
    lizard_ast_node_t *tf = normalize_rec(t->data.tt_comp.type_family, heap, memo);
    lizard_ast_node_t *f  = normalize_rec(t->data.tt_comp.face, heap, memo);
    lizard_ast_node_t *p  = normalize_rec(t->data.tt_comp.partial, heap, memo);
    lizard_ast_node_t *b  = normalize_rec(t->data.tt_comp.base, heap, memo);
    if (tf == t->data.tt_comp.type_family && f == t->data.tt_comp.face &&
        p == t->data.tt_comp.partial && b == t->data.tt_comp.base) result = t;
    else result = make_comp(heap, t->type, tf, f, p, b);
    break;
  }
  case AST_TT_EQUIV_TYPE: {
    lizard_ast_node_t *A = normalize_rec(t->data.tt_equiv_type.domain, heap, memo);
    lizard_ast_node_t *B = normalize_rec(t->data.tt_equiv_type.codomain, heap, memo);
    if (A == t->data.tt_equiv_type.domain && B == t->data.tt_equiv_type.codomain) result = t;
    else result = make_equiv_type(heap, A, B);
    break;
  }
  case AST_TT_ID_EQUIV: {
    lizard_ast_node_t *o = normalize_rec(t->data.tt_equiv_op.operand, heap, memo);
    if (o == t->data.tt_equiv_op.operand) result = t;
    else result = make_id_equiv(heap, o);
    break;
  }
  case AST_TT_EQUIV_FUN: {
    lizard_ast_node_t *o = normalize_rec(t->data.tt_equiv_op.operand, heap, memo);
    if (o == t->data.tt_equiv_op.operand) result = t;
    else result = make_equiv_fun(heap, o);
    break;
  }
  case AST_TT_EQUIV_INV: {
    lizard_ast_node_t *o = normalize_rec(t->data.tt_equiv_op.operand, heap, memo);
    if (o == t->data.tt_equiv_op.operand) result = t;
    else result = make_equiv_inv(heap, o);
    break;
  }
  case AST_TT_GLUE: {
    lizard_ast_node_t *A = normalize_rec(t->data.tt_glue.base, heap, memo);
    lizard_ast_node_t *f = normalize_rec(t->data.tt_glue.face, heap, memo);
    lizard_ast_node_t *T = normalize_rec(t->data.tt_glue.t, heap, memo);
    lizard_ast_node_t *e = normalize_rec(t->data.tt_glue.equiv, heap, memo);
    if (A == t->data.tt_glue.base && f == t->data.tt_glue.face &&
        T == t->data.tt_glue.t && e == t->data.tt_glue.equiv) result = t;
    else result = make_glue(heap, A, f, T, e);
    break;
  }
  case AST_TT_GLUE_INTRO: {
    lizard_ast_node_t *f = normalize_rec(t->data.tt_glue_intro.face, heap, memo);
    lizard_ast_node_t *tval = normalize_rec(t->data.tt_glue_intro.t, heap, memo);
    lizard_ast_node_t *aval = normalize_rec(t->data.tt_glue_intro.a, heap, memo);
    if (f == t->data.tt_glue_intro.face && tval == t->data.tt_glue_intro.t &&
        aval == t->data.tt_glue_intro.a) result = t;
    else result = make_glue_intro(heap, f, tval, aval);
    break;
  }
  case AST_TT_UNGLUE: {
    lizard_ast_node_t *e = normalize_rec(t->data.tt_unglue.equiv, heap, memo);
    lizard_ast_node_t *g = normalize_rec(t->data.tt_unglue.target, heap, memo);
    if (e == t->data.tt_unglue.equiv && g == t->data.tt_unglue.target) result = t;
    else result = make_unglue(heap, e, g);
    break;
  }
  case AST_TT_UA: {
    lizard_ast_node_t *e = normalize_rec(t->data.tt_ua.equiv, heap, memo);
    if (e == t->data.tt_ua.equiv) result = t;
    else result = make_ua(heap, e);
    break;
  }
  case AST_TT_SYSTEM_NIL:
    result = t;
    break;
  case AST_TT_SYSTEM_CONS: {
    lizard_ast_node_t *f = normalize_rec(t->data.tt_system_cons.face, heap, memo);
    lizard_ast_node_t *val = normalize_rec(t->data.tt_system_cons.value, heap, memo);
    lizard_ast_node_t *nx = normalize_rec(t->data.tt_system_cons.next, heap, memo);
    if (f == t->data.tt_system_cons.face && val == t->data.tt_system_cons.value &&
        nx == t->data.tt_system_cons.next) result = t;
    else result = make_system_cons(heap, f, val, nx);
    break;
  }
  case AST_TT_UNIT:
  case AST_TT_TT:
  case AST_TT_BOT:
    result = t;
    break;
  /* Phase H.1: HIT structures. Normalize subterms; no head reductions
   * for HIT instances (no computation rules at this phase). */
  case AST_TT_HIT_REF:
    result = t;
    break;
  case AST_TT_HIT_PATH: {
    lizard_ast_node_t *src = normalize_rec(t->data.tt_hit_path.source, heap, memo);
    lizard_ast_node_t *tgt = normalize_rec(t->data.tt_hit_path.target, heap, memo);
    if (src == t->data.tt_hit_path.source && tgt == t->data.tt_hit_path.target) result = t;
    else {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_HIT_PATH;
      n->data.tt_hit_path.name = t->data.tt_hit_path.name;
      n->data.tt_hit_path.source = src;
      n->data.tt_hit_path.target = tgt;
      result = n;
    }
    break;
  }
  case AST_TT_HIT_APP: {
    lz_list_node_t *p;
    lz_list_t *new_args;
    int changed = 0;
    new_args = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
    for (p = t->data.tt_hit_app.args->head;
         p != t->data.tt_hit_app.args->nil; p = p->next) {
      lizard_ast_node_t *old = ((lizard_ast_list_node_t *)p)->ast;
      lizard_ast_node_t *new_ast = normalize_rec(old, heap, memo);
      lizard_ast_list_node_t *cell = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
      cell->ast = new_ast;
      if (new_ast != old) changed = 1;
      list_append(new_args, &cell->node);
    }
    if (!changed) result = t;
    else {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_HIT_APP;
      n->data.tt_hit_app.name = t->data.tt_hit_app.name;
      n->data.tt_hit_app.args = new_args;
      result = n;
    }
    break;
  }
  /* HIT_DECL and HIT_CONSTRUCTOR contain types-of-args which COULD
   * be normalized, but at H.1 we leave declarations literal. They're
   * metadata, not computation. */
  case AST_TT_HIT_DECL:
  case AST_TT_HIT_CONSTRUCTOR:
    result = t;
    break;
  default:
    /* No subterms to normalize. */
    result = t;
    break;
  }

  /* Then, apply head rewrites until they stop firing. Each rewrite
   * may expose new structure; we recursively normalize anything new. */
  for (;;) {
    lizard_ast_node_t *rewritten;
    changed = 0;
    rewritten = try_head_rewrites(result, heap, &changed);
    if (!changed) break;
    /* The rewritten term may need re-normalization, because the head
     * rewrite can introduce new subterms (e.g. trans-assoc creates
     * new nested trans nodes). Recurse to normalize them. */
    result = normalize_rec(rewritten, heap, memo);
  }

  memo_put(memo, t, result);
  return result;
}

/* ----- Head rewrites — applied to a term whose subterms are
 * already in normal form. Returns a possibly-rewritten term and
 * sets *changed to 1 if a rule fired. */

static lizard_ast_node_t *try_head_rewrites(lizard_ast_node_t *t,
                                            lizard_heap_t *heap,
                                            int *changed) {
  if (t == NULL) return NULL;
  switch (t->type) {
  case AST_TT_ID_SYM: {
    lizard_ast_node_t *inner = t->data.tt_id_sym.path;
    if (inner == NULL) break;
    /* sym(refl_a) --> refl_a */
    if (inner->type == AST_TT_REFL && lizard_tt_flag_get("reduce-sym-refl")) {
      *changed = 1;
      return inner;
    }
    /* sym(sym(p)) --> p */
    if (inner->type == AST_TT_ID_SYM &&
        lizard_tt_flag_get("reduce-sym-involutive")) {
      *changed = 1;
      return inner->data.tt_id_sym.path;
    }
    break;
  }
  case AST_TT_ID_TRANS: {
    lizard_ast_node_t *p = t->data.tt_id_trans.p;
    lizard_ast_node_t *q = t->data.tt_id_trans.q;
    if (p == NULL || q == NULL) break;
    /* trans(refl, p) --> p */
    if (p->type == AST_TT_REFL && lizard_tt_flag_get("reduce-trans-refl-l")) {
      *changed = 1;
      return q;
    }
    /* trans(p, refl) --> p */
    if (q->type == AST_TT_REFL && lizard_tt_flag_get("reduce-trans-refl-r")) {
      *changed = 1;
      return p;
    }
    /* trans(p, sym(p)) --> refl. ONLY when p is concretely a refl,
     * so we know the endpoint. For an abstract p (a variable, an
     * opaque path) we can't synthesize the right refl — the endpoint
     * isn't in p, it's in the Id type. Firing on abstract p would
     * produce a malformed refl (a refl whose value is a path, not a
     * term) and worse, would fire pre-substitution if p is a bound
     * variable, corrupting Lambda bodies. */
    if (q->type == AST_TT_ID_SYM &&
        p->type == AST_TT_REFL &&
        lizard_tt_structurally_equal(p, q->data.tt_id_sym.path) &&
        lizard_tt_flag_get("reduce-trans-inverse")) {
      *changed = 1;
      return p;
    }
    /* trans(sym(p), p) --> refl, same constraint */
    if (p->type == AST_TT_ID_SYM &&
        q->type == AST_TT_REFL &&
        lizard_tt_structurally_equal(p->data.tt_id_sym.path, q) &&
        lizard_tt_flag_get("reduce-trans-inverse")) {
      *changed = 1;
      return q;
    }
    /* trans(trans(p, q), r) --> trans(p, trans(q, r)) */
    if (p->type == AST_TT_ID_TRANS &&
        lizard_tt_flag_get("reduce-trans-assoc")) {
      *changed = 1;
      return make_trans(heap,
                        p->data.tt_id_trans.p,
                        make_trans(heap, p->data.tt_id_trans.q, q));
    }
    break;
  }
  case AST_TT_TRANSPORT: {
    lizard_ast_node_t *p = t->data.tt_transport.path;
    lizard_ast_node_t *v = t->data.tt_transport.value;
    if (p == NULL || v == NULL) break;
    /* transport(refl, v) --> v */
    if (p->type == AST_TT_REFL && lizard_tt_flag_get("reduce-transport-refl")) {
      *changed = 1;
      return v;
    }
    break;
  }
  case AST_TT_APP: {
    /* Pi-beta: (@ (Lambda 'x b) a) --> b[a/x]
     * This is the rule that makes function application compute. The
     * Lambda must have a symbol binder; we use capture-avoiding
     * substitution to avoid variable capture.
     *
     * Also handles equiv-fun/equiv-inv of id-equiv (Turn 9):
     *   ((equiv-fun (id-equiv A)) x)  -->  x
     *   ((equiv-inv (id-equiv A)) x)  -->  x
     */
    lizard_ast_node_t *fn = t->data.tt_app.fun;
    lizard_ast_node_t *ar = t->data.tt_app.arg;
    if (fn == NULL || ar == NULL) break;
    if ((fn->type == AST_TT_EQUIV_FUN &&
         lizard_tt_flag_get("reduce-equiv-fun-id")) ||
        (fn->type == AST_TT_EQUIV_INV &&
         lizard_tt_flag_get("reduce-equiv-inv-id"))) {
      lizard_ast_node_t *inner = fn->data.tt_equiv_op.operand;
      if (inner && inner->type == AST_TT_ID_EQUIV) {
        *changed = 1;
        return ar;
      }
    }
    if (fn->type == AST_TT_LAMBDA &&
        fn->data.tt_lambda.binder &&
        fn->data.tt_lambda.binder->type == AST_SYMBOL &&
        lizard_tt_flag_get("reduce-pi-beta")) {
      *changed = 1;
      return lizard_tt_subst(fn->data.tt_lambda.body,
                             fn->data.tt_lambda.binder->data.variable,
                             ar, heap);
    }
    break;
  }
  case AST_TT_AP: {
    /* HOTT-fragment: congruence rules for ap.
     *   ap(f, refl_a)       --> refl_{f a}
     *   ap(f, sym p)         --> sym (ap f p)
     *   ap(f, trans p q)     --> trans (ap f p) (ap f q)
     * Together these make ap a functor on the path category. */
    lizard_ast_node_t *fn = t->data.tt_ap.fn;
    lizard_ast_node_t *p  = t->data.tt_ap.path;
    if (fn == NULL || p == NULL) break;
    if (p->type == AST_TT_REFL && lizard_tt_flag_get("reduce-ap-refl")) {
      *changed = 1;
      return make_refl(heap, make_app(heap, fn, p->data.tt_refl.value));
    }
    if (p->type == AST_TT_ID_SYM && lizard_tt_flag_get("reduce-ap-sym")) {
      *changed = 1;
      return make_sym(heap, make_ap(heap, fn, p->data.tt_id_sym.path));
    }
    if (p->type == AST_TT_ID_TRANS && lizard_tt_flag_get("reduce-ap-trans")) {
      *changed = 1;
      return make_trans(heap,
                        make_ap(heap, fn, p->data.tt_id_trans.p),
                        make_ap(heap, fn, p->data.tt_id_trans.q));
    }
    break;
  }
  case AST_TT_ID: {
    /* HOTT-fragment: Id (Pi x A B) f g --> Pi x A (Id B (f x) (g x))
     * Identity on Pi types computes to a Pi of identities — the
     * computational content of functional extensionality. */
    lizard_ast_node_t *T = t->data.tt_id.domain;
    lizard_ast_node_t *f = t->data.tt_id.a;
    lizard_ast_node_t *g = t->data.tt_id.b;
    if (T == NULL || f == NULL || g == NULL) break;
    if (T->type == AST_TT_PI && lizard_tt_flag_get("reduce-id-pi")) {
      /* Build (Pi x A (Id B (f x) (g x))). We use the binder of the
       * original Pi; the codomain B may mention x, which is fine
       * because (f x) and (g x) also mention x and will substitute
       * correctly when applied. */
      lizard_ast_node_t *x_var;
      lizard_ast_node_t *fx, *gx;
      lizard_ast_node_t *new_id;
      if (T->data.tt_pi.binder == NULL ||
          T->data.tt_pi.binder->type != AST_SYMBOL) break;
      /* x_var is the symbol AST node referencing the binder name. */
      x_var = T->data.tt_pi.binder;
      fx = make_app(heap, f, x_var);
      gx = make_app(heap, g, x_var);
      new_id = make_id(heap, T->data.tt_pi.codomain, fx, gx);
      *changed = 1;
      return make_pi(heap, x_var, T->data.tt_pi.domain, new_id);
    }
    /* HOTT-fragment: Id on Sigma (non-dependent case).
     *   Id (Sigma _ A B) (pair a b) (pair a' b')
     *     --> Sigma _ (Id A a a') (\_. Id B b b')
     * Real HOTT has a dependent version requiring transport on b;
     * we handle the non-dependent case where B doesn't mention the
     * binder. The dependent case is left as future work. */
    if (T->type == AST_TT_SIGMA && f->type == AST_TT_PAIR &&
        g->type == AST_TT_PAIR &&
        lizard_tt_flag_get("reduce-id-sigma")) {
      lizard_ast_node_t *id_fst = make_id(heap, T->data.tt_sigma.domain,
                                          f->data.tt_pair.fst,
                                          g->data.tt_pair.fst);
      lizard_ast_node_t *id_snd = make_id(heap, T->data.tt_sigma.codomain,
                                          f->data.tt_pair.snd,
                                          g->data.tt_pair.snd);
      lizard_ast_node_t *new_sigma = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      new_sigma->type = AST_TT_SIGMA;
      new_sigma->data.tt_sigma.binder = T->data.tt_sigma.binder;
      new_sigma->data.tt_sigma.domain = id_fst;
      new_sigma->data.tt_sigma.codomain = id_snd;
      *changed = 1;
      return new_sigma;
    }
    /* HOTT-fragment: Id on Sum.
     *   Id (Sum A B) (inl a) (inl a') --> Id A a a'
     *   Id (Sum A B) (inr b) (inr b') --> Id B b b'
     *   Id (Sum A B) (inl _) (inr _) --> Bot
     *   Id (Sum A B) (inr _) (inl _) --> Bot
     * These are the constructor-discrimination rules. The first two
     * are "structurally equal constructors give Id at component
     * level"; the last two are the discrimination that makes inl
     * and inr distinct points of the sum. */
    if (T->type == AST_TT_SUM && lizard_tt_flag_get("reduce-id-sum")) {
      if (f->type == AST_TT_INL && g->type == AST_TT_INL) {
        *changed = 1;
        return make_id(heap, T->data.tt_sum.left,
                       f->data.tt_inj.value, g->data.tt_inj.value);
      }
      if (f->type == AST_TT_INR && g->type == AST_TT_INR) {
        *changed = 1;
        return make_id(heap, T->data.tt_sum.right,
                       f->data.tt_inj.value, g->data.tt_inj.value);
      }
      if ((f->type == AST_TT_INL && g->type == AST_TT_INR) ||
          (f->type == AST_TT_INR && g->type == AST_TT_INL)) {
        *changed = 1;
        return make_bot(heap);
      }
    }
    /* HOTT-fragment: Id on Unit.
     *   Id Unit x y --> Unit
     * Unit is contractible, so any two of its inhabitants have a
     * unique identity proof (the canonical one), reflected here by
     * the Id type itself reducing to Unit. */
    if (T->type == AST_TT_UNIT && lizard_tt_flag_get("reduce-id-unit")) {
      *changed = 1;
      return make_unit(heap);
    }
    break;
  }
  case AST_TT_FST: {
    /* fst (pair a b) --> a */
    lizard_ast_node_t *tg = t->data.tt_proj.target;
    if (tg == NULL) break;
    if (tg->type == AST_TT_PAIR && lizard_tt_flag_get("reduce-fst-beta")) {
      *changed = 1;
      return tg->data.tt_pair.fst;
    }
    break;
  }
  case AST_TT_SND: {
    /* snd (pair a b) --> b */
    lizard_ast_node_t *tg = t->data.tt_proj.target;
    if (tg == NULL) break;
    if (tg->type == AST_TT_PAIR && lizard_tt_flag_get("reduce-snd-beta")) {
      *changed = 1;
      return tg->data.tt_pair.snd;
    }
    break;
  }
  case AST_TT_CASE: {
    /* case (inl a) f g --> (@ f a)
     * case (inr b) f g --> (@ g b) */
    lizard_ast_node_t *s = t->data.tt_case.scrutinee;
    lizard_ast_node_t *l = t->data.tt_case.left_branch;
    lizard_ast_node_t *r = t->data.tt_case.right_branch;
    if (s == NULL) break;
    if (s->type == AST_TT_INL && lizard_tt_flag_get("reduce-case-beta")) {
      *changed = 1;
      return make_app(heap, l, s->data.tt_inj.value);
    }
    if (s->type == AST_TT_INR && lizard_tt_flag_get("reduce-case-beta")) {
      *changed = 1;
      return make_app(heap, r, s->data.tt_inj.value);
    }
    break;
  }
  case AST_TT_J: {
    /* J P d (refl a) --> d
     * Path induction's computation rule. On refl, the eliminator
     * returns the refl-case. The motive P is consulted for typing
     * but doesn't affect the computation when the path is refl. */
    lizard_ast_node_t *p = t->data.tt_j.path;
    if (p == NULL) break;
    if (p->type == AST_TT_REFL && lizard_tt_flag_get("reduce-j-refl")) {
      *changed = 1;
      return t->data.tt_j.refl_case;
    }
    break;
  }
  case AST_TT_XPORT: {
    /* HOTT-fragment: transport on type formers.
     *
     *   xport _ (refl _) v               --> v
     *   xport (Lambda _ Unit) p tt       --> tt
     *   xport (Lambda _ (Sum A B)) p (inl a)
     *     --> inl (xport (Lambda _ A) p a)
     *   xport (Lambda _ (Sum A B)) p (inr b)
     *     --> inr (xport (Lambda _ B) p b)
     *   xport (Lambda _ (Sigma _ A B)) p (Pair a b)
     *     --> Pair (xport (Lambda _ A) p a) (xport (Lambda _ B) p b)
     *                                                      (non-dep)
     *   xport (Lambda _ (Pi y A B)) p f
     *     --> Lambda y (xport (Lambda _ B) p (@ f y))      (non-dep)
     *
     * These are non-dependent simplifications. The dependent
     * versions for Sigma and Pi require composing the transport
     * with the binder's substitution and are deferred. */
    lizard_ast_node_t *motive = t->data.tt_xport.motive;
    lizard_ast_node_t *path   = t->data.tt_xport.path;
    lizard_ast_node_t *value  = t->data.tt_xport.value;
    lizard_ast_node_t *T;       /* the motive body */
    if (path == NULL || value == NULL) break;
    /* xport _ (refl _) v --> v */
    if (path->type == AST_TT_REFL &&
        lizard_tt_flag_get("reduce-xport-refl")) {
      *changed = 1;
      return value;
    }
    /* Per-type-former rules require the motive to be a Lambda. */
    if (motive == NULL || motive->type != AST_TT_LAMBDA) break;
    T = motive->data.tt_lambda.body;
    if (T == NULL) break;
    /* Unit: transport in a constant Unit family is tt. */
    if (T->type == AST_TT_UNIT && lizard_tt_flag_get("reduce-xport-unit")) {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_TT;
      *changed = 1;
      return n;
    }
    /* Sum: transport an inl as inl with sub-transport on A. */
    if (T->type == AST_TT_SUM && lizard_tt_flag_get("reduce-xport-sum")) {
      if (value->type == AST_TT_INL) {
        lizard_ast_node_t *sub_motive = lizard_heap_alloc(sizeof(lizard_ast_node_t));
        sub_motive->type = AST_TT_LAMBDA;
        sub_motive->data.tt_lambda.binder = motive->data.tt_lambda.binder;
        sub_motive->data.tt_lambda.body = T->data.tt_sum.left;
        *changed = 1;
        return make_inl(heap, make_xport(heap, sub_motive, path,
                                         value->data.tt_inj.value));
      }
      if (value->type == AST_TT_INR) {
        lizard_ast_node_t *sub_motive = lizard_heap_alloc(sizeof(lizard_ast_node_t));
        sub_motive->type = AST_TT_LAMBDA;
        sub_motive->data.tt_lambda.binder = motive->data.tt_lambda.binder;
        sub_motive->data.tt_lambda.body = T->data.tt_sum.right;
        *changed = 1;
        return make_inr(heap, make_xport(heap, sub_motive, path,
                                         value->data.tt_inj.value));
      }
    }
    /* Sigma (non-dep): transport a Pair componentwise. */
    if (T->type == AST_TT_SIGMA && value->type == AST_TT_PAIR &&
        lizard_tt_flag_get("reduce-xport-sigma")) {
      lizard_ast_node_t *motive_fst = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      lizard_ast_node_t *motive_snd = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      motive_fst->type = AST_TT_LAMBDA;
      motive_fst->data.tt_lambda.binder = motive->data.tt_lambda.binder;
      motive_fst->data.tt_lambda.body = T->data.tt_sigma.domain;
      motive_snd->type = AST_TT_LAMBDA;
      motive_snd->data.tt_lambda.binder = motive->data.tt_lambda.binder;
      motive_snd->data.tt_lambda.body = T->data.tt_sigma.codomain;
      *changed = 1;
      return make_pair(heap,
                       make_xport(heap, motive_fst, path,
                                  value->data.tt_pair.fst),
                       make_xport(heap, motive_snd, path,
                                  value->data.tt_pair.snd));
    }
    /* Pi (non-dep): transport a function pointwise.
     *   xport (Lambda _ (Pi y A B)) p f
     *     --> Lambda y (xport (Lambda _ B) p (@ f y))
     * Note: this assumes B doesn't depend on y (non-dependent Pi).
     * The dependent case requires transport on the argument too. */
    if (T->type == AST_TT_PI && lizard_tt_flag_get("reduce-xport-pi")) {
      lizard_ast_node_t *y = T->data.tt_pi.binder;
      lizard_ast_node_t *cod_motive;
      lizard_ast_node_t *body;
      if (y == NULL || y->type != AST_SYMBOL) break;
      cod_motive = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      cod_motive->type = AST_TT_LAMBDA;
      cod_motive->data.tt_lambda.binder = motive->data.tt_lambda.binder;
      cod_motive->data.tt_lambda.body = T->data.tt_pi.codomain;
      body = make_xport(heap, cod_motive, path, make_app(heap, value, y));
      {
        lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
        n->type = AST_TT_LAMBDA;
        n->data.tt_lambda.binder = y;
        n->data.tt_lambda.body = body;
        *changed = 1;
        return n;
      }
    }
    break;
  }
  case AST_TT_U_SUC: {
    /* (U-suc (U n)) --> (U n+1) */
    lizard_ast_node_t *u = t->data.tt_u_suc.operand;
    if (u == NULL) break;
    if (u->type == AST_TT_UNIVERSE &&
        lizard_tt_flag_get("reduce-u-suc-concrete")) {
      *changed = 1;
      return make_universe(heap, u->data.tt_universe.level + 1);
    }
    break;
  }
  case AST_TT_U_MAX: {
    /* Concrete: (U-max (U n) (U m)) --> (U max(n,m)) */
    lizard_ast_node_t *l = t->data.tt_u_max.left;
    lizard_ast_node_t *r = t->data.tt_u_max.right;
    if (l == NULL || r == NULL) break;
    /* Phase L.2: (U-max (U-set ...) (U-set ...)) --> set union.
     * The lattice join is set union; this is where the multi-dim
     * lattice actually does its thing. */
    if (l->type == AST_TT_UNIVERSE_SET && r->type == AST_TT_UNIVERSE_SET &&
        lizard_tt_flag_get("reduce-u-set-concrete")) {
      *changed = 1;
      return universe_set_union(heap, l, r);
    }
    /* Mixed: lift single-nat universe to singleton set so we can
     * combine with a set on the other side. */
    if (l->type == AST_TT_UNIVERSE && r->type == AST_TT_UNIVERSE_SET &&
        lizard_tt_flag_get("reduce-u-set-concrete")) {
      long *dim = lizard_heap_alloc(sizeof(long));
      lizard_ast_node_t *lift;
      dim[0] = l->data.tt_universe.level;
      lift = make_universe_set(heap, dim, 1);
      *changed = 1;
      return universe_set_union(heap, lift, r);
    }
    if (l->type == AST_TT_UNIVERSE_SET && r->type == AST_TT_UNIVERSE &&
        lizard_tt_flag_get("reduce-u-set-concrete")) {
      long *dim = lizard_heap_alloc(sizeof(long));
      lizard_ast_node_t *lift;
      dim[0] = r->data.tt_universe.level;
      lift = make_universe_set(heap, dim, 1);
      *changed = 1;
      return universe_set_union(heap, l, lift);
    }
    if (l->type == AST_TT_UNIVERSE && r->type == AST_TT_UNIVERSE &&
        lizard_tt_flag_get("reduce-u-max-concrete")) {
      long ll = l->data.tt_universe.level;
      long rr = r->data.tt_universe.level;
      *changed = 1;
      return make_universe(heap, ll > rr ? ll : rr);
    }
    /* Idempotence: (U-max u u) --> u (alpha-equal check) */
    if (lizard_tt_alpha_equal(l, r) &&
        lizard_tt_flag_get("reduce-u-max-idem")) {
      *changed = 1;
      return l;
    }
    /* (U-max (U 0) u) --> u, (U-max u (U 0)) --> u (0 is bottom) */
    if (l->type == AST_TT_UNIVERSE && l->data.tt_universe.level == 0 &&
        lizard_tt_flag_get("reduce-u-max-concrete")) {
      *changed = 1;
      return r;
    }
    if (r->type == AST_TT_UNIVERSE && r->data.tt_universe.level == 0 &&
        lizard_tt_flag_get("reduce-u-max-concrete")) {
      *changed = 1;
      return l;
    }
    /* Absorption: (U-max u (U-min u v)) --> u
     *             (U-max (U-min u v) u) --> u */
    if (r->type == AST_TT_U_MIN &&
        lizard_tt_flag_get("reduce-u-lattice-absorb") &&
        (lizard_tt_alpha_equal(l, r->data.tt_u_min.left) ||
         lizard_tt_alpha_equal(l, r->data.tt_u_min.right))) {
      *changed = 1;
      return l;
    }
    if (l->type == AST_TT_U_MIN &&
        lizard_tt_flag_get("reduce-u-lattice-absorb") &&
        (lizard_tt_alpha_equal(r, l->data.tt_u_min.left) ||
         lizard_tt_alpha_equal(r, l->data.tt_u_min.right))) {
      *changed = 1;
      return r;
    }
    break;
  }
  case AST_TT_U_MIN: {
    /* Meet (greatest lower bound). Dual lattice operation to U-max.
     *
     *   - Concrete: (U-min (U n) (U m)) --> (U min(n,m))
     *   - Idempotence: (U-min u u) --> u
     *   - Bottom absorption: (U-min (U 0) u) --> (U 0)
     *     (since 0 is the bottom, meeting with bottom gives bottom)
     *   - Absorption with U-max: (U-min u (U-max u v)) --> u
     *
     * Note the bottom-absorption is the DUAL of U-max's: in U-max,
     * (U 0) is the unit (max with bottom is identity). In U-min,
     * (U 0) is the annihilator (min with bottom is bottom).
     */
    lizard_ast_node_t *l = t->data.tt_u_min.left;
    lizard_ast_node_t *r = t->data.tt_u_min.right;
    if (l == NULL || r == NULL) break;
    /* Phase L.2: (U-min (U-set ...) (U-set ...)) --> set intersection.
     * The lattice meet is set intersection. */
    if (l->type == AST_TT_UNIVERSE_SET && r->type == AST_TT_UNIVERSE_SET &&
        lizard_tt_flag_get("reduce-u-set-concrete")) {
      *changed = 1;
      return universe_set_intersect(heap, l, r);
    }
    if (l->type == AST_TT_UNIVERSE && r->type == AST_TT_UNIVERSE_SET &&
        lizard_tt_flag_get("reduce-u-set-concrete")) {
      long *dim = lizard_heap_alloc(sizeof(long));
      lizard_ast_node_t *lift;
      dim[0] = l->data.tt_universe.level;
      lift = make_universe_set(heap, dim, 1);
      *changed = 1;
      return universe_set_intersect(heap, lift, r);
    }
    if (l->type == AST_TT_UNIVERSE_SET && r->type == AST_TT_UNIVERSE &&
        lizard_tt_flag_get("reduce-u-set-concrete")) {
      long *dim = lizard_heap_alloc(sizeof(long));
      lizard_ast_node_t *lift;
      dim[0] = r->data.tt_universe.level;
      lift = make_universe_set(heap, dim, 1);
      *changed = 1;
      return universe_set_intersect(heap, l, lift);
    }
    if (l->type == AST_TT_UNIVERSE && r->type == AST_TT_UNIVERSE &&
        lizard_tt_flag_get("reduce-u-min-concrete")) {
      long ll = l->data.tt_universe.level;
      long rr = r->data.tt_universe.level;
      *changed = 1;
      return make_universe(heap, ll < rr ? ll : rr);
    }
    if (lizard_tt_alpha_equal(l, r) &&
        lizard_tt_flag_get("reduce-u-min-idem")) {
      *changed = 1;
      return l;
    }
    /* Bottom absorption: meet with (U 0) is (U 0). */
    if (l->type == AST_TT_UNIVERSE && l->data.tt_universe.level == 0 &&
        lizard_tt_flag_get("reduce-u-min-concrete")) {
      *changed = 1;
      return l;
    }
    if (r->type == AST_TT_UNIVERSE && r->data.tt_universe.level == 0 &&
        lizard_tt_flag_get("reduce-u-min-concrete")) {
      *changed = 1;
      return r;
    }
    /* Absorption: (U-min u (U-max u v)) --> u
     *             (U-min (U-max u v) u) --> u */
    if (r->type == AST_TT_U_MAX &&
        lizard_tt_flag_get("reduce-u-lattice-absorb") &&
        (lizard_tt_alpha_equal(l, r->data.tt_u_max.left) ||
         lizard_tt_alpha_equal(l, r->data.tt_u_max.right))) {
      *changed = 1;
      return l;
    }
    if (l->type == AST_TT_U_MAX &&
        lizard_tt_flag_get("reduce-u-lattice-absorb") &&
        (lizard_tt_alpha_equal(r, l->data.tt_u_max.left) ||
         lizard_tt_alpha_equal(r, l->data.tt_u_max.right))) {
      *changed = 1;
      return r;
    }
    break;
  }
  case AST_TT_CO_MAX: {
    /* Phase L.4: join in the couniverse lattice. Mirrors U-max but on
     * COUNIVERSE_SET only. We REFUSE to operate on UNIVERSE_SET values
     * here: the two lattices are kept separate. Mixed-kind Co-max
     * stays unreduced as a structural marker; the typing rule will
     * reject it at check time. */
    lizard_ast_node_t *l = t->data.tt_co_max.left;
    lizard_ast_node_t *r = t->data.tt_co_max.right;
    if (l == NULL || r == NULL) break;
    if (l->type == AST_TT_COUNIVERSE_SET && r->type == AST_TT_COUNIVERSE_SET &&
        lizard_tt_flag_get("reduce-co-set-concrete")) {
      *changed = 1;
      return couniverse_set_union(heap, l, r);
    }
    if (lizard_tt_alpha_equal(l, r) &&
        lizard_tt_flag_get("reduce-co-max-idem")) {
      *changed = 1;
      return l;
    }
    /* Absorption: (Co-max u (Co-min u v)) → u */
    if (r->type == AST_TT_CO_MIN &&
        lizard_tt_flag_get("reduce-co-lattice-absorb") &&
        (lizard_tt_alpha_equal(l, r->data.tt_co_min.left) ||
         lizard_tt_alpha_equal(l, r->data.tt_co_min.right))) {
      *changed = 1;
      return l;
    }
    if (l->type == AST_TT_CO_MIN &&
        lizard_tt_flag_get("reduce-co-lattice-absorb") &&
        (lizard_tt_alpha_equal(r, l->data.tt_co_min.left) ||
         lizard_tt_alpha_equal(r, l->data.tt_co_min.right))) {
      *changed = 1;
      return r;
    }
    break;
  }
  case AST_TT_CO_MIN: {
    /* Phase L.4: meet in the couniverse lattice. */
    lizard_ast_node_t *l = t->data.tt_co_min.left;
    lizard_ast_node_t *r = t->data.tt_co_min.right;
    if (l == NULL || r == NULL) break;
    if (l->type == AST_TT_COUNIVERSE_SET && r->type == AST_TT_COUNIVERSE_SET &&
        lizard_tt_flag_get("reduce-co-set-concrete")) {
      *changed = 1;
      return couniverse_set_intersect(heap, l, r);
    }
    if (lizard_tt_alpha_equal(l, r) &&
        lizard_tt_flag_get("reduce-co-min-idem")) {
      *changed = 1;
      return l;
    }
    /* Absorption with Co-max */
    if (r->type == AST_TT_CO_MAX &&
        lizard_tt_flag_get("reduce-co-lattice-absorb") &&
        (lizard_tt_alpha_equal(l, r->data.tt_co_max.left) ||
         lizard_tt_alpha_equal(l, r->data.tt_co_max.right))) {
      *changed = 1;
      return l;
    }
    if (l->type == AST_TT_CO_MAX &&
        lizard_tt_flag_get("reduce-co-lattice-absorb") &&
        (lizard_tt_alpha_equal(r, l->data.tt_co_max.left) ||
         lizard_tt_alpha_equal(r, l->data.tt_co_max.right))) {
      *changed = 1;
      return r;
    }
    break;
  }
  case AST_TT_I_AND: {
    /* Interval algebra for ∧:
     *   i0 ∧ _  --> i0       _ ∧ i0  --> i0
     *   i1 ∧ j  --> j        j ∧ i1  --> j
     *   i ∧ i   --> i        (idempotence)
     */
    lizard_ast_node_t *l = t->data.tt_i_binop.left;
    lizard_ast_node_t *r = t->data.tt_i_binop.right;
    if (l == NULL || r == NULL) break;
    if (!lizard_tt_flag_get("reduce-i-and")) break;
    if (l->type == AST_TT_I0 || r->type == AST_TT_I0) {
      *changed = 1;
      return make_i0(heap);
    }
    if (l->type == AST_TT_I1) {
      *changed = 1;
      return r;
    }
    if (r->type == AST_TT_I1) {
      *changed = 1;
      return l;
    }
    if (lizard_tt_alpha_equal(l, r)) {
      *changed = 1;
      return l;
    }
    break;
  }
  case AST_TT_I_OR: {
    /* Interval algebra for ∨:
     *   i1 ∨ _  --> i1       _ ∨ i1  --> i1
     *   i0 ∨ j  --> j        j ∨ i0  --> j
     *   i ∨ i   --> i        (idempotence)
     */
    lizard_ast_node_t *l = t->data.tt_i_binop.left;
    lizard_ast_node_t *r = t->data.tt_i_binop.right;
    if (l == NULL || r == NULL) break;
    if (!lizard_tt_flag_get("reduce-i-or")) break;
    if (l->type == AST_TT_I1 || r->type == AST_TT_I1) {
      *changed = 1;
      return make_i1(heap);
    }
    if (l->type == AST_TT_I0) {
      *changed = 1;
      return r;
    }
    if (r->type == AST_TT_I0) {
      *changed = 1;
      return l;
    }
    if (lizard_tt_alpha_equal(l, r)) {
      *changed = 1;
      return l;
    }
    break;
  }
  case AST_TT_I_NEG: {
    /* Interval negation:
     *   ~ i0     --> i1
     *   ~ i1     --> i0
     *   ~ (~ i)  --> i        (involution)
     */
    lizard_ast_node_t *o = t->data.tt_i_neg.operand;
    if (o == NULL) break;
    if (!lizard_tt_flag_get("reduce-i-neg")) break;
    if (o->type == AST_TT_I0) {
      *changed = 1;
      return make_i1(heap);
    }
    if (o->type == AST_TT_I1) {
      *changed = 1;
      return make_i0(heap);
    }
    if (o->type == AST_TT_I_NEG) {
      *changed = 1;
      return o->data.tt_i_neg.operand;
    }
    break;
  }
  case AST_TT_PATH_APP: {
    /* Path-app beta:
     *   ((<i> body) @ j)  -->  body[j/i]
     * Path application of an abstraction substitutes interval term j
     * for the interval variable i in body. This is the cubical
     * analog of pi-beta.
     *
     * Also handles the ua-endpoint rule (Turn 10):
     *   (ua (id-equiv A)) @ i  -->  A
     * For the identity equivalence, ua gives the reflexive path on A.
     */
    lizard_ast_node_t *p = t->data.tt_path_app.path;
    lizard_ast_node_t *i = t->data.tt_path_app.point;
    if (p == NULL || i == NULL) break;
    if (p->type == AST_TT_UA &&
        lizard_tt_flag_get("reduce-ua-endpoints")) {
      lizard_ast_node_t *e = p->data.tt_ua.equiv;
      if (e && e->type == AST_TT_ID_EQUIV) {
        *changed = 1;
        return e->data.tt_equiv_op.operand;
      }
    }
    if (!lizard_tt_flag_get("reduce-path-beta")) break;
    if (p->type == AST_TT_PATH_ABS &&
        p->data.tt_path_abs.binder &&
        p->data.tt_path_abs.binder->type == AST_SYMBOL) {
      *changed = 1;
      return subst_interval(p->data.tt_path_abs.body,
                            p->data.tt_path_abs.binder->data.variable,
                            i, heap);
    }
    break;
  }
  case AST_TT_F_EQ: {
    /* Face equation simplification:
     *   (F-eq i0 i0) --> F1     (F-eq i1 i1) --> F1
     *   (F-eq i0 i1) --> F0     (F-eq i1 i0) --> F0
     *   (F-eq i i)   --> F1     (reflexivity, modulo alpha-equality)
     *
     * Connection-on-face rules:
     *   (F-eq (I-and i j) i0)  -->  (F-or (F-eq i i0) (F-eq j i0))
     *   (F-eq (I-and i j) i1)  -->  (F-and (F-eq i i1) (F-eq j i1))
     *   (F-eq (I-or i j) i0)   -->  (F-and (F-eq i i0) (F-eq j i0))
     *   (F-eq (I-or i j) i1)   -->  (F-or (F-eq i i1) (F-eq j i1))
     *   (F-eq (I-neg i) i0)    -->  (F-eq i i1)
     *   (F-eq (I-neg i) i1)    -->  (F-eq i i0)
     * (and symmetric versions with endpoint on left)
     */
    lizard_ast_node_t *l = t->data.tt_f_eq.left;
    lizard_ast_node_t *r = t->data.tt_f_eq.right;
    if (l == NULL || r == NULL) break;
    if (lizard_tt_flag_get("reduce-f-eq-concrete")) {
      /* Both concrete endpoints */
      if (l->type == AST_TT_I0 && r->type == AST_TT_I0) {
        *changed = 1; return make_f1(heap);
      }
      if (l->type == AST_TT_I1 && r->type == AST_TT_I1) {
        *changed = 1; return make_f1(heap);
      }
      if ((l->type == AST_TT_I0 && r->type == AST_TT_I1) ||
          (l->type == AST_TT_I1 && r->type == AST_TT_I0)) {
        *changed = 1; return make_f0(heap);
      }
      /* Reflexivity: same interval term on both sides */
      if (lizard_tt_alpha_equal(l, r)) {
        *changed = 1; return make_f1(heap);
      }
    }
    if (lizard_tt_flag_get("reduce-f-conn-eq")) {
      /* Connection-on-face rules. Normalize so endpoint is on the
       * right (swap if needed). */
      lizard_ast_node_t *expr = l;
      lizard_ast_node_t *ep = r;
      if (ep->type != AST_TT_I0 && ep->type != AST_TT_I1) {
        /* Maybe the endpoint is on the left — swap. */
        if (l->type == AST_TT_I0 || l->type == AST_TT_I1) {
          expr = r;
          ep = l;
        } else {
          break;
        }
      }
      /* (F-eq (I-and i j) i0) --> (F-or (F-eq i i0) (F-eq j i0)) */
      if (expr->type == AST_TT_I_AND && ep->type == AST_TT_I0) {
        *changed = 1;
        return make_f_or(heap,
                         make_f_eq(heap, expr->data.tt_i_binop.left, ep),
                         make_f_eq(heap, expr->data.tt_i_binop.right, ep));
      }
      /* (F-eq (I-and i j) i1) --> (F-and (F-eq i i1) (F-eq j i1)) */
      if (expr->type == AST_TT_I_AND && ep->type == AST_TT_I1) {
        *changed = 1;
        return make_f_and(heap,
                          make_f_eq(heap, expr->data.tt_i_binop.left, ep),
                          make_f_eq(heap, expr->data.tt_i_binop.right, ep));
      }
      /* (F-eq (I-or i j) i0) --> (F-and (F-eq i i0) (F-eq j i0)) */
      if (expr->type == AST_TT_I_OR && ep->type == AST_TT_I0) {
        *changed = 1;
        return make_f_and(heap,
                          make_f_eq(heap, expr->data.tt_i_binop.left, ep),
                          make_f_eq(heap, expr->data.tt_i_binop.right, ep));
      }
      /* (F-eq (I-or i j) i1) --> (F-or (F-eq i i1) (F-eq j i1)) */
      if (expr->type == AST_TT_I_OR && ep->type == AST_TT_I1) {
        *changed = 1;
        return make_f_or(heap,
                         make_f_eq(heap, expr->data.tt_i_binop.left, ep),
                         make_f_eq(heap, expr->data.tt_i_binop.right, ep));
      }
      /* (F-eq (I-neg i) i0) --> (F-eq i i1)
       * (F-eq (I-neg i) i1) --> (F-eq i i0) */
      if (expr->type == AST_TT_I_NEG) {
        if (ep->type == AST_TT_I0) {
          *changed = 1;
          return make_f_eq(heap, expr->data.tt_i_neg.operand, make_i1(heap));
        }
        if (ep->type == AST_TT_I1) {
          *changed = 1;
          return make_f_eq(heap, expr->data.tt_i_neg.operand, make_i0(heap));
        }
      }
    }
    break;
  }
  case AST_TT_F_AND: {
    /* Face conjunction:
     *   F1 ∧ φ --> φ     φ ∧ F1 --> φ
     *   F0 ∧ φ --> F0    φ ∧ F0 --> F0
     *   φ ∧ φ --> φ      (idempotence)
     */
    lizard_ast_node_t *l = t->data.tt_f_binop.left;
    lizard_ast_node_t *r = t->data.tt_f_binop.right;
    if (l == NULL || r == NULL) break;
    if (!lizard_tt_flag_get("reduce-f-and")) break;
    if (l->type == AST_TT_F0 || r->type == AST_TT_F0) {
      *changed = 1; return make_f0(heap);
    }
    if (l->type == AST_TT_F1) { *changed = 1; return r; }
    if (r->type == AST_TT_F1) { *changed = 1; return l; }
    if (lizard_tt_alpha_equal(l, r)) {
      *changed = 1; return l;
    }
    break;
  }
  case AST_TT_F_OR: {
    /* Face disjunction:
     *   F1 ∨ φ --> F1    φ ∨ F1 --> F1
     *   F0 ∨ φ --> φ     φ ∨ F0 --> φ
     *   φ ∨ φ --> φ      (idempotence)
     */
    lizard_ast_node_t *l = t->data.tt_f_binop.left;
    lizard_ast_node_t *r = t->data.tt_f_binop.right;
    if (l == NULL || r == NULL) break;
    if (!lizard_tt_flag_get("reduce-f-or")) break;
    if (l->type == AST_TT_F1 || r->type == AST_TT_F1) {
      *changed = 1; return make_f1(heap);
    }
    if (l->type == AST_TT_F0) { *changed = 1; return r; }
    if (r->type == AST_TT_F0) { *changed = 1; return l; }
    if (lizard_tt_alpha_equal(l, r)) {
      *changed = 1; return l;
    }
    break;
  }
  case AST_TT_COMP: {
    /* Kan composition:
     *
     *   comp A [φ ↦ u] u0  :  A @ i1
     *
     * where A is an interval-indexed family (path-abs over types),
     * u is a partial element along the line (defined where φ holds),
     * and u0 is the bottom face. The result lives in A @ i1.
     *
     * Reduction rules:
     *
     *   1. φ = F1 boundary: comp A [F1 ↦ u] u0  -->  (u @ i1)
     *      When the partial element is total, just take its top face.
     *
     *   2. comp on Pi (non-dep): the type family is a path-abs whose
     *      body reduces to (Pi x A B). For non-dependent B, the result
     *      is a Lambda over A's domain.
     *      [The dependent case needs transport on the argument; we
     *       handle that when transport-along-interval is in.]
     *
     *   3. comp on Sigma (non-dep): the type family path-abs body is
     *      (Sigma _ A B). Result is a Pair of two comp's.
     *
     *   4. comp on Path: type family body is (Path C a b). Result is
     *      a path-abs whose body is comp over C.
     *
     *   5. comp on Unit: result is tt.
     *
     *   6. comp on Bot: vacuous (no element to compose); we don't
     *      add a rule for this — let it stay unreduced.
     */
    lizard_ast_node_t *type_family = t->data.tt_comp.type_family;
    lizard_ast_node_t *face = t->data.tt_comp.face;
    lizard_ast_node_t *partial = t->data.tt_comp.partial;
    /* unused in some cases */
    lizard_ast_node_t *base = t->data.tt_comp.base;
    lizard_ast_node_t *family_body;
    if (type_family == NULL || face == NULL || partial == NULL ||
        base == NULL) break;
    /* Rule 1: φ = F1 — partial is total */
    if (face->type == AST_TT_F1 && lizard_tt_flag_get("reduce-comp-f1")) {
      *changed = 1;
      return make_path_app(heap, partial, make_i1(heap));
    }
    /* Rule 1a (Turn 11): empty partial or F0 face means "no
     * non-base contribution"; the result is the base transported.
     * For a constant family this is just the base itself. We check
     * the family is a path-abs whose body doesn't depend on the
     * binder — i.e. constant — in which case base is already the
     * answer at any interval point. */
    if ((partial->type == AST_TT_SYSTEM_NIL || face->type == AST_TT_F0) &&
        lizard_tt_flag_get("reduce-system-nil-comp")) {
      if (type_family->type == AST_TT_PATH_ABS) {
        lizard_ast_node_t *binder = type_family->data.tt_path_abs.binder;
        lizard_ast_node_t *body = type_family->data.tt_path_abs.body;
        if (binder && binder->type == AST_SYMBOL && body) {
          /* Check whether the body uses the binder as an interval
           * variable. If not, the family is constant and the comp
           * result is base. */
          /* Conservative check: contains_free_var checks term-level;
           * for interval vars we approximate by structural inspection. */
          int uses_binder = 0;
          {
            /* Substitute i0 and i1; if results equal, family is constant. */
            lizard_ast_node_t *at_0 = subst_interval(body, binder->data.variable,
                                                    make_i0(heap), heap);
            lizard_ast_node_t *at_1 = subst_interval(body, binder->data.variable,
                                                    make_i1(heap), heap);
            if (!lizard_tt_alpha_equal(at_0, at_1)) uses_binder = 1;
          }
          if (!uses_binder) {
            *changed = 1;
            return base;
          }
        }
      }
    }
    /* For type-former rules, the family must be a path-abs and we
     * inspect its body. */
    if (type_family->type != AST_TT_PATH_ABS) break;
    family_body = type_family->data.tt_path_abs.body;
    if (family_body == NULL) break;
    /* Rule 5: comp Unit --> tt */
    if (family_body->type == AST_TT_UNIT &&
        lizard_tt_flag_get("reduce-comp-unit")) {
      lizard_ast_node_t *tt_node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      tt_node->type = AST_TT_TT;
      *changed = 1;
      return tt_node;
    }
    /* Rule 3: comp Sigma (non-dep)
     *   comp^i (Sigma _ A B) [φ ↦ u] (Pair a0 b0)
     *     --> Pair (comp^i A [φ ↦ <j> fst (u @ j)] a0)
     *              (comp^i B [φ ↦ <j> snd (u @ j)] b0)
     *
     * We need <j> to be a *fresh* interval variable to avoid capture.
     * For simplicity we reuse the family's binder name — assuming the
     * partial doesn't already bind it. (Sound when names are fresh,
     * which they typically are in practice.) */
    if (family_body->type == AST_TT_SIGMA && base->type == AST_TT_PAIR &&
        lizard_tt_flag_get("reduce-comp-sigma")) {
      lizard_ast_node_t *i_binder = type_family->data.tt_path_abs.binder;
      lizard_ast_node_t *A_family, *B_family;
      lizard_ast_node_t *fst_partial, *snd_partial;
      lizard_ast_node_t *fst_comp, *snd_comp;
      /* A_family = path-abs i (A) where A is the Sigma's domain. */
      A_family = make_path_abs(heap, i_binder, family_body->data.tt_sigma.domain);
      B_family = make_path_abs(heap, i_binder, family_body->data.tt_sigma.codomain);
      /* Build the partials: <j> fst (u @ j) and <j> snd (u @ j).
       * We reuse the family's binder name. */
      {
        lizard_ast_node_t *j_var = lizard_heap_alloc(sizeof(lizard_ast_node_t));
        lizard_ast_node_t *u_j, *u_j_fst, *u_j_snd;
        if (i_binder == NULL || i_binder->type != AST_SYMBOL) break;
        j_var->type = AST_TT_I_VAR;
        j_var->data.tt_i_var.name = i_binder->data.variable;
        u_j = make_path_app(heap, partial, j_var);
        u_j_fst = lizard_heap_alloc(sizeof(lizard_ast_node_t));
        u_j_fst->type = AST_TT_FST;
        u_j_fst->data.tt_proj.target = u_j;
        u_j_snd = lizard_heap_alloc(sizeof(lizard_ast_node_t));
        u_j_snd->type = AST_TT_SND;
        u_j_snd->data.tt_proj.target = u_j;
        fst_partial = make_path_abs(heap, i_binder, u_j_fst);
        snd_partial = make_path_abs(heap, i_binder, u_j_snd);
      }
      fst_comp = make_comp(heap, AST_TT_COMP, A_family, face, fst_partial,
                           base->data.tt_pair.fst);
      snd_comp = make_comp(heap, AST_TT_COMP, B_family, face, snd_partial,
                           base->data.tt_pair.snd);
      *changed = 1;
      {
        lizard_ast_node_t *p = lizard_heap_alloc(sizeof(lizard_ast_node_t));
        p->type = AST_TT_PAIR;
        p->data.tt_pair.fst = fst_comp;
        p->data.tt_pair.snd = snd_comp;
        return p;
      }
    }
    /* Rule 3b: comp Sum (non-dep) — base is canonical Inl or Inr
     *
     *   comp^i (Sum A B) [φ ↦ u] (Inl a0)  -->  Inl (comp^i A [...] a0)
     *   comp^i (Sum A B) [φ ↦ u] (Inr b0)  -->  Inr (comp^i B [...] b0)
     *
     * When the base is canonical we can push comp inside. The partial
     * element u would need to be unwrapped at each face — for canonical
     * bases we conservatively assume the partial is also of the same
     * shape (this is sound if the partial's face is the empty face,
     * which is the common case for transport). */
    if (family_body->type == AST_TT_SUM &&
        (base->type == AST_TT_INL || base->type == AST_TT_INR) &&
        lizard_tt_flag_get("reduce-comp-sigma")) {
      lizard_ast_node_t *i_binder = type_family->data.tt_path_abs.binder;
      lizard_ast_node_t *side_family;
      lizard_ast_node_t *inner_comp;
      lizard_ast_node_t *result;
      int is_inl = (base->type == AST_TT_INL);
      /* For empty partial (system-nil) or F0 face, transport is
       * straightforward — push through. For other partials we'd need
       * the case analysis at each face which requires more machinery. */
      if (partial->type != AST_TT_SYSTEM_NIL && face->type != AST_TT_F0) break;
      if (i_binder == NULL || i_binder->type != AST_SYMBOL) break;
      side_family = make_path_abs(heap, i_binder,
                                  is_inl ? family_body->data.tt_sum.left
                                         : family_body->data.tt_sum.right);
      inner_comp = make_comp(heap, AST_TT_COMP, side_family, face,
                             partial, base->data.tt_inj.value);
      result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      result->type = base->type;  /* same Inl or Inr */
      result->data.tt_inj.value = inner_comp;
      *changed = 1;
      return result;
    }
    /* Rule 2: comp Pi (non-dep)
     *   comp^i (Pi x A B) [φ ↦ u] u0
     *     --> Lambda y. comp^i B [φ ↦ <j> (u @ j) y] (u0 y)
     *
     * For the *non-dependent* case where B doesn't mention x. The
     * fully-dependent case needs transport on the argument which is
     * left for a future refinement. */
    if (family_body->type == AST_TT_PI &&
        lizard_tt_flag_get("reduce-comp-pi")) {
      lizard_ast_node_t *i_binder = type_family->data.tt_path_abs.binder;
      lizard_ast_node_t *x_binder = family_body->data.tt_pi.binder;
      lizard_ast_node_t *B = family_body->data.tt_pi.codomain;
      lizard_ast_node_t *B_family;
      lizard_ast_node_t *new_partial, *new_base;
      lizard_ast_node_t *body_app, *result_lambda;
      lizard_ast_node_t *y_var;
      if (i_binder == NULL || i_binder->type != AST_SYMBOL) break;
      if (x_binder == NULL || x_binder->type != AST_SYMBOL) break;
      /* For non-dependence we require that B doesn't mention x.
       * (If it does, we'd need the dependent rule.) Check
       * conservatively: only fire if B doesn't contain x. */
      if (contains_free_var(B, x_binder->data.variable)) break;
      y_var = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      y_var->type = AST_SYMBOL;
      y_var->data.variable = x_binder->data.variable;
      B_family = make_path_abs(heap, i_binder, B);
      /* new_partial: <j> (u @ j) y */
      {
        lizard_ast_node_t *j_var = lizard_heap_alloc(sizeof(lizard_ast_node_t));
        lizard_ast_node_t *u_j, *apply;
        j_var->type = AST_TT_I_VAR;
        j_var->data.tt_i_var.name = i_binder->data.variable;
        u_j = make_path_app(heap, partial, j_var);
        apply = make_app(heap, u_j, y_var);
        new_partial = make_path_abs(heap, i_binder, apply);
      }
      /* new_base: u0 y */
      new_base = make_app(heap, base, y_var);
      body_app = make_comp(heap, AST_TT_COMP, B_family, face,
                           new_partial, new_base);
      result_lambda = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      result_lambda->type = AST_TT_LAMBDA;
      result_lambda->data.tt_lambda.binder = x_binder;
      result_lambda->data.tt_lambda.body = body_app;
      *changed = 1;
      return result_lambda;
    }
    /* Rule 4: comp Path (now possible with systems, Turn 11+)
     *
     *   comp^i (Path C a b) [φ ↦ u] u0
     *     -->  path-abs k.
     *           comp^i C [φ ↦ <j>((u @ j) @ k),
     *                     (k=i0) ↦ a,
     *                     (k=i1) ↦ b] (u0 @ k)
     *
     * The new path-abs binds k. The inner comp has a three-clause
     * system: the original partial (continued via path-app on k),
     * plus two new clauses giving the path's endpoints.
     */
    if (family_body->type == AST_TT_PATH &&
        lizard_tt_flag_get("reduce-comp-path")) {
      lizard_ast_node_t *i_binder = type_family->data.tt_path_abs.binder;
      lizard_ast_node_t *C = family_body->data.tt_path.domain;
      lizard_ast_node_t *a = family_body->data.tt_path.a;
      lizard_ast_node_t *b = family_body->data.tt_path.b;
      lizard_ast_node_t *k_var, *k_interval;
      lizard_ast_node_t *C_family;
      lizard_ast_node_t *new_partial;
      lizard_ast_node_t *new_base;
      lizard_ast_node_t *inner_comp;
      lizard_ast_node_t *result;
      char *kname;
      if (i_binder == NULL || i_binder->type != AST_SYMBOL) break;
      /* Fresh interval var k. Use a distinct name to avoid collision
       * with i. */
      kname = lizard_heap_alloc(3);
      strcpy(kname, "k");
      k_var = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      k_var->type = AST_SYMBOL;
      k_var->data.variable = kname;
      k_interval = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      k_interval->type = AST_TT_I_VAR;
      k_interval->data.tt_i_var.name = kname;
      /* C-family for inner comp */
      C_family = make_path_abs(heap, i_binder, C);
      /* Build the inner partial as a system with three clauses:
       *   1. The original φ ↦ <j>((u @ j) @ k)
       *   2. (k = i0) ↦ a
       *   3. (k = i1) ↦ b
       * The face for the first clause is φ unchanged. */
      {
        lizard_ast_node_t *j_var = lizard_heap_alloc(sizeof(lizard_ast_node_t));
        lizard_ast_node_t *u_j, *u_j_at_k, *first_value;
        lizard_ast_node_t *k_eq_i0, *k_eq_i1;
        j_var->type = AST_TT_I_VAR;
        j_var->data.tt_i_var.name = i_binder->data.variable;
        u_j = make_path_app(heap, partial, j_var);
        u_j_at_k = make_path_app(heap, u_j, k_interval);
        first_value = make_path_abs(heap, i_binder, u_j_at_k);
        k_eq_i0 = make_f_eq(heap, k_interval, make_i0(heap));
        k_eq_i1 = make_f_eq(heap, k_interval, make_i1(heap));
        /* system-cons (k = i0) <i>a (system-cons (k = i1) <i>b
         *  (system-cons φ first_value system-nil)) */
        new_partial = make_system_cons(heap, k_eq_i0,
                                       make_path_abs(heap, i_binder, a),
          make_system_cons(heap, k_eq_i1,
                           make_path_abs(heap, i_binder, b),
            make_system_cons(heap, face, first_value, make_system_nil(heap))));
      }
      /* new_base = (u0 @ k) */
      new_base = make_path_app(heap, base, k_interval);
      /* inner comp */
      inner_comp = make_comp(heap, AST_TT_COMP, C_family, face,
                             new_partial, new_base);
      result = make_path_abs(heap, k_var, inner_comp);
      *changed = 1;
      return result;
    }

    /* Rule 7: comp Glue (Turn 11)
     *
     * For the constant Glue case — type family is path-abs binding
     * an interval var that doesn't appear in the Glue components:
     *
     *   comp^i (Glue A φ T e) [ψ ↦ u] u0
     *     -->  glue-intro φ
     *                     (comp^i T [ψ ↦ "u as T-element on φ"]
     *                                 (equiv-fun e u0 on φ))
     *                     (comp^i A [ψ ↦ unglue e u] (unglue e u0))
     *
     * This is the simplified form: glue back together the comps
     * in the constituent types. The full CCHM rule has more
     * coordination via the equivalence's fiber-contractibility
     * proof; for the cases we care about (id-equiv, where this
     * simplification is correct) this gives the right result. */
    if (family_body->type == AST_TT_GLUE &&
        lizard_tt_flag_get("reduce-comp-glue")) {
      lizard_ast_node_t *i_binder = type_family->data.tt_path_abs.binder;
      lizard_ast_node_t *A = family_body->data.tt_glue.base;
      lizard_ast_node_t *glue_face = family_body->data.tt_glue.face;
      lizard_ast_node_t *T = family_body->data.tt_glue.t;
      lizard_ast_node_t *e = family_body->data.tt_glue.equiv;
      lizard_ast_node_t *A_family, *T_family;
      lizard_ast_node_t *unglued_partial, *unglued_base;
      lizard_ast_node_t *A_comp, *T_comp;
      lizard_ast_node_t *j_var;
      if (i_binder == NULL || i_binder->type != AST_SYMBOL) break;
      /* Build interval var matching binder */
      j_var = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      j_var->type = AST_TT_I_VAR;
      j_var->data.tt_i_var.name = i_binder->data.variable;
      /* A_family = path-abs i. A (Glue's base type) */
      A_family = make_path_abs(heap, i_binder, A);
      /* T_family = path-abs i. T */
      T_family = make_path_abs(heap, i_binder, T);
      /* unglued_partial = <j> unglue e (u @ j) */
      {
        lizard_ast_node_t *u_j = make_path_app(heap, partial, j_var);
        lizard_ast_node_t *unglue_uj = make_unglue(heap, e, u_j);
        unglued_partial = make_path_abs(heap, i_binder, unglue_uj);
      }
      /* unglued_base = unglue e u0 */
      unglued_base = make_unglue(heap, e, base);
      /* Comp in A line */
      A_comp = make_comp(heap, AST_TT_COMP, A_family, face,
                         unglued_partial, unglued_base);
      /* Comp in T line — partial is u directly (already T on φ),
       * base is what we get from running u0 through equiv-fun
       * (since u0 is in Glue and on φ we need T). */
      {
        lizard_ast_node_t *e_fun = make_equiv_fun(heap, e);
        lizard_ast_node_t *base_as_T = make_app(heap, e_fun, base);
        T_comp = make_comp(heap, AST_TT_COMP, T_family, face,
                           partial, base_as_T);
      }
      /* glue-intro φ T_comp A_comp */
      *changed = 1;
      return make_glue_intro(heap, glue_face, T_comp, A_comp);
    }

    break;
  }
  case AST_TT_HCOMP: {
    /* Homogeneous composition: same as comp but the type doesn't
     * vary along the interval. Reduces to comp when the type-line
     * is constant.
     *
     *   hcomp A [F1 ↦ u] u0  -->  (u @ i1)
     *   hcomp A [system-nil] u0  -->  u0   (no partial: just base)
     *   hcomp A [φ ↦ u] u0   stays unreduced when type is constant
     *                         atom (no per-type-former handling here;
     *                         comp gives that).
     */
    lizard_ast_node_t *face = t->data.tt_comp.face;
    lizard_ast_node_t *partial = t->data.tt_comp.partial;
    lizard_ast_node_t *base = t->data.tt_comp.base;
    if (face == NULL || partial == NULL) break;
    if (face->type == AST_TT_F1 && lizard_tt_flag_get("reduce-hcomp-f1")) {
      *changed = 1;
      return make_path_app(heap, partial, make_i1(heap));
    }
    /* When the partial is empty (system-nil) or the face is F0,
     * the comp is just the base. Constructing make_system_nil here
     * also keeps that helper alive for downstream comp rules. */
    if ((partial->type == AST_TT_SYSTEM_NIL || face->type == AST_TT_F0) &&
        lizard_tt_flag_get("reduce-system-nil-comp")) {
      (void)make_system_nil; /* keep symbol referenced */
      *changed = 1;
      return base;
    }
    break;
  }
  case AST_TT_FILL: {
    /* Fill: gives the entire filling line as a path-abs.
     *
     *   fill^i A [φ ↦ u] u0
     *     -->  <k> comp^i (path-abs i. A_body[(i ∧ k)/i])
     *                     [φ ↦ <i> u@(i ∧ k),
     *                      (k = i0) ↦ <i> u0]
     *                     u0
     *
     * The CCHM rule. We construct a fresh interval var k, narrow the
     * type family and partial to (i ∧ k), and add a clause to the
     * partial system pinning the i=i0 face to u0. The result is a
     * path-abs of k giving the value at each point of the line.
     */
    if (!lizard_tt_flag_get("reduce-fill-to-comp")) break;
    {
      lizard_ast_node_t *family = t->data.tt_comp.type_family;
      lizard_ast_node_t *face = t->data.tt_comp.face;
      lizard_ast_node_t *partial = t->data.tt_comp.partial;
      lizard_ast_node_t *base = t->data.tt_comp.base;
      lizard_ast_node_t *i_binder;
      lizard_ast_node_t *k_binder, *k_interval;
      lizard_ast_node_t *i_and_k;
      lizard_ast_node_t *narrowed_family, *narrowed_partial;
      lizard_ast_node_t *new_system, *inner_comp, *result;
      char *kname;
      if (family == NULL || face == NULL || partial == NULL || base == NULL)
        break;
      if (family->type != AST_TT_PATH_ABS) break;
      i_binder = family->data.tt_path_abs.binder;
      if (i_binder == NULL || i_binder->type != AST_SYMBOL) break;
      /* Fresh interval var k. */
      kname = lizard_heap_alloc(3);
      strcpy(kname, "k");
      k_binder = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      k_binder->type = AST_SYMBOL;
      k_binder->data.variable = kname;
      k_interval = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      k_interval->type = AST_TT_I_VAR;
      k_interval->data.tt_i_var.name = kname;
      /* Build (i ∧ k) as an interval term. */
      {
        lizard_ast_node_t *i_interval = lizard_heap_alloc(sizeof(lizard_ast_node_t));
        i_interval->type = AST_TT_I_VAR;
        i_interval->data.tt_i_var.name = i_binder->data.variable;
        i_and_k = make_i_and(heap, i_interval, k_interval);
      }
      /* Narrow the family: substitute (i ∧ k) for i in the body, then
       * re-wrap as path-abs over i. */
      {
        lizard_ast_node_t *narrowed_body =
            subst_interval(family->data.tt_path_abs.body,
                           i_binder->data.variable, i_and_k, heap);
        narrowed_family = make_path_abs(heap, i_binder, narrowed_body);
      }
      /* Narrow the partial: <i> (partial @ (i ∧ k))
       * Note partial is itself typically a path-abs binding j. We
       * apply it at (i ∧ k) and re-abstract. */
      {
        lizard_ast_node_t *i_interval = lizard_heap_alloc(sizeof(lizard_ast_node_t));
        lizard_ast_node_t *partial_at;
        i_interval->type = AST_TT_I_VAR;
        i_interval->data.tt_i_var.name = i_binder->data.variable;
        i_interval = make_i_and(heap, i_interval, k_interval);
        partial_at = make_path_app(heap, partial, i_interval);
        narrowed_partial = make_path_abs(heap, i_binder, partial_at);
      }
      /* Build a system that includes the original (narrowed) partial
       * on φ, plus the new clause (k = i0) ↦ <i> base. */
      {
        lizard_ast_node_t *k_eq_i0 = make_f_eq(heap, k_interval, make_i0(heap));
        lizard_ast_node_t *base_abs = make_path_abs(heap, i_binder, base);
        new_system =
            make_system_cons(heap, k_eq_i0, base_abs,
              make_system_cons(heap, face, narrowed_partial,
                make_system_nil(heap)));
      }
      inner_comp = make_comp(heap, AST_TT_COMP, narrowed_family, face,
                             new_system, base);
      result = make_path_abs(heap, k_binder, inner_comp);
      *changed = 1;
      return result;
    }
  }
  case AST_TT_GLUE: {
    /* Glue boundary rules (Turn 9):
     *
     *   1. (Glue A F1 T e)  -->  T
     *      When the face is everywhere-true, Glue IS T.
     *
     *   2. (Glue A F0 T e)  -->  A
     *      When the face is empty, the Glue degenerates to A.
     */
    lizard_ast_node_t *A = t->data.tt_glue.base;
    lizard_ast_node_t *face = t->data.tt_glue.face;
    lizard_ast_node_t *T = t->data.tt_glue.t;
    if (A == NULL || face == NULL || T == NULL) break;
    if (face->type == AST_TT_F1 && lizard_tt_flag_get("reduce-glue-f1")) {
      *changed = 1;
      return T;
    }
    if (face->type == AST_TT_F0 && lizard_tt_flag_get("reduce-glue-f0")) {
      *changed = 1;
      return A;
    }
    break;
  }
  case AST_TT_SYSTEM_CONS: {
    /* System simplification (Turn 11):
     *
     *   1. If the head clause's face is F1, the whole system reduces
     *      to that clause's value (the face is true everywhere).
     *   2. If the head clause's face is F0, drop the clause (the
     *      face is true nowhere — clause is irrelevant).
     *
     * These rules let systems simplify when their faces become
     * concrete after substitution, which is essential for comp Glue
     * to compute when faces resolve.
     */
    lizard_ast_node_t *f = t->data.tt_system_cons.face;
    if (f == NULL) break;
    if (f->type == AST_TT_F1 &&
        lizard_tt_flag_get("reduce-system-true-clause")) {
      *changed = 1;
      return t->data.tt_system_cons.value;
    }
    if (f->type == AST_TT_F0 &&
        lizard_tt_flag_get("reduce-system-true-clause")) {
      *changed = 1;
      return t->data.tt_system_cons.next;
    }
    break;
  }
  case AST_TT_UNGLUE: {
    /* (unglue e (glue-intro φ t a))  -->  a
     *
     * unglue extracts the underlying A-element. When applied to a
     * glue-intro, we get the third argument directly. (The other
     * unglue cases — applied to a variable, or to comp/hcomp on
     * Glue types — are handled when those reduce.)
     */
    lizard_ast_node_t *g = t->data.tt_unglue.target;
    if (g == NULL) break;
    if (g->type == AST_TT_GLUE_INTRO &&
        lizard_tt_flag_get("reduce-unglue-intro")) {
      *changed = 1;
      return g->data.tt_glue_intro.a;
    }
    break;
  }
  default:
    break;
  }
  return t;
}

/* ----- Public entry: reduce a term to normal form ------------------- */

/* Termination: every rewrite rule above either strictly decreases the
 * node-count of identity-fragment operators, or re-associates with
 * a strict decrease in left-weight (the lex measure). The recursive
 * normalize calls only happen on rewritten terms, which are strictly
 * smaller. We bound the recursion depth anyway as a safety net. */

lizard_ast_node_t *lizard_tt_reduce(lizard_ast_node_t *t,
                                    lizard_heap_t *heap) {
  memo_table_t memo;
  lizard_ast_node_t *result;
  memo_init(&memo);
  result = normalize_rec(t, heap, &memo);
  memo_destroy(&memo);
  return result;
}

/* ----- Universe ordering ------------------------------------------- *
 *
 * universe_leq(u, v) decides whether u ≤ v as universe levels. The
 * result is:
 *   1   — definitely u ≤ v
 *   0   — definitely u > v
 *  -1   — undecidable (variables involved, etc.)
 *
 * The reasoning is structural:
 *   (U n) ≤ (U m) iff n ≤ m.
 *   (U-suc u) ≤ (U-suc v) iff u ≤ v.
 *   (U-suc u) ≤ (U n) iff u ≤ (U n-1) when n > 0; otherwise false.
 *   (U n) ≤ (U-suc v) iff (U n) ≤ v (then add 1) ... but also if n=0 it's true.
 *   (U-max u v) ≤ w iff u ≤ w AND v ≤ w.
 *   u ≤ (U-max v w) iff u ≤ v OR u ≤ w  (sufficient but not necessary
 *                                          when u is itself a max).
 *   (U-var i) ≤ (U-var j) iff i == j (no info otherwise).
 *
 * We reduce both sides first to put them in a normal form when
 * possible. */

/* Phase L.3 — per-session fresh-dimension counter.
 *
 * Each call returns a new, distinct natural-number dimension. Used
 * by the typing rule for `pi-fresh` and `sigma-fresh` to label the
 * dimension introduced by a quantification. The counter is process-
 * local (no global heap dependency), so it resets when a new lizard
 * session starts — useful for deterministic testing.
 *
 * Starts at 1000 to leave room for "ordinary" dimensions 0-999 that
 * users might write explicitly. Two pi-fresh calls in the same
 * session get dims 1000, 1001, 1002, ...
 */
long lizard_tt_next_fresh_dim(void) {
  static long counter = 1000;
  return counter++;
}

/* ===== Phase H.1 — HIT registry =====
 *
 * Simple linked-list registry of declared HITs, keyed by name. The
 * registry is per-process and lives in static storage; entries
 * accumulate as HIT declarations are evaluated.
 *
 * Two operations matter for correctness: register a new HIT (which
 * fails silently if the name already exists, treating re-declaration
 * as an overwrite), and look up by name (returns NULL if missing).
 */
typedef struct hit_registry_entry {
  const char *name;
  lizard_ast_node_t *decl;
  struct hit_registry_entry *next;
} hit_registry_entry_t;

static hit_registry_entry_t *hit_registry_head = NULL;

void lizard_tt_hit_register(lizard_ast_node_t *decl) {
  hit_registry_entry_t *entry;
  const char *name;
  if (decl == NULL || decl->type != AST_TT_HIT_DECL) return;
  if (decl->data.tt_hit_decl.name == NULL ||
      decl->data.tt_hit_decl.name->type != AST_SYMBOL) return;
  name = decl->data.tt_hit_decl.name->data.variable;
  /* Overwrite if already exists. */
  for (entry = hit_registry_head; entry != NULL; entry = entry->next) {
    if (strcmp(entry->name, name) == 0) {
      entry->decl = decl;
      return;
    }
  }
  /* Otherwise prepend. Note: registry entries leak across heap
   * lifetimes by design — they're per-process metadata, not per-heap
   * data. The name string is borrowed from the decl AST node, which
   * is heap-allocated in lizard_heap. If the heap is destroyed, the
   * registry will dangle. This is acceptable for H.1; a future
   * iteration should either copy strings or tie registry life to
   * the heap. */
  entry = malloc(sizeof(hit_registry_entry_t));
  if (entry == NULL) return;
  entry->name = name;
  entry->decl = decl;
  entry->next = hit_registry_head;
  hit_registry_head = entry;
}

lizard_ast_node_t *lizard_tt_hit_lookup(const char *name) {
  hit_registry_entry_t *entry;
  if (name == NULL) return NULL;
  for (entry = hit_registry_head; entry != NULL; entry = entry->next) {
    if (strcmp(entry->name, name) == 0) {
      return entry->decl;
    }
  }
  return NULL;
}

void lizard_tt_hit_registry_reset(void) {
  hit_registry_entry_t *entry = hit_registry_head;
  while (entry != NULL) {
    hit_registry_entry_t *next = entry->next;
    free(entry);
    entry = next;
  }
  hit_registry_head = NULL;
}

long lizard_tt_hit_registry_size(void) {
  hit_registry_entry_t *entry;
  long count = 0;
  for (entry = hit_registry_head; entry != NULL; entry = entry->next) {
    count++;
  }
  return count;
}

/* Look up which HIT declaration owns a constructor with the given
 * name. Returns the HIT_DECL or NULL if not found. Used by the
 * typing rule for HIT_APP. */
lizard_ast_node_t *lizard_tt_hit_lookup_constructor_host(const char *cname) {
  hit_registry_entry_t *entry;
  if (cname == NULL) return NULL;
  for (entry = hit_registry_head; entry != NULL; entry = entry->next) {
    lz_list_node_t *p;
    lz_list_t *ctors = entry->decl->data.tt_hit_decl.constructors;
    for (p = ctors->head; p != ctors->nil; p = p->next) {
      lizard_ast_node_t *ctor = ((lizard_ast_list_node_t *)p)->ast;
      if (ctor->type == AST_TT_HIT_CONSTRUCTOR &&
          ctor->data.tt_hit_constructor.name != NULL &&
          ctor->data.tt_hit_constructor.name->type == AST_SYMBOL &&
          strcmp(ctor->data.tt_hit_constructor.name->data.variable, cname) == 0) {
        return entry->decl;
      }
    }
  }
  return NULL;
}

/* ===== Phase M.1 — logic-rule configuration registry =====
 *
 * Lizard is intended as a configurable type-theory framework: which
 * inference rules are active determines which logic the kernel
 * implements. The lambda cube formalism is the inspiration — the
 * cube's eight corners (STLC, F, F2, Fω, LF, λP, λPω, CoC) are
 * obtained by toggling three orthogonal axes. Beyond the cube,
 * modalities, structural rules, lattice features, and HIT support
 * can all become toggleable axes of a larger configuration space.
 *
 * M.1 lands ONLY the infrastructure — the registry, the snapshot/
 * restore mechanism, and the user-facing primitives. NO rule is
 * pre-registered, and NO type-checking site yet consults the
 * registry. M.2 and beyond will wire specific rules to it.
 *
 * The registry is per-process, mirroring the precedent set by the
 * fresh-dim counter (L.3) and the HIT registry (H.1). A snapshot
 * is a copied linked list, used for "switch logic, do work, switch
 * back" patterns where the user wants to be sure nothing leaks
 * between configurations.
 */
typedef struct logic_rule_entry {
  const char *name;
  int enabled;            /* 0 or 1 */
  struct logic_rule_entry *next;
} logic_rule_entry_t;

static logic_rule_entry_t *logic_config_head = NULL;

/* Internal: find a rule by name, return entry or NULL. */
static logic_rule_entry_t *logic_rule_find(const char *name) {
  logic_rule_entry_t *e;
  if (name == NULL) return NULL;
  for (e = logic_config_head; e != NULL; e = e->next) {
    if (strcmp(e->name, name) == 0) return e;
  }
  return NULL;
}

/* Register a new rule with a default enabled state. If the rule
 * already exists, this is a no-op (we don't overwrite). */
void lizard_logic_rule_register(const char *name, int default_enabled) {
  logic_rule_entry_t *e;
  size_t namelen;
  char *namedup;
  if (name == NULL) return;
  if (logic_rule_find(name) != NULL) return;  /* already registered */
  e = malloc(sizeof(logic_rule_entry_t));
  if (e == NULL) return;
  /* Copy the name so callers' strings can be freed without dangling
   * the registry. We use plain malloc; the registry leaks at process
   * exit, which is acceptable for M.1. */
  namelen = strlen(name) + 1;
  namedup = malloc(namelen);
  if (namedup == NULL) { free(e); return; }
  memcpy(namedup, name, namelen);
  e->name = namedup;
  e->enabled = default_enabled ? 1 : 0;
  e->next = logic_config_head;
  logic_config_head = e;
}

/* Enable a rule. Auto-registers if not yet present. */
int lizard_logic_rule_enable(const char *name) {
  logic_rule_entry_t *e;
  if (name == NULL) return 0;
  e = logic_rule_find(name);
  if (e == NULL) {
    lizard_logic_rule_register(name, 1);
    return 1;
  }
  e->enabled = 1;
  return 1;
}

/* Disable a rule. Auto-registers as disabled if not yet present. */
int lizard_logic_rule_disable(const char *name) {
  logic_rule_entry_t *e;
  if (name == NULL) return 0;
  e = logic_rule_find(name);
  if (e == NULL) {
    lizard_logic_rule_register(name, 0);
    return 1;
  }
  e->enabled = 0;
  return 1;
}

/* Query a rule's enabled state. Returns 1 if enabled, 0 if disabled,
 * -1 if not registered. The -1 case lets callers distinguish "off by
 * default because configured off" from "unknown rule". */
int lizard_logic_rule_enabled(const char *name) {
  logic_rule_entry_t *e;
  if (name == NULL) return -1;
  e = logic_rule_find(name);
  if (e == NULL) return -1;
  return e->enabled;
}

long lizard_logic_config_size(void) {
  logic_rule_entry_t *e;
  long count = 0;
  for (e = logic_config_head; e != NULL; e = e->next) count++;
  return count;
}

/* Reset the entire configuration. Mostly for tests. */
void lizard_logic_config_reset(void) {
  logic_rule_entry_t *e = logic_config_head;
  while (e != NULL) {
    logic_rule_entry_t *next = e->next;
    /* Cast away const for free(); we own the string via the malloc
     * in register/snapshot. */
    char *name_mut;
    memcpy(&name_mut, &e->name, sizeof(char *));
    free(name_mut);
    free(e);
    e = next;
  }
  logic_config_head = NULL;
}

/* Snapshot: take a deep copy of the current configuration and return
 * an opaque handle. The handle is a pointer to a copy of the linked
 * list. Callers must restore or free it later. */
void *lizard_logic_snapshot(void) {
  logic_rule_entry_t *src, *new_head, *new_tail;
  new_head = NULL;
  new_tail = NULL;
  for (src = logic_config_head; src != NULL; src = src->next) {
    logic_rule_entry_t *copy;
    char *namedup;
    size_t namelen;
    copy = malloc(sizeof(logic_rule_entry_t));
    if (copy == NULL) return new_head;  /* partial snapshot; OK */
    namelen = strlen(src->name) + 1;
    namedup = malloc(namelen);
    if (namedup == NULL) { free(copy); return new_head; }
    memcpy(namedup, src->name, namelen);
    copy->name = namedup;
    copy->enabled = src->enabled;
    copy->next = NULL;
    if (new_head == NULL) {
      new_head = copy;
    } else {
      new_tail->next = copy;
    }
    new_tail = copy;
  }
  return new_head;
}

/* Restore: replace the current configuration with the snapshot. The
 * snapshot is CONSUMED (becomes the new active config); callers
 * must not reuse it. To keep a snapshot for multiple restores, take
 * multiple snapshots. */
void lizard_logic_restore(void *snapshot) {
  lizard_logic_config_reset();
  logic_config_head = (logic_rule_entry_t *)snapshot;
}

/* Iterator: walk the configuration in registration order (reverse
 * of insertion, since we prepend). Returns 0 to continue, non-zero
 * to stop. */
void lizard_logic_config_walk(int (*cb)(const char *name, int enabled,
                                        void *userdata),
                              void *userdata) {
  logic_rule_entry_t *e;
  for (e = logic_config_head; e != NULL; e = e->next) {
    if (cb(e->name, e->enabled, userdata)) return;
  }
}

/* ===== Phase M.3 — named logic bundles =====
 *
 * Sugar on top of the M.2 cube toggles. Each named logic corresponds
 * to a specific combination of the three cube axes:
 *
 *                   term-on-type  type-on-term  type-on-type
 *   STLC            off           off           off
 *   F               on            off           off
 *   LF (= lambda-P) off           on            off
 *   F-omega         off           off           on
 *   lambda-P2       on            on            off
 *   lambda-P-omega  off           on            on
 *   lambda-omega    on            off           on
 *   CoC             on            on            on
 *
 * M.3 is sugar — it doesn't add new behavior. `(set-logic 'STLC)`
 * is equivalent to disabling all three cube axes. The reverse,
 * `(current-logic)`, looks at the active config and returns the
 * matching bundle name (or 'custom if no match).
 */
typedef struct logic_bundle {
  const char *name;
  int term_on_type;
  int type_on_term;
  int type_on_type;
  /* Phase M.4: structural rules. -1 = "don't care". */
  int weakening;
  int contraction;
  int exchange;
  /* Phase M.6: lattice and HIT features. -1 = "don't care". */
  int pi_fresh_enabled;
  int co_pi_fresh_enabled;
  int HIT_enabled;
  int lattice_universes;
  int couniverse_lattice;
  /* Phase M.5.3: modal logic toggles. -1 = "don't care". */
  int modalities_enabled;
  int modal_strict_typing;
} logic_bundle_t;

/* Table of predefined logics. The order matters for reverse lookup:
 * we walk this table and return the FIRST match where all specified
 * (non -1) fields agree with the active config. Cube-only bundles
 * leave structural and feature fields at -1 (which match only when
 * the active config has those at their defaults, all on).
 *
 * Phase M.6: two new bundles demonstrate the matrix:
 *   STLC-strict     — STLC with ALL lizard extras disabled
 *   CoC-plus-lattice — CoC with lattice on, HITs off
 *
 * Phase M.5.3: modal-logic bundles. At lizard's current depth, K, T,
 * S4, and S5 are operationally indistinguishable — the strict box-
 * intro and unbox rules implemented in M.5.2 Turn 2 are the same
 * across these logics. What differs between them are AXIOMS:
 *
 *   K: no extra axioms
 *   T: K + (□A → A)
 *   S4: T + (□A → □□A)
 *   S5: S4 + (◇A → □◇A)
 *
 * These axioms are not yet wired as type-checkable rules. Selecting
 * 'K vs 'S4 currently flips the same toggles and produces identical
 * type-checking behavior. The bundle names are forward-looking:
 * they declare intent and reserve the names for future axiom work.
 */
static logic_bundle_t logic_bundles[] = {
  /* name           cube           structural     features          modal */
  /*                tot ton too    wk ct ex       pf cpf H lat colat me ms */
  {"STLC",            0, 0, 0,    -1,-1,-1,    -1,-1,-1,-1,-1,   -1,-1},
  {"F",               1, 0, 0,    -1,-1,-1,    -1,-1,-1,-1,-1,   -1,-1},
  {"LF",              0, 1, 0,    -1,-1,-1,    -1,-1,-1,-1,-1,   -1,-1},
  {"lambda-P",        0, 1, 0,    -1,-1,-1,    -1,-1,-1,-1,-1,   -1,-1},
  {"F-omega",         0, 0, 1,    -1,-1,-1,    -1,-1,-1,-1,-1,   -1,-1},
  {"lambda-P2",       1, 1, 0,    -1,-1,-1,    -1,-1,-1,-1,-1,   -1,-1},
  {"lambda-P-omega",  0, 1, 1,    -1,-1,-1,    -1,-1,-1,-1,-1,   -1,-1},
  {"lambda-omega",    1, 0, 1,    -1,-1,-1,    -1,-1,-1,-1,-1,   -1,-1},
  {"CoC",             1, 1, 1,    -1,-1,-1,    -1,-1,-1,-1,-1,   -1,-1},
  /* M.4 substructural variants */
  {"linear-STLC",     0, 0, 0,     0, 0, 1,    -1,-1,-1,-1,-1,   -1,-1},
  {"affine-STLC",     0, 0, 0,     1, 0, 1,    -1,-1,-1,-1,-1,   -1,-1},
  {"relevant-STLC",   0, 0, 0,     0, 1, 1,    -1,-1,-1,-1,-1,   -1,-1},
  /* M.6 feature-matrix variants */
  {"STLC-strict",     0, 0, 0,    -1,-1,-1,     0, 0, 0, 0, 0,   -1,-1},
  {"CoC-plus-lattice",1, 1, 1,    -1,-1,-1,     1, 1, 0, 1, 1,   -1,-1},
  /* M.5.3 modal-logic bundles. Each is CoC-base with both modal
   * toggles ON. K/T/S4/S5 are currently aliases — see comment above. */
  {"K",               1, 1, 1,    -1,-1,-1,    -1,-1,-1,-1,-1,    1, 1},
  {"T",               1, 1, 1,    -1,-1,-1,    -1,-1,-1,-1,-1,    1, 1},
  {"S4",              1, 1, 1,    -1,-1,-1,    -1,-1,-1,-1,-1,    1, 1},
  {"S5",              1, 1, 1,    -1,-1,-1,    -1,-1,-1,-1,-1,    1, 1},
  /* Composite: STLC base with modalities (minimal modal logic). */
  {"modal-STLC",      0, 0, 0,    -1,-1,-1,    -1,-1,-1,-1,-1,    1, 1},
  {NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

int lizard_logic_set_bundle(const char *name) {
  logic_bundle_t *b;
  if (name == NULL) return 0;
  for (b = logic_bundles; b->name != NULL; b++) {
    if (strcmp(b->name, name) == 0) {
      if (b->term_on_type)  lizard_logic_rule_enable("term-depends-on-type");
      else                  lizard_logic_rule_disable("term-depends-on-type");
      if (b->type_on_term)  lizard_logic_rule_enable("type-depends-on-term");
      else                  lizard_logic_rule_disable("type-depends-on-term");
      if (b->type_on_type)  lizard_logic_rule_enable("type-depends-on-type");
      else                  lizard_logic_rule_disable("type-depends-on-type");
      if (b->weakening != -1) {
        if (b->weakening)   lizard_logic_rule_enable("weakening");
        else                lizard_logic_rule_disable("weakening");
      }
      if (b->contraction != -1) {
        if (b->contraction) lizard_logic_rule_enable("contraction");
        else                lizard_logic_rule_disable("contraction");
      }
      if (b->exchange != -1) {
        if (b->exchange)    lizard_logic_rule_enable("exchange");
        else                lizard_logic_rule_disable("exchange");
      }
      /* M.6 feature toggles. */
      if (b->pi_fresh_enabled != -1) {
        if (b->pi_fresh_enabled) lizard_logic_rule_enable("pi-fresh-enabled");
        else                     lizard_logic_rule_disable("pi-fresh-enabled");
      }
      if (b->co_pi_fresh_enabled != -1) {
        if (b->co_pi_fresh_enabled) lizard_logic_rule_enable("co-pi-fresh-enabled");
        else                        lizard_logic_rule_disable("co-pi-fresh-enabled");
      }
      if (b->HIT_enabled != -1) {
        if (b->HIT_enabled)      lizard_logic_rule_enable("HIT-enabled");
        else                     lizard_logic_rule_disable("HIT-enabled");
      }
      if (b->lattice_universes != -1) {
        if (b->lattice_universes) lizard_logic_rule_enable("lattice-universes");
        else                      lizard_logic_rule_disable("lattice-universes");
      }
      if (b->couniverse_lattice != -1) {
        if (b->couniverse_lattice) lizard_logic_rule_enable("couniverse-lattice");
        else                       lizard_logic_rule_disable("couniverse-lattice");
      }
      /* M.5.3 modal toggles. */
      if (b->modalities_enabled != -1) {
        if (b->modalities_enabled) lizard_logic_rule_enable("modalities-enabled");
        else                       lizard_logic_rule_disable("modalities-enabled");
      }
      if (b->modal_strict_typing != -1) {
        if (b->modal_strict_typing) lizard_logic_rule_enable("modal-strict-typing");
        else                        lizard_logic_rule_disable("modal-strict-typing");
      }
      return 1;
    }
  }
  return 0;
}

/* Returns the name of the current logic, or "custom" if no bundle
 * matches. Unregistered axes are treated as enabled, matching M.2's
 * default-allow convention. Returns a static string; do not free. */
const char *lizard_logic_current_bundle(void) {
  int term_on, type_on_term, type_on_type;
  int weakening, contraction, exchange;
  int pi_fresh, co_pi_fresh, hit_en, lat_u, lat_co;
  logic_bundle_t *b;
  int modalities_en, modal_strict;
  term_on      = lizard_logic_rule_enabled("term-depends-on-type");
  type_on_term = lizard_logic_rule_enabled("type-depends-on-term");
  type_on_type = lizard_logic_rule_enabled("type-depends-on-type");
  weakening    = lizard_logic_rule_enabled("weakening");
  contraction  = lizard_logic_rule_enabled("contraction");
  exchange     = lizard_logic_rule_enabled("exchange");
  pi_fresh     = lizard_logic_rule_enabled("pi-fresh-enabled");
  co_pi_fresh  = lizard_logic_rule_enabled("co-pi-fresh-enabled");
  hit_en       = lizard_logic_rule_enabled("HIT-enabled");
  lat_u        = lizard_logic_rule_enabled("lattice-universes");
  lat_co       = lizard_logic_rule_enabled("couniverse-lattice");
  modalities_en = lizard_logic_rule_enabled("modalities-enabled");
  modal_strict  = lizard_logic_rule_enabled("modal-strict-typing");
  if (term_on == -1)      term_on = 1;
  if (type_on_term == -1) type_on_term = 1;
  if (type_on_type == -1) type_on_type = 1;
  if (weakening == -1)    weakening = 1;
  if (contraction == -1)  contraction = 1;
  if (exchange == -1)     exchange = 1;
  if (pi_fresh == -1)     pi_fresh = 1;
  if (co_pi_fresh == -1)  co_pi_fresh = 1;
  if (hit_en == -1)       hit_en = 1;
  if (lat_u == -1)        lat_u = 1;
  if (lat_co == -1)       lat_co = 1;
  if (modalities_en == -1) modalities_en = 1;
  if (modal_strict == -1)  modal_strict = 1;
  for (b = logic_bundles; b->name != NULL; b++) {
    int match = 1;
    if (b->term_on_type != term_on) continue;
    if (b->type_on_term != type_on_term) continue;
    if (b->type_on_type != type_on_type) continue;
    /* Structural rules. */
    if (b->weakening != -1) {
      if (b->weakening != weakening) match = 0;
    } else if (weakening != 1) match = 0;
    if (b->contraction != -1) {
      if (b->contraction != contraction) match = 0;
    } else if (contraction != 1) match = 0;
    if (b->exchange != -1) {
      if (b->exchange != exchange) match = 0;
    } else if (exchange != 1) match = 0;
    /* M.6 features. */
    if (b->pi_fresh_enabled != -1) {
      if (b->pi_fresh_enabled != pi_fresh) match = 0;
    } else if (pi_fresh != 1) match = 0;
    if (b->co_pi_fresh_enabled != -1) {
      if (b->co_pi_fresh_enabled != co_pi_fresh) match = 0;
    } else if (co_pi_fresh != 1) match = 0;
    if (b->HIT_enabled != -1) {
      if (b->HIT_enabled != hit_en) match = 0;
    } else if (hit_en != 1) match = 0;
    if (b->lattice_universes != -1) {
      if (b->lattice_universes != lat_u) match = 0;
    } else if (lat_u != 1) match = 0;
    if (b->couniverse_lattice != -1) {
      if (b->couniverse_lattice != lat_co) match = 0;
    } else if (lat_co != 1) match = 0;
    /* M.5.3 modal toggles. */
    if (b->modalities_enabled != -1) {
      if (b->modalities_enabled != modalities_en) match = 0;
    } else if (modalities_en != 1) match = 0;
    if (b->modal_strict_typing != -1) {
      if (b->modal_strict_typing != modal_strict) match = 0;
    } else if (modal_strict != 1) match = 0;
    if (match) return b->name;
  }
  return "custom";
}

void lizard_logic_bundles_walk(int (*cb)(const char *name, void *userdata),
                                void *userdata) {
  logic_bundle_t *b;
  for (b = logic_bundles; b->name != NULL; b++) {
    if (cb(b->name, userdata)) return;
  }
}

/* Public wrapper around the static contains_free_var used by the
 * Phase M.2 lambda-cube classifier. */
int contains_free_var_public(lizard_ast_node_t *t, const char *name) {
  return contains_free_var(t, name);
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
    return universe_set_subset(u, v) ? 1 : 0;
  }
  /* Mixed: lift (U n) to {n}-singleton for the comparison. */
  if (u->type == AST_TT_UNIVERSE && v->type == AST_TT_UNIVERSE_SET) {
    long *dim = lizard_heap_alloc(sizeof(long));
    lizard_ast_node_t lift;
    dim[0] = u->data.tt_universe.level;
    lift.type = AST_TT_UNIVERSE_SET;
    lift.data.tt_universe_set.dims = dim;
    lift.data.tt_universe_set.count = 1;
    return universe_set_subset(&lift, v) ? 1 : 0;
  }
  if (u->type == AST_TT_UNIVERSE_SET && v->type == AST_TT_UNIVERSE) {
    long *dim = lizard_heap_alloc(sizeof(long));
    lizard_ast_node_t lift;
    dim[0] = v->data.tt_universe.level;
    lift.type = AST_TT_UNIVERSE_SET;
    lift.data.tt_universe_set.dims = dim;
    lift.data.tt_universe_set.count = 1;
    return universe_set_subset(u, &lift) ? 1 : 0;
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
    return couniverse_set_subset(u, v) ? 1 : 0;
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

static int single_arg_local(lz_list_t *args) {
  return args->head != args->nil && args->head->next == args->nil;
}
static int two_args_local(lz_list_t *args) {
  return args->head != args->nil && args->head->next != args->nil &&
         args->head->next->next == args->nil;
}
static lizard_ast_node_t *nth_local(lz_list_t *args, int n) {
  lz_list_node_t *it = args->head;
  while (n-- > 0 && it != args->nil) it = it->next;
  if (it == args->nil) return NULL;
  return ((lizard_ast_list_node_t *)it)->ast;
}

/* ----- Face entailment decision procedure (Turn 7) -----
 *
 * Decides whether face formula φ entails ψ: φ ⊨ ψ.
 * Returns:
 *   1 — definitely entails
 *   0 — definitely does not entail
 *  -1 — undecidable by the procedure
 *
 * Strategy is structural:
 *   F0 ⊨ anything                      (anything follows from false)
 *   anything ⊨ F1                      (truth follows from anything)
 *   φ ⊨ φ                              (reflexivity via alpha-equal)
 *   (F-or φ ψ) ⊨ χ  iff  φ ⊨ χ AND ψ ⊨ χ
 *   φ ⊨ (F-and ψ χ) iff  φ ⊨ ψ AND φ ⊨ χ
 *   (F-and φ _) ⊨ φ                    (and-left projection)
 *   (F-and _ φ) ⊨ φ                    (and-right projection)
 *   φ ⊨ (F-or ψ _) if φ ⊨ ψ           (or-left injection, incomplete)
 *   φ ⊨ (F-or _ ψ) if φ ⊨ ψ           (or-right injection, incomplete)
 *
 * Used by comp (Turn 8) and Sub typing. */

int lizard_tt_face_entails(lizard_ast_node_t *phi,
                           lizard_ast_node_t *psi) {  if (phi == NULL || psi == NULL) return -1;
  if (phi->type == AST_TT_F0) return 1;        /* F0 entails anything */
  if (psi->type == AST_TT_F1) return 1;        /* anything entails F1 */
  if (lizard_tt_alpha_equal(phi, psi)) return 1; /* reflexive */
  /* (F-or phi1 phi2) ⊨ psi iff phi1 ⊨ psi AND phi2 ⊨ psi */
  if (phi->type == AST_TT_F_OR) {
    int l = lizard_tt_face_entails(phi->data.tt_f_binop.left, psi);
    int r = lizard_tt_face_entails(phi->data.tt_f_binop.right, psi);
    if (l == 1 && r == 1) return 1;
    if (l == 0 || r == 0) return 0;
    return -1;
  }
  /* phi ⊨ (F-and psi1 psi2) iff phi ⊨ psi1 AND phi ⊨ psi2 */
  if (psi->type == AST_TT_F_AND) {
    int l = lizard_tt_face_entails(phi, psi->data.tt_f_binop.left);
    int r = lizard_tt_face_entails(phi, psi->data.tt_f_binop.right);
    if (l == 1 && r == 1) return 1;
    if (l == 0 || r == 0) return 0;
    return -1;
  }
  /* (F-and phi1 phi2) ⊨ phi1 and ⊨ phi2 (projection). */
  if (phi->type == AST_TT_F_AND) {
    if (lizard_tt_alpha_equal(phi->data.tt_f_binop.left, psi)) return 1;
    if (lizard_tt_alpha_equal(phi->data.tt_f_binop.right, psi)) return 1;
    /* Try recursively. */
    {
      int l = lizard_tt_face_entails(phi->data.tt_f_binop.left, psi);
      int r = lizard_tt_face_entails(phi->data.tt_f_binop.right, psi);
      if (l == 1 || r == 1) return 1;
    }
  }
  /* phi ⊨ (F-or psi1 psi2) if phi ⊨ psi1 OR phi ⊨ psi2 (sufficient
   * but incomplete: e.g. (i=i0 ∨ i=i1) on i ⊨ (i=i0 ∨ i=i1) by
   * reflexivity, but the disjunction-elim-style version would fail). */
  if (psi->type == AST_TT_F_OR) {
    int l = lizard_tt_face_entails(phi, psi->data.tt_f_binop.left);
    int r = lizard_tt_face_entails(phi, psi->data.tt_f_binop.right);
    if (l == 1 || r == 1) return 1;
  }
  return -1;
}

lizard_ast_node_t *lizard_primitive_tt_face_entails(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  lizard_ast_node_t *phi, *psi;
  int r;
  (void)env;
  if (!two_args_local(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  phi = nth_local(args, 0);
  psi = nth_local(args, 1);
  phi = lizard_tt_reduce(phi, heap);
  psi = lizard_tt_reduce(psi, heap);
  r = lizard_tt_face_entails(phi, psi);
  if (r == 1) return lizard_make_bool(heap, 1);
  if (r == 0) return lizard_make_bool(heap, 0);
  {
    char *buf = lizard_heap_alloc(8);
    lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    strcpy(buf, "unknown");
    n->type = AST_SYMBOL;
    n->data.variable = buf;
    return n;
  }
}

/* System lookup (Turn 11):
 *
 * Given a system and a "context face" φ_ctx (the face we know to be
 * true), find the first clause (φ_i, u_i) in the system such that
 * φ_ctx ⊨ φ_i (the context face entails this clause's face). If
 * found, return u_i. If not, return NULL.
 *
 * Used by comp Glue to look up the equiv/T for the active face,
 * and by system reduction when one of the clause faces becomes F1
 * (which is entailed by everything, so its clause is selected).
 */
lizard_ast_node_t *lizard_tt_system_lookup(lizard_ast_node_t *system,
                                           lizard_ast_node_t *phi_ctx) {
  while (system && system->type == AST_TT_SYSTEM_CONS) {
    lizard_ast_node_t *clause_face = system->data.tt_system_cons.face;
    /* If phi_ctx entails this clause's face, the clause fires. */
    if (lizard_tt_face_entails(phi_ctx, clause_face) == 1) {
      return system->data.tt_system_cons.value;
    }
    system = system->data.tt_system_cons.next;
  }
  return NULL;
}

/* Primitive wrapper. */
lizard_ast_node_t *lizard_primitive_tt_system_lookup(lz_list_t *args,
                                                     lizard_env_t *env,
                                                     lizard_heap_t *heap) {
  lizard_ast_node_t *sys, *phi, *r;
  (void)env;
  if (!two_args_local(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  sys = lizard_tt_reduce(nth_local(args, 0), heap);
  phi = lizard_tt_reduce(nth_local(args, 1), heap);
  r = lizard_tt_system_lookup(sys, phi);
  if (r == NULL) return lizard_make_nil(heap);
  return r;
}

lizard_ast_node_t *lizard_primitive_tt_universe_leq(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  lizard_ast_node_t *u, *v;
  int r;
  (void)env;
  if (!two_args_local(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  u = nth_local(args, 0);
  v = nth_local(args, 1);
  u = lizard_tt_reduce(u, heap);
  v = lizard_tt_reduce(v, heap);
  r = lizard_tt_universe_leq(u, v);
  if (r == 1)  return lizard_make_bool(heap, 1);
  if (r == 0)  return lizard_make_bool(heap, 0);
  {
    char *buf = lizard_heap_alloc(8);
    lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    strcpy(buf, "unknown");
    n->type = AST_SYMBOL;
    n->data.variable = buf;
    return n;
  }
}

/* Phase L.4: couniverse-leq? primitive. */
lizard_ast_node_t *lizard_primitive_tt_couniverse_leq(lz_list_t *args,
                                                     lizard_env_t *env,
                                                     lizard_heap_t *heap) {
  lizard_ast_node_t *u, *v;
  int r;
  (void)env;
  if (!two_args_local(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  u = nth_local(args, 0);
  v = nth_local(args, 1);
  u = lizard_tt_reduce(u, heap);
  v = lizard_tt_reduce(v, heap);
  r = lizard_tt_couniverse_leq(u, v);
  if (r == 1)  return lizard_make_bool(heap, 1);
  if (r == 0)  return lizard_make_bool(heap, 0);
  {
    char *buf = lizard_heap_alloc(8);
    lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    strcpy(buf, "unknown");
    n->type = AST_SYMBOL;
    n->data.variable = buf;
    return n;
  }
}

lizard_ast_node_t *lizard_primitive_tt_reduce(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  (void)env;
  if (!single_arg_local(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  return lizard_tt_reduce(nth_local(args, 0), heap);
}

lizard_ast_node_t *lizard_primitive_tt_equal(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *a, *b, *ra, *rb;
  (void)env;
  if (!two_args_local(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  a = nth_local(args, 0);
  b = nth_local(args, 1);
  ra = lizard_tt_reduce(a, heap);
  rb = lizard_tt_reduce(b, heap);
  return lizard_make_bool(heap, lizard_tt_alpha_equal(ra, rb));
}

/* Flag primitives. */
lizard_ast_node_t *lizard_primitive_flag_set(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *name, *val;
  (void)env;
  if (!two_args_local(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name = nth_local(args, 0);
  val = nth_local(args, 1);
  if (!name || name->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  lizard_tt_flag_set(name->data.variable,
                     !(val && val->type == AST_BOOL && !val->data.boolean));
  return lizard_make_nil(heap);
}

lizard_ast_node_t *lizard_primitive_flag_get(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *name;
  (void)env;
  if (!single_arg_local(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name = nth_local(args, 0);
  if (!name || name->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  return lizard_make_bool(heap, lizard_tt_flag_get(name->data.variable));
}

lizard_ast_node_t *lizard_primitive_flag_list(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_tt_flag_t *f;
  lizard_ast_node_t *result;
  (void)env;
  (void)args;
  result = lizard_make_nil(heap);
  for (f = flag_list; f != NULL; f = f->next) {
    char *buf = lizard_heap_alloc(strlen(f->name) + 1);
    lizard_ast_node_t *sym = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    lizard_ast_node_t *pair = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    strcpy(buf, f->name);
    sym->type = AST_SYMBOL;
    sym->data.variable = buf;
    pair->type = AST_PAIR;
    pair->data.pair.car = sym;
    pair->data.pair.cdr = result;
    result = pair;
  }
  return result;
}
