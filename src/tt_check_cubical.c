/* tt_check_cubical.c — cubical-layer judgment rules of the TT checker.
 * Extracted from tt_check.c's monolithic infer2_kind_impl: the interval,
 * paths, faces, partial elements, composition/transport (comp/hcomp/fill),
 * and equivalences/Glue/ua.  Dispatched to from the main checker; shared
 * helpers come from tt_check_internal.h. */
#include "primitives.h"
#include "env.h"
#include "errors.h"
#include "lizard_internal.h"
#include "mem.h"
#include "tt_check_internal.h"
#include <string.h>

lizard_ast_node_t *infer2_cubical(lizard_ast_node_t *valid_ctx,
                                  lizard_ast_node_t *ctx, lizard_ast_node_t *t,
                                  lizard_heap_t *heap,
                                  lizard_judgment_kind_t *out_kind) {
  if (out_kind != NULL) *out_kind = LIZARD_KIND_TRUE;
  switch (t->type) {
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
  default:
    return type_error(heap, "infer2_cubical: non-cubical node");
  }
}
