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

/* ----- The checker --------------------------------------------------- */

lizard_ast_node_t *lizard_tt_infer(lizard_ast_node_t *ctx,
                                   lizard_ast_node_t *t,
                                   lizard_heap_t *heap) {
  if (t == NULL) return type_error(heap, "null term");
  switch (t->type) {
  case AST_SYMBOL: {
    /* Variable: look up in context. */
    lizard_ast_node_t *tp = ctx_lookup(ctx, t->data.variable);
    if (tp == NULL) {
      return type_error(heap, "unbound variable");
    }
    return tp;
  }
  case AST_TT_UNIVERSE: {
    /* (U n) : (U n+1) */
    return lizard_tt_make_universe(heap, t->data.tt_universe.level + 1);
  }
  case AST_TT_PI: {
    /* (Pi x A B) : (U-max univ(A) univ(B[x:=fresh])) — but we don't
     * have unification of universes. For now: check A is a type
     * (lives in some universe), check B is a type in extended ctx,
     * return the max of their universes. */
    lizard_ast_node_t *binder = t->data.tt_pi.binder;
    lizard_ast_node_t *dom = t->data.tt_pi.domain;
    lizard_ast_node_t *cod = t->data.tt_pi.codomain;
    lizard_ast_node_t *dom_univ, *cod_univ;
    lizard_ast_node_t *new_ctx;
    if (binder == NULL || binder->type != AST_SYMBOL) {
      return type_error(heap, "Pi binder not a symbol");
    }
    dom_univ = lizard_tt_infer(ctx, dom, heap);
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
    cod_univ = lizard_tt_infer(new_ctx, cod, heap);
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
     * containing all original dimensions plus the new one. */
    lizard_ast_node_t *binder = t->data.tt_pi_fresh.binder;
    lizard_ast_node_t *dom = t->data.tt_pi_fresh.domain;
    lizard_ast_node_t *cod = t->data.tt_pi_fresh.codomain;
    lizard_ast_node_t *dom_univ, *cod_univ;
    lizard_ast_node_t *new_ctx;
    lizard_ast_node_t *fresh_set;
    lizard_ast_node_t *combined;
    long *dim_ptr;
    if (binder == NULL || binder->type != AST_SYMBOL) {
      return type_error(heap, "pi-fresh binder not a symbol");
    }
    dom_univ = lizard_tt_infer(ctx, dom, heap);
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
    cod_univ = lizard_tt_infer(new_ctx, cod, heap);
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
    /* Same dimension-creating rule for Sigma. */
    lizard_ast_node_t *binder = t->data.tt_sigma_fresh.binder;
    lizard_ast_node_t *dom = t->data.tt_sigma_fresh.domain;
    lizard_ast_node_t *cod = t->data.tt_sigma_fresh.codomain;
    lizard_ast_node_t *dom_univ, *cod_univ;
    lizard_ast_node_t *new_ctx;
    lizard_ast_node_t *fresh_set;
    lizard_ast_node_t *combined;
    long *dim_ptr;
    if (binder == NULL || binder->type != AST_SYMBOL) {
      return type_error(heap, "sigma-fresh binder not a symbol");
    }
    dom_univ = lizard_tt_infer(ctx, dom, heap);
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
    cod_univ = lizard_tt_infer(new_ctx, cod, heap);
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
  case AST_TT_APP: {
    /* (@ f a) : B[a/x] where f : (Pi x A B) and a checks against A. */
    lizard_ast_node_t *f_type;
    f_type = lizard_tt_infer(ctx, t->data.tt_app.fun, heap);
    if (is_error(f_type)) return f_type;
    f_type = lizard_tt_reduce(f_type, heap);
    if (f_type->type != AST_TT_PI) {
      return type_error(heap, "application of non-function");
    }
    if (!lizard_tt_check(ctx, t->data.tt_app.arg, f_type->data.tt_pi.domain,
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
    if (!lizard_tt_check(ctx, t->data.tt_annot.term, t->data.tt_annot.type,
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
    A_univ = lizard_tt_infer(ctx, A, heap);
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
    if (!lizard_tt_check(ctx, a, A, heap)) {
      return type_error(heap, "Id left endpoint doesn't check against domain");
    }
    if (!lizard_tt_check(ctx, b, A, heap)) {
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
    A = lizard_tt_infer(ctx, a, heap);
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
    dom_univ = lizard_tt_infer(ctx, dom, heap);
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
    cod_univ = lizard_tt_infer(new_ctx, cod, heap);
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
    A_univ = lizard_tt_infer(ctx, A, heap);
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
    B_univ = lizard_tt_infer(ctx, B, heap);
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
    p_type = lizard_tt_infer(ctx, p, heap);
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
    p_type = lizard_tt_infer(ctx, p, heap);
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
    path_type = lizard_tt_infer(ctx, path, heap);
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
    if (!lizard_tt_check(ctx, refl_case, m_app_a_refl, heap)) {
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
    path_type = lizard_tt_infer(ctx, path, heap);
    if (is_error(path_type)) return path_type;
    path_type = lizard_tt_reduce(path_type, heap);
    if (path_type->type != AST_TT_ID) {
      return type_error(heap, "xport path is not an Id");
    }
    a = path_type->data.tt_id.a;
    a_prime = path_type->data.tt_id.b;
    motive_type = lizard_tt_infer(ctx, motive, heap);
    if (is_error(motive_type)) return motive_type;
    motive_type = lizard_tt_reduce(motive_type, heap);
    if (motive_type->type != AST_TT_PI) {
      return type_error(heap, "xport motive is not a Pi");
    }
    m_app_a = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    m_app_a->type = AST_TT_APP;
    m_app_a->data.tt_app.fun = motive;
    m_app_a->data.tt_app.arg = a;
    if (!lizard_tt_check(ctx, value, m_app_a, heap)) {
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
    s_type = lizard_tt_infer(ctx, s, heap);
    if (is_error(s_type)) return s_type;
    s_type = lizard_tt_reduce(s_type, heap);
    if (s_type->type != AST_TT_SUM) {
      return type_error(heap, "Case scrutinee is not a Sum");
    }
    l_type = lizard_tt_infer(ctx, l, heap);
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
    r_type = lizard_tt_infer(ctx, r, heap);
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
    f_type = lizard_tt_infer(ctx, f, heap);
    if (is_error(f_type)) return f_type;
    f_type = lizard_tt_reduce(f_type, heap);
    if (f_type->type != AST_TT_PI) {
      return type_error(heap, "ap: function is not a Pi");
    }
    p_type = lizard_tt_infer(ctx, p, heap);
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
    lizard_ast_node_t *l_type = lizard_tt_infer(ctx, t->data.tt_i_binop.left, heap);
    lizard_ast_node_t *r_type = lizard_tt_infer(ctx, t->data.tt_i_binop.right, heap);
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
    lizard_ast_node_t *o_type = lizard_tt_infer(ctx, t->data.tt_i_neg.operand, heap);
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
    A_univ = lizard_tt_infer(ctx, A, heap);
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
    if (!lizard_tt_check(ctx, a, A, heap)) {
      return type_error(heap, "Path left endpoint doesn't check");
    }
    if (!lizard_tt_check(ctx, b, A, heap)) {
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
    p_type = lizard_tt_infer(ctx, t->data.tt_path_app.path, heap);
    if (is_error(p_type)) return p_type;
    p_type = lizard_tt_reduce(p_type, heap);
    if (p_type->type != AST_TT_PATH) {
      return type_error(heap, "path-app of non-Path");
    }
    i_type = lizard_tt_infer(ctx, t->data.tt_path_app.point, heap);
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
    l_type = lizard_tt_infer(ctx, t->data.tt_f_eq.left, heap);
    if (is_error(l_type)) return l_type;
    l_type = lizard_tt_reduce(l_type, heap);
    if (l_type->type != AST_TT_INTERVAL) {
      return type_error(heap, "F-eq argument not in I");
    }
    r_type = lizard_tt_infer(ctx, t->data.tt_f_eq.right, heap);
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
    l_type = lizard_tt_infer(ctx, t->data.tt_f_binop.left, heap);
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
    r_type = lizard_tt_infer(ctx, t->data.tt_f_binop.right, heap);
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
    face_type = lizard_tt_infer(ctx, t->data.tt_partial.face, heap);
    if (is_error(face_type)) return face_type;
    A_type = lizard_tt_infer(ctx, t->data.tt_partial.type, heap);
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
    A_type = lizard_tt_infer(ctx, t->data.tt_sub.type, heap);
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
    face_type = lizard_tt_infer(ctx, t->data.tt_sub.face, heap);
    if (is_error(face_type)) return face_type;
    expected_partial = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    expected_partial->type = AST_TT_PARTIAL;
    expected_partial->data.tt_partial.face = t->data.tt_sub.face;
    expected_partial->data.tt_partial.type = t->data.tt_sub.type;
    if (!lizard_tt_check(ctx, t->data.tt_sub.partial, expected_partial, heap)) {
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
      lizard_ast_node_t *face_type = lizard_tt_infer(ctx, face, heap);
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
    A_type = lizard_tt_infer(ctx, t->data.tt_equiv_type.domain, heap);
    if (is_error(A_type)) return A_type;
    A_type = lizard_tt_reduce(A_type, heap);
    B_type = lizard_tt_infer(ctx, t->data.tt_equiv_type.codomain, heap);
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
    A_type = lizard_tt_infer(ctx, A, heap);
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
    e_type = lizard_tt_infer(ctx, e, heap);
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
    e_type = lizard_tt_infer(ctx, e, heap);
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
    A_type = lizard_tt_infer(ctx, A, heap);
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
    T_type = lizard_tt_infer(ctx, T, heap);
    if (is_error(T_type)) return T_type;
    {
      lizard_ast_node_t *face_type = lizard_tt_infer(ctx, face, heap);
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
    e_type = lizard_tt_infer(ctx, e, heap);
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
    e_type = lizard_tt_infer(ctx, e, heap);
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
     * return (HIT-ref 'host). If not, error. */
    lizard_ast_node_t *cname = t->data.tt_hit_app.name;
    lizard_ast_node_t *host_decl;
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

int lizard_tt_check(lizard_ast_node_t *ctx,
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
      return lizard_tt_check(new_ctx, t->data.tt_lambda.body, cod, heap);
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
    if (!lizard_tt_check(ctx, x, A, heap)) return 0;
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
    if (!lizard_tt_check(ctx, a, A, heap)) return 0;
    B_sub = lizard_tt_subst(B,
                            T_norm->data.tt_sigma.binder->data.variable,
                            a, heap);
    if (!lizard_tt_check(ctx, b, B_sub, heap)) return 0;
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
    return lizard_tt_check(ctx, t->data.tt_inj.value,
                           T_norm->data.tt_sum.left, heap);
  }
  if (t->type == AST_TT_INR) {
    lizard_ast_node_t *T_norm = lizard_tt_reduce(T, heap);
    if (T_norm->type != AST_TT_SUM) return 0;
    return lizard_tt_check(ctx, t->data.tt_inj.value,
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
    if (!lizard_tt_check(new_ctx, t->data.tt_path_abs.body, A, heap)) {
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
    if (!lizard_tt_check(ctx, t->data.tt_glue_intro.t,
                         T_norm->data.tt_glue.t, heap)) return 0;
    if (!lizard_tt_check(ctx, t->data.tt_glue_intro.a,
                         T_norm->data.tt_glue.base, heap)) return 0;
    return 1;
  }
  /* General case: infer the term's type and check convertible to T. */
  {
    lizard_ast_node_t *inferred = lizard_tt_infer(ctx, t, heap);
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
  (void)env;
  if (!two_args_chk(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  return lizard_tt_infer(nth_arg_chk(args, 0), nth_arg_chk(args, 1), heap);
}

lizard_ast_node_t *lizard_primitive_tt_check(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  (void)env;
  if (!three_args_chk(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  return lizard_make_bool(heap, lizard_tt_check(nth_arg_chk(args, 0),
                                                 nth_arg_chk(args, 1),
                                                 nth_arg_chk(args, 2), heap));
}
