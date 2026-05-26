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
  case AST_TT_UNIT:
  case AST_TT_TT:
  case AST_TT_BOT:
    return 0;
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
  case AST_TT_UNIT:
  case AST_TT_TT:
  case AST_TT_BOT:
    return t;
  default:
    return t;
  }
}

lizard_ast_node_t *lizard_tt_subst(lizard_ast_node_t *t, const char *x,
                                   lizard_ast_node_t *v,
                                   lizard_heap_t *heap) {
  return subst_rec(t, x, v, heap);
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
  case AST_TT_UNIT:
  case AST_TT_TT:
  case AST_TT_BOT:
    return 1;       /* nullary — same type means equal */
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
  case AST_TT_UNIT:
  case AST_TT_TT:
  case AST_TT_BOT:
    return 1;
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
  case AST_TT_UNIT:
  case AST_TT_TT:
  case AST_TT_BOT:
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
     * substitution to avoid variable capture. */
    lizard_ast_node_t *fn = t->data.tt_app.fun;
    lizard_ast_node_t *ar = t->data.tt_app.arg;
    if (fn == NULL || ar == NULL) break;
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

int lizard_tt_universe_leq(lizard_ast_node_t *u, lizard_ast_node_t *v) {
  if (u == NULL || v == NULL) return -1;
  /* Both concrete integers */
  if (u->type == AST_TT_UNIVERSE && v->type == AST_TT_UNIVERSE) {
    return u->data.tt_universe.level <= v->data.tt_universe.level ? 1 : 0;
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
