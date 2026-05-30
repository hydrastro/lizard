/* tt_check_modal.c — modal (S4) judgment rules of the cubical/TT checker.
 * Extracted from tt_check.c's monolithic infer2_kind_impl: Box / Diamond and
 * their intro/elim/app/bind forms.  Dispatched to from the main checker;
 * shared helpers come from tt_check_internal.h. */
#include "primitives.h"
#include "env.h"
#include "errors.h"
#include "lizard_internal.h"
#include "mem.h"
#include "tt_check_internal.h"
#include <string.h>

lizard_ast_node_t *infer2_modal(lizard_ast_node_t *valid_ctx,
                                lizard_ast_node_t *ctx, lizard_ast_node_t *t,
                                lizard_heap_t *heap,
                                lizard_judgment_kind_t *out_kind) {
  if (out_kind != NULL) *out_kind = LIZARD_KIND_TRUE;
  switch (t->type) {
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
    lizard_judgment_kind_t body_kind = LIZARD_KIND_TRUE;
    if (lizard_logic_rule_enabled("modalities-enabled") == 0) {
      return type_error(heap, "box rejected: modalities-enabled disabled");
    }
    if (body == NULL) {
      return type_error(heap, "box missing argument");
    }
    if (lizard_logic_rule_enabled("modal-strict-typing") == 0) {
      /* Turn 1 loose typing: full ctx available inside box. */
      body_type = lizard_tt_infer2_kind(valid_ctx, ctx, body, heap, &body_kind);
    } else if (lizard_logic_rule_enabled("modal-4-axiom") == 0) {
      /* T modal logic: strict box-intro but NO 4-axiom. */
      lizard_ast_node_t *empty_valid = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      empty_valid->type = AST_NIL;
      body_type = lizard_tt_infer2_kind(empty_valid, valid_ctx, body, heap, &body_kind);
    } else {
      /* S4 modal logic: strict box-intro WITH the 4-axiom. */
      lizard_ast_node_t *empty_truth = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      empty_truth->type = AST_NIL;
      body_type = lizard_tt_infer2_kind(valid_ctx, empty_truth, body, heap, &body_kind);
    }
    if (is_error(body_type)) return body_type;
    /* M.5.9 Turn 2b — under modal-symmetric, body must be TRUE-typed.
     * Rejects (box (poss-coerce e)) and similar attempts to box a
     * poss judgment. No-op for existing code (all rules produce TRUE). */
    if (lizard_logic_rule_enabled("modal-symmetric") != 0) {
      if (body_kind != LIZARD_KIND_TRUE) {
        return type_error(heap,
          "symmetric box-intro body must be true-typed (cannot box a poss judgment)");
      }
    }
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
      lizard_judgment_kind_t body_kind = LIZARD_KIND_TRUE;
      lizard_ast_node_t *body_type;
      new_truth_ctx = ctx_extend(ctx, binder->data.variable,
                                 scrut_type->data.tt_diamond.argument, heap);
      body_type = lizard_tt_infer2_kind(valid_ctx, new_truth_ctx, body, heap, &body_kind);
      if (is_error(body_type)) return body_type;
      /* M.5.9 Turn 2b — propagate body kind to result. TRUE → TRUE
       * (M.5.5 contract); POSS body → POSS result. */
      if (out_kind != NULL) *out_kind = body_kind;
      return body_type;
    }
    /* S5: extend VALID context — the 5-axiom encoding. */
    {
      lizard_judgment_kind_t body_kind = LIZARD_KIND_TRUE;
      lizard_ast_node_t *body_type;
      new_valid_ctx = ctx_extend(valid_ctx, binder->data.variable,
                                 scrut_type->data.tt_diamond.argument, heap);
      body_type = lizard_tt_infer2_kind(new_valid_ctx, ctx, body, heap, &body_kind);
      if (is_error(body_type)) return body_type;
      /* M.5.9 Turn 2b — propagate body kind to result. */
      if (out_kind != NULL) *out_kind = body_kind;
      return body_type;
    }
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
        sprintf(fresh_name, "__boxapp_%ld",
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
        sprintf(fresh_name, "__dbind_%ld",
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
  default:
    return type_error(heap, "infer2_modal: non-modal node");
  }
}
