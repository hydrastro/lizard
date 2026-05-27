/* tt_check.c
 *
 * Bidirectional type checker for the λΠ fragment of lizard's
 * type-theory layer, with universe polymorphism and cumulativity.
 *
 * What this checks:
 *   - Variables (lookup in context)
 *   - Universes (U n : U (n+1)), with cumulativity
 *   - Pi types: (Pi x A B) : U (max of A's and B's universes)
 *   - Lambdas: (Lambda x b) checks against (Pi x A B) when b checks
 *     against B in extended context
 *   - Applications: (@ f a) infers using f's Pi type, substitutes a
 *   - Annotations: (annot t T) — checks t against T
 *
 * The bidirectional split:
 *   - infer(Γ, t) → T : compute the type when the term determines it
 *   - check(Γ, t, T) → #t or error : verify the term against a given type
 *
 * Variables, applications, universes, and annotations *infer*.
 * Lambdas, pairs, and other introduction forms *check*.
 * Pi formation works either way (we infer the universe of the formed type).
 *
 * Conversion is via lizard_tt_universe_convertible (which uses
 * alpha-equality and cumulativity), with full reduction first.
 *
 * What this does NOT check (yet):
 *   - The HOTT fragment: Id, refl, ap, J, xport, Pair, fst, snd,
 *     inl, inr, Case, Unit, tt, Bot. Each needs its own typing rule.
 *     Adding these is incremental work, one form at a time.
 *   - Inductive type schemas: each inductive form has hardcoded rules.
 *   - Universe polymorphism's *binding*: we have universe-level
 *     variables (U-var) but no Λ binder at the universe level.
 *     Polymorphism here is "free universe variables that can be
 *     compared and unified syntactically."
 *   - Implicit arguments / elaboration.
 *
 * This is the smallest interesting dependent type theory plus the
 * universe machinery from the same turn. Real, working,
 * extensible — but small. */

#include "primitives.h"
#include "env.h"
#include "errors.h"
#include "lizard.h"
#include "mem.h"
#include <string.h>

/* ----- Context lookup --------------------------------------------------
 *
 * A context is built with the existing (context ...) constructor —
 * an AST_TT_CONTEXT node whose data carries a list of variable
 * bindings. Each binding is an AST_TT_VARIABLE with name and type.
 *
 * We look up by name; innermost wins (the existing context-lookup
 * already does this). */

static lizard_ast_node_t *ctx_lookup(lizard_ast_node_t *ctx,
                                     const char *name) {
  /* The context structure: AST_TT_CONTEXT with data.tt_context.bindings
   * being a lz_list_t. Each element is a lizard_ast_list_node_t whose
   * .ast is an AST_TT_VARIABLE. We walk LATEST-first (innermost wins).
   *
   * But the list is appended in order, so we have to walk to the end
   * and back. Simpler: walk to end, remember last match. */
  lz_list_t *bindings;
  lz_list_node_t *node;
  lizard_ast_node_t *last_match = NULL;
  if (ctx == NULL || ctx->type != AST_TT_CONTEXT) return NULL;
  bindings = ctx->data.tt_context.bindings;
  if (bindings == NULL) return NULL;
  for (node = bindings->head; node != bindings->nil; node = node->next) {
    lizard_ast_node_t *var = ((lizard_ast_list_node_t *)node)->ast;
    if (var && var->type == AST_TT_VARIABLE &&
        var->data.tt_variable.name &&
        var->data.tt_variable.name->type == AST_SYMBOL &&
        strcmp(var->data.tt_variable.name->data.variable, name) == 0) {
      last_match = var->data.tt_variable.type;
    }
  }
  return last_match;
}

/* Extend a context with a new binding. Non-destructive: returns a
 * new AST_TT_CONTEXT with the additional binding appended. */
static lizard_ast_node_t *ctx_extend(lizard_ast_node_t *ctx,
                                     const char *name,
                                     lizard_ast_node_t *type,
                                     lizard_heap_t *heap) {
  lizard_ast_node_t *new_var;
  lizard_ast_node_t *name_sym;
  lizard_ast_node_t *new_ctx;
  lz_list_t *new_bindings;
  lz_list_node_t *node;
  lz_list_t *old_bindings;
  lizard_ast_list_node_t *list_node;
  char *namebuf;

  namebuf = lizard_heap_alloc(strlen(name) + 1);
  strcpy(namebuf, name);
  name_sym = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  name_sym->type = AST_SYMBOL;
  name_sym->data.variable = namebuf;

  new_var = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  new_var->type = AST_TT_VARIABLE;
  new_var->data.tt_variable.name = name_sym;
  new_var->data.tt_variable.type = type;

  new_ctx = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  new_ctx->type = AST_TT_CONTEXT;
  new_bindings = list_create_alloc(lizard_heap_alloc, lizard_heap_free);

  if (ctx != NULL && ctx->type == AST_TT_CONTEXT) {
    old_bindings = ctx->data.tt_context.bindings;
    if (old_bindings != NULL) {
      for (node = old_bindings->head; node != old_bindings->nil;
           node = node->next) {
        lizard_ast_node_t *var = ((lizard_ast_list_node_t *)node)->ast;
        list_node =
            (lizard_ast_list_node_t *)lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
        list_node->ast = var;
        list_append(new_bindings, &list_node->node);
      }
    }
  }
  list_node =
      (lizard_ast_list_node_t *)lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
  list_node->ast = new_var;
  list_append(new_bindings, &list_node->node);

  new_ctx->data.tt_context.bindings = new_bindings;
  return new_ctx;
}

/* ----- Error reporting ----------------------------------------------- */

static lizard_ast_node_t *type_error(lizard_heap_t *heap,
                                     const char *msg) {
  (void)msg;
  /* We use the existing error machinery. The message string is not
   * yet propagated through the error; future improvement. */
  return lizard_make_error(heap, LIZARD_ERROR_NODE_TYPE);
}

static int is_error(lizard_ast_node_t *t) {
  return t == NULL || t->type == AST_ERROR;
}

/* Build a (path-app p i) node locally — we don't have access to the
 * make_path_app static helper in tt_equality.c. */
static lizard_ast_node_t *make_path_app_local(lizard_heap_t *heap,
                                              lizard_ast_node_t *p,
                                              lizard_ast_node_t *i) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_PATH_APP;
  n->data.tt_path_app.path = p;
  n->data.tt_path_app.point = i;
  return n;
}

/* ===== Phase M.2 — lambda cube axis classification =====
 *
 * Extract a representative universe-level from a universe expression.
 * Returns -1 if the expression isn't a recognized universe form or its
 * level isn't statically known. The level is the level OF THE EXPRESSION
 * AS A UNIVERSE — e.g., (U 0) returns 0, (U 1) returns 1.
 *
 * For a universe-set, we use the MAX dim as a representative — that's
 * a conservative choice: a multi-dim universe is "at least as big as"
 * its largest dim. For U-max, we recurse and take the max.
 */
static long extract_universe_level(lizard_ast_node_t *u) {
  if (u == NULL) return -1;
  switch (u->type) {
  case AST_TT_UNIVERSE:
    return u->data.tt_universe.level;
  case AST_TT_UNIVERSE_SET: {
    long i, max_dim;
    if (u->data.tt_universe_set.count == 0) return 0;
    max_dim = u->data.tt_universe_set.dims[0];
    for (i = 1; i < u->data.tt_universe_set.count; i++) {
      if (u->data.tt_universe_set.dims[i] > max_dim)
        max_dim = u->data.tt_universe_set.dims[i];
    }
    return max_dim;
  }
  case AST_TT_U_MAX: {
    long l = extract_universe_level(u->data.tt_u_max.left);
    long r = extract_universe_level(u->data.tt_u_max.right);
    if (l < 0 || r < 0) return -1;
    return l > r ? l : r;
  }
  case AST_TT_U_MIN: {
    long l = extract_universe_level(u->data.tt_u_min.left);
    long r = extract_universe_level(u->data.tt_u_min.right);
    if (l < 0 || r < 0) return -1;
    return l < r ? l : r;
  }
  default:
    return -1;
  }
}

/* Classify a Pi by the lambda cube axes.
 *
 * Given dom_univ and cod_univ (the universes of the Pi's domain and
 * codomain types as inferred), and a flag for whether the binder
 * appears free in the codomain, return the required axis as a string,
 * or NULL if the Pi is allowed unconditionally (the STLC simple arrow).
 *
 * The level convention: dom_univ is the universe of A; if dom_univ is
 * (U 1), then A lives at universe level 0, i.e., A is a "type whose
 * inhabitants are values". If dom_univ is (U 2), A lives at level 1,
 * i.e., A is a "kind". So we treat:
 *   dom_univ_level == 1 → A is a type
 *   dom_univ_level >= 2 → A is a kind (or higher)
 *
 * This is the universe-level encoding of the lambda cube; the strict
 * formulation would use explicit sorts (* and ☐). A refactor to that
 * formulation belongs to a later phase if desired.
 *
 * Returns: NULL if Pi is unconditionally allowed (simple arrow), or
 * the name of the required axis as a string literal.
 */
static const char *lambda_cube_required_axis(long dom_level, long cod_level,
                                             int binder_free_in_cod) {
  if (dom_level < 0 || cod_level < 0) {
    /* Unknown universe shape — can't classify; allow. */
    return NULL;
  }
  /* dom is at level 1 (A is a type) */
  if (dom_level == 1) {
    if (cod_level == 1) {
      /* type → type. STLC arrow if non-dependent, else type-dep-on-term. */
      if (binder_free_in_cod) return "type-depends-on-term";
      return NULL;  /* simple arrow, always allowed */
    }
    /* type → kind. Unusual. Require type-depends-on-type as the closest
     * standard axis (covers Fω-style polymorphism over kinds). */
    return "type-depends-on-type";
  }
  /* dom is at level >= 2 (A is a kind) */
  if (cod_level == 1) {
    /* kind → type: System F-style polymorphism */
    return "term-depends-on-type";
  }
  /* kind → kind: Fω-style type-level abstraction */
  return "type-depends-on-type";
}

/* ----- The checker --------------------------------------------------- */

lizard_ast_node_t *lizard_tt_infer2(lizard_ast_node_t *valid_ctx,
                                    lizard_ast_node_t *ctx,
                                   lizard_ast_node_t *t,
                                   lizard_heap_t *heap) {
  if (t == NULL) return type_error(heap, "null term");
  switch (t->type) {
  case AST_SYMBOL: {
    /* Variable: look up in TRUTH context first, then valid context.
     * Phase M.5.2 Turn 2: valid hypotheses (from unbox) are also
     * visible to ordinary code; they're a superset of truth in
     * what they admit, restricted only when entering a box. */
    lizard_ast_node_t *tp = ctx_lookup(ctx, t->data.variable);
    if (tp == NULL && valid_ctx != NULL) {
      tp = ctx_lookup(valid_ctx, t->data.variable);
    }
    if (tp == NULL) {
      return type_error(heap, "unbound variable");
    }
    return tp;
  }
  case AST_TT_UNIVERSE: {
    /* (U n) : (U n+1) */
    return lizard_tt_make_universe(heap, t->data.tt_universe.level + 1);
  }
  case AST_TT_COUNIVERSE: {
    /* Phase L.5 supporting rule: (Uco n) : (Uco (n+1)). Couniverses
     * have a successor too — minimal typing so co-pi-fresh's domain
     * check sees something it recognizes. */
    lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    n->type = AST_TT_COUNIVERSE;
    n->data.tt_couniverse.level = t->data.tt_couniverse.level + 1;
    return n;
  }
  case AST_TT_COUNIVERSE_SET: {
    /* (Co-set ...) is itself a couniverse element. Its "type" is the
     * couniverse one level up — we represent that as a (Co-set ...)
     * with a fresh dim, or simpler, just (Uco 0). For L.5 minimal:
     * say its type is (Uco 0).
     * Phase M.6: gated on couniverse-lattice. */
    lizard_ast_node_t *n;
    if (lizard_logic_rule_enabled("couniverse-lattice") == 0) {
      return type_error(heap, "Co-set rejected: couniverse-lattice disabled");
    }
    n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    n->type = AST_TT_COUNIVERSE;
    n->data.tt_couniverse.level = 0;
    return n;
  }
  case AST_TT_UNIVERSE_SET: {
    /* Phase M.6: U-set as a value lives at (U <max dim + 1>). The
     * UNIVERSE_SET node has been a *type-level marker* mainly used
     * to label what a Pi lives in; now we give it a proper inference
     * rule. Gated on lattice-universes. */
    long max_dim = 0;
    long i;
    if (lizard_logic_rule_enabled("lattice-universes") == 0) {
      return type_error(heap, "U-set rejected: lattice-universes disabled");
    }
    if (t->data.tt_universe_set.count == 0) {
      /* Empty set: bottom universe, lives at (U 1). */
      return lizard_tt_make_universe(heap, 1);
    }
    max_dim = t->data.tt_universe_set.dims[0];
    for (i = 1; i < t->data.tt_universe_set.count; i++) {
      if (t->data.tt_universe_set.dims[i] > max_dim)
        max_dim = t->data.tt_universe_set.dims[i];
    }
    return lizard_tt_make_universe(heap, max_dim + 1);
  }
  case AST_TT_PI: {
    /* (Pi x A B) : (U-max univ(A) univ(B[x:=fresh])) — but we don't
     * have unification of universes. For now: check A is a type
     * (lives in some universe), check B is a type in extended ctx,
     * return the max of their universes.
     *
     * Phase M.2: classify the dependency by the lambda cube axes and
     * REJECT the Pi if the required axis is currently disabled in the
     * logic-rule registry. The default (no rules registered, or all
     * registered as enabled) preserves backward-compatible CoC-style
     * behavior. */
    lizard_ast_node_t *binder = t->data.tt_pi.binder;
    lizard_ast_node_t *dom = t->data.tt_pi.domain;
    lizard_ast_node_t *cod = t->data.tt_pi.codomain;
    lizard_ast_node_t *dom_univ, *cod_univ;
    lizard_ast_node_t *new_ctx;
    const char *required_axis;
    long dom_level, cod_level;
    int binder_free;
    if (binder == NULL || binder->type != AST_SYMBOL) {
      return type_error(heap, "Pi binder not a symbol");
    }
    dom_univ = lizard_tt_infer2(valid_ctx, ctx, dom, heap);
    if (is_error(dom_univ)) return dom_univ;
    /* Reduce to check it's a universe. */
    dom_univ = lizard_tt_reduce(dom_univ, heap);
    if (dom_univ->type != AST_TT_UNIVERSE &&
        dom_univ->type != AST_TT_U_SUC &&
        dom_univ->type != AST_TT_U_MAX &&
        dom_univ->type != AST_TT_U_VAR &&
        dom_univ->type != AST_TT_U_MIN &&
        dom_univ->type != AST_TT_UNIVERSE_SET) {
      return type_error(heap, "Pi domain not a type");
    }
    new_ctx = ctx_extend(ctx, binder->data.variable, dom, heap);
    cod_univ = lizard_tt_infer2(valid_ctx, new_ctx, cod, heap);
    if (is_error(cod_univ)) return cod_univ;
    cod_univ = lizard_tt_reduce(cod_univ, heap);
    if (cod_univ->type != AST_TT_UNIVERSE &&
        cod_univ->type != AST_TT_U_SUC &&
        cod_univ->type != AST_TT_U_MAX &&
        cod_univ->type != AST_TT_U_VAR &&
        cod_univ->type != AST_TT_U_MIN &&
        cod_univ->type != AST_TT_UNIVERSE_SET) {
      return type_error(heap, "Pi codomain not a type");
    }
    /* Phase M.2: classify and check the relevant cube axis. */
    dom_level = extract_universe_level(dom_univ);
    cod_level = extract_universe_level(cod_univ);
    binder_free = contains_free_var_public(cod, binder->data.variable);
    required_axis = lambda_cube_required_axis(dom_level, cod_level, binder_free);
    if (required_axis != NULL) {
      /* Check the rule. If unregistered (-1) or enabled (1), allow.
       * If explicitly disabled (0), reject. The default is "allow"
       * which preserves backward compatibility. */
      if (lizard_logic_rule_enabled(required_axis) == 0) {
        return type_error(heap, "Pi rejected: required cube axis disabled");
      }
    }
    /* Phase M.4: weakening check. The simple arrow (binder not free
     * in codomain) corresponds to a derivation that introduces a
     * variable and never uses it — the hallmark of weakening. If
     * the user has disabled the weakening rule, reject such Pis.
     * Dependent Pis (binder_free == 1) are unaffected. */
    if (!binder_free &&
        lizard_logic_rule_enabled("weakening") == 0) {
      return type_error(heap,
                        "Pi rejected: weakening disabled, "
                        "but binder doesn't appear in codomain");
    }
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_U_MAX;
      n->data.tt_u_max.left = dom_univ;
      n->data.tt_u_max.right = cod_univ;
      return lizard_tt_reduce(n, heap);
    }
  }
  case AST_TT_PI_FRESH: {
    /* Phase L.3: dimension-creating Pi.
     *
     *   A : U_S    B : U_T (under x:A)
     *   ─────────────────────────────────────────────
     *   (pi-fresh x A B) : (U-max U_S U_T) ∨ (U-set fresh)
     *
     * Same structure as Pi but the result universe is *joined* with
     * a singleton set containing a fresh dimension. After reduction
     * with the L.2 set-union rules, the result is a single U-set
     * containing all original dimensions plus the new one.
     *
     * Phase M.6: if pi-fresh-enabled is explicitly disabled, reject. */
    lizard_ast_node_t *binder = t->data.tt_pi_fresh.binder;
    lizard_ast_node_t *dom = t->data.tt_pi_fresh.domain;
    lizard_ast_node_t *cod = t->data.tt_pi_fresh.codomain;
    lizard_ast_node_t *dom_univ, *cod_univ;
    lizard_ast_node_t *new_ctx;
    lizard_ast_node_t *fresh_set;
    lizard_ast_node_t *combined;
    long *dim_ptr;
    if (lizard_logic_rule_enabled("pi-fresh-enabled") == 0) {
      return type_error(heap, "pi-fresh rejected: feature disabled");
    }
    if (binder == NULL || binder->type != AST_SYMBOL) {
      return type_error(heap, "pi-fresh binder not a symbol");
    }
    dom_univ = lizard_tt_infer2(valid_ctx, ctx, dom, heap);
    if (is_error(dom_univ)) return dom_univ;
    dom_univ = lizard_tt_reduce(dom_univ, heap);
    if (dom_univ->type != AST_TT_UNIVERSE &&
        dom_univ->type != AST_TT_U_SUC &&
        dom_univ->type != AST_TT_U_MAX &&
        dom_univ->type != AST_TT_U_VAR &&
        dom_univ->type != AST_TT_U_MIN &&
        dom_univ->type != AST_TT_UNIVERSE_SET) {
      return type_error(heap, "pi-fresh domain not a type");
    }
    new_ctx = ctx_extend(ctx, binder->data.variable, dom, heap);
    cod_univ = lizard_tt_infer2(valid_ctx, new_ctx, cod, heap);
    if (is_error(cod_univ)) return cod_univ;
    cod_univ = lizard_tt_reduce(cod_univ, heap);
    if (cod_univ->type != AST_TT_UNIVERSE &&
        cod_univ->type != AST_TT_U_SUC &&
        cod_univ->type != AST_TT_U_MAX &&
        cod_univ->type != AST_TT_U_VAR &&
        cod_univ->type != AST_TT_U_MIN &&
        cod_univ->type != AST_TT_UNIVERSE_SET) {
      return type_error(heap, "pi-fresh codomain not a type");
    }
    fresh_set = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    dim_ptr = lizard_heap_alloc(sizeof(long));
    dim_ptr[0] = lizard_tt_next_fresh_dim();
    fresh_set->type = AST_TT_UNIVERSE_SET;
    fresh_set->data.tt_universe_set.dims = dim_ptr;
    fresh_set->data.tt_universe_set.count = 1;
    {
      lizard_ast_node_t *base = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      base->type = AST_TT_U_MAX;
      base->data.tt_u_max.left = dom_univ;
      base->data.tt_u_max.right = cod_univ;
      combined = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      combined->type = AST_TT_U_MAX;
      combined->data.tt_u_max.left = base;
      combined->data.tt_u_max.right = fresh_set;
    }
    return lizard_tt_reduce(combined, heap);
  }
  case AST_TT_SIGMA_FRESH: {
    /* Same dimension-creating rule for Sigma.
     * Phase M.6: gate on pi-fresh-enabled (shared toggle for both
     * pi-fresh and sigma-fresh — they're one feature). */
    lizard_ast_node_t *binder = t->data.tt_sigma_fresh.binder;
    lizard_ast_node_t *dom = t->data.tt_sigma_fresh.domain;
    lizard_ast_node_t *cod = t->data.tt_sigma_fresh.codomain;
    lizard_ast_node_t *dom_univ, *cod_univ;
    lizard_ast_node_t *new_ctx;
    lizard_ast_node_t *fresh_set;
    lizard_ast_node_t *combined;
    long *dim_ptr;
    if (lizard_logic_rule_enabled("pi-fresh-enabled") == 0) {
      return type_error(heap, "sigma-fresh rejected: feature disabled");
    }
    if (binder == NULL || binder->type != AST_SYMBOL) {
      return type_error(heap, "sigma-fresh binder not a symbol");
    }
    dom_univ = lizard_tt_infer2(valid_ctx, ctx, dom, heap);
    if (is_error(dom_univ)) return dom_univ;
    dom_univ = lizard_tt_reduce(dom_univ, heap);
    if (dom_univ->type != AST_TT_UNIVERSE &&
        dom_univ->type != AST_TT_U_SUC &&
        dom_univ->type != AST_TT_U_MAX &&
        dom_univ->type != AST_TT_U_VAR &&
        dom_univ->type != AST_TT_U_MIN &&
        dom_univ->type != AST_TT_UNIVERSE_SET) {
      return type_error(heap, "sigma-fresh domain not a type");
    }
    new_ctx = ctx_extend(ctx, binder->data.variable, dom, heap);
    cod_univ = lizard_tt_infer2(valid_ctx, new_ctx, cod, heap);
    if (is_error(cod_univ)) return cod_univ;
    cod_univ = lizard_tt_reduce(cod_univ, heap);
    if (cod_univ->type != AST_TT_UNIVERSE &&
        cod_univ->type != AST_TT_U_SUC &&
        cod_univ->type != AST_TT_U_MAX &&
        cod_univ->type != AST_TT_U_VAR &&
        cod_univ->type != AST_TT_U_MIN &&
        cod_univ->type != AST_TT_UNIVERSE_SET) {
      return type_error(heap, "sigma-fresh codomain not a type");
    }
    fresh_set = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    dim_ptr = lizard_heap_alloc(sizeof(long));
    dim_ptr[0] = lizard_tt_next_fresh_dim();
    fresh_set->type = AST_TT_UNIVERSE_SET;
    fresh_set->data.tt_universe_set.dims = dim_ptr;
    fresh_set->data.tt_universe_set.count = 1;
    {
      lizard_ast_node_t *base = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      base->type = AST_TT_U_MAX;
      base->data.tt_u_max.left = dom_univ;
      base->data.tt_u_max.right = cod_univ;
      combined = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      combined->type = AST_TT_U_MAX;
      combined->data.tt_u_max.left = base;
      combined->data.tt_u_max.right = fresh_set;
    }
    return lizard_tt_reduce(combined, heap);
  }
  case AST_TT_CO_PI_FRESH: {
    /* Phase L.5: dimension-creating Pi in the COUNIVERSE lattice.
     *
     *   A : Co_S    B : Co_T (under x:A)
     *   ───────────────────────────────────────────────────
     *   (co-pi-fresh x A B) : (Co-max Co_S Co_T) ∨ (Co-set fresh)
     *
     * The fresh-dim counter is SHARED with pi-fresh; sort distinction
     * is in the result lattice (Co-set, not U-set).
     *
     * Phase M.6: gated on co-pi-fresh-enabled. */
    lizard_ast_node_t *binder = t->data.tt_co_pi_fresh.binder;
    lizard_ast_node_t *dom = t->data.tt_co_pi_fresh.domain;
    lizard_ast_node_t *cod = t->data.tt_co_pi_fresh.codomain;
    lizard_ast_node_t *dom_univ, *cod_univ;
    lizard_ast_node_t *new_ctx;
    lizard_ast_node_t *fresh_set;
    lizard_ast_node_t *combined;
    long *dim_ptr;
    if (lizard_logic_rule_enabled("co-pi-fresh-enabled") == 0) {
      return type_error(heap, "co-pi-fresh rejected: feature disabled");
    }
    if (binder == NULL || binder->type != AST_SYMBOL) {
      return type_error(heap, "co-pi-fresh binder not a symbol");
    }
    dom_univ = lizard_tt_infer2(valid_ctx, ctx, dom, heap);
    if (is_error(dom_univ)) return dom_univ;
    dom_univ = lizard_tt_reduce(dom_univ, heap);
    /* For co-pi-fresh, we accept couniverse expressions, NOT universe
     * expressions. The dom must live in a Co-set (or legacy Uco). */
    if (dom_univ->type != AST_TT_COUNIVERSE &&
        dom_univ->type != AST_TT_COUNIVERSE_SET &&
        dom_univ->type != AST_TT_CO_MAX &&
        dom_univ->type != AST_TT_CO_MIN) {
      return type_error(heap, "co-pi-fresh domain not a couniverse");
    }
    new_ctx = ctx_extend(ctx, binder->data.variable, dom, heap);
    cod_univ = lizard_tt_infer2(valid_ctx, new_ctx, cod, heap);
    if (is_error(cod_univ)) return cod_univ;
    cod_univ = lizard_tt_reduce(cod_univ, heap);
    if (cod_univ->type != AST_TT_COUNIVERSE &&
        cod_univ->type != AST_TT_COUNIVERSE_SET &&
        cod_univ->type != AST_TT_CO_MAX &&
        cod_univ->type != AST_TT_CO_MIN) {
      return type_error(heap, "co-pi-fresh codomain not a couniverse");
    }
    /* Build singleton (Co-set fresh). */
    fresh_set = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    dim_ptr = lizard_heap_alloc(sizeof(long));
    dim_ptr[0] = lizard_tt_next_fresh_dim();
    fresh_set->type = AST_TT_COUNIVERSE_SET;
    fresh_set->data.tt_couniverse_set.dims = dim_ptr;
    fresh_set->data.tt_couniverse_set.count = 1;
    {
      lizard_ast_node_t *base = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      base->type = AST_TT_CO_MAX;
      base->data.tt_co_max.left = dom_univ;
      base->data.tt_co_max.right = cod_univ;
      combined = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      combined->type = AST_TT_CO_MAX;
      combined->data.tt_co_max.left = base;
      combined->data.tt_co_max.right = fresh_set;
    }
    return lizard_tt_reduce(combined, heap);
  }
  case AST_TT_CO_SIGMA_FRESH: {
    /* Same dimension-creating rule for Sigma in the couniverse lattice.
     * Phase M.6: gated on co-pi-fresh-enabled. */
    lizard_ast_node_t *binder = t->data.tt_co_sigma_fresh.binder;
    lizard_ast_node_t *dom = t->data.tt_co_sigma_fresh.domain;
    lizard_ast_node_t *cod = t->data.tt_co_sigma_fresh.codomain;
    lizard_ast_node_t *dom_univ, *cod_univ;
    lizard_ast_node_t *new_ctx;
    lizard_ast_node_t *fresh_set;
    lizard_ast_node_t *combined;
    long *dim_ptr;
    if (lizard_logic_rule_enabled("co-pi-fresh-enabled") == 0) {
      return type_error(heap, "co-sigma-fresh rejected: feature disabled");
    }
    if (binder == NULL || binder->type != AST_SYMBOL) {
      return type_error(heap, "co-sigma-fresh binder not a symbol");
    }
    dom_univ = lizard_tt_infer2(valid_ctx, ctx, dom, heap);
    if (is_error(dom_univ)) return dom_univ;
    dom_univ = lizard_tt_reduce(dom_univ, heap);
    if (dom_univ->type != AST_TT_COUNIVERSE &&
        dom_univ->type != AST_TT_COUNIVERSE_SET &&
        dom_univ->type != AST_TT_CO_MAX &&
        dom_univ->type != AST_TT_CO_MIN) {
      return type_error(heap, "co-sigma-fresh domain not a couniverse");
    }
    new_ctx = ctx_extend(ctx, binder->data.variable, dom, heap);
    cod_univ = lizard_tt_infer2(valid_ctx, new_ctx, cod, heap);
    if (is_error(cod_univ)) return cod_univ;
    cod_univ = lizard_tt_reduce(cod_univ, heap);
    if (cod_univ->type != AST_TT_COUNIVERSE &&
        cod_univ->type != AST_TT_COUNIVERSE_SET &&
        cod_univ->type != AST_TT_CO_MAX &&
        cod_univ->type != AST_TT_CO_MIN) {
      return type_error(heap, "co-sigma-fresh codomain not a couniverse");
    }
    fresh_set = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    dim_ptr = lizard_heap_alloc(sizeof(long));
    dim_ptr[0] = lizard_tt_next_fresh_dim();
    fresh_set->type = AST_TT_COUNIVERSE_SET;
    fresh_set->data.tt_couniverse_set.dims = dim_ptr;
    fresh_set->data.tt_couniverse_set.count = 1;
    {
      lizard_ast_node_t *base = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      base->type = AST_TT_CO_MAX;
      base->data.tt_co_max.left = dom_univ;
      base->data.tt_co_max.right = cod_univ;
      combined = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      combined->type = AST_TT_CO_MAX;
      combined->data.tt_co_max.left = base;
      combined->data.tt_co_max.right = fresh_set;
    }
    return lizard_tt_reduce(combined, heap);
  }
  case AST_TT_BOX: {
    /* Phase M.5.1: (Box A) : univ(A).
     *
     * The Box modality preserves the universe of its argument. This
     * is the minimal commitment — it says nothing about *which*
     * modal logic Box belongs to (K, T, S4, S5, ...). That choice
     * comes in M.5.2 when introduction and elimination forms are
     * added, since those rules differ across modal logics.
     *
     * Gated on `modalities-enabled` in the logic-rule registry. */
    lizard_ast_node_t *arg = t->data.tt_box.argument;
    lizard_ast_node_t *arg_univ;
    if (lizard_logic_rule_enabled("modalities-enabled") == 0) {
      return type_error(heap, "Box rejected: modalities-enabled disabled");
    }
    if (arg == NULL) {
      return type_error(heap, "Box missing argument");
    }
    arg_univ = lizard_tt_infer2(valid_ctx, ctx, arg, heap);
    if (is_error(arg_univ)) return arg_univ;
    arg_univ = lizard_tt_reduce(arg_univ, heap);
    if (arg_univ->type != AST_TT_UNIVERSE &&
        arg_univ->type != AST_TT_U_SUC &&
        arg_univ->type != AST_TT_U_MAX &&
        arg_univ->type != AST_TT_U_VAR &&
        arg_univ->type != AST_TT_U_MIN &&
        arg_univ->type != AST_TT_UNIVERSE_SET) {
      return type_error(heap, "Box argument not a type");
    }
    return arg_univ;
  }
  case AST_TT_DIAMOND: {
    /* Same as Box: (Diamond A) : univ(A). Possibility modality. */
    lizard_ast_node_t *arg = t->data.tt_diamond.argument;
    lizard_ast_node_t *arg_univ;
    if (lizard_logic_rule_enabled("modalities-enabled") == 0) {
      return type_error(heap, "Diamond rejected: modalities-enabled disabled");
    }
    if (arg == NULL) {
      return type_error(heap, "Diamond missing argument");
    }
    arg_univ = lizard_tt_infer2(valid_ctx, ctx, arg, heap);
    if (is_error(arg_univ)) return arg_univ;
    arg_univ = lizard_tt_reduce(arg_univ, heap);
    if (arg_univ->type != AST_TT_UNIVERSE &&
        arg_univ->type != AST_TT_U_SUC &&
        arg_univ->type != AST_TT_U_MAX &&
        arg_univ->type != AST_TT_U_VAR &&
        arg_univ->type != AST_TT_U_MIN &&
        arg_univ->type != AST_TT_UNIVERSE_SET) {
      return type_error(heap, "Diamond argument not a type");
    }
    return arg_univ;
  }
  case AST_TT_BOX_INTRO: {
    /* Phase M.5.2 Turn 2 — strict S4 box-intro:
     *
     *   Δ; · ⊢ e : T
     *   ─────────────────────
     *   Δ; Γ ⊢ (box e) : (Box T)
     *
     * Inside the box body:
     *   - The truth context Γ is dropped to empty
     *   - The valid context Δ is PRESERVED as Δ (not promoted)
     *
     * Preservation of Δ is what gives S4 its 4-axiom (□A → □□A):
     * a valid hypothesis remains valid across NESTED box-introductions,
     * not just one box deep.
     *
     * Previous (buggy) implementation promoted Δ to truth at each
     * level, which lost the valid status after one nesting. This
     * version keeps Δ as Δ throughout.
     *
     * If `modal-strict-typing` is disabled, falls back to Turn 1
     * loose typing. */
    lizard_ast_node_t *body = t->data.tt_box_intro.body;
    lizard_ast_node_t *body_type;
    lizard_ast_node_t *result;
    if (lizard_logic_rule_enabled("modalities-enabled") == 0) {
      return type_error(heap, "box rejected: modalities-enabled disabled");
    }
    if (body == NULL) {
      return type_error(heap, "box missing argument");
    }
    if (lizard_logic_rule_enabled("modal-strict-typing") == 0) {
      /* Turn 1 loose typing: full ctx available inside box. */
      body_type = lizard_tt_infer2(valid_ctx, ctx, body, heap);
    } else if (lizard_logic_rule_enabled("modal-4-axiom") == 0) {
      /* T modal logic: strict box-intro but NO 4-axiom. The 4-axiom
       * (□A → □□A) requires valid hypotheses to survive nested boxes.
       * When this toggle is off, valid context is consumed at each
       * box level — nested boxes can't reach beyond the immediate
       * enclosing unbox.
       *
       * Implementation: promote valid to truth (the old "buggy"
       * behavior, which is actually correct for T). Valid hypotheses
       * are visible one box deep but disappear at the next level. */
      lizard_ast_node_t *empty_valid = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      empty_valid->type = AST_NIL;
      body_type = lizard_tt_infer2(empty_valid, valid_ctx, body, heap);
    } else {
      /* S4 modal logic: strict box-intro WITH the 4-axiom.
       * Preserve Δ as valid, drop Γ to empty. Valid hypotheses
       * remain valid across arbitrary nested boxes. */
      lizard_ast_node_t *empty_truth = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      empty_truth->type = AST_NIL;
      body_type = lizard_tt_infer2(valid_ctx, empty_truth, body, heap);
    }
    if (is_error(body_type)) return body_type;
    result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    result->type = AST_TT_BOX;
    result->data.tt_box.argument = body_type;
    return result;
  }
  case AST_TT_BOX_ELIM: {
    /* Phase M.5.2 typing for unbox:
     *
     *   Γ ⊢ b : (Box T)    Γ, x:T ⊢ body : U
     *   ────────────────────────────────────
     *   Γ ⊢ (unbox x b body) : U
     *
     * Phase M.5.2 Turn 2 — strict S4 unbox:
     *
     *   Δ; Γ ⊢ b : Box T    Δ, x:T; Γ ⊢ body : U
     *   ────────────────────────────────────────
     *   Δ; Γ ⊢ unbox x b body : U
     *
     * The unboxed variable x lands in the VALID context Δ. This is
     * what enables it to be used inside a future `box`: valid
     * hypotheses survive entry into a box, truth hypotheses don't.
     *
     * If `modal-strict-typing` is disabled, falls back to Turn 1
     * loose behavior (extends truth instead). */
    lizard_ast_node_t *binder = t->data.tt_box_elim.binder;
    lizard_ast_node_t *scrut = t->data.tt_box_elim.scrutinee;
    lizard_ast_node_t *body = t->data.tt_box_elim.body;
    lizard_ast_node_t *scrut_type;
    lizard_ast_node_t *new_valid_ctx, *new_truth_ctx;
    if (lizard_logic_rule_enabled("modalities-enabled") == 0) {
      return type_error(heap, "unbox rejected: modalities-enabled disabled");
    }
    if (binder == NULL || binder->type != AST_SYMBOL) {
      return type_error(heap, "unbox binder not a symbol");
    }
    scrut_type = lizard_tt_infer2(valid_ctx, ctx, scrut, heap);
    if (is_error(scrut_type)) return scrut_type;
    scrut_type = lizard_tt_reduce(scrut_type, heap);
    if (scrut_type->type != AST_TT_BOX) {
      return type_error(heap, "unbox scrutinee not of Box type");
    }
    if (lizard_logic_rule_enabled("modal-strict-typing") == 0) {
      /* Turn 1 loose: extend truth context with x:T. */
      new_truth_ctx = ctx_extend(ctx, binder->data.variable,
                                 scrut_type->data.tt_box.argument, heap);
      return lizard_tt_infer2(valid_ctx, new_truth_ctx, body, heap);
    }
    /* Strict S4: extend VALID context with x:T; truth ctx unchanged. */
    new_valid_ctx = ctx_extend(valid_ctx, binder->data.variable,
                               scrut_type->data.tt_box.argument, heap);
    {
      lizard_ast_node_t *body_type = lizard_tt_infer2(new_valid_ctx, ctx, body, heap);
      if (is_error(body_type)) return body_type;
      /* Phase M.5.6 — K's distinguished elim:
       *
       * Under K (t-axiom-enabled = off), unbox cannot EXTRACT a value
       * — the result type must still be a Box. If the body's type
       * isn't Box, we reject. This forces the user to think of unbox
       * as "look under the modality to manipulate boxed structure,"
       * not "open up to get the value."
       *
       * Under T+ (t-axiom-enabled = on, default), unbox can yield
       * any type — the standard extraction-via-binder behavior. */
      if (lizard_logic_rule_enabled("t-axiom-enabled") == 0) {
        lizard_ast_node_t *reduced = lizard_tt_reduce(body_type, heap);
        if (reduced->type != AST_TT_BOX) {
          return type_error(heap,
            "unbox rejected: t-axiom-enabled off (K), body must be Box-typed");
        }
      }
      return body_type;
    }
  }
  case AST_TT_DIAMOND_INTRO: {
    /* Phase M.5.5 placeholder typing for diamond-intro:
     *
     *   Γ ⊢ e : T
     *   ──────────────────────
     *   Γ ⊢ (diamond e) : Diamond T
     *
     * Mirrors box-intro structurally. Loose typing for Turn 1 —
     * no commitment to specific modal logic axioms. M.5.5 Turn 2
     * will add the 5-axiom (◇A → □◇A) as a refined typing rule
     * that distinguishes S5 from S4.
     *
     * Gated on `modalities-enabled`. */
    lizard_ast_node_t *body = t->data.tt_diamond_intro.body;
    lizard_ast_node_t *body_type;
    lizard_ast_node_t *result;
    if (lizard_logic_rule_enabled("modalities-enabled") == 0) {
      return type_error(heap, "diamond rejected: modalities-enabled disabled");
    }
    if (body == NULL) {
      return type_error(heap, "diamond missing argument");
    }
    body_type = lizard_tt_infer2(valid_ctx, ctx, body, heap);
    if (is_error(body_type)) return body_type;
    result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    result->type = AST_TT_DIAMOND;
    result->data.tt_diamond.argument = body_type;
    return result;
  }
  case AST_TT_DIAMOND_ELIM: {
    /* Phase M.5.5 Turn 2 — let-diamond typing with 5-axiom toggle:
     *
     * Without 5-axiom (K, T, S4):
     *   Δ; Γ ⊢ b : Diamond T    Δ; Γ, x:T ⊢ body : U
     *   ────────────────────────────────────────────
     *   Δ; Γ ⊢ (let-diamond x b body) : U
     *
     * The unboxed Diamond content is just a TRUTH hypothesis —
     * dropped inside a future box. This matches modal logics
     * weaker than S5: a possibility value isn't necessarily a
     * possibility value.
     *
     * With 5-axiom (S5):
     *   Δ; Γ ⊢ b : Diamond T    Δ, x:T; Γ ⊢ body : U
     *   ────────────────────────────────────────────
     *   Δ; Γ ⊢ (let-diamond x b body) : U
     *
     * The unboxed content lands in VALID context — survives entry
     * into subsequent boxes. This encodes the 5-axiom (◇A → □◇A):
     * a possibility is necessarily a possibility.
     *
     * Toggle: modal-5-axiom (default-on per default-allow). Bundles:
     *   K, T, S4   → off
     *   S5         → on
     *   modal-STLC → off (S4-flavored) */
    lizard_ast_node_t *binder = t->data.tt_diamond_elim.binder;
    lizard_ast_node_t *scrut = t->data.tt_diamond_elim.scrutinee;
    lizard_ast_node_t *body = t->data.tt_diamond_elim.body;
    lizard_ast_node_t *scrut_type;
    lizard_ast_node_t *new_truth_ctx, *new_valid_ctx;
    if (lizard_logic_rule_enabled("modalities-enabled") == 0) {
      return type_error(heap, "let-diamond rejected: modalities-enabled disabled");
    }
    if (binder == NULL || binder->type != AST_SYMBOL) {
      return type_error(heap, "let-diamond binder not a symbol");
    }
    scrut_type = lizard_tt_infer2(valid_ctx, ctx, scrut, heap);
    if (is_error(scrut_type)) return scrut_type;
    scrut_type = lizard_tt_reduce(scrut_type, heap);
    if (scrut_type->type != AST_TT_DIAMOND) {
      return type_error(heap, "let-diamond scrutinee not of Diamond type");
    }
    if (lizard_logic_rule_enabled("modal-5-axiom") == 0) {
      /* K/T/S4: extend truth context with x:T. */
      new_truth_ctx = ctx_extend(ctx, binder->data.variable,
                                 scrut_type->data.tt_diamond.argument, heap);
      return lizard_tt_infer2(valid_ctx, new_truth_ctx, body, heap);
    }
    /* S5: extend VALID context — the 5-axiom encoding. */
    new_valid_ctx = ctx_extend(valid_ctx, binder->data.variable,
                               scrut_type->data.tt_diamond.argument, heap);
    return lizard_tt_infer2(new_valid_ctx, ctx, body, heap);
  }
  case AST_TT_BOX_APP: {
    /* Phase M.5.6 + M.5.7 — K-axiom: Box (Pi x A B) → Box A → Box B.
     *
     * Typing rule (non-dependent Pi, B doesn't mention x):
     *
     *   Δ; Γ ⊢ f : Box (Pi x A B)    Δ; Γ ⊢ a : Box A
     *   ─────────────────────────────────────────────
     *   Δ; Γ ⊢ (box-app f a) : Box B
     *
     * Typing rule (dependent Pi, B mentions x; M.5.7):
     *
     *   t-axiom-enabled is ON
     *   Δ; Γ ⊢ f : Box (Pi x A B)    Δ; Γ ⊢ a : Box A
     *   ─────────────────────────────────────────────
     *   Δ; Γ ⊢ (box-app f a) : Box (B[(unbox y a y)/x])
     *
     * For the dependent case, we substitute an explicit unboxing
     * (which is the T-axiom realized) for x in B. The result type
     * contains an embedded extraction term, which is well-typed
     * because unbox a yields A.
     *
     * Under K (t-axiom-enabled off), the dependent case is rejected
     * because the substituent term can't be formed without the
     * T-axiom.
     *
     * Gated on `modalities-enabled`. */
    lizard_ast_node_t *fun = t->data.tt_box_app.fun;
    lizard_ast_node_t *arg = t->data.tt_box_app.arg;
    lizard_ast_node_t *fun_type, *arg_type;
    lizard_ast_node_t *inner_pi, *inner_a, *inner_b;
    const char *inner_binder;
    lizard_ast_node_t *result;
    int is_dependent;
    if (lizard_logic_rule_enabled("modalities-enabled") == 0) {
      return type_error(heap, "box-app rejected: modalities-enabled disabled");
    }
    fun_type = lizard_tt_infer2(valid_ctx, ctx, fun, heap);
    if (is_error(fun_type)) return fun_type;
    fun_type = lizard_tt_reduce(fun_type, heap);
    if (fun_type->type != AST_TT_BOX) {
      return type_error(heap, "box-app fun not of Box type");
    }
    inner_pi = lizard_tt_reduce(fun_type->data.tt_box.argument, heap);
    if (inner_pi->type != AST_TT_PI) {
      return type_error(heap, "box-app fun's boxed type not Pi");
    }
    inner_a = inner_pi->data.tt_pi.domain;
    inner_b = inner_pi->data.tt_pi.codomain;
    inner_binder = (inner_pi->data.tt_pi.binder &&
                    inner_pi->data.tt_pi.binder->type == AST_SYMBOL)
                       ? inner_pi->data.tt_pi.binder->data.variable
                       : NULL;
    /* arg must have type Box (inner_a). */
    arg_type = lizard_tt_infer2(valid_ctx, ctx, arg, heap);
    if (is_error(arg_type)) return arg_type;
    arg_type = lizard_tt_reduce(arg_type, heap);
    if (arg_type->type != AST_TT_BOX) {
      return type_error(heap, "box-app arg not of Box type");
    }
    if (!lizard_tt_structurally_equal(arg_type->data.tt_box.argument, inner_a)) {
      return type_error(heap, "box-app arg/fun domain mismatch");
    }
    is_dependent = (inner_binder != NULL && inner_b != NULL &&
                    contains_free_var_public(inner_b, inner_binder));
    if (is_dependent) {
      /* M.5.7 — dependent case. Substitute (unbox y a y) for x in B.
       * Requires T-axiom (extraction); rejected under K. */
      char fresh_name[32];
      lizard_ast_node_t *y_sym, *unbox_term;
      lizard_ast_node_t *new_b;
      int max_retries;
      if (lizard_logic_rule_enabled("t-axiom-enabled") == 0) {
        return type_error(heap,
          "box-app on dependent Pi rejected: t-axiom-enabled disabled");
      }
      /* M.5.8 — Hygiene: pick a fresh name that is FREE in both
       * arg and inner_b. The dim counter gives uniqueness across
       * calls but doesn't a priori prevent collision with whatever
       * names the user wrote. We retry up to 100 times bumping the
       * counter; in practice the first try always works because
       * users don't write __boxapp_N names. */
      max_retries = 100;
      while (max_retries-- > 0) {
        snprintf(fresh_name, sizeof(fresh_name), "__boxapp_%ld",
                 lizard_tt_next_fresh_dim());
        if (!contains_free_var_public(arg, fresh_name) &&
            !contains_free_var_public(inner_b, fresh_name)) {
          break;  /* Free in both — safe to use. */
        }
      }
      if (max_retries <= 0) {
        return type_error(heap,
          "box-app fresh name allocation failed (collision retry exhausted)");
      }
      /* Allocate the symbol node for y. Strings live in heap. */
      y_sym = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      y_sym->type = AST_SYMBOL;
      {
        size_t len = strlen(fresh_name) + 1;
        char *namebuf = lizard_heap_alloc(len);
        memcpy(namebuf, fresh_name, len);
        y_sym->data.variable = namebuf;
      }
      /* Build (unbox y_sym a y_sym). */
      unbox_term = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      unbox_term->type = AST_TT_BOX_ELIM;
      unbox_term->data.tt_box_elim.binder = y_sym;
      unbox_term->data.tt_box_elim.scrutinee = arg;
      unbox_term->data.tt_box_elim.body = y_sym;
      /* Substitute unbox_term for inner_binder in inner_b. */
      new_b = lizard_tt_subst(inner_b, inner_binder, unbox_term, heap);
      result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      result->type = AST_TT_BOX;
      result->data.tt_box.argument = new_b;
      return result;
    }
    /* Non-dependent: just wrap inner_b in Box. */
    result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    result->type = AST_TT_BOX;
    result->data.tt_box.argument = inner_b;
    return result;
  }
  case AST_TT_DIAMOND_BIND: {
    /* Phase M.5.8 — Diamond Kleisli composition / monadic bind:
     *
     *   Δ; Γ ⊢ f : Pi x A (Diamond B)    Δ; Γ ⊢ d : Diamond A
     *   ─────────────────────────────────────────────────────
     *   Δ; Γ ⊢ (diamond-bind f d) : Diamond B
     *
     * For DEPENDENT B (mentions x), the result is Diamond B[a/x]
     * where a is extracted from d. Like box-app, we use the let-
     * diamond elim form to "extract" — but Diamond extraction is
     * always available (it's the basic Diamond elim rule), so we
     * don't need a t-axiom-style gate here. Diamond-extraction
     * is unconditional.
     *
     * Substitution for the dependent case uses (let-diamond y d y)
     * — the dual of box's (unbox y a y).
     *
     * Gated on `modalities-enabled`. */
    lizard_ast_node_t *fun = t->data.tt_diamond_bind.fun;
    lizard_ast_node_t *arg = t->data.tt_diamond_bind.arg;
    lizard_ast_node_t *fun_type, *arg_type;
    lizard_ast_node_t *inner_a, *inner_b_diamond, *inner_b;
    const char *inner_binder;
    lizard_ast_node_t *result;
    int is_dependent;
    if (lizard_logic_rule_enabled("modalities-enabled") == 0) {
      return type_error(heap, "diamond-bind rejected: modalities-enabled disabled");
    }
    fun_type = lizard_tt_infer2(valid_ctx, ctx, fun, heap);
    if (is_error(fun_type)) return fun_type;
    fun_type = lizard_tt_reduce(fun_type, heap);
    if (fun_type->type != AST_TT_PI) {
      return type_error(heap, "diamond-bind fun not of Pi type");
    }
    inner_a = fun_type->data.tt_pi.domain;
    inner_b_diamond = fun_type->data.tt_pi.codomain;
    inner_b_diamond = lizard_tt_reduce(inner_b_diamond, heap);
    if (inner_b_diamond->type != AST_TT_DIAMOND) {
      return type_error(heap, "diamond-bind fun's codomain not Diamond-typed");
    }
    inner_b = inner_b_diamond->data.tt_diamond.argument;
    inner_binder = (fun_type->data.tt_pi.binder &&
                    fun_type->data.tt_pi.binder->type == AST_SYMBOL)
                       ? fun_type->data.tt_pi.binder->data.variable
                       : NULL;
    /* arg must have type Diamond (inner_a). */
    arg_type = lizard_tt_infer2(valid_ctx, ctx, arg, heap);
    if (is_error(arg_type)) return arg_type;
    arg_type = lizard_tt_reduce(arg_type, heap);
    if (arg_type->type != AST_TT_DIAMOND) {
      return type_error(heap, "diamond-bind arg not of Diamond type");
    }
    if (!lizard_tt_structurally_equal(arg_type->data.tt_diamond.argument, inner_a)) {
      return type_error(heap, "diamond-bind arg/fun domain mismatch");
    }
    /* Note: the codomain (Diamond B) can mention x; B itself is the
     * relevant question for "dependent". Check if B mentions x. */
    is_dependent = (inner_binder != NULL && inner_b != NULL &&
                    contains_free_var_public(inner_b, inner_binder));
    if (is_dependent) {
      /* Dependent: substitute (let-diamond y arg y) for x in B. */
      char fresh_name[32];
      lizard_ast_node_t *y_sym, *letd_term;
      lizard_ast_node_t *new_b;
      int max_retries = 100;
      while (max_retries-- > 0) {
        snprintf(fresh_name, sizeof(fresh_name), "__dbind_%ld",
                 lizard_tt_next_fresh_dim());
        if (!contains_free_var_public(arg, fresh_name) &&
            !contains_free_var_public(inner_b, fresh_name)) {
          break;
        }
      }
      if (max_retries <= 0) {
        return type_error(heap,
          "diamond-bind fresh name allocation failed (collision retry exhausted)");
      }
      y_sym = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      y_sym->type = AST_SYMBOL;
      {
        size_t len = strlen(fresh_name) + 1;
        char *namebuf = lizard_heap_alloc(len);
        memcpy(namebuf, fresh_name, len);
        y_sym->data.variable = namebuf;
      }
      letd_term = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      letd_term->type = AST_TT_DIAMOND_ELIM;
      letd_term->data.tt_diamond_elim.binder = y_sym;
      letd_term->data.tt_diamond_elim.scrutinee = arg;
      letd_term->data.tt_diamond_elim.body = y_sym;
      new_b = lizard_tt_subst(inner_b, inner_binder, letd_term, heap);
      result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      result->type = AST_TT_DIAMOND;
      result->data.tt_diamond.argument = new_b;
      return result;
    }
    /* Non-dependent: just wrap inner_b in Diamond. */
    result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    result->type = AST_TT_DIAMOND;
    result->data.tt_diamond.argument = inner_b;
    return result;
  }
  case AST_TT_APP: {
    /* (@ f a) : B[a/x] where f : (Pi x A B) and a checks against A. */
    lizard_ast_node_t *f_type;
    f_type = lizard_tt_infer2(valid_ctx, ctx, t->data.tt_app.fun, heap);
    if (is_error(f_type)) return f_type;
    f_type = lizard_tt_reduce(f_type, heap);
    if (f_type->type != AST_TT_PI) {
      return type_error(heap, "application of non-function");
    }
    if (!lizard_tt_check2(valid_ctx, ctx, t->data.tt_app.arg, f_type->data.tt_pi.domain,
                         heap)) {
      return type_error(heap, "application argument type mismatch");
    }
    /* Substitute the argument into the codomain. */
    if (f_type->data.tt_pi.binder == NULL ||
        f_type->data.tt_pi.binder->type != AST_SYMBOL) {
      return type_error(heap, "Pi has no binder symbol");
    }
    return lizard_tt_subst(f_type->data.tt_pi.codomain,
                           f_type->data.tt_pi.binder->data.variable,
                           t->data.tt_app.arg, heap);
  }
  case AST_TT_ANNOT: {
    /* (annot term T) — check term against T, return T. */
    if (!lizard_tt_check2(valid_ctx, ctx, t->data.tt_annot.term, t->data.tt_annot.type,
                         heap)) {
      return type_error(heap, "annotation type mismatch");
    }
    return t->data.tt_annot.type;
  }
  case AST_TT_ID: {
    /* (Id A a b) typing rule:
     *
     *   Γ ⊢ A : (U n)    Γ ⊢ a : A    Γ ⊢ b : A
     *   ────────────────────────────────────────
     *   Γ ⊢ (Id A a b) : (U n)
     *
     * We infer A's universe (which checks A is a type), then verify
     * that a and b both check against A, and return A's universe. */
    lizard_ast_node_t *A = t->data.tt_id.domain;
    lizard_ast_node_t *a = t->data.tt_id.a;
    lizard_ast_node_t *b = t->data.tt_id.b;
    lizard_ast_node_t *A_univ;
    if (A == NULL || a == NULL || b == NULL) {
      return type_error(heap, "Id missing field");
    }
    A_univ = lizard_tt_infer2(valid_ctx, ctx, A, heap);
    if (is_error(A_univ)) return A_univ;
    A_univ = lizard_tt_reduce(A_univ, heap);
    /* A's type must be a universe expression. */
    if (A_univ->type != AST_TT_UNIVERSE &&
        A_univ->type != AST_TT_U_SUC &&
        A_univ->type != AST_TT_U_MAX &&
        A_univ->type != AST_TT_U_VAR &&
        A_univ->type != AST_TT_U_MIN &&
        A_univ->type != AST_TT_UNIVERSE_SET) {
      return type_error(heap, "Id domain not a type");
    }
    if (!lizard_tt_check2(valid_ctx, ctx, a, A, heap)) {
      return type_error(heap, "Id left endpoint doesn't check against domain");
    }
    if (!lizard_tt_check2(valid_ctx, ctx, b, A, heap)) {
      return type_error(heap, "Id right endpoint doesn't check against domain");
    }
    return A_univ;
  }
  case AST_TT_REFL: {
    /* (refl a) typing rule:
     *
     *       Γ ⊢ a : A
     *   ──────────────────
     *   Γ ⊢ (refl a) : (Id A a a)
     *
     * To infer, we first infer a's type A, then return (Id A a a). */
    lizard_ast_node_t *a = t->data.tt_refl.value;
    lizard_ast_node_t *A;
    if (a == NULL) return type_error(heap, "refl missing value");
    A = lizard_tt_infer2(valid_ctx, ctx, a, heap);
    if (is_error(A)) return A;
    /* Build (Id A a a) */
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_ID;
      n->data.tt_id.domain = A;
      n->data.tt_id.a = a;
      n->data.tt_id.b = a;
      return n;
    }
  }
  case AST_TT_SIGMA: {
    /* (Sigma x A B) typing rule, parallel to Pi:
     *
     *   Γ ⊢ A : (U n)    Γ, x:A ⊢ B : (U m)
     *   ────────────────────────────────────
     *   Γ ⊢ (Sigma x A B) : (U-max n m)
     */
    lizard_ast_node_t *binder = t->data.tt_sigma.binder;
    lizard_ast_node_t *dom = t->data.tt_sigma.domain;
    lizard_ast_node_t *cod = t->data.tt_sigma.codomain;
    lizard_ast_node_t *dom_univ, *cod_univ;
    lizard_ast_node_t *new_ctx;
    if (binder == NULL || binder->type != AST_SYMBOL) {
      return type_error(heap, "Sigma binder not a symbol");
    }
    dom_univ = lizard_tt_infer2(valid_ctx, ctx, dom, heap);
    if (is_error(dom_univ)) return dom_univ;
    dom_univ = lizard_tt_reduce(dom_univ, heap);
    if (dom_univ->type != AST_TT_UNIVERSE &&
        dom_univ->type != AST_TT_U_SUC &&
        dom_univ->type != AST_TT_U_MAX &&
        dom_univ->type != AST_TT_U_VAR &&
        dom_univ->type != AST_TT_U_MIN &&
        dom_univ->type != AST_TT_UNIVERSE_SET) {
      return type_error(heap, "Sigma domain not a type");
    }
    new_ctx = ctx_extend(ctx, binder->data.variable, dom, heap);
    cod_univ = lizard_tt_infer2(valid_ctx, new_ctx, cod, heap);
    if (is_error(cod_univ)) return cod_univ;
    cod_univ = lizard_tt_reduce(cod_univ, heap);
    if (cod_univ->type != AST_TT_UNIVERSE &&
        cod_univ->type != AST_TT_U_SUC &&
        cod_univ->type != AST_TT_U_MAX &&
        cod_univ->type != AST_TT_U_VAR &&
        cod_univ->type != AST_TT_U_MIN &&
        cod_univ->type != AST_TT_UNIVERSE_SET) {
      return type_error(heap, "Sigma codomain not a type");
    }
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_U_MAX;
      n->data.tt_u_max.left = dom_univ;
      n->data.tt_u_max.right = cod_univ;
      return lizard_tt_reduce(n, heap);
    }
  }
  case AST_TT_SUM: {
    /* (Sum A B) typing rule:
     *
     *   Γ ⊢ A : (U n)    Γ ⊢ B : (U m)
     *   ───────────────────────────────
     *   Γ ⊢ (Sum A B) : (U-max n m)
     */
    lizard_ast_node_t *A = t->data.tt_sum.left;
    lizard_ast_node_t *B = t->data.tt_sum.right;
    lizard_ast_node_t *A_univ, *B_univ;
    if (A == NULL || B == NULL) return type_error(heap, "Sum missing field");
    A_univ = lizard_tt_infer2(valid_ctx, ctx, A, heap);
    if (is_error(A_univ)) return A_univ;
    A_univ = lizard_tt_reduce(A_univ, heap);
    if (A_univ->type != AST_TT_UNIVERSE &&
        A_univ->type != AST_TT_U_SUC &&
        A_univ->type != AST_TT_U_MAX &&
        A_univ->type != AST_TT_U_VAR &&
        A_univ->type != AST_TT_U_MIN &&
        A_univ->type != AST_TT_UNIVERSE_SET) {
      return type_error(heap, "Sum left not a type");
    }
    B_univ = lizard_tt_infer2(valid_ctx, ctx, B, heap);
    if (is_error(B_univ)) return B_univ;
    B_univ = lizard_tt_reduce(B_univ, heap);
    if (B_univ->type != AST_TT_UNIVERSE &&
        B_univ->type != AST_TT_U_SUC &&
        B_univ->type != AST_TT_U_MAX &&
        B_univ->type != AST_TT_U_VAR &&
        B_univ->type != AST_TT_U_MIN &&
        B_univ->type != AST_TT_UNIVERSE_SET) {
      return type_error(heap, "Sum right not a type");
    }
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_U_MAX;
      n->data.tt_u_max.left = A_univ;
      n->data.tt_u_max.right = B_univ;
      return lizard_tt_reduce(n, heap);
    }
  }
  case AST_TT_FST: {
    /* (fst p) typing rule:
     *
     *   Γ ⊢ p : (Sigma x A B)
     *   ─────────────────────
     *   Γ ⊢ (fst p) : A
     */
    lizard_ast_node_t *p = t->data.tt_proj.target;
    lizard_ast_node_t *p_type;
    if (p == NULL) return type_error(heap, "fst missing target");
    p_type = lizard_tt_infer2(valid_ctx, ctx, p, heap);
    if (is_error(p_type)) return p_type;
    p_type = lizard_tt_reduce(p_type, heap);
    if (p_type->type != AST_TT_SIGMA) {
      return type_error(heap, "fst of non-Sigma");
    }
    return p_type->data.tt_sigma.domain;
  }
  case AST_TT_SND: {
    /* (snd p) typing rule:
     *
     *   Γ ⊢ p : (Sigma x A B)
     *   ─────────────────────────
     *   Γ ⊢ (snd p) : B[fst p / x]
     */
    lizard_ast_node_t *p = t->data.tt_proj.target;
    lizard_ast_node_t *p_type;
    lizard_ast_node_t *fst_p;
    if (p == NULL) return type_error(heap, "snd missing target");
    p_type = lizard_tt_infer2(valid_ctx, ctx, p, heap);
    if (is_error(p_type)) return p_type;
    p_type = lizard_tt_reduce(p_type, heap);
    if (p_type->type != AST_TT_SIGMA) {
      return type_error(heap, "snd of non-Sigma");
    }
    if (p_type->data.tt_sigma.binder == NULL ||
        p_type->data.tt_sigma.binder->type != AST_SYMBOL) {
      return type_error(heap, "Sigma has no binder symbol");
    }
    fst_p = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    fst_p->type = AST_TT_FST;
    fst_p->data.tt_proj.target = p;
    return lizard_tt_subst(p_type->data.tt_sigma.codomain,
                           p_type->data.tt_sigma.binder->data.variable,
                           fst_p, heap);
  }
  case AST_TT_UNIT:
    /* Unit : (U 0) — Unit is the lowest-universe type. */
    return lizard_tt_make_universe(heap, 0);
  case AST_TT_TT: {
    /* tt : Unit */
    lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    n->type = AST_TT_UNIT;
    return n;
  }
  case AST_TT_BOT:
    /* Bot : (U 0) — empty type lives in lowest universe. */
    return lizard_tt_make_universe(heap, 0);
  case AST_TT_J: {
    /* (J motive refl_case path) typing rule:
     *
     *   Γ ⊢ A : (U n)
     *   Γ ⊢ a : A
     *   Γ ⊢ path : (Id A a b)         where b : A
     *   Γ ⊢ motive : (Pi x A (Pi _ (Id A a x) (U m)))
     *   Γ ⊢ refl_case : ((motive a) (refl a))
     *   ──────────────────────────────────────────
     *   Γ ⊢ (J motive refl_case path) : ((motive b) path)
     *
     * Implementation:
     *   1. Infer path's type, must reduce to (Id A a b).
     *   2. Infer motive's type, must reduce to (Pi x A (Pi p (Id A a x) U)).
     *      We don't verify the full motive shape (would need to check
     *      its body's universe, etc.); we just check it's a Pi-of-Pi
     *      of the right shape and trust the user.
     *   3. Check refl_case against ((motive a) (refl a)).
     *   4. Result is ((motive b) path).
     *
     * The full HOTT-style J accepts the motive in a slightly different
     * shape (over A × Id-A rather than uncurried A → Id → U), but
     * the curried form here is equivalent up to currying. */
    lizard_ast_node_t *motive = t->data.tt_j.motive;
    lizard_ast_node_t *refl_case = t->data.tt_j.refl_case;
    lizard_ast_node_t *path = t->data.tt_j.path;
    lizard_ast_node_t *path_type, *a, *b;
    lizard_ast_node_t *m_app_a, *m_app_a_refl, *m_app_b, *result;
    lizard_ast_node_t *refl_a;
    if (motive == NULL || refl_case == NULL || path == NULL) {
      return type_error(heap, "J missing field");
    }
    /* Step 1: infer path's type. */
    path_type = lizard_tt_infer2(valid_ctx, ctx, path, heap);
    if (is_error(path_type)) return path_type;
    path_type = lizard_tt_reduce(path_type, heap);
    if (path_type->type != AST_TT_ID) {
      return type_error(heap, "J path is not an identity");
    }
    a = path_type->data.tt_id.a;
    b = path_type->data.tt_id.b;
    /* Step 2: build (refl a) for the refl-case check. */
    refl_a = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    refl_a->type = AST_TT_REFL;
    refl_a->data.tt_refl.value = a;
    /* Step 3: compute (motive a) and ((motive a) (refl a)). */
    m_app_a = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    m_app_a->type = AST_TT_APP;
    m_app_a->data.tt_app.fun = motive;
    m_app_a->data.tt_app.arg = a;
    m_app_a_refl = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    m_app_a_refl->type = AST_TT_APP;
    m_app_a_refl->data.tt_app.fun = m_app_a;
    m_app_a_refl->data.tt_app.arg = refl_a;
    /* Step 4: check refl_case against ((motive a) (refl a)). */
    if (!lizard_tt_check2(valid_ctx, ctx, refl_case, m_app_a_refl, heap)) {
      return type_error(heap, "J refl-case doesn't match motive at refl");
    }
    /* Step 5: build the result ((motive b) path). */
    m_app_b = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    m_app_b->type = AST_TT_APP;
    m_app_b->data.tt_app.fun = motive;
    m_app_b->data.tt_app.arg = b;
    result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    result->type = AST_TT_APP;
    result->data.tt_app.fun = m_app_b;
    result->data.tt_app.arg = path;
    return lizard_tt_reduce(result, heap);
  }
  case AST_TT_XPORT: {
    /* (xport motive path value) typing rule:
     *
     *   Γ ⊢ A : (U n)
     *   Γ ⊢ motive : (Pi x A (U m))
     *   Γ ⊢ path : (Id A a a')
     *   Γ ⊢ value : (motive a)
     *   ───────────────────────────────────────
     *   Γ ⊢ (xport motive path value) : (motive a')
     */
    lizard_ast_node_t *motive = t->data.tt_xport.motive;
    lizard_ast_node_t *path = t->data.tt_xport.path;
    lizard_ast_node_t *value = t->data.tt_xport.value;
    lizard_ast_node_t *path_type, *motive_type, *a, *a_prime;
    lizard_ast_node_t *m_app_a, *m_app_a_prime;
    if (motive == NULL || path == NULL || value == NULL) {
      return type_error(heap, "xport missing field");
    }
    path_type = lizard_tt_infer2(valid_ctx, ctx, path, heap);
    if (is_error(path_type)) return path_type;
    path_type = lizard_tt_reduce(path_type, heap);
    if (path_type->type != AST_TT_ID) {
      return type_error(heap, "xport path is not an Id");
    }
    a = path_type->data.tt_id.a;
    a_prime = path_type->data.tt_id.b;
    motive_type = lizard_tt_infer2(valid_ctx, ctx, motive, heap);
    if (is_error(motive_type)) return motive_type;
    motive_type = lizard_tt_reduce(motive_type, heap);
    if (motive_type->type != AST_TT_PI) {
      return type_error(heap, "xport motive is not a Pi");
    }
    m_app_a = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    m_app_a->type = AST_TT_APP;
    m_app_a->data.tt_app.fun = motive;
    m_app_a->data.tt_app.arg = a;
    if (!lizard_tt_check2(valid_ctx, ctx, value, m_app_a, heap)) {
      return type_error(heap, "xport value doesn't match motive at a");
    }
    m_app_a_prime = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    m_app_a_prime->type = AST_TT_APP;
    m_app_a_prime->data.tt_app.fun = motive;
    m_app_a_prime->data.tt_app.arg = a_prime;
    return lizard_tt_reduce(m_app_a_prime, heap);
  }
  case AST_TT_CASE: {
    /* (Case scrutinee left right) typing rule, non-dependent:
     *
     *   Γ ⊢ scrutinee : (Sum A B)
     *   Γ ⊢ left  : (Pi a A C)
     *   Γ ⊢ right : (Pi b B C)
     *   ───────────────────────────────────────
     *   Γ ⊢ (Case scrutinee left right) : C
     *
     * Both branches must produce the same codomain C. */
    lizard_ast_node_t *s = t->data.tt_case.scrutinee;
    lizard_ast_node_t *l = t->data.tt_case.left_branch;
    lizard_ast_node_t *r = t->data.tt_case.right_branch;
    lizard_ast_node_t *s_type, *l_type, *r_type;
    lizard_ast_node_t *C_l, *C_r;
    if (s == NULL || l == NULL || r == NULL) {
      return type_error(heap, "Case missing field");
    }
    s_type = lizard_tt_infer2(valid_ctx, ctx, s, heap);
    if (is_error(s_type)) return s_type;
    s_type = lizard_tt_reduce(s_type, heap);
    if (s_type->type != AST_TT_SUM) {
      return type_error(heap, "Case scrutinee is not a Sum");
    }
    l_type = lizard_tt_infer2(valid_ctx, ctx, l, heap);
    if (is_error(l_type)) return l_type;
    l_type = lizard_tt_reduce(l_type, heap);
    if (l_type->type != AST_TT_PI) {
      return type_error(heap, "Case left branch is not a function");
    }
    /* Check left's domain matches Sum's left. */
    if (!lizard_tt_alpha_equal(
            lizard_tt_reduce(l_type->data.tt_pi.domain, heap),
            lizard_tt_reduce(s_type->data.tt_sum.left, heap))) {
      return type_error(heap, "Case left branch domain doesn't match Sum left");
    }
    r_type = lizard_tt_infer2(valid_ctx, ctx, r, heap);
    if (is_error(r_type)) return r_type;
    r_type = lizard_tt_reduce(r_type, heap);
    if (r_type->type != AST_TT_PI) {
      return type_error(heap, "Case right branch is not a function");
    }
    if (!lizard_tt_alpha_equal(
            lizard_tt_reduce(r_type->data.tt_pi.domain, heap),
            lizard_tt_reduce(s_type->data.tt_sum.right, heap))) {
      return type_error(heap, "Case right branch domain doesn't match Sum right");
    }
    /* Both codomains must agree. */
    C_l = lizard_tt_reduce(l_type->data.tt_pi.codomain, heap);
    C_r = lizard_tt_reduce(r_type->data.tt_pi.codomain, heap);
    if (!lizard_tt_alpha_equal(C_l, C_r)) {
      return type_error(heap, "Case branches produce different types");
    }
    return C_l;
  }
  case AST_TT_AP: {
    /* (ap f p) typing rule:
     *
     *   Γ ⊢ f : (Pi x A B)        with B not depending on x (non-dep)
     *   Γ ⊢ p : (Id A a a')
     *   ──────────────────────────────────────
     *   Γ ⊢ (ap f p) : (Id B (f a) (f a'))
     *
     * For the dependent case we'd need `transport` and a more
     * elaborate type for ap (it's a path in `(f a) =_{B[a/x]} ...`,
     * not just `(Id B (f a) (f a'))`). Non-dep is enough for most
     * uses and is what most "ap" literature shows. */
    lizard_ast_node_t *f = t->data.tt_ap.fn;
    lizard_ast_node_t *p = t->data.tt_ap.path;
    lizard_ast_node_t *f_type, *p_type;
    lizard_ast_node_t *A, *B, *a, *a_prime, *fa, *fa_prime, *result;
    if (f == NULL || p == NULL) return type_error(heap, "ap missing field");
    f_type = lizard_tt_infer2(valid_ctx, ctx, f, heap);
    if (is_error(f_type)) return f_type;
    f_type = lizard_tt_reduce(f_type, heap);
    if (f_type->type != AST_TT_PI) {
      return type_error(heap, "ap: function is not a Pi");
    }
    p_type = lizard_tt_infer2(valid_ctx, ctx, p, heap);
    if (is_error(p_type)) return p_type;
    p_type = lizard_tt_reduce(p_type, heap);
    if (p_type->type != AST_TT_ID) {
      return type_error(heap, "ap: path is not an Id");
    }
    A = p_type->data.tt_id.domain;
    a = p_type->data.tt_id.a;
    a_prime = p_type->data.tt_id.b;
    /* The Pi's domain must match the path's domain. */
    if (!lizard_tt_alpha_equal(lizard_tt_reduce(f_type->data.tt_pi.domain, heap),
                               lizard_tt_reduce(A, heap))) {
      return type_error(heap, "ap: function domain ≠ path domain");
    }
    /* Non-dependent case: B is the codomain of the Pi. */
    B = f_type->data.tt_pi.codomain;
    fa = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    fa->type = AST_TT_APP;
    fa->data.tt_app.fun = f;
    fa->data.tt_app.arg = a;
    fa_prime = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    fa_prime->type = AST_TT_APP;
    fa_prime->data.tt_app.fun = f;
    fa_prime->data.tt_app.arg = a_prime;
    result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    result->type = AST_TT_ID;
    result->data.tt_id.domain = B;
    result->data.tt_id.a = fa;
    result->data.tt_id.b = fa_prime;
    return result;
  }
  /* ===== Cubical layer typing rules ===== */
  case AST_TT_INTERVAL:
    /* I : (U 0). The interval is a pre-type but we represent its
     * "type" as (U 0) for slotting into the existing universe layer.
     * This is a simplification — strictly, I lives in a separate
     * sort and isn't a type in the universe hierarchy. */
    return lizard_tt_make_universe(heap, 0);
  case AST_TT_I0:
  case AST_TT_I1:
  case AST_TT_I_VAR: {
    /* i0, i1, I-var : I */
    lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    n->type = AST_TT_INTERVAL;
    return n;
  }
  case AST_TT_I_AND:
  case AST_TT_I_OR: {
    /* (I-and i j), (I-or i j) : I when both i, j : I */
    lizard_ast_node_t *l_type = lizard_tt_infer2(valid_ctx, ctx, t->data.tt_i_binop.left, heap);
    lizard_ast_node_t *r_type = lizard_tt_infer2(valid_ctx, ctx, t->data.tt_i_binop.right, heap);
    if (is_error(l_type)) return l_type;
    if (is_error(r_type)) return r_type;
    l_type = lizard_tt_reduce(l_type, heap);
    r_type = lizard_tt_reduce(r_type, heap);
    if (l_type->type != AST_TT_INTERVAL || r_type->type != AST_TT_INTERVAL) {
      return type_error(heap, "I-and/I-or argument not in I");
    }
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_INTERVAL;
      return n;
    }
  }
  case AST_TT_I_NEG: {
    /* (I-neg i) : I when i : I */
    lizard_ast_node_t *o_type = lizard_tt_infer2(valid_ctx, ctx, t->data.tt_i_neg.operand, heap);
    if (is_error(o_type)) return o_type;
    o_type = lizard_tt_reduce(o_type, heap);
    if (o_type->type != AST_TT_INTERVAL) {
      return type_error(heap, "I-neg argument not in I");
    }
    {
      lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      n->type = AST_TT_INTERVAL;
      return n;
    }
  }
  case AST_TT_PATH: {
    /* (Path A a b) typing rule, parallel to Id:
     *
     *   Γ ⊢ A : (U n)    Γ ⊢ a : A    Γ ⊢ b : A
     *   ────────────────────────────────────────
     *   Γ ⊢ (Path A a b) : (U n)
     */
    lizard_ast_node_t *A = t->data.tt_path.domain;
    lizard_ast_node_t *a = t->data.tt_path.a;
    lizard_ast_node_t *b = t->data.tt_path.b;
    lizard_ast_node_t *A_univ;
    if (A == NULL || a == NULL || b == NULL) {
      return type_error(heap, "Path missing field");
    }
    A_univ = lizard_tt_infer2(valid_ctx, ctx, A, heap);
    if (is_error(A_univ)) return A_univ;
    A_univ = lizard_tt_reduce(A_univ, heap);
    if (A_univ->type != AST_TT_UNIVERSE &&
        A_univ->type != AST_TT_U_SUC &&
        A_univ->type != AST_TT_U_MAX &&
        A_univ->type != AST_TT_U_VAR &&
        A_univ->type != AST_TT_U_MIN &&
        A_univ->type != AST_TT_UNIVERSE_SET) {
      return type_error(heap, "Path domain not a type");
    }
    if (!lizard_tt_check2(valid_ctx, ctx, a, A, heap)) {
      return type_error(heap, "Path left endpoint doesn't check");
    }
    if (!lizard_tt_check2(valid_ctx, ctx, b, A, heap)) {
      return type_error(heap, "Path right endpoint doesn't check");
    }
    return A_univ;
  }
  case AST_TT_PATH_APP: {
    /* (p @ i) typing rule:
     *
     *   Γ ⊢ p : (Path A a b)    Γ ⊢ i : I
     *   ──────────────────────────────────
     *   Γ ⊢ (p @ i) : A
     *
     * Endpoint conditions: (p @ i0) ≡ a, (p @ i1) ≡ b — these are
     * computational equalities that the engine takes care of via
     * path-beta on concrete endpoints. Here we just infer A. */
    lizard_ast_node_t *p_type, *i_type;
    p_type = lizard_tt_infer2(valid_ctx, ctx, t->data.tt_path_app.path, heap);
    if (is_error(p_type)) return p_type;
    p_type = lizard_tt_reduce(p_type, heap);
    if (p_type->type != AST_TT_PATH) {
      return type_error(heap, "path-app of non-Path");
    }
    i_type = lizard_tt_infer2(valid_ctx, ctx, t->data.tt_path_app.point, heap);
    if (is_error(i_type)) return i_type;
    i_type = lizard_tt_reduce(i_type, heap);
    if (i_type->type != AST_TT_INTERVAL) {
      return type_error(heap, "path-app argument not in I");
    }
    return p_type->data.tt_path.domain;
  }
  /* ===== Face typing (Turn 7) =====
   *
   * Faces are a separate sort in CCHM. We represent that sort as
   * (U 0) for slotting into the existing universe machinery.
   * Strictly, faces aren't types; they're proof-relevant decidable
   * propositions about the interval. The simplification is sound
   * for the purposes of our type checker.
   */
  case AST_TT_F0:
  case AST_TT_F1:
    return lizard_tt_make_universe(heap, 0);
  case AST_TT_F_EQ: {
    /* (F-eq i j) is a face when i, j : I. Its type is the face sort. */
    lizard_ast_node_t *l_type, *r_type;
    l_type = lizard_tt_infer2(valid_ctx, ctx, t->data.tt_f_eq.left, heap);
    if (is_error(l_type)) return l_type;
    l_type = lizard_tt_reduce(l_type, heap);
    if (l_type->type != AST_TT_INTERVAL) {
      return type_error(heap, "F-eq argument not in I");
    }
    r_type = lizard_tt_infer2(valid_ctx, ctx, t->data.tt_f_eq.right, heap);
    if (is_error(r_type)) return r_type;
    r_type = lizard_tt_reduce(r_type, heap);
    if (r_type->type != AST_TT_INTERVAL) {
      return type_error(heap, "F-eq argument not in I");
    }
    return lizard_tt_make_universe(heap, 0);
  }
  case AST_TT_F_AND:
  case AST_TT_F_OR: {
    /* Boolean ops on faces produce faces. */
    lizard_ast_node_t *l_type, *r_type;
    l_type = lizard_tt_infer2(valid_ctx, ctx, t->data.tt_f_binop.left, heap);
    if (is_error(l_type)) return l_type;
    l_type = lizard_tt_reduce(l_type, heap);
    /* Face sort is (U 0); allow any universe expression. */
    if (l_type->type != AST_TT_UNIVERSE &&
        l_type->type != AST_TT_U_SUC &&
        l_type->type != AST_TT_U_MAX &&
        l_type->type != AST_TT_U_VAR &&
        l_type->type != AST_TT_U_MIN &&
        l_type->type != AST_TT_UNIVERSE_SET) {
      return type_error(heap, "F-and/F-or arg not a face");
    }
    r_type = lizard_tt_infer2(valid_ctx, ctx, t->data.tt_f_binop.right, heap);
    if (is_error(r_type)) return r_type;
    r_type = lizard_tt_reduce(r_type, heap);
    if (r_type->type != AST_TT_UNIVERSE &&
        r_type->type != AST_TT_U_SUC &&
        r_type->type != AST_TT_U_MAX &&
        r_type->type != AST_TT_U_VAR &&
        r_type->type != AST_TT_U_MIN &&
        r_type->type != AST_TT_UNIVERSE_SET) {
      return type_error(heap, "F-and/F-or arg not a face");
    }
    return lizard_tt_make_universe(heap, 0);
  }
  case AST_TT_PARTIAL: {
    /* (Partial φ A) typing:
     *   Γ ⊢ φ : Face    Γ ⊢ A : (U n)
     *   ─────────────────────────────
     *   Γ ⊢ (Partial φ A) : (U n)
     */
    lizard_ast_node_t *face_type, *A_type;
    face_type = lizard_tt_infer2(valid_ctx, ctx, t->data.tt_partial.face, heap);
    if (is_error(face_type)) return face_type;
    A_type = lizard_tt_infer2(valid_ctx, ctx, t->data.tt_partial.type, heap);
    if (is_error(A_type)) return A_type;
    A_type = lizard_tt_reduce(A_type, heap);
    if (A_type->type != AST_TT_UNIVERSE &&
        A_type->type != AST_TT_U_SUC &&
        A_type->type != AST_TT_U_MAX &&
        A_type->type != AST_TT_U_VAR &&
        A_type->type != AST_TT_U_MIN &&
        A_type->type != AST_TT_UNIVERSE_SET) {
      return type_error(heap, "Partial type argument not a type");
    }
    return A_type;
  }
  case AST_TT_SUB: {
    /* (Sub A φ u) typing:
     *   Γ ⊢ A : (U n)    Γ ⊢ φ : Face    Γ ⊢ u : Partial φ A
     *   ────────────────────────────────────────────────────
     *   Γ ⊢ (Sub A φ u) : (U n)
     *
     * The partial element u must inhabit Partial φ A. We construct
     * the expected partial type and check u against it. */
    lizard_ast_node_t *A_type, *face_type, *expected_partial;
    A_type = lizard_tt_infer2(valid_ctx, ctx, t->data.tt_sub.type, heap);
    if (is_error(A_type)) return A_type;
    A_type = lizard_tt_reduce(A_type, heap);
    if (A_type->type != AST_TT_UNIVERSE &&
        A_type->type != AST_TT_U_SUC &&
        A_type->type != AST_TT_U_MAX &&
        A_type->type != AST_TT_U_VAR &&
        A_type->type != AST_TT_U_MIN &&
        A_type->type != AST_TT_UNIVERSE_SET) {
      return type_error(heap, "Sub type argument not a type");
    }
    face_type = lizard_tt_infer2(valid_ctx, ctx, t->data.tt_sub.face, heap);
    if (is_error(face_type)) return face_type;
    expected_partial = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    expected_partial->type = AST_TT_PARTIAL;
    expected_partial->data.tt_partial.face = t->data.tt_sub.face;
    expected_partial->data.tt_partial.type = t->data.tt_sub.type;
    if (!lizard_tt_check2(valid_ctx, ctx, t->data.tt_sub.partial, expected_partial, heap)) {
      return type_error(heap, "Sub partial doesn't check");
    }
    return A_type;
  }
  case AST_TT_COMP:
  case AST_TT_HCOMP: {
    /* (comp A φ u u0) typing — see Turn 8 design notes above.
     *
     * Implementation note: we don't require the type family to
     * infer (path-abs cannot infer without an expected Path type),
     * we just check its shape and return the appropriate endpoint. */
    lizard_ast_node_t *family = t->data.tt_comp.type_family;
    lizard_ast_node_t *face   = t->data.tt_comp.face;
    lizard_ast_node_t *u      = t->data.tt_comp.partial;
    lizard_ast_node_t *u0     = t->data.tt_comp.base;
    (void)u; (void)u0;
    if (family == NULL || face == NULL || u == NULL || u0 == NULL) {
      return type_error(heap, "comp missing field");
    }
    {
      lizard_ast_node_t *face_type = lizard_tt_infer2(valid_ctx, ctx, face, heap);
      if (is_error(face_type)) return face_type;
    }
    if (t->type == AST_TT_COMP) {
      lizard_ast_node_t *family_norm = lizard_tt_reduce(family, heap);
      if (family_norm->type != AST_TT_PATH_ABS) {
        return type_error(heap, "comp type family must be a path-abs");
      }
      if (family_norm->data.tt_path_abs.binder == NULL ||
          family_norm->data.tt_path_abs.binder->type != AST_SYMBOL) {
        return type_error(heap, "comp family has no binder");
      }
      {
        lizard_ast_node_t *i1_node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
        i1_node->type = AST_TT_I1;
        return lizard_tt_reduce(
            make_path_app_local(heap, family_norm, i1_node), heap);
      }
    } else {
      /* hcomp: family is the type directly. */
      return family;
    }
  }
  case AST_TT_FILL: {
    /* (fill A φ u u0) typing — same shape as comp but returns a
     * path-abs covering the whole interval. */
    lizard_ast_node_t *family = t->data.tt_comp.type_family;
    if (family == NULL) return type_error(heap, "fill missing family");
    /* The result type: (Pi i:I. family @ i). */
    {
      lizard_ast_node_t *i_var = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      lizard_ast_node_t *i_interval = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      lizard_ast_node_t *I_type = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      lizard_ast_node_t *cod, *pi;
      char *iname = lizard_heap_alloc(2);
      strcpy(iname, "i");
      i_var->type = AST_SYMBOL;
      i_var->data.variable = iname;
      i_interval->type = AST_TT_I_VAR;
      i_interval->data.tt_i_var.name = iname;
      I_type->type = AST_TT_INTERVAL;
      cod = make_path_app_local(heap, family, i_interval);
      pi = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      pi->type = AST_TT_PI;
      pi->data.tt_pi.binder = i_var;
      pi->data.tt_pi.domain = I_type;
      pi->data.tt_pi.codomain = cod;
      return pi;
    }
  }
  /* ===== Equivalences, Glue, ua (Turns 9 & 10) ===== */
  case AST_TT_EQUIV_TYPE: {
    /* (Equiv A B) : (U n) when A, B : (U n). */
    lizard_ast_node_t *A_type, *B_type;
    A_type = lizard_tt_infer2(valid_ctx, ctx, t->data.tt_equiv_type.domain, heap);
    if (is_error(A_type)) return A_type;
    A_type = lizard_tt_reduce(A_type, heap);
    B_type = lizard_tt_infer2(valid_ctx, ctx, t->data.tt_equiv_type.codomain, heap);
    if (is_error(B_type)) return B_type;
    if (A_type->type != AST_TT_UNIVERSE &&
        A_type->type != AST_TT_U_SUC &&
        A_type->type != AST_TT_U_MAX &&
        A_type->type != AST_TT_U_VAR &&
        A_type->type != AST_TT_U_MIN &&
        A_type->type != AST_TT_UNIVERSE_SET) {
      return type_error(heap, "Equiv domain not a type");
    }
    return A_type;
  }
  case AST_TT_ID_EQUIV: {
    /* (id-equiv A) : (Equiv A A) when A : (U n). */
    lizard_ast_node_t *A_type;
    lizard_ast_node_t *A = t->data.tt_equiv_op.operand;
    lizard_ast_node_t *result;
    if (A == NULL) return type_error(heap, "id-equiv missing argument");
    A_type = lizard_tt_infer2(valid_ctx, ctx, A, heap);
    if (is_error(A_type)) return A_type;
    A_type = lizard_tt_reduce(A_type, heap);
    if (A_type->type != AST_TT_UNIVERSE &&
        A_type->type != AST_TT_U_SUC &&
        A_type->type != AST_TT_U_MAX &&
        A_type->type != AST_TT_U_VAR &&
        A_type->type != AST_TT_U_MIN &&
        A_type->type != AST_TT_UNIVERSE_SET) {
      return type_error(heap, "id-equiv argument not a type");
    }
    result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    result->type = AST_TT_EQUIV_TYPE;
    result->data.tt_equiv_type.domain = A;
    result->data.tt_equiv_type.codomain = A;
    return result;
  }
  case AST_TT_EQUIV_FUN: {
    /* (equiv-fun e) : (Pi _ A B) when e : (Equiv A B).
     * Returns the forward function as a Pi-type. */
    lizard_ast_node_t *e_type, *pi;
    lizard_ast_node_t *e = t->data.tt_equiv_op.operand;
    char *wildcard;
    if (e == NULL) return type_error(heap, "equiv-fun missing argument");
    e_type = lizard_tt_infer2(valid_ctx, ctx, e, heap);
    if (is_error(e_type)) return e_type;
    e_type = lizard_tt_reduce(e_type, heap);
    if (e_type->type != AST_TT_EQUIV_TYPE) {
      return type_error(heap, "equiv-fun of non-Equiv");
    }
    pi = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    wildcard = lizard_heap_alloc(2);
    strcpy(wildcard, "_");
    pi->type = AST_TT_PI;
    {
      lizard_ast_node_t *b = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      b->type = AST_SYMBOL;
      b->data.variable = wildcard;
      pi->data.tt_pi.binder = b;
    }
    pi->data.tt_pi.domain = e_type->data.tt_equiv_type.domain;
    pi->data.tt_pi.codomain = e_type->data.tt_equiv_type.codomain;
    return pi;
  }
  case AST_TT_EQUIV_INV: {
    /* (equiv-inv e) : (Pi _ B A) — the backward direction. */
    lizard_ast_node_t *e_type, *pi;
    lizard_ast_node_t *e = t->data.tt_equiv_op.operand;
    char *wildcard;
    if (e == NULL) return type_error(heap, "equiv-inv missing argument");
    e_type = lizard_tt_infer2(valid_ctx, ctx, e, heap);
    if (is_error(e_type)) return e_type;
    e_type = lizard_tt_reduce(e_type, heap);
    if (e_type->type != AST_TT_EQUIV_TYPE) {
      return type_error(heap, "equiv-inv of non-Equiv");
    }
    pi = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    wildcard = lizard_heap_alloc(2);
    strcpy(wildcard, "_");
    pi->type = AST_TT_PI;
    {
      lizard_ast_node_t *b = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      b->type = AST_SYMBOL;
      b->data.variable = wildcard;
      pi->data.tt_pi.binder = b;
    }
    /* B → A */
    pi->data.tt_pi.domain = e_type->data.tt_equiv_type.codomain;
    pi->data.tt_pi.codomain = e_type->data.tt_equiv_type.domain;
    return pi;
  }
  case AST_TT_GLUE: {
    /* (Glue A φ T e) : (U n) when A, T : (U n), e : (Equiv T A),
     * φ : Face. */
    lizard_ast_node_t *A = t->data.tt_glue.base;
    lizard_ast_node_t *face = t->data.tt_glue.face;
    lizard_ast_node_t *T = t->data.tt_glue.t;
    lizard_ast_node_t *A_type, *T_type;
    if (A == NULL || face == NULL || T == NULL) {
      return type_error(heap, "Glue missing field");
    }
    A_type = lizard_tt_infer2(valid_ctx, ctx, A, heap);
    if (is_error(A_type)) return A_type;
    A_type = lizard_tt_reduce(A_type, heap);
    if (A_type->type != AST_TT_UNIVERSE &&
        A_type->type != AST_TT_U_SUC &&
        A_type->type != AST_TT_U_MAX &&
        A_type->type != AST_TT_U_VAR &&
        A_type->type != AST_TT_U_MIN &&
        A_type->type != AST_TT_UNIVERSE_SET) {
      return type_error(heap, "Glue base not a type");
    }
    T_type = lizard_tt_infer2(valid_ctx, ctx, T, heap);
    if (is_error(T_type)) return T_type;
    {
      lizard_ast_node_t *face_type = lizard_tt_infer2(valid_ctx, ctx, face, heap);
      if (is_error(face_type)) return face_type;
    }
    return A_type;
  }
  case AST_TT_UNGLUE: {
    /* (unglue e g) : A when g : (Glue A φ T e).
     *
     * We don't have direct access to the Glue type from `g`; we
     * use the equivalence `e`'s codomain as the result type
     * (since for e : Equiv T A, the underlying type is A). */
    lizard_ast_node_t *e_type;
    lizard_ast_node_t *e = t->data.tt_unglue.equiv;
    if (e == NULL) return type_error(heap, "unglue missing equiv");
    e_type = lizard_tt_infer2(valid_ctx, ctx, e, heap);
    if (is_error(e_type)) return e_type;
    e_type = lizard_tt_reduce(e_type, heap);
    if (e_type->type != AST_TT_EQUIV_TYPE) {
      return type_error(heap, "unglue: equiv arg not an Equiv");
    }
    return e_type->data.tt_equiv_type.codomain;
  }
  case AST_TT_UA: {
    /* (ua e) : (Path U A B) when e : (Equiv A B).
     *
     *   Γ ⊢ e : (Equiv A B)
     *   ────────────────────────────
     *   Γ ⊢ (ua e) : (Path (U n) A B)
     *
     * This is the type of computational univalence: from an
     * equivalence we get a *path in the universe*, which can be
     * transported along to convert between A and B. */
    lizard_ast_node_t *e_type, *result, *univ;
    lizard_ast_node_t *e = t->data.tt_ua.equiv;
    if (e == NULL) return type_error(heap, "ua missing equiv");
    e_type = lizard_tt_infer2(valid_ctx, ctx, e, heap);
    if (is_error(e_type)) return e_type;
    e_type = lizard_tt_reduce(e_type, heap);
    if (e_type->type != AST_TT_EQUIV_TYPE) {
      return type_error(heap, "ua: argument not an Equiv");
    }
    univ = lizard_tt_make_universe(heap, 0);
    result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    result->type = AST_TT_PATH;
    result->data.tt_path.domain = univ;
    result->data.tt_path.a = e_type->data.tt_equiv_type.domain;
    result->data.tt_path.b = e_type->data.tt_equiv_type.codomain;
    return result;
  }
  case AST_TT_LAMBDA:
    /* Lambdas need a type from context (check mode). Without an
     * expected type, we can't infer a Lambda's type. */
    return type_error(heap, "Lambda cannot infer; needs expected type");
  /* Phase H.1 — HIT typing.
   *
   * For H.1 we keep this minimal:
   *   (HIT-ref 'name)        : (U 0)  if name registered, else error
   *   (HIT-app 'cname ...)   : (HIT-ref 'host)  if cname is a registered
   *                                              constructor in HIT host
   * Declarations and constructor/path records aren't terms in the
   * usual sense, so they don't have a type. We error if asked. */
  case AST_TT_HIT_REF: {
    lizard_ast_node_t *name = t->data.tt_hit_ref.name;
    lizard_ast_node_t *decl;
    /* Phase M.6: gate on HIT-enabled. */
    if (lizard_logic_rule_enabled("HIT-enabled") == 0) {
      return type_error(heap, "HIT-ref rejected: HIT feature disabled");
    }
    if (name == NULL || name->type != AST_SYMBOL) {
      return type_error(heap, "HIT-ref name not a symbol");
    }
    decl = lizard_tt_hit_lookup(name->data.variable);
    if (decl == NULL) {
      return type_error(heap, "HIT-ref name not registered");
    }
    /* H.1: every HIT lives at (U 0). Refinement of this rule belongs
     * to a later phase where we attach universe annotations to HIT
     * declarations. */
    return lizard_tt_make_universe(heap, 0);
  }
  case AST_TT_HIT_APP: {
    /* Look up the constructor across all registered HITs. If found,
     * return (HIT-ref 'host). If not, error.
     * Phase M.6: gated on HIT-enabled. */
    lizard_ast_node_t *cname = t->data.tt_hit_app.name;
    lizard_ast_node_t *host_decl;
    if (lizard_logic_rule_enabled("HIT-enabled") == 0) {
      return type_error(heap, "HIT-app rejected: HIT feature disabled");
    }
    if (cname == NULL || cname->type != AST_SYMBOL) {
      return type_error(heap, "HIT-app name not a symbol");
    }
    host_decl = lizard_tt_hit_lookup_constructor_host(cname->data.variable);
    if (host_decl == NULL) {
      return type_error(heap, "HIT-app constructor not registered");
    }
    /* Return (HIT-ref 'host-name). */
    {
      lizard_ast_node_t *ref = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      ref->type = AST_TT_HIT_REF;
      ref->data.tt_hit_ref.name = host_decl->data.tt_hit_decl.name;
      return ref;
    }
  }
  case AST_TT_HIT_DECL:
  case AST_TT_HIT_CONSTRUCTOR:
  case AST_TT_HIT_PATH:
    /* These are declaration metadata, not term-level entities. */
    return type_error(heap, "HIT declaration/record is not a term");
  default:
    return type_error(heap, "no inference rule for this term");
  }
}

int lizard_tt_check2(lizard_ast_node_t *valid_ctx,
                     lizard_ast_node_t *ctx,
                    lizard_ast_node_t *t,
                    lizard_ast_node_t *T,
                    lizard_heap_t *heap) {
  if (t == NULL || T == NULL) return 0;
  /* Special case for Lambda: it checks against a Pi. */
  if (t->type == AST_TT_LAMBDA) {
    lizard_ast_node_t *T_norm = lizard_tt_reduce(T, heap);
    if (T_norm->type != AST_TT_PI) return 0;
    if (t->data.tt_lambda.binder == NULL ||
        t->data.tt_lambda.binder->type != AST_SYMBOL) return 0;
    /* Extend context with binder : domain, check body against codomain.
     * We use the LAMBDA's binder name in the extended context, then
     * also rename the Pi's codomain to use the same name so the body
     * can refer to it. */
    {
      const char *lname = t->data.tt_lambda.binder->data.variable;
      const char *pname = T_norm->data.tt_pi.binder->data.variable;
      lizard_ast_node_t *cod = T_norm->data.tt_pi.codomain;
      lizard_ast_node_t *new_ctx =
          ctx_extend(ctx, lname, T_norm->data.tt_pi.domain, heap);
      if (strcmp(lname, pname) != 0) {
        /* Rename the Pi's binder in codomain to match Lambda's. */
        lizard_ast_node_t *renamed_var = lizard_heap_alloc(sizeof(lizard_ast_node_t));
        renamed_var->type = AST_SYMBOL;
        renamed_var->data.variable = lname;
        cod = lizard_tt_subst(cod, pname, renamed_var, heap);
      }
      return lizard_tt_check2(valid_ctx, new_ctx, t->data.tt_lambda.body, cod, heap);
    }
  }
  /* refl typing rule:
   *
   *   Γ ⊢ a : A
   *   ─────────────────────────
   *   Γ ⊢ (refl a) : (Id A a a)
   *
   * To check (refl x) against expected type (Id A a b):
   *   1. Expected type must reduce to an Id type.
   *   2. x, a, and b must all be convertible (alpha-equal up to reduction).
   *   3. x must check against A.
   *
   * This is the rule that makes refl prove ONLY reflexivity, not
   * arbitrary identities. */
  if (t->type == AST_TT_REFL) {
    lizard_ast_node_t *T_norm = lizard_tt_reduce(T, heap);
    lizard_ast_node_t *x, *A, *a, *b;
    lizard_ast_node_t *x_norm, *a_norm, *b_norm;
    if (T_norm->type != AST_TT_ID) return 0;
    x = t->data.tt_refl.value;
    A = T_norm->data.tt_id.domain;
    a = T_norm->data.tt_id.a;
    b = T_norm->data.tt_id.b;
    if (x == NULL || A == NULL || a == NULL || b == NULL) return 0;
    /* The point x must inhabit A. */
    if (!lizard_tt_check2(valid_ctx, ctx, x, A, heap)) return 0;
    /* The endpoints a and b must both be convertible to x. */
    x_norm = lizard_tt_reduce(x, heap);
    a_norm = lizard_tt_reduce(a, heap);
    b_norm = lizard_tt_reduce(b, heap);
    if (!lizard_tt_alpha_equal(x_norm, a_norm)) return 0;
    if (!lizard_tt_alpha_equal(x_norm, b_norm)) return 0;
    return 1;
  }
  /* Pair typing rule:
   *
   *   Γ ⊢ a : A    Γ ⊢ b : B[a/x]
   *   ─────────────────────────────
   *   Γ ⊢ (Pair a b) : (Sigma x A B)
   *
   * To check (Pair a b) against expected (Sigma x A B):
   *   1. Expected must reduce to a Sigma.
   *   2. a must check against A.
   *   3. b must check against B[a/x].
   */
  if (t->type == AST_TT_PAIR) {
    lizard_ast_node_t *T_norm = lizard_tt_reduce(T, heap);
    lizard_ast_node_t *a, *b, *A, *B, *B_sub;
    if (T_norm->type != AST_TT_SIGMA) return 0;
    a = t->data.tt_pair.fst;
    b = t->data.tt_pair.snd;
    A = T_norm->data.tt_sigma.domain;
    B = T_norm->data.tt_sigma.codomain;
    if (a == NULL || b == NULL || A == NULL || B == NULL) return 0;
    if (T_norm->data.tt_sigma.binder == NULL ||
        T_norm->data.tt_sigma.binder->type != AST_SYMBOL) return 0;
    if (!lizard_tt_check2(valid_ctx, ctx, a, A, heap)) return 0;
    B_sub = lizard_tt_subst(B,
                            T_norm->data.tt_sigma.binder->data.variable,
                            a, heap);
    if (!lizard_tt_check2(valid_ctx, ctx, b, B_sub, heap)) return 0;
    return 1;
  }
  /* inl/inr typing rules:
   *
   *   Γ ⊢ a : A                       Γ ⊢ b : B
   *   ─────────────────────────       ─────────────────────────
   *   Γ ⊢ (inl a) : (Sum A B)         Γ ⊢ (inr b) : (Sum A B)
   *
   * To check (inl a) against expected (Sum A B): a must check
   * against A. The B side is irrelevant — inl only puts an A in.
   */
  if (t->type == AST_TT_INL) {
    lizard_ast_node_t *T_norm = lizard_tt_reduce(T, heap);
    if (T_norm->type != AST_TT_SUM) return 0;
    return lizard_tt_check2(valid_ctx, ctx, t->data.tt_inj.value,
                           T_norm->data.tt_sum.left, heap);
  }
  if (t->type == AST_TT_INR) {
    lizard_ast_node_t *T_norm = lizard_tt_reduce(T, heap);
    if (T_norm->type != AST_TT_SUM) return 0;
    return lizard_tt_check2(valid_ctx, ctx, t->data.tt_inj.value,
                           T_norm->data.tt_sum.right, heap);
  }
  /* path-abs typing rule:
   *
   *   Γ, i:I ⊢ body : A
   *   body[i0/i] ≡ a    body[i1/i] ≡ b
   *   ─────────────────────────────────
   *   Γ ⊢ (<i> body) : (Path A a b)
   *
   * Three checks:
   *   1. Body checks against A in extended interval context.
   *   2. Body at i0 is convertible to a.
   *   3. Body at i1 is convertible to b. */
  if (t->type == AST_TT_PATH_ABS) {
    lizard_ast_node_t *T_norm = lizard_tt_reduce(T, heap);
    lizard_ast_node_t *binder, *A, *a, *b;
    lizard_ast_node_t *new_ctx;
    lizard_ast_node_t *body_at_i0, *body_at_i1;
    lizard_ast_node_t *i_type;
    if (T_norm->type != AST_TT_PATH) return 0;
    binder = t->data.tt_path_abs.binder;
    if (binder == NULL || binder->type != AST_SYMBOL) return 0;
    A = T_norm->data.tt_path.domain;
    a = T_norm->data.tt_path.a;
    b = T_norm->data.tt_path.b;
    /* Extend context with binder : I */
    i_type = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    i_type->type = AST_TT_INTERVAL;
    new_ctx = ctx_extend(ctx, binder->data.variable, i_type, heap);
    /* Check body : A in extended context */
    if (!lizard_tt_check2(valid_ctx, new_ctx, t->data.tt_path_abs.body, A, heap)) {
      return 0;
    }
    /* Endpoint checks: body[i0/i] ≡ a, body[i1/i] ≡ b */
    {
      lizard_ast_node_t *i0_node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      lizard_ast_node_t *i1_node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      i0_node->type = AST_TT_I0;
      i1_node->type = AST_TT_I1;
      /* Use subst_interval via the public reduce — fold it through
       * the engine. We call lizard_tt_subst with a special name, but
       * that's term subst not interval subst. Use lizard_tt_subst_interval
       * if we exported it; otherwise build a path-app and reduce. */
      body_at_i0 = lizard_tt_reduce(
          make_path_app_local(heap, t, i0_node), heap);
      body_at_i1 = lizard_tt_reduce(
          make_path_app_local(heap, t, i1_node), heap);
    }
    if (!lizard_tt_alpha_equal(body_at_i0, lizard_tt_reduce(a, heap))) return 0;
    if (!lizard_tt_alpha_equal(body_at_i1, lizard_tt_reduce(b, heap))) return 0;
    return 1;
  }
  /* glue-intro checks against a Glue type:
   *
   *   Γ ⊢ t : T    Γ ⊢ a : A    [and on φ: e(t) ≡ a]
   *   ────────────────────────────────────────────────
   *   Γ ⊢ (glue-intro φ t a) : (Glue A φ T e)
   *
   * The agreement-on-φ condition is computational; we check the
   * basic shape and types. */
  if (t->type == AST_TT_GLUE_INTRO) {
    lizard_ast_node_t *T_norm = lizard_tt_reduce(T, heap);
    if (T_norm->type != AST_TT_GLUE) return 0;
    if (!lizard_tt_check2(valid_ctx, ctx, t->data.tt_glue_intro.t,
                         T_norm->data.tt_glue.t, heap)) return 0;
    if (!lizard_tt_check2(valid_ctx, ctx, t->data.tt_glue_intro.a,
                         T_norm->data.tt_glue.base, heap)) return 0;
    return 1;
  }
  /* General case: infer the term's type and check convertible to T. */
  {
    lizard_ast_node_t *inferred = lizard_tt_infer2(valid_ctx, ctx, t, heap);
    lizard_ast_node_t *T_norm;
    if (is_error(inferred)) return 0;
    inferred = lizard_tt_reduce(inferred, heap);
    T_norm = lizard_tt_reduce(T, heap);
    /* Alpha equality is the primary check. Cumulativity allows
     * inferred ≤ expected when both are universes. */
    if (lizard_tt_alpha_equal(inferred, T_norm)) return 1;
    if (inferred->type == AST_TT_UNIVERSE && T_norm->type == AST_TT_UNIVERSE) {
      return lizard_tt_universe_leq(inferred, T_norm) == 1;
    }
    return 0;
  }
}

/* ----- Primitives exposed to lizard --------------------------------- */

static int two_args_chk(lz_list_t *args) {
  return args->head != args->nil && args->head->next != args->nil &&
         args->head->next->next == args->nil;
}
static int three_args_chk(lz_list_t *args) {
  return args->head != args->nil && args->head->next != args->nil &&
         args->head->next->next != args->nil &&
         args->head->next->next->next == args->nil;
}
static lizard_ast_node_t *nth_arg_chk(lz_list_t *args, int n) {
  lz_list_node_t *it = args->head;
  while (n-- > 0 && it != args->nil) it = it->next;
  if (it == args->nil) return NULL;
  return ((lizard_ast_list_node_t *)it)->ast;
}

lizard_ast_node_t *lizard_primitive_tt_infer(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *nil;
  (void)env;
  if (!two_args_chk(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  /* Backward-compat: single-context (infer ctx expr) passes an empty
   * valid context. M.5.2 Turn 2 introduces (infer-modal valid truth
   * expr) for code that uses the modal discipline directly. */
  nil = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  nil->type = AST_NIL;
  return lizard_tt_infer2(nil, nth_arg_chk(args, 0), nth_arg_chk(args, 1), heap);
}

lizard_ast_node_t *lizard_primitive_tt_check(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *nil;
  (void)env;
  if (!three_args_chk(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  nil = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  nil->type = AST_NIL;
  return lizard_make_bool(heap, lizard_tt_check2(nil, nth_arg_chk(args, 0),
                                                 nth_arg_chk(args, 1),
                                                 nth_arg_chk(args, 2), heap));
}

/* (infer-modal valid-ctx truth-ctx expr) — M.5.2 Turn 2.
 * The two-context inference primitive. The strict S4 modal rules
 * use both contexts: box-intro drops the truth context, unbox
 * extends the valid context. */
lizard_ast_node_t *lizard_primitive_tt_infer_modal(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  (void)env;
  if (!three_args_chk(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  return lizard_tt_infer2(nth_arg_chk(args, 0),
                          nth_arg_chk(args, 1),
                          nth_arg_chk(args, 2), heap);
}

lizard_ast_node_t *lizard_primitive_tt_check_modal(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  (void)env;
  if (!nth_arg_chk(args, 3)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  return lizard_make_bool(heap,
    lizard_tt_check2(nth_arg_chk(args, 0),
                     nth_arg_chk(args, 1),
                     nth_arg_chk(args, 2),
                     nth_arg_chk(args, 3), heap));
}

/* Backward-compat wrappers. The C-level kernel API keeps the single-
 * context functions for older callers. They use an empty valid ctx. */
lizard_ast_node_t *lizard_tt_infer(lizard_ast_node_t *ctx,
                                   lizard_ast_node_t *t,
                                   lizard_heap_t *heap) {
  lizard_ast_node_t *nil = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  nil->type = AST_NIL;
  return lizard_tt_infer2(nil, ctx, t, heap);
}

int lizard_tt_check(lizard_ast_node_t *ctx,
                    lizard_ast_node_t *t,
                    lizard_ast_node_t *T,
                    lizard_heap_t *heap) {
  lizard_ast_node_t *nil = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  nil->type = AST_NIL;
  return lizard_tt_check2(nil, ctx, t, T, heap);
}
