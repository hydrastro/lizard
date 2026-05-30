/* src/elaborator.c — bidirectional elaborator: holes, unification, implicits.
 *
 * Phase 7 (TT1).  A surface term may contain holes (?0, ?1 = KT_META) and may
 * apply functions whose type has IMPLICIT leading arguments ({A} -> ...).  The
 * elaborator:
 *   - pushes expected types inward so a hole learns its goal;
 *   - threads a metavariable context and unifies inferred vs expected, so a
 *     determined hole is *solved*;
 *   - inserts a fresh metavariable for each leading implicit Pi at an
 *     application (and when an implicit function is used in a value position),
 *     solving it by unification — so `(id n)` elaborates to `(id Nat n)`.
 * It PRODUCES an elaborated term (with inserted implicit arguments).  After
 * zonking, that term is handed to the trusted kernel for the real verdict.
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
  default: return 0;
  }
}

/* Does the term contain an application?  Such terms must be elaborated (an
 * application head may be an implicit function needing argument insertion). */
static int has_app(kterm_t *t) {
  if (t == NULL) return 0;
  switch (t->tag) {
  case KT_APP: return 1;
  case KT_PI:  return has_app(t->data.pi.domain) || has_app(t->data.pi.codomain);
  case KT_LAM: return has_app(t->data.lam.domain) || has_app(t->data.lam.body);
  case KT_SIGMA: return has_app(t->data.sigma.fst_type) || has_app(t->data.sigma.snd_type);
  case KT_PAIR: return has_app(t->data.pair.fst) || has_app(t->data.pair.snd);
  case KT_PROJ1: case KT_PROJ2: return has_app(t->data.proj.target);
  case KT_SUCC: return has_app(t->data.succ.pred);
  case KT_ID: return has_app(t->data.id.type) || has_app(t->data.id.a) || has_app(t->data.id.b);
  case KT_REFL: return has_app(t->data.refl.value);
  case KT_NAT_REC: return has_app(t->data.nat_rec.motive) || has_app(t->data.nat_rec.zero_case)
                     || has_app(t->data.nat_rec.succ_case) || has_app(t->data.nat_rec.scrutinee);
  case KT_BOOL_REC: return has_app(t->data.bool_rec.motive) || has_app(t->data.bool_rec.true_case)
                     || has_app(t->data.bool_rec.false_case) || has_app(t->data.bool_rec.scrutinee);
  default: return 0;
  }
}

static int scan_max_meta(kterm_t *t, int acc) {
  if (t == NULL) return acc;
  if (t->tag == KT_META) return t->data.meta.id > acc ? t->data.meta.id : acc;
  switch (t->tag) {
  case KT_PI:  acc = scan_max_meta(t->data.pi.domain, acc); return scan_max_meta(t->data.pi.codomain, acc);
  case KT_LAM: acc = scan_max_meta(t->data.lam.domain, acc); return scan_max_meta(t->data.lam.body, acc);
  case KT_APP: acc = scan_max_meta(t->data.app.fun, acc); return scan_max_meta(t->data.app.arg, acc);
  case KT_SUCC: return scan_max_meta(t->data.succ.pred, acc);
  case KT_ID: acc = scan_max_meta(t->data.id.type, acc); acc = scan_max_meta(t->data.id.a, acc); return scan_max_meta(t->data.id.b, acc);
  case KT_REFL: return scan_max_meta(t->data.refl.value, acc);
  case KT_BOOL_REC: acc = scan_max_meta(t->data.bool_rec.motive, acc); acc = scan_max_meta(t->data.bool_rec.true_case, acc);
                    acc = scan_max_meta(t->data.bool_rec.false_case, acc); return scan_max_meta(t->data.bool_rec.scrutinee, acc);
  case KT_NAT_REC: acc = scan_max_meta(t->data.nat_rec.motive, acc); acc = scan_max_meta(t->data.nat_rec.zero_case, acc);
                   acc = scan_max_meta(t->data.nat_rec.succ_case, acc); return scan_max_meta(t->data.nat_rec.scrutinee, acc);
  default: return acc;
  }
}

static meta_entry_t *ensure_user_hole(elab_state_t *st, int id, kctx_t *ctx) {
  meta_entry_t *e = meta_lookup(st->mctx, id);
  int i;
  if (e == NULL) {
    kterm_t *tymeta = meta_fresh(st->heap, st->mctx, NULL);
    e = (meta_entry_t *)lizard_heap_alloc(sizeof(meta_entry_t));
    e->id = id; e->type = tymeta; e->solution = NULL;
    e->next = st->mctx->entries; st->mctx->entries = e;
  }
  for (i = 0; i < st->n_holes; i++)
    if (st->holes[i].id == id) return e;
  if (st->n_holes < ELAB_MAX_HOLES) {
    st->holes[st->n_holes].id = id;
    st->holes[st->n_holes].ctx = ctx;
    st->holes[st->n_holes].goal = e->type;
    st->n_holes++;
  }
  return e;
}

/* Peel leading implicit Pis off `*type`, applying `*term` to a fresh meta for
 * each — this is implicit-argument insertion. */
static void insert_implicits(elab_state_t *st, kctx_t *ctx,
                             kterm_t **term, kterm_t **type) {
  kterm_t *wt = kt_whnf(st->heap, ctx, *type);
  while (wt->tag == KT_PI && wt->data.pi.implicit) {
    kterm_t *m = meta_fresh(st->heap, st->mctx, wt->data.pi.domain);
    *term = kt_app(st->heap, *term, m);
    *type = kt_subst(st->heap, wt->data.pi.codomain, 0, m);
    wt = kt_whnf(st->heap, ctx, *type);
  }
  *type = wt;
}

static int elab_check(elab_state_t *st, kctx_t *ctx, kterm_t *t,
                      kterm_t *expected, kterm_t **out);
static kterm_t *elab_infer(elab_state_t *st, kctx_t *ctx, kterm_t *t,
                           kterm_t **out);

static int elab_check(elab_state_t *st, kctx_t *ctx, kterm_t *t,
                      kterm_t *expected, kterm_t **out) {
  kterm_t *ty, *we, *elab;
  if (t == NULL) return 0;
  if (t->tag == KT_META) {
    meta_entry_t *e = ensure_user_hole(st, t->data.meta.id, ctx);
    kt_unify(st->heap, ctx, st->mctx, e->type, expected);
    *out = t;
    return 1;
  }
  we = kt_whnf(st->heap, ctx, expected);
  if (t->tag == KT_LAM && we->tag == KT_PI) {
    kterm_t *body_elab;
    kctx_t *ext = kctx_extend(st->heap, ctx, t->data.lam.name,
                              we->data.pi.domain, NULL);
    if (!elab_check(st, ext, t->data.lam.body, we->data.pi.codomain, &body_elab))
      return 0;
    *out = kt_lam(st->heap, t->data.lam.name, we->data.pi.domain, body_elab);
    return 1;
  }
  ty = elab_infer(st, ctx, t, &elab);
  if (ty == NULL) return 0;
  insert_implicits(st, ctx, &elab, &ty);  /* implicit fn in value position */
  *out = elab;
  return kt_unify(st->heap, ctx, st->mctx, ty, expected);
}

static kterm_t *elab_infer(elab_state_t *st, kctx_t *ctx, kterm_t *t,
                           kterm_t **out) {
  if (t == NULL) return NULL;
  if (!has_hole(t) && !has_app(t)) {
    *out = t;                       /* simple term: kernel types it directly */
    return kt_infer(st->heap, ctx, t);
  }
  switch (t->tag) {
  case KT_META: {
    meta_entry_t *e = ensure_user_hole(st, t->data.meta.id, ctx);
    *out = t;
    return e->type;
  }
  case KT_APP: {
    kterm_t *fun_elab, *arg_elab, *fty, *wf;
    fty = elab_infer(st, ctx, t->data.app.fun, &fun_elab);
    if (fty == NULL) return NULL;
    insert_implicits(st, ctx, &fun_elab, &fty);  /* leading implicits */
    wf = kt_whnf(st->heap, ctx, fty);
    if (wf->tag != KT_PI) return NULL;
    if (!elab_check(st, ctx, t->data.app.arg, wf->data.pi.domain, &arg_elab))
      return NULL;
    *out = kt_app(st->heap, fun_elab, arg_elab);
    return kt_subst(st->heap, wf->data.pi.codomain, 0, arg_elab);
  }
  case KT_LAM: {
    kterm_t *body_elab, *bty;
    kctx_t *ext;
    if (has_hole(t->data.lam.domain)) return NULL;
    ext = kctx_extend(st->heap, ctx, t->data.lam.name, t->data.lam.domain, NULL);
    bty = elab_infer(st, ext, t->data.lam.body, &body_elab);
    if (bty == NULL) return NULL;
    *out = kt_lam(st->heap, t->data.lam.name, t->data.lam.domain, body_elab);
    return kt_pi(st->heap, t->data.lam.name, t->data.lam.domain, bty);
  }
  case KT_REFL: {
    kterm_t *v_elab, *vt;
    vt = elab_infer(st, ctx, t->data.refl.value, &v_elab);
    if (vt == NULL) return NULL;
    *out = kt_refl(st->heap, v_elab);
    return kt_id(st->heap, vt, v_elab, v_elab);
  }
  case KT_SUCC: {
    kterm_t *p_elab;
    if (!elab_check(st, ctx, t->data.succ.pred, kt_nat(st->heap), &p_elab)) return NULL;
    *out = kt_succ(st->heap, p_elab);
    return kt_nat(st->heap);
  }
  case KT_BOOL_REC: {
    kterm_t *motive = t->data.bool_rec.motive;
    kterm_t *res, *tc, *fc, *r;
    if (motive == NULL || has_hole(motive) || has_hole(t->data.bool_rec.scrutinee)) return NULL;
    res = kt_app(st->heap, motive, t->data.bool_rec.scrutinee);
    if (kt_infer(st->heap, ctx, res) == NULL) return NULL;
    if (!elab_check(st, ctx, t->data.bool_rec.true_case,
                    kt_whnf(st->heap, ctx, kt_app(st->heap, motive, mk(st->heap, KT_TRUE))), &tc)) return NULL;
    if (!elab_check(st, ctx, t->data.bool_rec.false_case,
                    kt_whnf(st->heap, ctx, kt_app(st->heap, motive, mk(st->heap, KT_FALSE))), &fc)) return NULL;
    r = mk(st->heap, KT_BOOL_REC);
    r->data.bool_rec.motive = motive;
    r->data.bool_rec.true_case = tc;
    r->data.bool_rec.false_case = fc;
    r->data.bool_rec.scrutinee = t->data.bool_rec.scrutinee;
    *out = r;
    return res;
  }
  case KT_NAT_REC: {
    kterm_t *motive = t->data.nat_rec.motive;
    kterm_t *res, *zc, *sc, *m1, *m2, *step_t, *r;
    if (motive == NULL || has_hole(motive) || has_hole(t->data.nat_rec.scrutinee)) return NULL;
    res = kt_app(st->heap, motive, t->data.nat_rec.scrutinee);
    if (kt_infer(st->heap, ctx, res) == NULL) return NULL;
    if (!elab_check(st, ctx, t->data.nat_rec.zero_case,
                    kt_whnf(st->heap, ctx, kt_app(st->heap, motive, kt_zero(st->heap))), &zc)) return NULL;
    m1 = kt_shift(st->heap, motive, 0, 1);
    m2 = kt_shift(st->heap, motive, 0, 2);
    step_t = kt_pi(st->heap, "k", kt_nat(st->heap),
               kt_pi(st->heap, "ih",
                 kt_app(st->heap, m1, kt_var(st->heap, 0)),
                 kt_app(st->heap, m2, kt_succ(st->heap, kt_var(st->heap, 1)))));
    if (!elab_check(st, ctx, t->data.nat_rec.succ_case, step_t, &sc)) return NULL;
    r = mk(st->heap, KT_NAT_REC);
    r->data.nat_rec.motive = motive;
    r->data.nat_rec.zero_case = zc;
    r->data.nat_rec.succ_case = sc;
    r->data.nat_rec.scrutinee = t->data.nat_rec.scrutinee;
    *out = r;
    return res;
  }
  default:
    return NULL;
  }
}

int elab_run(lizard_heap_t *heap, kctx_t *ctx, kterm_t *term,
             kterm_t *expected, elab_state_t *out) {
  int maxid;
  kterm_t *elaborated = NULL;
  out->heap = heap;
  out->n_holes = 0;
  out->mctx = meta_ctx_create(heap);
  maxid = scan_max_meta(term, -1);
  out->mctx->next_id = maxid + 1;
  out->ok = elab_check(out, ctx, term, expected, &elaborated);
  out->elaborated = out->ok ? meta_zonk(heap, out->mctx, elaborated) : term;
  return out->ok;
}
