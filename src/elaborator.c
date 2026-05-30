/* src/elaborator.c — bidirectional elaborator with holes (Phase 7).
 *
 * Strategy: a hole-free subterm is handed straight to the trusted kernel
 * (kt_infer / kt_check).  Only when a subterm *contains* a hole does the
 * elaborator descend, pushing the expected type down so a hole in checking
 * position records its goal.  This keeps the elaborator small and lets the
 * kernel remain the single source of truth.
 */
#include "elaborator.h"
#include "mem.h"
#include <string.h>

static kterm_t *mk(lizard_heap_t *heap, kterm_tag_t tag) {
  kterm_t *t = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
  memset(t, 0, sizeof(*t));
  t->tag = tag;
  return t;
}

/* Does the term contain a metavariable (hole) anywhere? */
static int has_hole(kterm_t *t) {
  if (t == NULL) return 0;
  switch (t->tag) {
  case KT_META: return 1;
  case KT_PI:  return has_hole(t->data.pi.domain) || has_hole(t->data.pi.codomain);
  case KT_LAM: return has_hole(t->data.lam.domain) || has_hole(t->data.lam.body);
  case KT_APP: return has_hole(t->data.app.fun) || has_hole(t->data.app.arg);
  case KT_SIGMA: return has_hole(t->data.sigma.fst_type) || has_hole(t->data.sigma.snd_type);
  case KT_PAIR: return has_hole(t->data.pair.fst) || has_hole(t->data.pair.snd);
  case KT_PROJ1: case KT_PROJ2: return has_hole(t->data.proj.target);
  case KT_SUCC: return has_hole(t->data.succ.pred);
  case KT_ID: return has_hole(t->data.id.type) || has_hole(t->data.id.a) || has_hole(t->data.id.b);
  case KT_REFL: return has_hole(t->data.refl.value);
  case KT_NAT_REC: return has_hole(t->data.nat_rec.motive) || has_hole(t->data.nat_rec.zero_case)
                     || has_hole(t->data.nat_rec.succ_case) || has_hole(t->data.nat_rec.scrutinee);
  case KT_BOOL_REC: return has_hole(t->data.bool_rec.motive) || has_hole(t->data.bool_rec.true_case)
                     || has_hole(t->data.bool_rec.false_case) || has_hole(t->data.bool_rec.scrutinee);
  case KT_LIST_REC: return has_hole(t->data.list_rec.motive) || has_hole(t->data.list_rec.nil_case)
                     || has_hole(t->data.list_rec.cons_case) || has_hole(t->data.list_rec.scrutinee);
  case KT_MAYBE_REC: return has_hole(t->data.maybe_rec.motive) || has_hole(t->data.maybe_rec.nothing_case)
                     || has_hole(t->data.maybe_rec.just_case) || has_hole(t->data.maybe_rec.scrutinee);
  case KT_SUM_REC: return has_hole(t->data.sum_rec.motive) || has_hole(t->data.sum_rec.left_case)
                     || has_hole(t->data.sum_rec.right_case) || has_hole(t->data.sum_rec.scrutinee);
  case KT_J: return has_hole(t->data.j.motive) || has_hole(t->data.j.base_case)
                     || has_hole(t->data.j.a) || has_hole(t->data.j.b) || has_hole(t->data.j.proof);
  case KT_ANNOT: return has_hole(t->data.annot.term) || has_hole(t->data.annot.type);
  case KT_LET: return has_hole(t->data.let.value) || has_hole(t->data.let.body);
  case KT_INL: return has_hole(t->data.inl.value);
  case KT_INR: return has_hole(t->data.inr.value);
  case KT_JUST: return has_hole(t->data.just.value);
  case KT_MAYBE: return has_hole(t->data.maybe.elem_type);
  case KT_SUM_K: return has_hole(t->data.sum_k.left_type) || has_hole(t->data.sum_k.right_type);
  case KT_ABSURD: return has_hole(t->data.absurd.target_type);
  case KT_LIST: return has_hole(t->data.list.elem_type);
  case KT_CONS_K: return has_hole(t->data.cons_k.head) || has_hole(t->data.cons_k.tail);
  default: return 0;
  }
}

static void record_hole(elab_state_t *st, int id, kctx_t *ctx, kterm_t *goal) {
  int i;
  for (i = 0; i < st->n_holes; i++) {
    if (st->holes[i].id == id) {
      /* If we previously saw it without a goal, fill it in now. */
      if (st->holes[i].goal == NULL && goal != NULL) {
        st->holes[i].goal = goal;
        st->holes[i].ctx = ctx;
      }
      return;
    }
  }
  if (st->n_holes < ELAB_MAX_HOLES) {
    st->holes[st->n_holes].id = id;
    st->holes[st->n_holes].ctx = ctx;
    st->holes[st->n_holes].goal = goal;
    st->n_holes++;
  }
}

static int elab_check(elab_state_t *st, kctx_t *ctx, kterm_t *t, kterm_t *expected);
static kterm_t *elab_infer(elab_state_t *st, kctx_t *ctx, kterm_t *t);

static int elab_check(elab_state_t *st, kctx_t *ctx, kterm_t *t,
                      kterm_t *expected) {
  kterm_t *ty, *we;
  if (t == NULL) return 0;
  if (t->tag == KT_META) {
    record_hole(st, t->data.meta.id, ctx,
                kt_whnf(st->heap, ctx, expected));
    return 1;
  }
  if (!has_hole(t)) {
    /* No holes anywhere: the trusted kernel gives the verdict. */
    return kt_check(st->heap, ctx, t, expected) == KERNEL_OK;
  }
  we = kt_whnf(st->heap, ctx, expected);
  if (t->tag == KT_LAM && we->tag == KT_PI) {
    kctx_t *ext = kctx_extend(st->heap, ctx, t->data.lam.name,
                              we->data.pi.domain, NULL);
    return elab_check(st, ext, t->data.lam.body, we->data.pi.codomain);
  }
  ty = elab_infer(st, ctx, t);
  if (ty == NULL) return 0;
  return kt_equal(st->heap, ctx, ty, expected);
}

static kterm_t *elab_infer(elab_state_t *st, kctx_t *ctx, kterm_t *t) {
  if (t == NULL) return NULL;
  if (!has_hole(t)) return kt_infer(st->heap, ctx, t);

  switch (t->tag) {
  case KT_META:
    /* A hole with nothing pushing a type onto it: goal unknown. */
    record_hole(st, t->data.meta.id, ctx, NULL);
    return NULL;

  case KT_APP: {
    kterm_t *fty = elab_infer(st, ctx, t->data.app.fun);
    kterm_t *wf;
    if (fty == NULL) return NULL;
    wf = kt_whnf(st->heap, ctx, fty);
    if (wf->tag != KT_PI) return NULL;
    if (!elab_check(st, ctx, t->data.app.arg, wf->data.pi.domain))
      return NULL;
    return kt_subst(st->heap, wf->data.pi.codomain, 0, t->data.app.arg);
  }

  case KT_LAM: {
    kctx_t *ext;
    kterm_t *bty;
    if (has_hole(t->data.lam.domain)) return NULL; /* need an expected type */
    ext = kctx_extend(st->heap, ctx, t->data.lam.name,
                      t->data.lam.domain, NULL);
    bty = elab_infer(st, ext, t->data.lam.body);
    if (bty == NULL) return NULL;
    return kt_pi(st->heap, t->data.lam.name, t->data.lam.domain, bty);
  }

  case KT_BOOL_REC: {
    kterm_t *motive = t->data.bool_rec.motive;
    kterm_t *res;
    if (motive == NULL || has_hole(motive)
        || has_hole(t->data.bool_rec.scrutinee)) return NULL;
    res = kt_app(st->heap, motive, t->data.bool_rec.scrutinee);
    if (kt_infer(st->heap, ctx, res) == NULL) return NULL; /* motive ok */
    if (!elab_check(st, ctx, t->data.bool_rec.true_case,
                    kt_whnf(st->heap, ctx,
                            kt_app(st->heap, motive, mk(st->heap, KT_TRUE)))))
      return NULL;
    if (!elab_check(st, ctx, t->data.bool_rec.false_case,
                    kt_whnf(st->heap, ctx,
                            kt_app(st->heap, motive, mk(st->heap, KT_FALSE)))))
      return NULL;
    return res;
  }

  case KT_NAT_REC: {
    kterm_t *motive = t->data.nat_rec.motive;
    kterm_t *res, *m1, *m2, *step_t;
    if (motive == NULL || has_hole(motive)
        || has_hole(t->data.nat_rec.scrutinee)) return NULL;
    res = kt_app(st->heap, motive, t->data.nat_rec.scrutinee);
    if (kt_infer(st->heap, ctx, res) == NULL) return NULL;
    /* zero_case : motive zero */
    if (!elab_check(st, ctx, t->data.nat_rec.zero_case,
                    kt_whnf(st->heap, ctx,
                            kt_app(st->heap, motive, kt_zero(st->heap)))))
      return NULL;
    /* succ_case : Pi(k:Nat). Pi(ih: motive k). motive (succ k) */
    m1 = kt_shift(st->heap, motive, 0, 1);
    m2 = kt_shift(st->heap, motive, 0, 2);
    step_t = kt_pi(st->heap, "k", kt_nat(st->heap),
               kt_pi(st->heap, "ih",
                 kt_app(st->heap, m1, kt_var(st->heap, 0)),
                 kt_app(st->heap, m2,
                        kt_succ(st->heap, kt_var(st->heap, 1)))));
    if (!elab_check(st, ctx, t->data.nat_rec.succ_case, step_t))
      return NULL;
    return res;
  }

  default:
    return NULL;
  }
}

int elab_run(lizard_heap_t *heap, kctx_t *ctx, kterm_t *term,
             kterm_t *expected, elab_state_t *out) {
  out->heap = heap;
  out->n_holes = 0;
  out->ok = elab_check(out, ctx, term, expected);
  return out->ok;
}
