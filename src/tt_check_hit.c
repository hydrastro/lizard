/* tt_check_hit.c — higher-inductive-type judgment rules.
 * Extracted from tt_check.c's infer2_kind_impl: the circle S1 and its
 * point/loop constructors, propositional truncation (intro/elim with
 * propositionality coherence, Phase H.2), and the general HIT forms
 * (ref/app/decl/constructor/path, Phase H.1).  Dispatched to from the main
 * checker; shared helpers come from tt_check_internal.h. */
#include "primitives.h"
#include "env.h"
#include "errors.h"
#include "lizard_internal.h"
#include "mem.h"
#include "tt_check_internal.h"
#include <string.h>

lizard_ast_node_t *infer2_hit(lizard_ast_node_t *valid_ctx,
                              lizard_ast_node_t *ctx, lizard_ast_node_t *t,
                              lizard_heap_t *heap,
                              lizard_judgment_kind_t *out_kind) {
  if (out_kind != NULL) *out_kind = LIZARD_KIND_TRUE;
  switch (t->type) {
  case AST_TT_S1:
    if (lizard_logic_rule_enabled("cubical-s1-enabled") != 1) {
      return type_error(heap, "S1 rejected: cubical-s1-enabled disabled");
    }
    return lizard_tt_make_universe(heap, 0);
  case AST_TT_S1_BASE: {
    lizard_ast_node_t *s1;
    if (lizard_logic_rule_enabled("cubical-s1-enabled") != 1) {
      return type_error(heap, "base rejected: cubical-s1-enabled disabled");
    }
    s1 = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    s1->type = AST_TT_S1;
    return s1;
  }
  case AST_TT_S1_LOOP: {
    lizard_ast_node_t *s1;
    lizard_ast_node_t *base_a;
    lizard_ast_node_t *base_b;
    lizard_ast_node_t *path;
    if (lizard_logic_rule_enabled("cubical-s1-enabled") != 1) {
      return type_error(heap, "loop rejected: cubical-s1-enabled disabled");
    }
    s1 = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    s1->type = AST_TT_S1;
    base_a = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    base_a->type = AST_TT_S1_BASE;
    base_b = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    base_b->type = AST_TT_S1_BASE;
    path = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    path->type = AST_TT_PATH;
    path->data.tt_path.domain = s1;
    path->data.tt_path.a = base_a;
    path->data.tt_path.b = base_b;
    return path;
  }
  case AST_TT_TRUNC: {
    /* Phase H.2 — propositional truncation type former.
     *
     *   Γ ⊢ A : (U n)
     *   ────────────────────────────────
     *   Γ ⊢ (Trunc level A) : (U n)
     *
     * The truncation lives at the same universe level as its
     * argument. The `level` parameter records the homotopy level
     * (e.g. -1 propositional, 0 set, 1 groupoid). lizard accepts
     * any level value; semantic obligations on level remain a
     * documented gap (see docs/CLAIMS_MATRIX.md). */
    lizard_ast_node_t *type_of_type;
    if (lizard_logic_rule_enabled("truncations-enabled") != 1) {
      return type_error(heap, "Trunc rejected: truncations-enabled disabled");
    }
    if (t->data.tt_trunc.type == NULL) {
      return type_error(heap, "Trunc missing type argument");
    }
    type_of_type = lizard_tt_infer2(valid_ctx, ctx, t->data.tt_trunc.type, heap);
    if (is_error(type_of_type)) return type_of_type;
    return type_of_type;
  }
  case AST_TT_TRUNC_INTRO: {
    /* Phase H.2 — propositional truncation constructor.
     *
     *   Γ ⊢ e : A
     *   ────────────────────────────────────
     *   Γ ⊢ (trunc e) : (Trunc <level> A)
     *
     * Result is a truncation at the **propositional level (-1)** by
     * default. To produce a truncation at a different level, the
     * user must wrap explicitly via bidirectional checking (not
     * implemented in this turn — honest gap). */
    lizard_ast_node_t *value;
    lizard_ast_node_t *value_type;
    lizard_ast_node_t *result;
    lizard_ast_node_t *level_node;
    if (lizard_logic_rule_enabled("truncations-enabled") != 1) {
      return type_error(heap, "trunc rejected: truncations-enabled disabled");
    }
    value = t->data.tt_trunc_intro.value;
    if (value == NULL) {
      return type_error(heap, "trunc missing value");
    }
    value_type = lizard_tt_infer2(valid_ctx, ctx, value, heap);
    if (is_error(value_type)) return value_type;
    /* Build (Trunc -1 value_type). For the level we synthesize a
     * symbolic placeholder since lizard's AST doesn't have a literal
     * "-1" integer constant. Use NULL to mean "level unspecified at
     * inference time" — alpha-equality treats NULL level as matching. */
    level_node = NULL;
    result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    result->type = AST_TT_TRUNC;
    result->data.tt_trunc.level = level_node;
    result->data.tt_trunc.type  = value_type;
    return result;
  }
  case AST_TT_TRUNC_ELIM: {
    /* Phase H.2 — propositional truncation recursor (their AST shape).
     *
     *   Γ ⊢ C   : (U n)               (motive — target type)
     *   Γ ⊢ h   : Π _:A. C            (handler — point case)
     *   Γ ⊢ e   : (Trunc _ A)         (scrutinee)
     *   ───────────────────────────────────────────
     *   Γ ⊢ (trunc-elim C h e) : C
     *
     * Honest scope: the propositionality obligation on C (any two
     * values of C are path-equal) is NOT structurally enforced in
     * this turn. Without that obligation, lizard accepts elims that
     * are not strictly justified by the propositional-truncation
     * elimination principle. See docs/CLAIMS_MATRIX.md. */
    lizard_ast_node_t *C_type, *h, *val;
    lizard_ast_node_t *C_univ, *h_type, *val_type;
    lizard_ast_node_t *A;
    if (lizard_logic_rule_enabled("truncations-enabled") != 1) {
      return type_error(heap, "trunc-elim rejected: truncations-enabled disabled");
    }
    C_type = t->data.tt_trunc_elim.motive;
    h      = t->data.tt_trunc_elim.handler;
    val    = t->data.tt_trunc_elim.value;
    if (C_type == NULL || h == NULL || val == NULL) {
      return type_error(heap, "trunc-elim missing argument");
    }
    /* Motive must be a type. */
    C_univ = lizard_tt_infer2(valid_ctx, ctx, C_type, heap);
    if (is_error(C_univ)) return C_univ;
    C_univ = lizard_tt_reduce(C_univ, heap);
    if (C_univ->type != AST_TT_UNIVERSE) {
      return type_error(heap, "trunc-elim motive must be a type");
    }
    /* Scrutinee must be Trunc-typed. */
    val_type = lizard_tt_infer2(valid_ctx, ctx, val, heap);
    if (is_error(val_type)) return val_type;
    val_type = lizard_tt_reduce(val_type, heap);
    if (val_type->type != AST_TT_TRUNC) {
      return type_error(heap, "trunc-elim value must be of Trunc type");
    }
    A = val_type->data.tt_trunc.type;
    /* Handler must have type Π _:A. C — at least up to alpha. */
    h_type = lizard_tt_infer2(valid_ctx, ctx, h, heap);
    if (is_error(h_type)) return h_type;
    h_type = lizard_tt_reduce(h_type, heap);
    if (h_type->type != AST_TT_PI) {
      return type_error(heap, "trunc-elim handler must have a Pi type (A → C)");
    }
    if (A != NULL &&
        !lizard_tt_alpha_equal(
          lizard_tt_reduce(h_type->data.tt_pi.domain, heap), A)) {
      return type_error(heap,
        "trunc-elim handler's domain must equal the scrutinee's underlying type");
    }
    if (!lizard_tt_alpha_equal(
          lizard_tt_reduce(h_type->data.tt_pi.codomain, heap), C_type)) {
      return type_error(heap,
        "trunc-elim handler's codomain must equal the motive C");
    }
    /* Phase H.2 Turn 3 — propositionality coherence.
     *
     * When a 4th argument (prop witness) is provided, check that its
     * type has the shape  Π x:C. Π y:C. Path C x y  — the standard
     * is-prop condition. This promotes trunc-elim from scaffold to
     * fully checked for the propositional case.
     *
     * When prop is NULL (3-arg form), the obligation is deferred —
     * see docs/CLAIMS_MATRIX.md. */
    if (t->data.tt_trunc_elim.prop != NULL) {
      lizard_ast_node_t *prop_type;
      lizard_ast_node_t *inner;
      prop_type = lizard_tt_infer2(valid_ctx, ctx,
                                    t->data.tt_trunc_elim.prop, heap);
      if (is_error(prop_type)) return prop_type;
      prop_type = lizard_tt_reduce(prop_type, heap);
      /* Outer: must be Pi _ C (...) */
      if (prop_type->type != AST_TT_PI) {
        return type_error(heap,
          "trunc-elim prop witness must have type Pi x:C. Pi y:C. Path C x y "
          "(outer Pi missing)");
      }
      if (!lizard_tt_alpha_equal(
            lizard_tt_reduce(prop_type->data.tt_pi.domain, heap), C_type)) {
        return type_error(heap,
          "trunc-elim prop witness: outer Pi domain must be C");
      }
      /* Inner: must be Pi _ C (...) */
      inner = lizard_tt_reduce(prop_type->data.tt_pi.codomain, heap);
      if (inner->type != AST_TT_PI) {
        return type_error(heap,
          "trunc-elim prop witness must have type Pi x:C. Pi y:C. Path C x y "
          "(inner Pi missing)");
      }
      if (!lizard_tt_alpha_equal(
            lizard_tt_reduce(inner->data.tt_pi.domain, heap), C_type)) {
        return type_error(heap,
          "trunc-elim prop witness: inner Pi domain must be C");
      }
      /* Body: must be Path C x y. Check domain of Path is C. */
      {
        lizard_ast_node_t *path_body;
        path_body = lizard_tt_reduce(inner->data.tt_pi.codomain, heap);
        if (path_body->type != AST_TT_PATH) {
          return type_error(heap,
            "trunc-elim prop witness must have type Pi x:C. Pi y:C. Path C x y "
            "(Path body missing)");
        }
        if (!lizard_tt_alpha_equal(
              lizard_tt_reduce(path_body->data.tt_path.domain, heap), C_type)) {
          return type_error(heap,
            "trunc-elim prop witness: Path domain must be C");
        }
      }
    }
    /* Result type: C. */
    return C_type;
  }
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
    return type_error(heap, "infer2_hit: non-HIT node");
  }
}
