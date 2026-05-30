/* tt_check_fresh.c — dimension/couniverse fresh-binder judgment rules.
 * Extracted from tt_check.c's infer2_kind_impl: the dimension-creating Pi
 * (pi-fresh / sigma-fresh, Phase L.3) and their couniverse-lattice duals
 * (co-pi-fresh / co-sigma-fresh, Phase L.5).  Dispatched to from the main
 * checker; shared helpers come from tt_check_internal.h. */
#include "primitives.h"
#include "env.h"
#include "errors.h"
#include "lizard_internal.h"
#include "mem.h"
#include "tt_check_internal.h"
#include <string.h>

lizard_ast_node_t *infer2_fresh(lizard_ast_node_t *valid_ctx,
                                lizard_ast_node_t *ctx, lizard_ast_node_t *t,
                                lizard_heap_t *heap,
                                lizard_judgment_kind_t *out_kind) {
  if (out_kind != NULL) *out_kind = LIZARD_KIND_TRUE;
  switch (t->type) {
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
  default:
    return type_error(heap, "infer2_fresh: non-fresh-binder node");
  }
}
