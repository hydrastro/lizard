/* src/kernel.c — Trusted kernel implementation (Track K).
 *
 * This is the code that must be correct for soundness. It is
 * intentionally kept small and simple.
 */
#include "kernel.h"
#include "mem.h"
#include <stdio.h>
#include <string.h>

/* ---- constructors ---- */

static kterm_t *kt_alloc(lizard_heap_t *heap, kterm_tag_t tag) {
  kterm_t *t = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
  memset(t, 0, sizeof(kterm_t));
  t->tag = tag;
  return t;
}

kterm_t *kt_var(lizard_heap_t *heap, int index) {
  kterm_t *t = kt_alloc(heap, KT_VAR);
  t->data.var.index = index;
  return t;
}

kterm_t *kt_sort(lizard_heap_t *heap, int level) {
  kterm_t *t = kt_alloc(heap, KT_SORT);
  t->data.sort.level = level;
  return t;
}

kterm_t *kt_pi(lizard_heap_t *heap, const char *name,
               kterm_t *domain, kterm_t *codomain) {
  kterm_t *t = kt_alloc(heap, KT_PI);
  t->data.pi.name = name;
  t->data.pi.domain = domain;
  t->data.pi.codomain = codomain;
  return t;
}

kterm_t *kt_lam(lizard_heap_t *heap, const char *name,
                kterm_t *domain, kterm_t *body) {
  kterm_t *t = kt_alloc(heap, KT_LAM);
  t->data.lam.name = name;
  t->data.lam.domain = domain;
  t->data.lam.body = body;
  return t;
}

kterm_t *kt_app(lizard_heap_t *heap, kterm_t *fun, kterm_t *arg) {
  kterm_t *t = kt_alloc(heap, KT_APP);
  t->data.app.fun = fun;
  t->data.app.arg = arg;
  return t;
}

kterm_t *kt_nat(lizard_heap_t *heap) {
  return kt_alloc(heap, KT_NAT);
}

kterm_t *kt_zero(lizard_heap_t *heap) {
  return kt_alloc(heap, KT_ZERO);
}

kterm_t *kt_succ(lizard_heap_t *heap, kterm_t *pred) {
  kterm_t *t = kt_alloc(heap, KT_SUCC);
  t->data.succ.pred = pred;
  return t;
}

kterm_t *kt_id(lizard_heap_t *heap, kterm_t *type, kterm_t *a, kterm_t *b) {
  kterm_t *t = kt_alloc(heap, KT_ID);
  t->data.id.type = type;
  t->data.id.a = a;
  t->data.id.b = b;
  return t;
}

kterm_t *kt_refl(lizard_heap_t *heap, kterm_t *value) {
  kterm_t *t = kt_alloc(heap, KT_REFL);
  t->data.refl.value = value;
  return t;
}

/* Sigma constructors. */
static kterm_t *kt_sigma(lizard_heap_t *heap, const char *name,
                          kterm_t *fst_type, kterm_t *snd_type) {
  kterm_t *t = kt_alloc(heap, KT_SIGMA);
  t->data.sigma.name = name;
  t->data.sigma.fst_type = fst_type;
  t->data.sigma.snd_type = snd_type;
  return t;
}

static kterm_t *kt_pair(lizard_heap_t *heap, kterm_t *fst, kterm_t *snd) {
  kterm_t *t = kt_alloc(heap, KT_PAIR);
  t->data.pair.fst = fst;
  t->data.pair.snd = snd;
  return t;
}

static kterm_t *kt_proj1(lizard_heap_t *heap, kterm_t *target) {
  kterm_t *t = kt_alloc(heap, KT_PROJ1);
  t->data.proj.target = target;
  return t;
}

static kterm_t *kt_proj2(lizard_heap_t *heap, kterm_t *target) {
  kterm_t *t = kt_alloc(heap, KT_PROJ2);
  t->data.proj.target = target;
  return t;
}

/* ---- context ---- */

kctx_t *kctx_create(lizard_heap_t *heap) {
  kctx_t *ctx = (kctx_t *)lizard_heap_alloc(sizeof(kctx_t));
  ctx->entries = NULL;
  ctx->depth = 0;
  ctx->heap = heap;
  return ctx;
}

kctx_t *kctx_extend(lizard_heap_t *heap, kctx_t *ctx,
                     const char *name, kterm_t *type, kterm_t *value) {
  kctx_t *new_ctx = (kctx_t *)lizard_heap_alloc(sizeof(kctx_t));
  kctx_entry_t *entry = (kctx_entry_t *)lizard_heap_alloc(sizeof(kctx_entry_t));
  entry->name = name;
  entry->type = type;
  entry->value = value;
  entry->next = ctx->entries;
  new_ctx->entries = entry;
  new_ctx->depth = ctx->depth + 1;
  new_ctx->heap = heap;
  return new_ctx;
}

kctx_entry_t *kctx_lookup(kctx_t *ctx, int index) {
  kctx_entry_t *e = ctx->entries;
  int i;
  for (i = 0; i < index && e != NULL; i++) {
    e = e->next;
  }
  return e;
}

/* ---- substitution ---- */

static kterm_t *kt_shift(lizard_heap_t *heap, kterm_t *t, int cutoff, int delta) {
  if (t == NULL) return NULL;
  switch (t->tag) {
  case KT_VAR:
    if (t->data.var.index >= cutoff)
      return kt_var(heap, t->data.var.index + delta);
    return t;
  case KT_SORT: case KT_NAT: case KT_ZERO:
  case KT_BOOL: case KT_TRUE: case KT_FALSE:
  case KT_UNIT: case KT_STAR:
  case KT_META:
  case KT_NIL_K:
    return t;
  case KT_SUCC:
    return kt_succ(heap, kt_shift(heap, t->data.succ.pred, cutoff, delta));
  case KT_PI:
    return kt_pi(heap, t->data.pi.name,
                 kt_shift(heap, t->data.pi.domain, cutoff, delta),
                 kt_shift(heap, t->data.pi.codomain, cutoff + 1, delta));
  case KT_LAM:
    return kt_lam(heap, t->data.lam.name,
                  kt_shift(heap, t->data.lam.domain, cutoff, delta),
                  kt_shift(heap, t->data.lam.body, cutoff + 1, delta));
  case KT_APP:
    return kt_app(heap, kt_shift(heap, t->data.app.fun, cutoff, delta),
                        kt_shift(heap, t->data.app.arg, cutoff, delta));
  case KT_ID:
    return kt_id(heap, kt_shift(heap, t->data.id.type, cutoff, delta),
                       kt_shift(heap, t->data.id.a, cutoff, delta),
                       kt_shift(heap, t->data.id.b, cutoff, delta));
  case KT_REFL:
    return kt_refl(heap, kt_shift(heap, t->data.refl.value, cutoff, delta));
  case KT_SIGMA:
    return kt_sigma(heap, t->data.sigma.name,
                    kt_shift(heap, t->data.sigma.fst_type, cutoff, delta),
                    kt_shift(heap, t->data.sigma.snd_type, cutoff + 1, delta));
  case KT_PAIR:
    return kt_pair(heap, kt_shift(heap, t->data.pair.fst, cutoff, delta),
                         kt_shift(heap, t->data.pair.snd, cutoff, delta));
  case KT_PROJ1:
    return kt_proj1(heap, kt_shift(heap, t->data.proj.target, cutoff, delta));
  case KT_PROJ2:
    return kt_proj2(heap, kt_shift(heap, t->data.proj.target, cutoff, delta));
  case KT_LIST: {
    kterm_t *l = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(l, 0, sizeof(*l));
    l->tag = KT_LIST;
    l->data.list.elem_type = kt_shift(heap, t->data.list.elem_type, cutoff, delta);
    return l;
  }
  case KT_CONS_K: {
    kterm_t *c = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(c, 0, sizeof(*c));
    c->tag = KT_CONS_K;
    c->data.cons_k.head = kt_shift(heap, t->data.cons_k.head, cutoff, delta);
    c->data.cons_k.tail = kt_shift(heap, t->data.cons_k.tail, cutoff, delta);
    return c;
  }
  default:
    return t;
  }
}

kterm_t *kt_subst(lizard_heap_t *heap, kterm_t *t, int n, kterm_t *s) {
  if (t == NULL) return NULL;
  switch (t->tag) {
  case KT_VAR:
    if (t->data.var.index == n) return s;
    if (t->data.var.index > n) return kt_var(heap, t->data.var.index - 1);
    return t;
  case KT_SORT: case KT_NAT: case KT_ZERO:
  case KT_BOOL: case KT_TRUE: case KT_FALSE:
  case KT_UNIT: case KT_STAR:
  case KT_META:
  case KT_NIL_K:
    return t;
  case KT_SUCC:
    return kt_succ(heap, kt_subst(heap, t->data.succ.pred, n, s));
  case KT_PI:
    return kt_pi(heap, t->data.pi.name,
                 kt_subst(heap, t->data.pi.domain, n, s),
                 kt_subst(heap, t->data.pi.codomain, n + 1,
                           kt_shift(heap, s, 0, 1)));
  case KT_LAM:
    return kt_lam(heap, t->data.lam.name,
                  kt_subst(heap, t->data.lam.domain, n, s),
                  kt_subst(heap, t->data.lam.body, n + 1,
                            kt_shift(heap, s, 0, 1)));
  case KT_APP:
    return kt_app(heap, kt_subst(heap, t->data.app.fun, n, s),
                        kt_subst(heap, t->data.app.arg, n, s));
  case KT_ID:
    return kt_id(heap, kt_subst(heap, t->data.id.type, n, s),
                       kt_subst(heap, t->data.id.a, n, s),
                       kt_subst(heap, t->data.id.b, n, s));
  case KT_REFL:
    return kt_refl(heap, kt_subst(heap, t->data.refl.value, n, s));
  case KT_SIGMA:
    return kt_sigma(heap, t->data.sigma.name,
                    kt_subst(heap, t->data.sigma.fst_type, n, s),
                    kt_subst(heap, t->data.sigma.snd_type, n + 1,
                              kt_shift(heap, s, 0, 1)));
  case KT_PAIR:
    return kt_pair(heap, kt_subst(heap, t->data.pair.fst, n, s),
                         kt_subst(heap, t->data.pair.snd, n, s));
  case KT_PROJ1:
    return kt_proj1(heap, kt_subst(heap, t->data.proj.target, n, s));
  case KT_PROJ2:
    return kt_proj2(heap, kt_subst(heap, t->data.proj.target, n, s));
  case KT_LIST: {
    kterm_t *l = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(l, 0, sizeof(*l));
    l->tag = KT_LIST;
    l->data.list.elem_type = kt_subst(heap, t->data.list.elem_type, n, s);
    return l;
  }
  case KT_CONS_K: {
    kterm_t *c = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(c, 0, sizeof(*c));
    c->tag = KT_CONS_K;
    c->data.cons_k.head = kt_subst(heap, t->data.cons_k.head, n, s);
    c->data.cons_k.tail = kt_subst(heap, t->data.cons_k.tail, n, s);
    return c;
  }
  default:
    return t;
  }
}

/* ---- weak head normal form ---- */

kterm_t *kt_whnf(lizard_heap_t *heap, kctx_t *ctx, kterm_t *t) {
  if (t == NULL) return NULL;
  switch (t->tag) {
  case KT_VAR: {
    kctx_entry_t *e = kctx_lookup(ctx, t->data.var.index);
    if (e != NULL && e->value != NULL)
      return kt_whnf(heap, ctx, e->value);
    return t;
  }
  case KT_APP: {
    kterm_t *fn = kt_whnf(heap, ctx, t->data.app.fun);
    if (fn->tag == KT_LAM) {
      kterm_t *result = kt_subst(heap, fn->data.lam.body, 0, t->data.app.arg);
      return kt_whnf(heap, ctx, result);
    }
    if (fn != t->data.app.fun)
      return kt_app(heap, fn, t->data.app.arg);
    return t;
  }
  case KT_LET: {
    kterm_t *result = kt_subst(heap, t->data.let.body, 0, t->data.let.value);
    return kt_whnf(heap, ctx, result);
  }
  case KT_ANNOT:
    return kt_whnf(heap, ctx, t->data.annot.term);
  case KT_PROJ1: {
    kterm_t *target = kt_whnf(heap, ctx, t->data.proj.target);
    if (target->tag == KT_PAIR)
      return kt_whnf(heap, ctx, target->data.pair.fst);
    if (target != t->data.proj.target)
      return kt_proj1(heap, target);
    return t;
  }
  case KT_PROJ2: {
    kterm_t *target = kt_whnf(heap, ctx, t->data.proj.target);
    if (target->tag == KT_PAIR)
      return kt_whnf(heap, ctx, target->data.pair.snd);
    if (target != t->data.proj.target)
      return kt_proj2(heap, target);
    return t;
  }
  case KT_NAT_REC: {
    kterm_t *scrut = kt_whnf(heap, ctx, t->data.nat_rec.scrutinee);
    if (scrut->tag == KT_ZERO)
      return kt_whnf(heap, ctx, t->data.nat_rec.zero_case);
    if (scrut->tag == KT_SUCC) {
      kterm_t *rec = kt_alloc(heap, KT_NAT_REC);
      kterm_t *step;
      rec->data.nat_rec.motive = t->data.nat_rec.motive;
      rec->data.nat_rec.zero_case = t->data.nat_rec.zero_case;
      rec->data.nat_rec.succ_case = t->data.nat_rec.succ_case;
      rec->data.nat_rec.scrutinee = scrut->data.succ.pred;
      step = kt_app(heap, kt_app(heap, t->data.nat_rec.succ_case,
                                   scrut->data.succ.pred), rec);
      return kt_whnf(heap, ctx, step);
    }
    return t;
  }
  case KT_BOOL_REC: {
    kterm_t *scrut = kt_whnf(heap, ctx, t->data.bool_rec.scrutinee);
    if (scrut->tag == KT_TRUE)
      return kt_whnf(heap, ctx, t->data.bool_rec.true_case);
    if (scrut->tag == KT_FALSE)
      return kt_whnf(heap, ctx, t->data.bool_rec.false_case);
    return t;
  }

  case KT_LIST_REC: {
    kterm_t *scrut = kt_whnf(heap, ctx, t->data.list_rec.scrutinee);
    if (scrut->tag == KT_NIL_K)
      return kt_whnf(heap, ctx, t->data.list_rec.nil_case);
    if (scrut->tag == KT_CONS_K) {
      /* listrec C n c (cons h t) → c h t (listrec C n c t) */
      kterm_t *rec = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
      kterm_t *step;
      memset(rec, 0, sizeof(*rec));
      rec->tag = KT_LIST_REC;
      rec->data.list_rec.motive = t->data.list_rec.motive;
      rec->data.list_rec.nil_case = t->data.list_rec.nil_case;
      rec->data.list_rec.cons_case = t->data.list_rec.cons_case;
      rec->data.list_rec.scrutinee = scrut->data.cons_k.tail;
      step = kt_app(heap,
               kt_app(heap,
                 kt_app(heap, t->data.list_rec.cons_case, scrut->data.cons_k.head),
                 scrut->data.cons_k.tail),
               rec);
      return kt_whnf(heap, ctx, step);
    }
    return t;
  }
  case KT_J: {
    /* J C d A a b (refl a) → d a */
    kterm_t *pf = kt_whnf(heap, ctx, t->data.j.proof);
    if (pf->tag == KT_REFL) {
      /* J-beta: J C d A a a (refl a) → d a */
      return kt_whnf(heap, ctx, kt_app(heap, t->data.j.base_case,
                                         pf->data.refl.value));
    }
    return t;
  }
  default:
    return t;
  }
}

/* ---- definitional equality ---- */

int kt_equal(lizard_heap_t *heap, kctx_t *ctx, kterm_t *a, kterm_t *b) {
  kterm_t *na, *nb;
  if (a == b) return 1;
  if (a == NULL || b == NULL) return 0;
  na = kt_whnf(heap, ctx, a);
  nb = kt_whnf(heap, ctx, b);
  if (na->tag != nb->tag) return 0;
  switch (na->tag) {
  case KT_VAR:
    return na->data.var.index == nb->data.var.index;
  case KT_SORT:
    return na->data.sort.level == nb->data.sort.level;
  case KT_NAT: case KT_ZERO:
  case KT_BOOL: case KT_TRUE: case KT_FALSE:
  case KT_UNIT: case KT_STAR:
  case KT_NIL_K:
    return 1;
  case KT_LIST:
    return kt_equal(heap, ctx, na->data.list.elem_type, nb->data.list.elem_type);
  case KT_CONS_K:
    return kt_equal(heap, ctx, na->data.cons_k.head, nb->data.cons_k.head) &&
           kt_equal(heap, ctx, na->data.cons_k.tail, nb->data.cons_k.tail);
  case KT_META:
    return na->data.meta.id == nb->data.meta.id;
  case KT_SUCC:
    return kt_equal(heap, ctx, na->data.succ.pred, nb->data.succ.pred);
  case KT_PI:
    return kt_equal(heap, ctx, na->data.pi.domain, nb->data.pi.domain) &&
           kt_equal(heap, ctx, na->data.pi.codomain, nb->data.pi.codomain);
  case KT_LAM:
    return kt_equal(heap, ctx, na->data.lam.body, nb->data.lam.body);
  case KT_APP:
    return kt_equal(heap, ctx, na->data.app.fun, nb->data.app.fun) &&
           kt_equal(heap, ctx, na->data.app.arg, nb->data.app.arg);
  case KT_SIGMA:
    return kt_equal(heap, ctx, na->data.sigma.fst_type, nb->data.sigma.fst_type) &&
           kt_equal(heap, ctx, na->data.sigma.snd_type, nb->data.sigma.snd_type);
  case KT_PAIR:
    return kt_equal(heap, ctx, na->data.pair.fst, nb->data.pair.fst) &&
           kt_equal(heap, ctx, na->data.pair.snd, nb->data.pair.snd);
  case KT_PROJ1:
    return kt_equal(heap, ctx, na->data.proj.target, nb->data.proj.target);
  case KT_PROJ2:
    return kt_equal(heap, ctx, na->data.proj.target, nb->data.proj.target);
  case KT_ID:
    return kt_equal(heap, ctx, na->data.id.type, nb->data.id.type) &&
           kt_equal(heap, ctx, na->data.id.a, nb->data.id.a) &&
           kt_equal(heap, ctx, na->data.id.b, nb->data.id.b);
  case KT_REFL:
    return kt_equal(heap, ctx, na->data.refl.value, nb->data.refl.value);
  default:
    return 0;
  }
}

/* ---- type inference ---- */

kterm_t *kt_infer(lizard_heap_t *heap, kctx_t *ctx, kterm_t *t) {
  if (t == NULL) return NULL;
  switch (t->tag) {
  case KT_VAR: {
    kctx_entry_t *e = kctx_lookup(ctx, t->data.var.index);
    return e ? e->type : NULL;
  }
  case KT_SORT:
    return kt_sort(heap, t->data.sort.level + 1);
  case KT_NAT:
    return kt_sort(heap, 0);
  case KT_ZERO:
    return kt_nat(heap);
  case KT_BOOL:
    return kt_sort(heap, 0);
  case KT_TRUE: case KT_FALSE: {
    kterm_t *b = kt_alloc(heap, KT_BOOL);
    return b;
  }
  case KT_UNIT:
    return kt_sort(heap, 0);
  case KT_STAR: {
    kterm_t *u = kt_alloc(heap, KT_UNIT);
    return u;
  }
  case KT_SUCC: {
    kterm_t *pt = kt_infer(heap, ctx, t->data.succ.pred);
    if (pt == NULL) return NULL;
    pt = kt_whnf(heap, ctx, pt);
    return (pt->tag == KT_NAT) ? kt_nat(heap) : NULL;
  }
  case KT_PI: {
    kterm_t *dt = kt_infer(heap, ctx, t->data.pi.domain);
    kterm_t *ct2;
    kctx_t *ext;
    int l, r;
    if (dt == NULL) return NULL;
    dt = kt_whnf(heap, ctx, dt);
    if (dt->tag != KT_SORT) return NULL;
    ext = kctx_extend(heap, ctx, t->data.pi.name, t->data.pi.domain, NULL);
    ct2 = kt_infer(heap, ext, t->data.pi.codomain);
    if (ct2 == NULL) return NULL;
    ct2 = kt_whnf(heap, ext, ct2);
    if (ct2->tag != KT_SORT) return NULL;
    l = dt->data.sort.level;
    r = ct2->data.sort.level;
    return kt_sort(heap, l > r ? l : r);
  }
  case KT_SIGMA: {
    kterm_t *dt = kt_infer(heap, ctx, t->data.sigma.fst_type);
    kterm_t *ct2;
    kctx_t *ext;
    int l, r;
    if (dt == NULL) return NULL;
    dt = kt_whnf(heap, ctx, dt);
    if (dt->tag != KT_SORT) return NULL;
    ext = kctx_extend(heap, ctx, t->data.sigma.name, t->data.sigma.fst_type, NULL);
    ct2 = kt_infer(heap, ext, t->data.sigma.snd_type);
    if (ct2 == NULL) return NULL;
    ct2 = kt_whnf(heap, ext, ct2);
    if (ct2->tag != KT_SORT) return NULL;
    l = dt->data.sort.level;
    r = ct2->data.sort.level;
    return kt_sort(heap, l > r ? l : r);
  }
  case KT_PAIR: {
    kterm_t *ft = kt_infer(heap, ctx, t->data.pair.fst);
    kterm_t *st = kt_infer(heap, ctx, t->data.pair.snd);
    if (ft == NULL || st == NULL) return NULL;
    return kt_sigma(heap, "_", ft, kt_shift(heap, st, 0, 1));
  }
  case KT_PROJ1: {
    kterm_t *tt = kt_infer(heap, ctx, t->data.proj.target);
    if (tt == NULL) return NULL;
    tt = kt_whnf(heap, ctx, tt);
    if (tt->tag != KT_SIGMA) return NULL;
    return tt->data.sigma.fst_type;
  }
  case KT_PROJ2: {
    kterm_t *tt = kt_infer(heap, ctx, t->data.proj.target);
    kterm_t *fst;
    if (tt == NULL) return NULL;
    tt = kt_whnf(heap, ctx, tt);
    if (tt->tag != KT_SIGMA) return NULL;
    fst = kt_proj1(heap, t->data.proj.target);
    return kt_subst(heap, tt->data.sigma.snd_type, 0, fst);
  }
  case KT_LAM: {
    kctx_t *ext = kctx_extend(heap, ctx, t->data.lam.name,
                               t->data.lam.domain, NULL);
    kterm_t *body_type = kt_infer(heap, ext, t->data.lam.body);
    if (body_type == NULL) return NULL;
    return kt_pi(heap, t->data.lam.name, t->data.lam.domain, body_type);
  }
  case KT_APP: {
    kterm_t *fn_type = kt_infer(heap, ctx, t->data.app.fun);
    kterm_t *arg_type;
    if (fn_type == NULL) return NULL;
    fn_type = kt_whnf(heap, ctx, fn_type);
    if (fn_type->tag != KT_PI) return NULL;
    arg_type = kt_infer(heap, ctx, t->data.app.arg);
    if (arg_type == NULL) return NULL;
    if (!kt_equal(heap, ctx, arg_type, fn_type->data.pi.domain)) return NULL;
    return kt_subst(heap, fn_type->data.pi.codomain, 0, t->data.app.arg);
  }
  case KT_ID: {
    kterm_t *tt = kt_infer(heap, ctx, t->data.id.type);
    if (tt == NULL) return NULL;
    tt = kt_whnf(heap, ctx, tt);
    if (tt->tag != KT_SORT) return NULL;
    return kt_sort(heap, tt->data.sort.level);
  }
  case KT_REFL: {
    kterm_t *vt = kt_infer(heap, ctx, t->data.refl.value);
    if (vt == NULL) return NULL;
    return kt_id(heap, vt, t->data.refl.value, t->data.refl.value);
  }

  case KT_LIST: {
    kterm_t *et = kt_infer(heap, ctx, t->data.list.elem_type);
    if (et == NULL) return NULL;
    et = kt_whnf(heap, ctx, et);
    if (et->tag != KT_SORT) return NULL;
    return kt_sort(heap, et->data.sort.level);
  }
  case KT_NIL_K: {
    /* nil needs a type annotation to infer; without one, use Sort(0). */
    kterm_t *l = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(l, 0, sizeof(*l));
    l->tag = KT_LIST;
    l->data.list.elem_type = t->data.nil_k.elem_type;
    return (t->data.nil_k.elem_type != NULL) ? l : NULL;
  }
  case KT_CONS_K: {
    kterm_t *ht = kt_infer(heap, ctx, t->data.cons_k.head);
    kterm_t *l;
    if (ht == NULL) return NULL;
    l = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(l, 0, sizeof(*l));
    l->tag = KT_LIST;
    l->data.list.elem_type = ht;
    return l;
  }
  case KT_ANNOT: {
    kernel_result_t r = kt_check(heap, ctx, t->data.annot.term,
                                  t->data.annot.type);
    return (r == KERNEL_OK) ? t->data.annot.type : NULL;
  }
  default:
    return NULL;
  }
}

kernel_result_t kt_check(lizard_heap_t *heap, kctx_t *ctx,
                          kterm_t *term, kterm_t *expected_type) {
  kterm_t *inferred = kt_infer(heap, ctx, term);
  if (inferred == NULL) return KERNEL_TYPE_ERROR;
  if (!kt_equal(heap, ctx, inferred, expected_type))
    return KERNEL_TYPE_ERROR;
  return KERNEL_OK;
}

/* ---- printing ---- */

void kt_fprint(FILE *fp, kterm_t *t) {
  if (t == NULL) { fprintf(fp, "?"); return; }
  switch (t->tag) {
  case KT_VAR: fprintf(fp, "#%d", t->data.var.index); break;
  case KT_SORT: fprintf(fp, "Sort(%d)", t->data.sort.level); break;
  case KT_NAT: fprintf(fp, "Nat"); break;
  case KT_ZERO: fprintf(fp, "0"); break;
  case KT_SUCC:
    fprintf(fp, "(succ "); kt_fprint(fp, t->data.succ.pred); fprintf(fp, ")");
    break;
  case KT_PI:
    fprintf(fp, "(Pi (%s : ", t->data.pi.name ? t->data.pi.name : "_");
    kt_fprint(fp, t->data.pi.domain);
    fprintf(fp, ") "); kt_fprint(fp, t->data.pi.codomain); fprintf(fp, ")");
    break;
  case KT_SIGMA:
    fprintf(fp, "(Sigma (%s : ", t->data.sigma.name ? t->data.sigma.name : "_");
    kt_fprint(fp, t->data.sigma.fst_type);
    fprintf(fp, ") "); kt_fprint(fp, t->data.sigma.snd_type); fprintf(fp, ")");
    break;
  case KT_LAM:
    fprintf(fp, "(lam (%s : ", t->data.lam.name ? t->data.lam.name : "_");
    kt_fprint(fp, t->data.lam.domain);
    fprintf(fp, ") "); kt_fprint(fp, t->data.lam.body); fprintf(fp, ")");
    break;
  case KT_APP:
    fprintf(fp, "("); kt_fprint(fp, t->data.app.fun);
    fprintf(fp, " "); kt_fprint(fp, t->data.app.arg); fprintf(fp, ")");
    break;
  case KT_PAIR:
    fprintf(fp, "(pair "); kt_fprint(fp, t->data.pair.fst);
    fprintf(fp, " "); kt_fprint(fp, t->data.pair.snd); fprintf(fp, ")");
    break;
  case KT_PROJ1:
    fprintf(fp, "(fst "); kt_fprint(fp, t->data.proj.target); fprintf(fp, ")");
    break;
  case KT_PROJ2:
    fprintf(fp, "(snd "); kt_fprint(fp, t->data.proj.target); fprintf(fp, ")");
    break;
  case KT_ID:
    fprintf(fp, "(Id "); kt_fprint(fp, t->data.id.type);
    fprintf(fp, " "); kt_fprint(fp, t->data.id.a);
    fprintf(fp, " "); kt_fprint(fp, t->data.id.b); fprintf(fp, ")");
    break;
  case KT_REFL:
    fprintf(fp, "(refl "); kt_fprint(fp, t->data.refl.value); fprintf(fp, ")");
    break;
  case KT_J:
    fprintf(fp, "(J ...)");
    break;
  case KT_NAT_REC:
    fprintf(fp, "(natrec ...)");
    break;

  case KT_BOOL: fprintf(fp, "Bool"); break;
  case KT_TRUE: fprintf(fp, "true"); break;
  case KT_FALSE: fprintf(fp, "false"); break;
  case KT_UNIT: fprintf(fp, "Unit"); break;
  case KT_STAR: fprintf(fp, "*"); break;
  case KT_BOOL_REC:
    fprintf(fp, "(if "); kt_fprint(fp, t->data.bool_rec.scrutinee);
    fprintf(fp, " "); kt_fprint(fp, t->data.bool_rec.true_case);
    fprintf(fp, " "); kt_fprint(fp, t->data.bool_rec.false_case);
    fprintf(fp, ")"); break;
  case KT_META:
    fprintf(fp, "?%d", t->data.meta.id);
    break;

  case KT_LIST:
    fprintf(fp, "(List "); kt_fprint(fp, t->data.list.elem_type); fprintf(fp, ")");
    break;
  case KT_NIL_K:
    fprintf(fp, "nil"); break;
  case KT_CONS_K:
    fprintf(fp, "(cons "); kt_fprint(fp, t->data.cons_k.head);
    fprintf(fp, " "); kt_fprint(fp, t->data.cons_k.tail); fprintf(fp, ")");
    break;
  case KT_LIST_REC:
    fprintf(fp, "(listrec ...)"); break;
  default:
    fprintf(fp, "<kterm:%d>", t->tag);
    break;
  }
}


/* ---- metavariables ---- */

meta_ctx_t *meta_ctx_create(lizard_heap_t *heap) {
  meta_ctx_t *mctx = (meta_ctx_t *)lizard_heap_alloc(sizeof(meta_ctx_t));
  mctx->entries = NULL;
  mctx->next_id = 0;
  return mctx;
}

kterm_t *meta_fresh(lizard_heap_t *heap, meta_ctx_t *mctx, kterm_t *type) {
  meta_entry_t *e = (meta_entry_t *)lizard_heap_alloc(sizeof(meta_entry_t));
  kterm_t *t = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
  memset(t, 0, sizeof(*t));
  t->tag = KT_META;
  t->data.meta.id = mctx->next_id;
  e->id = mctx->next_id++;
  e->type = type;
  e->solution = NULL;
  e->next = mctx->entries;
  mctx->entries = e;
  return t;
}

meta_entry_t *meta_lookup(meta_ctx_t *mctx, int id) {
  meta_entry_t *e;
  for (e = mctx->entries; e != NULL; e = e->next) {
    if (e->id == id) return e;
  }
  return NULL;
}

int meta_solve(meta_ctx_t *mctx, int id, kterm_t *solution) {
  meta_entry_t *e = meta_lookup(mctx, id);
  if (e == NULL) return -1;
  if (e->solution != NULL) return -1;  /* already solved */
  e->solution = solution;
  return 0;
}

/* Zonk: substitute all solved metas in a term. */
kterm_t *meta_zonk(lizard_heap_t *heap, meta_ctx_t *mctx, kterm_t *t) {
  meta_entry_t *e;
  if (t == NULL) return NULL;
  if (t->tag == KT_META) {
    e = meta_lookup(mctx, t->data.meta.id);
    if (e != NULL && e->solution != NULL) {
      return meta_zonk(heap, mctx, e->solution);
    }
    return t;
  }
  switch (t->tag) {
  case KT_VAR: case KT_SORT: case KT_NAT: case KT_ZERO:
  case KT_BOOL: case KT_TRUE: case KT_FALSE:
  case KT_UNIT: case KT_STAR:
    return t;
  case KT_SUCC:
    return kt_succ(heap, meta_zonk(heap, mctx, t->data.succ.pred));
  case KT_PI:
    return kt_pi(heap, t->data.pi.name,
                 meta_zonk(heap, mctx, t->data.pi.domain),
                 meta_zonk(heap, mctx, t->data.pi.codomain));
  case KT_LAM:
    return kt_lam(heap, t->data.lam.name,
                  meta_zonk(heap, mctx, t->data.lam.domain),
                  meta_zonk(heap, mctx, t->data.lam.body));
  case KT_APP:
    return kt_app(heap, meta_zonk(heap, mctx, t->data.app.fun),
                        meta_zonk(heap, mctx, t->data.app.arg));
  case KT_ID:
    return kt_id(heap, meta_zonk(heap, mctx, t->data.id.type),
                       meta_zonk(heap, mctx, t->data.id.a),
                       meta_zonk(heap, mctx, t->data.id.b));
  case KT_REFL:
    return kt_refl(heap, meta_zonk(heap, mctx, t->data.refl.value));
  default:
    return t;
  }
}

int meta_unsolved_count(meta_ctx_t *mctx) {
  int n = 0;
  meta_entry_t *e;
  for (e = mctx->entries; e != NULL; e = e->next) {
    if (e->solution == NULL) n++;
  }
  return n;
}

void meta_ctx_fprint(FILE *fp, meta_ctx_t *mctx) {
  meta_entry_t *e;
  int total = 0, solved = 0;
  for (e = mctx->entries; e != NULL; e = e->next) {
    total++;
    if (e->solution != NULL) solved++;
  }
  fprintf(fp, "metavars: %d total, %d solved, %d unsolved\n",
          total, solved, total - solved);
  for (e = mctx->entries; e != NULL; e = e->next) {
    fprintf(fp, "  ?%d : ", e->id);
    kt_fprint(fp, e->type);
    if (e->solution != NULL) {
      fprintf(fp, " = ");
      kt_fprint(fp, e->solution);
    }
    fprintf(fp, "\n");
  }
}

/* ---- unification ---- */

/* First-order unification: try to solve metavariables to make
 * two terms definitionally equal. This is the "flex-rigid" pattern:
 * if one side is an unsolved meta, solve it with the other side. */
int kt_unify(lizard_heap_t *heap, kctx_t *ctx, meta_ctx_t *mctx,
             kterm_t *a, kterm_t *b) {
  kterm_t *na, *nb;
  meta_entry_t *e;
  if (a == b) return 1;
  if (a == NULL || b == NULL) return 0;

  /* Resolve solved metas first. */
  if (a->tag == KT_META) {
    e = meta_lookup(mctx, a->data.meta.id);
    if (e != NULL && e->solution != NULL) {
      return kt_unify(heap, ctx, mctx, e->solution, b);
    }
  }
  if (b->tag == KT_META) {
    e = meta_lookup(mctx, b->data.meta.id);
    if (e != NULL && e->solution != NULL) {
      return kt_unify(heap, ctx, mctx, a, e->solution);
    }
  }

  /* Flex-rigid: solve unsolved meta with the other term. */
  if (a->tag == KT_META) {
    e = meta_lookup(mctx, a->data.meta.id);
    if (e != NULL && e->solution == NULL) {
      e->solution = b;
      return 1;
    }
  }
  if (b->tag == KT_META) {
    e = meta_lookup(mctx, b->data.meta.id);
    if (e != NULL && e->solution == NULL) {
      e->solution = a;
      return 1;
    }
  }

  /* Reduce to WHNF and compare structurally. */
  na = kt_whnf(heap, ctx, a);
  nb = kt_whnf(heap, ctx, b);

  /* After WHNF, check for metas again. */
  if (na->tag == KT_META) {
    e = meta_lookup(mctx, na->data.meta.id);
    if (e != NULL && e->solution == NULL) {
      e->solution = nb;
      return 1;
    }
  }
  if (nb->tag == KT_META) {
    e = meta_lookup(mctx, nb->data.meta.id);
    if (e != NULL && e->solution == NULL) {
      e->solution = na;
      return 1;
    }
  }

  if (na->tag != nb->tag) return 0;

  switch (na->tag) {
  case KT_VAR:
    return na->data.var.index == nb->data.var.index;
  case KT_SORT:
    return na->data.sort.level == nb->data.sort.level;
  case KT_NAT: case KT_ZERO:
  case KT_BOOL: case KT_TRUE: case KT_FALSE:
  case KT_UNIT: case KT_STAR:
  case KT_NIL_K:
    return 1;
  case KT_LIST:
    return kt_unify(heap, ctx, mctx, na->data.list.elem_type, nb->data.list.elem_type);
  case KT_CONS_K:
    return kt_unify(heap, ctx, mctx, na->data.cons_k.head, nb->data.cons_k.head) &&
           kt_unify(heap, ctx, mctx, na->data.cons_k.tail, nb->data.cons_k.tail);
  case KT_SUCC:
    return kt_unify(heap, ctx, mctx, na->data.succ.pred, nb->data.succ.pred);
  case KT_PI:
    return kt_unify(heap, ctx, mctx, na->data.pi.domain, nb->data.pi.domain) &&
           kt_unify(heap, ctx, mctx, na->data.pi.codomain, nb->data.pi.codomain);
  case KT_SIGMA:
    return kt_unify(heap, ctx, mctx, na->data.sigma.fst_type, nb->data.sigma.fst_type) &&
           kt_unify(heap, ctx, mctx, na->data.sigma.snd_type, nb->data.sigma.snd_type);
  case KT_LAM:
    return kt_unify(heap, ctx, mctx, na->data.lam.body, nb->data.lam.body);
  case KT_APP:
    return kt_unify(heap, ctx, mctx, na->data.app.fun, nb->data.app.fun) &&
           kt_unify(heap, ctx, mctx, na->data.app.arg, nb->data.app.arg);
  case KT_ID:
    return kt_unify(heap, ctx, mctx, na->data.id.type, nb->data.id.type) &&
           kt_unify(heap, ctx, mctx, na->data.id.a, nb->data.id.a) &&
           kt_unify(heap, ctx, mctx, na->data.id.b, nb->data.id.b);
  case KT_REFL:
    return kt_unify(heap, ctx, mctx, na->data.refl.value, nb->data.refl.value);
  case KT_PAIR:
    return kt_unify(heap, ctx, mctx, na->data.pair.fst, nb->data.pair.fst) &&
           kt_unify(heap, ctx, mctx, na->data.pair.snd, nb->data.pair.snd);
  default:
    return 0;
  }
}
