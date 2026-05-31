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

kterm_t *kt_interval(lizard_heap_t *heap) { return kt_alloc(heap, KT_INTERVAL); }
kterm_t *kt_i0(lizard_heap_t *heap) { return kt_alloc(heap, KT_I0); }
kterm_t *kt_i1(lizard_heap_t *heap) { return kt_alloc(heap, KT_I1); }
kterm_t *kt_path(lizard_heap_t *heap, kterm_t *type, kterm_t *a, kterm_t *b) {
  kterm_t *t = kt_alloc(heap, KT_PATH);
  t->data.path.type = type; t->data.path.a = a; t->data.path.b = b;
  return t;
}
kterm_t *kt_plam(lizard_heap_t *heap, const char *name, kterm_t *body) {
  kterm_t *t = kt_alloc(heap, KT_PLAM);
  t->data.plam.name = name; t->data.plam.body = body;
  return t;
}
kterm_t *kt_papp(lizard_heap_t *heap, kterm_t *path, kterm_t *arg) {
  kterm_t *t = kt_alloc(heap, KT_PAPP);
  t->data.papp.path = path; t->data.papp.arg = arg;
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
  ctx->defs = NULL;
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
  new_ctx->defs = ctx->defs;
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

kterm_t *kt_shift(lizard_heap_t *heap, kterm_t *t, int cutoff, int delta) {
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
  case KT_NOTHING:
  case KT_EMPTY:
  case KT_CONST:
    return t;
  case KT_SUCC:
    return kt_succ(heap, kt_shift(heap, t->data.succ.pred, cutoff, delta));
  case KT_PI: {
    kterm_t *r = kt_pi(heap, t->data.pi.name,
                 kt_shift(heap, t->data.pi.domain, cutoff, delta),
                 kt_shift(heap, t->data.pi.codomain, cutoff + 1, delta));
    r->data.pi.implicit = t->data.pi.implicit;
    return r;
  }
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
  case KT_NAT_REC: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_NAT_REC;
    r->data.nat_rec.motive = kt_shift(heap, t->data.nat_rec.motive, cutoff, delta);
    r->data.nat_rec.zero_case = kt_shift(heap, t->data.nat_rec.zero_case, cutoff, delta);
    r->data.nat_rec.succ_case = kt_shift(heap, t->data.nat_rec.succ_case, cutoff, delta);
    r->data.nat_rec.scrutinee = kt_shift(heap, t->data.nat_rec.scrutinee, cutoff, delta);
    return r;
  }
  case KT_BOOL_REC: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_BOOL_REC;
    r->data.bool_rec.motive = kt_shift(heap, t->data.bool_rec.motive, cutoff, delta);
    r->data.bool_rec.true_case = kt_shift(heap, t->data.bool_rec.true_case, cutoff, delta);
    r->data.bool_rec.false_case = kt_shift(heap, t->data.bool_rec.false_case, cutoff, delta);
    r->data.bool_rec.scrutinee = kt_shift(heap, t->data.bool_rec.scrutinee, cutoff, delta);
    return r;
  }
  case KT_LIST_REC: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_LIST_REC;
    r->data.list_rec.motive = kt_shift(heap, t->data.list_rec.motive, cutoff, delta);
    r->data.list_rec.nil_case = kt_shift(heap, t->data.list_rec.nil_case, cutoff, delta);
    r->data.list_rec.cons_case = kt_shift(heap, t->data.list_rec.cons_case, cutoff, delta);
    r->data.list_rec.scrutinee = kt_shift(heap, t->data.list_rec.scrutinee, cutoff, delta);
    return r;
  }
  case KT_MAYBE_REC: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_MAYBE_REC;
    r->data.maybe_rec.motive = kt_shift(heap, t->data.maybe_rec.motive, cutoff, delta);
    r->data.maybe_rec.nothing_case = kt_shift(heap, t->data.maybe_rec.nothing_case, cutoff, delta);
    r->data.maybe_rec.just_case = kt_shift(heap, t->data.maybe_rec.just_case, cutoff, delta);
    r->data.maybe_rec.scrutinee = kt_shift(heap, t->data.maybe_rec.scrutinee, cutoff, delta);
    return r;
  }
  case KT_SUM_REC: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_SUM_REC;
    r->data.sum_rec.motive = kt_shift(heap, t->data.sum_rec.motive, cutoff, delta);
    r->data.sum_rec.left_case = kt_shift(heap, t->data.sum_rec.left_case, cutoff, delta);
    r->data.sum_rec.right_case = kt_shift(heap, t->data.sum_rec.right_case, cutoff, delta);
    r->data.sum_rec.scrutinee = kt_shift(heap, t->data.sum_rec.scrutinee, cutoff, delta);
    return r;
  }
  case KT_J: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_J;
    r->data.j.motive = kt_shift(heap, t->data.j.motive, cutoff, delta);
    r->data.j.base_case = kt_shift(heap, t->data.j.base_case, cutoff, delta);
    r->data.j.type = kt_shift(heap, t->data.j.type, cutoff, delta);
    r->data.j.a = kt_shift(heap, t->data.j.a, cutoff, delta);
    r->data.j.b = kt_shift(heap, t->data.j.b, cutoff, delta);
    r->data.j.proof = kt_shift(heap, t->data.j.proof, cutoff, delta);
    return r;
  }
  case KT_ANNOT: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_ANNOT;
    r->data.annot.term = kt_shift(heap, t->data.annot.term, cutoff, delta);
    r->data.annot.type = kt_shift(heap, t->data.annot.type, cutoff, delta);
    return r;
  }
  case KT_INL: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_INL;
    r->data.inl.value = kt_shift(heap, t->data.inl.value, cutoff, delta);
    r->data.inl.right_type = kt_shift(heap, t->data.inl.right_type, cutoff, delta);
    return r;
  }
  case KT_INR: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_INR;
    r->data.inr.value = kt_shift(heap, t->data.inr.value, cutoff, delta);
    r->data.inr.left_type = kt_shift(heap, t->data.inr.left_type, cutoff, delta);
    return r;
  }
  case KT_JUST: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_JUST;
    r->data.just.value = kt_shift(heap, t->data.just.value, cutoff, delta);
    return r;
  }
  case KT_MAYBE: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_MAYBE;
    r->data.maybe.elem_type = kt_shift(heap, t->data.maybe.elem_type, cutoff, delta);
    return r;
  }
  case KT_SUM_K: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_SUM_K;
    r->data.sum_k.left_type = kt_shift(heap, t->data.sum_k.left_type, cutoff, delta);
    r->data.sum_k.right_type = kt_shift(heap, t->data.sum_k.right_type, cutoff, delta);
    return r;
  }
  case KT_ABSURD: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_ABSURD;
    r->data.absurd.target_type = kt_shift(heap, t->data.absurd.target_type, cutoff, delta);
    return r;
  }
  case KT_LET: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_LET;
    r->data.let.name  = t->data.let.name;
    r->data.let.value = kt_shift(heap, t->data.let.value, cutoff, delta);
    r->data.let.body  = kt_shift(heap, t->data.let.body, cutoff + 1, delta);
    return r;
  }
  case KT_INTERVAL: case KT_I0: case KT_I1:
    return t;
  case KT_PATH:
    return kt_path(heap, kt_shift(heap, t->data.path.type, cutoff, delta),
                         kt_shift(heap, t->data.path.a, cutoff, delta),
                         kt_shift(heap, t->data.path.b, cutoff, delta));
  case KT_PLAM:
    return kt_plam(heap, t->data.plam.name,
                   kt_shift(heap, t->data.plam.body, cutoff + 1, delta));
  case KT_PAPP:
    return kt_papp(heap, kt_shift(heap, t->data.papp.path, cutoff, delta),
                         kt_shift(heap, t->data.papp.arg, cutoff, delta));
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
  case KT_NOTHING:
  case KT_EMPTY:
  case KT_CONST:
    return t;
  case KT_SUCC:
    return kt_succ(heap, kt_subst(heap, t->data.succ.pred, n, s));
  case KT_PI: {
    kterm_t *r = kt_pi(heap, t->data.pi.name,
                 kt_subst(heap, t->data.pi.domain, n, s),
                 kt_subst(heap, t->data.pi.codomain, n + 1,
                           kt_shift(heap, s, 0, 1)));
    r->data.pi.implicit = t->data.pi.implicit;
    return r;
  }
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
  case KT_NAT_REC: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_NAT_REC;
    r->data.nat_rec.motive = kt_subst(heap, t->data.nat_rec.motive, n, s);
    r->data.nat_rec.zero_case = kt_subst(heap, t->data.nat_rec.zero_case, n, s);
    r->data.nat_rec.succ_case = kt_subst(heap, t->data.nat_rec.succ_case, n, s);
    r->data.nat_rec.scrutinee = kt_subst(heap, t->data.nat_rec.scrutinee, n, s);
    return r;
  }
  case KT_BOOL_REC: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_BOOL_REC;
    r->data.bool_rec.motive = kt_subst(heap, t->data.bool_rec.motive, n, s);
    r->data.bool_rec.true_case = kt_subst(heap, t->data.bool_rec.true_case, n, s);
    r->data.bool_rec.false_case = kt_subst(heap, t->data.bool_rec.false_case, n, s);
    r->data.bool_rec.scrutinee = kt_subst(heap, t->data.bool_rec.scrutinee, n, s);
    return r;
  }
  case KT_LIST_REC: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_LIST_REC;
    r->data.list_rec.motive = kt_subst(heap, t->data.list_rec.motive, n, s);
    r->data.list_rec.nil_case = kt_subst(heap, t->data.list_rec.nil_case, n, s);
    r->data.list_rec.cons_case = kt_subst(heap, t->data.list_rec.cons_case, n, s);
    r->data.list_rec.scrutinee = kt_subst(heap, t->data.list_rec.scrutinee, n, s);
    return r;
  }
  case KT_MAYBE_REC: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_MAYBE_REC;
    r->data.maybe_rec.motive = kt_subst(heap, t->data.maybe_rec.motive, n, s);
    r->data.maybe_rec.nothing_case = kt_subst(heap, t->data.maybe_rec.nothing_case, n, s);
    r->data.maybe_rec.just_case = kt_subst(heap, t->data.maybe_rec.just_case, n, s);
    r->data.maybe_rec.scrutinee = kt_subst(heap, t->data.maybe_rec.scrutinee, n, s);
    return r;
  }
  case KT_SUM_REC: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_SUM_REC;
    r->data.sum_rec.motive = kt_subst(heap, t->data.sum_rec.motive, n, s);
    r->data.sum_rec.left_case = kt_subst(heap, t->data.sum_rec.left_case, n, s);
    r->data.sum_rec.right_case = kt_subst(heap, t->data.sum_rec.right_case, n, s);
    r->data.sum_rec.scrutinee = kt_subst(heap, t->data.sum_rec.scrutinee, n, s);
    return r;
  }
  case KT_J: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_J;
    r->data.j.motive = kt_subst(heap, t->data.j.motive, n, s);
    r->data.j.base_case = kt_subst(heap, t->data.j.base_case, n, s);
    r->data.j.type = kt_subst(heap, t->data.j.type, n, s);
    r->data.j.a = kt_subst(heap, t->data.j.a, n, s);
    r->data.j.b = kt_subst(heap, t->data.j.b, n, s);
    r->data.j.proof = kt_subst(heap, t->data.j.proof, n, s);
    return r;
  }
  case KT_ANNOT: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_ANNOT;
    r->data.annot.term = kt_subst(heap, t->data.annot.term, n, s);
    r->data.annot.type = kt_subst(heap, t->data.annot.type, n, s);
    return r;
  }
  case KT_INL: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_INL;
    r->data.inl.value = kt_subst(heap, t->data.inl.value, n, s);
    r->data.inl.right_type = kt_subst(heap, t->data.inl.right_type, n, s);
    return r;
  }
  case KT_INR: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_INR;
    r->data.inr.value = kt_subst(heap, t->data.inr.value, n, s);
    r->data.inr.left_type = kt_subst(heap, t->data.inr.left_type, n, s);
    return r;
  }
  case KT_JUST: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_JUST;
    r->data.just.value = kt_subst(heap, t->data.just.value, n, s);
    return r;
  }
  case KT_MAYBE: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_MAYBE;
    r->data.maybe.elem_type = kt_subst(heap, t->data.maybe.elem_type, n, s);
    return r;
  }
  case KT_SUM_K: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_SUM_K;
    r->data.sum_k.left_type = kt_subst(heap, t->data.sum_k.left_type, n, s);
    r->data.sum_k.right_type = kt_subst(heap, t->data.sum_k.right_type, n, s);
    return r;
  }
  case KT_ABSURD: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_ABSURD;
    r->data.absurd.target_type = kt_subst(heap, t->data.absurd.target_type, n, s);
    return r;
  }
  case KT_LET: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_LET;
    r->data.let.name  = t->data.let.name;
    r->data.let.value = kt_subst(heap, t->data.let.value, n, s);
    r->data.let.body  = kt_subst(heap, t->data.let.body, n + 1,
                                 kt_shift(heap, s, 0, 1));
    return r;
  }
  case KT_INTERVAL: case KT_I0: case KT_I1:
    return t;
  case KT_PATH:
    return kt_path(heap, kt_subst(heap, t->data.path.type, n, s),
                         kt_subst(heap, t->data.path.a, n, s),
                         kt_subst(heap, t->data.path.b, n, s));
  case KT_PLAM:
    return kt_plam(heap, t->data.plam.name,
                   kt_subst(heap, t->data.plam.body, n + 1,
                            kt_shift(heap, s, 0, 1)));
  case KT_PAPP:
    return kt_papp(heap, kt_subst(heap, t->data.papp.path, n, s),
                         kt_subst(heap, t->data.papp.arg, n, s));
  default:
    return t;
  }
}

/* ---- weak head normal form ---- */

/* forward decl: iota-reduction for user inductives (defined below) */
static kterm_t *try_iota(lizard_heap_t *heap, kctx_t *ctx, kterm_t *t);

kterm_t *kt_whnf(lizard_heap_t *heap, kctx_t *ctx, kterm_t *t) {
  if (t == NULL) return NULL;
  switch (t->tag) {
  case KT_VAR: {
    kctx_entry_t *e = kctx_lookup(ctx, t->data.var.index);
    if (e != NULL && e->value != NULL)
      return kt_whnf(heap, ctx, e->value);
    return t;
  }
  case KT_CONST: {
    /* delta-reduction: a definition (value present) unfolds to its body;
     * an axiom (no value) is opaque. */
    if (ctx != NULL && ctx->defs != NULL) {
      kdef_t *d = kdef_lookup((kdef_ctx_t *)ctx->defs,
                              t->data.constant.name);
      if (d != NULL && d->value != NULL)
        return kt_whnf(heap, ctx, d->value);
    }
    return t;
  }
  case KT_APP: {
    kterm_t *fn;
    kterm_t *iota = try_iota(heap, ctx, t);
    if (iota != NULL) return kt_whnf(heap, ctx, iota);
    fn = kt_whnf(heap, ctx, t->data.app.fun);
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

  case KT_MAYBE_REC: {
    kterm_t *scrut = kt_whnf(heap, ctx, t->data.maybe_rec.scrutinee);
    if (scrut->tag == KT_NOTHING)
      return kt_whnf(heap, ctx, t->data.maybe_rec.nothing_case);
    if (scrut->tag == KT_JUST)
      return kt_whnf(heap, ctx, kt_app(heap, t->data.maybe_rec.just_case, scrut->data.just.value));
    return t;
  }
  case KT_SUM_REC: {
    kterm_t *scrut = kt_whnf(heap, ctx, t->data.sum_rec.scrutinee);
    if (scrut->tag == KT_INL)
      return kt_whnf(heap, ctx, kt_app(heap, t->data.sum_rec.left_case, scrut->data.inl.value));
    if (scrut->tag == KT_INR)
      return kt_whnf(heap, ctx, kt_app(heap, t->data.sum_rec.right_case, scrut->data.inr.value));
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
  case KT_PAPP: {
    kterm_t *p = kt_whnf(heap, ctx, t->data.papp.path);
    kterm_t *r;
    if (p->tag == KT_PLAM)
      return kt_whnf(heap, ctx,
                     kt_subst(heap, p->data.plam.body, 0, t->data.papp.arg));
    r = kt_whnf(heap, ctx, t->data.papp.arg);
    if (r->tag == KT_I0 || r->tag == KT_I1) {
      /* boundary: a path's endpoints are recoverable from its type, so even a
       * neutral path applied to an endpoint reduces to that endpoint. */
      kterm_t *pt = kt_infer(heap, ctx, p);
      if (pt != NULL) {
        kterm_t *wpt = kt_whnf(heap, ctx, pt);
        if (wpt->tag == KT_PATH)
          return kt_whnf(heap, ctx,
                         r->tag == KT_I0 ? wpt->data.path.a : wpt->data.path.b);
      }
    }
    if (p != t->data.papp.path || r != t->data.papp.arg)
      return kt_papp(heap, p, r);
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
  case KT_CONST:
    return strcmp(na->data.constant.name,
                  nb->data.constant.name) == 0;
  case KT_SORT:
    return na->data.sort.level == nb->data.sort.level;
  case KT_NAT: case KT_ZERO:
  case KT_BOOL: case KT_TRUE: case KT_FALSE:
  case KT_UNIT: case KT_STAR:
  case KT_NIL_K:
  case KT_NOTHING:
  case KT_EMPTY:
    return 1;
  case KT_SUM_K:
    return kt_equal(heap, ctx, na->data.sum_k.left_type, nb->data.sum_k.left_type) &&
           kt_equal(heap, ctx, na->data.sum_k.right_type, nb->data.sum_k.right_type);
  case KT_INL:
    return kt_equal(heap, ctx, na->data.inl.value, nb->data.inl.value);
  case KT_INR:
    return kt_equal(heap, ctx, na->data.inr.value, nb->data.inr.value);
  case KT_MAYBE:
    return kt_equal(heap, ctx, na->data.maybe.elem_type, nb->data.maybe.elem_type);
  case KT_JUST:
    return kt_equal(heap, ctx, na->data.just.value, nb->data.just.value);
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
           kt_equal(heap, kctx_extend(heap, ctx, na->data.pi.name,
                                      na->data.pi.domain, NULL),
                    na->data.pi.codomain, nb->data.pi.codomain);
  case KT_LAM:
    return kt_equal(heap, kctx_extend(heap, ctx, na->data.lam.name,
                                      na->data.lam.domain, NULL),
                    na->data.lam.body, nb->data.lam.body);
  case KT_APP:
    return kt_equal(heap, ctx, na->data.app.fun, nb->data.app.fun) &&
           kt_equal(heap, ctx, na->data.app.arg, nb->data.app.arg);
  case KT_SIGMA:
    return kt_equal(heap, ctx, na->data.sigma.fst_type, nb->data.sigma.fst_type) &&
           kt_equal(heap, kctx_extend(heap, ctx, na->data.sigma.name,
                                      na->data.sigma.fst_type, NULL),
                    na->data.sigma.snd_type, nb->data.sigma.snd_type);
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
  case KT_NAT_REC:
    return kt_equal(heap, ctx, na->data.nat_rec.motive, nb->data.nat_rec.motive) &&
           kt_equal(heap, ctx, na->data.nat_rec.zero_case, nb->data.nat_rec.zero_case) &&
           kt_equal(heap, ctx, na->data.nat_rec.succ_case, nb->data.nat_rec.succ_case) &&
           kt_equal(heap, ctx, na->data.nat_rec.scrutinee, nb->data.nat_rec.scrutinee);
  case KT_BOOL_REC:
    return kt_equal(heap, ctx, na->data.bool_rec.motive, nb->data.bool_rec.motive) &&
           kt_equal(heap, ctx, na->data.bool_rec.true_case, nb->data.bool_rec.true_case) &&
           kt_equal(heap, ctx, na->data.bool_rec.false_case, nb->data.bool_rec.false_case) &&
           kt_equal(heap, ctx, na->data.bool_rec.scrutinee, nb->data.bool_rec.scrutinee);
  case KT_LIST_REC:
    return kt_equal(heap, ctx, na->data.list_rec.motive, nb->data.list_rec.motive) &&
           kt_equal(heap, ctx, na->data.list_rec.nil_case, nb->data.list_rec.nil_case) &&
           kt_equal(heap, ctx, na->data.list_rec.cons_case, nb->data.list_rec.cons_case) &&
           kt_equal(heap, ctx, na->data.list_rec.scrutinee, nb->data.list_rec.scrutinee);
  case KT_MAYBE_REC:
    return kt_equal(heap, ctx, na->data.maybe_rec.motive, nb->data.maybe_rec.motive) &&
           kt_equal(heap, ctx, na->data.maybe_rec.nothing_case, nb->data.maybe_rec.nothing_case) &&
           kt_equal(heap, ctx, na->data.maybe_rec.just_case, nb->data.maybe_rec.just_case) &&
           kt_equal(heap, ctx, na->data.maybe_rec.scrutinee, nb->data.maybe_rec.scrutinee);
  case KT_SUM_REC:
    return kt_equal(heap, ctx, na->data.sum_rec.motive, nb->data.sum_rec.motive) &&
           kt_equal(heap, ctx, na->data.sum_rec.left_case, nb->data.sum_rec.left_case) &&
           kt_equal(heap, ctx, na->data.sum_rec.right_case, nb->data.sum_rec.right_case) &&
           kt_equal(heap, ctx, na->data.sum_rec.scrutinee, nb->data.sum_rec.scrutinee);
  case KT_J:
    return kt_equal(heap, ctx, na->data.j.motive, nb->data.j.motive) &&
           kt_equal(heap, ctx, na->data.j.base_case, nb->data.j.base_case) &&
           kt_equal(heap, ctx, na->data.j.a, nb->data.j.a) &&
           kt_equal(heap, ctx, na->data.j.b, nb->data.j.b) &&
           kt_equal(heap, ctx, na->data.j.proof, nb->data.j.proof);
  case KT_ABSURD:
    return kt_equal(heap, ctx, na->data.absurd.target_type,
                    nb->data.absurd.target_type);
  case KT_INTERVAL: case KT_I0: case KT_I1:
    return 1;
  case KT_PATH:
    return kt_equal(heap, ctx, na->data.path.type, nb->data.path.type)
        && kt_equal(heap, ctx, na->data.path.a, nb->data.path.a)
        && kt_equal(heap, ctx, na->data.path.b, nb->data.path.b);
  case KT_PLAM: {
    kctx_t *e = kctx_extend(heap, ctx, na->data.plam.name, kt_interval(heap), NULL);
    return kt_equal(heap, e, na->data.plam.body, nb->data.plam.body);
  }
  case KT_PAPP:
    return kt_equal(heap, ctx, na->data.papp.path, nb->data.papp.path)
        && kt_equal(heap, ctx, na->data.papp.arg, nb->data.papp.arg);
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
    if (e == NULL) return NULL;
    /* The entry's type was recorded at its binding site; lift it past the
     * binders crossed to reach this occurrence (de Bruijn shift). */
    return kt_shift(heap, e->type, 0, t->data.var.index + 1);
  }
  case KT_CONST: {
    kdef_t *d;
    if (ctx->defs == NULL) return NULL;
    d = kdef_lookup((kdef_ctx_t *)ctx->defs,
                    t->data.constant.name);
    return d ? d->type : NULL;
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
    kterm_t *dom_type = kt_infer(heap, ctx, t->data.lam.domain);
    kctx_t *ext;
    kterm_t *body_type;
    /* The domain annotation must itself be a type (its type is a sort),
     * exactly as required for Pi.  Without this check a lambda whose domain
     * is a value (e.g. lambda (x : 0). x) would be accepted, yielding a Pi
     * with a non-type domain — a soundness hole. */
    if (dom_type == NULL) return NULL;
    if (dom_type->tag != KT_SORT) return NULL;
    ext = kctx_extend(heap, ctx, t->data.lam.name, t->data.lam.domain, NULL);
    body_type = kt_infer(heap, ext, t->data.lam.body);
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
    /* both endpoints must inhabit the stated type; without this,
     * Id Nat true false would be accepted as a type. */
    if (kt_check(heap, ctx, t->data.id.a, t->data.id.type) != KERNEL_OK)
      return NULL;
    if (kt_check(heap, ctx, t->data.id.b, t->data.id.type) != KERNEL_OK)
      return NULL;
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
    /* nil {A} : List A — the element annotation must be a type. */
    kterm_t *et;
    kterm_t *l;
    if (t->data.nil_k.elem_type == NULL) return NULL;
    et = kt_infer(heap, ctx, t->data.nil_k.elem_type);
    if (et == NULL) return NULL;
    et = kt_whnf(heap, ctx, et);
    if (et->tag != KT_SORT) return NULL;
    l = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(l, 0, sizeof(*l));
    l->tag = KT_LIST;
    l->data.list.elem_type = t->data.nil_k.elem_type;
    return l;
  }
  case KT_CONS_K: {
    kterm_t *ht = kt_infer(heap, ctx, t->data.cons_k.head);
    kterm_t *tt;
    kterm_t *l;
    if (ht == NULL) return NULL;
    /* the tail must be a List whose element type matches the head's type;
     * without this check (cons 0 true) would type as List Nat. */
    tt = kt_infer(heap, ctx, t->data.cons_k.tail);
    if (tt == NULL) return NULL;
    tt = kt_whnf(heap, ctx, tt);
    if (tt->tag != KT_LIST) return NULL;
    if (!kt_equal(heap, ctx, tt->data.list.elem_type, ht)) return NULL;
    l = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(l, 0, sizeof(*l));
    l->tag = KT_LIST;
    l->data.list.elem_type = ht;
    return l;
  }
  case KT_MAYBE: {
    kterm_t *et = kt_infer(heap, ctx, t->data.maybe.elem_type);
    if (et == NULL) return NULL;
    et = kt_whnf(heap, ctx, et);
    if (et->tag != KT_SORT) return NULL;
    return kt_sort(heap, et->data.sort.level);
  }
  case KT_NOTHING: {
    /* nothing needs type annotation */
    return NULL;
  }
  case KT_JUST: {
    kterm_t *vt = kt_infer(heap, ctx, t->data.just.value);
    kterm_t *m;
    if (vt == NULL) return NULL;
    m = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(m, 0, sizeof(*m));
    m->tag = KT_MAYBE;
    m->data.maybe.elem_type = vt;
    return m;
  }
  case KT_EMPTY:
    return kt_sort(heap, 0);
  case KT_SUM_K: {
    kterm_t *lt = kt_infer(heap, ctx, t->data.sum_k.left_type);
    kterm_t *rt;
    int l, r;
    if (lt == NULL) return NULL;
    lt = kt_whnf(heap, ctx, lt);
    if (lt->tag != KT_SORT) return NULL;
    rt = kt_infer(heap, ctx, t->data.sum_k.right_type);
    if (rt == NULL) return NULL;
    rt = kt_whnf(heap, ctx, rt);
    if (rt->tag != KT_SORT) return NULL;
    l = lt->data.sort.level;
    r = rt->data.sort.level;
    return kt_sort(heap, l > r ? l : r);
  }
  case KT_INL: {
    kterm_t *vt = kt_infer(heap, ctx, t->data.inl.value);
    kterm_t *rt;
    kterm_t *s;
    if (vt == NULL) return NULL;
    /* the right summand annotation must itself be a type. */
    if (t->data.inl.right_type == NULL) return NULL;
    rt = kt_infer(heap, ctx, t->data.inl.right_type);
    if (rt == NULL) return NULL;
    rt = kt_whnf(heap, ctx, rt);
    if (rt->tag != KT_SORT) return NULL;
    s = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(s, 0, sizeof(*s));
    s->tag = KT_SUM_K;
    s->data.sum_k.left_type = vt;
    s->data.sum_k.right_type = t->data.inl.right_type;
    return s;
  }
  case KT_INR: {
    kterm_t *vt = kt_infer(heap, ctx, t->data.inr.value);
    kterm_t *lt;
    kterm_t *s;
    if (vt == NULL) return NULL;
    /* the left summand annotation must itself be a type. */
    if (t->data.inr.left_type == NULL) return NULL;
    lt = kt_infer(heap, ctx, t->data.inr.left_type);
    if (lt == NULL) return NULL;
    lt = kt_whnf(heap, ctx, lt);
    if (lt->tag != KT_SORT) return NULL;
    s = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(s, 0, sizeof(*s));
    s->tag = KT_SUM_K;
    s->data.sum_k.left_type = t->data.inr.left_type;
    s->data.sum_k.right_type = vt;
    return s;
  }
  case KT_ANNOT: {
    kernel_result_t r = kt_check(heap, ctx, t->data.annot.term,
                                  t->data.annot.type);
    return (r == KERNEL_OK) ? t->data.annot.type : NULL;
  }
  case KT_ABSURD: {
    /* absurd {A} : Empty -> A.  The target annotation A must be a type;
     * the result is the function type Empty -> A. */
    kterm_t *at = kt_infer(heap, ctx, t->data.absurd.target_type);
    if (at == NULL) return NULL;
    at = kt_whnf(heap, ctx, at);
    if (at->tag != KT_SORT) return NULL;
    return kt_pi(heap, "_", kt_alloc(heap, KT_EMPTY),
                 kt_shift(heap, t->data.absurd.target_type, 0, 1));
  }
  case KT_BOOL_REC: {
    /* bool_rec C t_case f_case b : C b, with
     *   C : Bool -> Sort, t_case : C true, f_case : C false, b : Bool. */
    kterm_t *motive = t->data.bool_rec.motive;
    kterm_t *st, *res, *rt;
    if (motive == NULL) return NULL;
    st = kt_infer(heap, ctx, t->data.bool_rec.scrutinee);
    if (st == NULL) return NULL;
    if (kt_whnf(heap, ctx, st)->tag != KT_BOOL) return NULL;
    res = kt_app(heap, motive, t->data.bool_rec.scrutinee);
    rt = kt_infer(heap, ctx, res); /* validates motive applies to Bool -> type */
    if (rt == NULL || kt_whnf(heap, ctx, rt)->tag != KT_SORT) return NULL;
    if (kt_check(heap, ctx, t->data.bool_rec.true_case,
                 kt_app(heap, motive, kt_alloc(heap, KT_TRUE))) != KERNEL_OK)
      return NULL;
    if (kt_check(heap, ctx, t->data.bool_rec.false_case,
                 kt_app(heap, motive, kt_alloc(heap, KT_FALSE))) != KERNEL_OK)
      return NULL;
    return res;
  }
  case KT_NAT_REC: {
    /* nat_rec C z s n : C n, with
     *   C : Nat -> Sort, z : C 0,
     *   s : Pi(k:Nat). Pi(_:C k). C (succ k), n : Nat. */
    kterm_t *motive = t->data.nat_rec.motive;
    kterm_t *st, *res, *rt, *exp_succ, *ih_dom, *succ_body;
    if (motive == NULL) return NULL;
    st = kt_infer(heap, ctx, t->data.nat_rec.scrutinee);
    if (st == NULL) return NULL;
    if (kt_whnf(heap, ctx, st)->tag != KT_NAT) return NULL;
    res = kt_app(heap, motive, t->data.nat_rec.scrutinee);
    rt = kt_infer(heap, ctx, res);
    if (rt == NULL || kt_whnf(heap, ctx, rt)->tag != KT_SORT) return NULL;
    /* zero_case : C 0 */
    if (kt_check(heap, ctx, t->data.nat_rec.zero_case,
                 kt_app(heap, motive, kt_zero(heap))) != KERNEL_OK)
      return NULL;
    /* succ_case : Pi(k:Nat). Pi(_ : C k). C (succ k)
     *   under k (var0): inner Pi domain = (shift C 1) k
     *   under k, ih (k=var1): body = (shift C 2) (succ k) */
    ih_dom = kt_app(heap, kt_shift(heap, motive, 0, 1), kt_var(heap, 0));
    succ_body = kt_app(heap, kt_shift(heap, motive, 0, 2),
                       kt_succ(heap, kt_var(heap, 1)));
    exp_succ = kt_pi(heap, "k", kt_nat(heap),
                     kt_pi(heap, "_", ih_dom, succ_body));
    if (kt_check(heap, ctx, t->data.nat_rec.succ_case, exp_succ) != KERNEL_OK)
      return NULL;
    return res;
  }
  case KT_MAYBE_REC: {
    /* maybe_rec C n_case j_case m : C m, with
     *   C : Maybe A -> Sort, n_case : C nothing,
     *   j_case : Pi(a:A). C (just a), m : Maybe A. */
    kterm_t *motive = t->data.maybe_rec.motive;
    kterm_t *st, *A, *res, *rt, *just_app, *exp_just;
    if (motive == NULL) return NULL;
    st = kt_infer(heap, ctx, t->data.maybe_rec.scrutinee);
    if (st == NULL) return NULL;
    st = kt_whnf(heap, ctx, st);
    if (st->tag != KT_MAYBE) return NULL;
    A = st->data.maybe.elem_type;
    res = kt_app(heap, motive, t->data.maybe_rec.scrutinee);
    rt = kt_infer(heap, ctx, res);
    if (rt == NULL || kt_whnf(heap, ctx, rt)->tag != KT_SORT) return NULL;
    if (kt_check(heap, ctx, t->data.maybe_rec.nothing_case,
                 kt_app(heap, motive, kt_alloc(heap, KT_NOTHING))) != KERNEL_OK)
      return NULL;
    just_app = kt_alloc(heap, KT_JUST);
    just_app->data.just.value = kt_var(heap, 0);
    exp_just = kt_pi(heap, "a", A,
                     kt_app(heap, kt_shift(heap, motive, 0, 1), just_app));
    if (kt_check(heap, ctx, t->data.maybe_rec.just_case, exp_just) != KERNEL_OK)
      return NULL;
    return res;
  }
  case KT_SUM_REC: {
    /* sum_rec C l_case r_case s : C s, with
     *   C : Sum A B -> Sort,
     *   l_case : Pi(a:A). C (inl a), r_case : Pi(b:B). C (inr b). */
    kterm_t *motive = t->data.sum_rec.motive;
    kterm_t *st, *A, *B, *res, *rt, *inl_app, *inr_app, *exp_left, *exp_right;
    if (motive == NULL) return NULL;
    st = kt_infer(heap, ctx, t->data.sum_rec.scrutinee);
    if (st == NULL) return NULL;
    st = kt_whnf(heap, ctx, st);
    if (st->tag != KT_SUM_K) return NULL;
    A = st->data.sum_k.left_type;
    B = st->data.sum_k.right_type;
    res = kt_app(heap, motive, t->data.sum_rec.scrutinee);
    rt = kt_infer(heap, ctx, res);
    if (rt == NULL || kt_whnf(heap, ctx, rt)->tag != KT_SORT) return NULL;
    inl_app = kt_alloc(heap, KT_INL);
    inl_app->data.inl.value = kt_var(heap, 0);
    inl_app->data.inl.right_type = kt_shift(heap, B, 0, 1);
    exp_left = kt_pi(heap, "a", A,
                     kt_app(heap, kt_shift(heap, motive, 0, 1), inl_app));
    if (kt_check(heap, ctx, t->data.sum_rec.left_case, exp_left) != KERNEL_OK)
      return NULL;
    inr_app = kt_alloc(heap, KT_INR);
    inr_app->data.inr.value = kt_var(heap, 0);
    inr_app->data.inr.left_type = kt_shift(heap, A, 0, 1);
    exp_right = kt_pi(heap, "b", B,
                      kt_app(heap, kt_shift(heap, motive, 0, 1), inr_app));
    if (kt_check(heap, ctx, t->data.sum_rec.right_case, exp_right) != KERNEL_OK)
      return NULL;
    return res;
  }
  case KT_LIST_REC: {
    /* list_rec C n_case c_case xs : C xs, with
     *   C : List A -> Sort, n_case : C nil,
     *   c_case : Pi(h:A). Pi(t:List A). Pi(_:C t). C (cons h t). */
    kterm_t *motive = t->data.list_rec.motive;
    kterm_t *st, *A, *res, *rt, *nil_t, *list_A1, *ih_dom, *cons_app,
        *cons_body, *exp_cons;
    if (motive == NULL) return NULL;
    st = kt_infer(heap, ctx, t->data.list_rec.scrutinee);
    if (st == NULL) return NULL;
    st = kt_whnf(heap, ctx, st);
    if (st->tag != KT_LIST) return NULL;
    A = st->data.list.elem_type;
    res = kt_app(heap, motive, t->data.list_rec.scrutinee);
    rt = kt_infer(heap, ctx, res);
    if (rt == NULL || kt_whnf(heap, ctx, rt)->tag != KT_SORT) return NULL;
    nil_t = kt_alloc(heap, KT_NIL_K);
    nil_t->data.nil_k.elem_type = A;
    if (kt_check(heap, ctx, t->data.list_rec.nil_case,
                 kt_app(heap, motive, nil_t)) != KERNEL_OK)
      return NULL;
    /* binders: h (var2), t (var1), ih (var0) in the innermost body. */
    list_A1 = kt_alloc(heap, KT_LIST);
    list_A1->data.list.elem_type = kt_shift(heap, A, 0, 1); /* List A under h */
    /* ih domain is under h,t: there t is the most recent binder (var0). */
    ih_dom = kt_app(heap, kt_shift(heap, motive, 0, 2), kt_var(heap, 0));
    cons_app = kt_alloc(heap, KT_CONS_K);
    cons_app->data.cons_k.head = kt_var(heap, 2);
    cons_app->data.cons_k.tail = kt_var(heap, 1);
    cons_body = kt_app(heap, kt_shift(heap, motive, 0, 3), cons_app);
    exp_cons = kt_pi(heap, "h", A,
                     kt_pi(heap, "t", list_A1,
                           kt_pi(heap, "_", ih_dom, cons_body)));
    if (kt_check(heap, ctx, t->data.list_rec.cons_case, exp_cons) != KERNEL_OK)
      return NULL;
    return res;
  }
  case KT_J: {
    /* J C d A a b proof : C a b proof, with
     *   C : Pi(x:A). Pi(y:A). Pi(_:Id A x y). Sort
     *   d : Pi(x:A). C x x (refl x)
     *   a,b : A ; proof : Id A a b.
     * Reduction: J C d A a a (refl a) -> d a, so d is a function of one
     * argument and the motive ranges over both endpoints and the path. */
    kterm_t *motive = t->data.j.motive;
    kterm_t *A = t->data.j.type;
    kterm_t *a = t->data.j.a;
    kterm_t *b = t->data.j.b;
    kterm_t *proof = t->data.j.proof;
    kterm_t *At, *res, *rt, *cxx, *exp_d, *sC;
    if (motive == NULL || A == NULL) return NULL;
    At = kt_infer(heap, ctx, A);
    if (At == NULL || kt_whnf(heap, ctx, At)->tag != KT_SORT) return NULL;
    if (kt_check(heap, ctx, a, A) != KERNEL_OK) return NULL;
    if (kt_check(heap, ctx, b, A) != KERNEL_OK) return NULL;
    if (kt_check(heap, ctx, proof, kt_id(heap, A, a, b)) != KERNEL_OK)
      return NULL;
    /* result type = C a b proof; require it to be a type. */
    res = kt_app(heap, kt_app(heap, kt_app(heap, motive, a), b), proof);
    rt = kt_infer(heap, ctx, res);
    if (rt == NULL || kt_whnf(heap, ctx, rt)->tag != KT_SORT) return NULL;
    /* base_case : Pi(x:A). C x x (refl x) — under x (var0). */
    sC = kt_shift(heap, motive, 0, 1);
    cxx = kt_app(heap,
                 kt_app(heap, kt_app(heap, sC, kt_var(heap, 0)),
                        kt_var(heap, 0)),
                 kt_refl(heap, kt_var(heap, 0)));
    exp_d = kt_pi(heap, "x", A, cxx);
    if (kt_check(heap, ctx, t->data.j.base_case, exp_d) != KERNEL_OK)
      return NULL;
    return res;
  }
  case KT_LET: {
    /* (let x = value in body) is definitional: type the body with x bound to
     * value's inferred type, then substitute the value back into the body's
     * type so a dependent body type remains correct in the outer context.
     * This agrees with the reduction rule (KT_LET in kt_whnf), which
     * substitutes the value into the body. */
    kterm_t *vty = kt_infer(heap, ctx, t->data.let.value);
    kterm_t *bty;
    kctx_t *ext;
    if (vty == NULL) return NULL;
    ext = kctx_extend(heap, ctx, t->data.let.name, vty, NULL);
    bty = kt_infer(heap, ext, t->data.let.body);
    if (bty == NULL) return NULL;
    return kt_subst(heap, bty, 0, t->data.let.value);
  }
  case KT_INTERVAL:
    return kt_sort(heap, 0);
  case KT_I0: case KT_I1:
    return kt_interval(heap);
  case KT_PATH: {
    kterm_t *st = kt_infer(heap, ctx, t->data.path.type);
    if (st == NULL || kt_whnf(heap, ctx, st)->tag != KT_SORT) return NULL;
    if (kt_check(heap, ctx, t->data.path.a, t->data.path.type) != KERNEL_OK) return NULL;
    if (kt_check(heap, ctx, t->data.path.b, t->data.path.type) != KERNEL_OK) return NULL;
    return st;
  }
  case KT_PLAM: {
    kctx_t *e = kctx_extend(heap, ctx, t->data.plam.name, kt_interval(heap), NULL);
    kterm_t *bt = kt_infer(heap, e, t->data.plam.body);
    kterm_t *A, *a, *b, *path;
    if (bt == NULL) return NULL;
    A = kt_subst(heap, bt, 0, kt_i0(heap));
    a = kt_subst(heap, t->data.plam.body, 0, kt_i0(heap));
    b = kt_subst(heap, t->data.plam.body, 0, kt_i1(heap));
    path = kt_path(heap, A, a, b);
    if (kt_infer(heap, ctx, path) == NULL) return NULL; /* rejects dependent paths */
    return path;
  }
  case KT_PAPP: {
    kterm_t *pt = kt_infer(heap, ctx, t->data.papp.path);
    kterm_t *wpt;
    if (pt == NULL) return NULL;
    wpt = kt_whnf(heap, ctx, pt);
    if (wpt->tag != KT_PATH) return NULL;
    if (kt_check(heap, ctx, t->data.papp.arg, kt_interval(heap)) != KERNEL_OK) return NULL;
    return wpt->data.path.type;
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
  case KT_CONST: fprintf(fp, "%s", t->data.constant.name); break;
  case KT_SORT: fprintf(fp, "Sort(%d)", t->data.sort.level); break;
  case KT_NAT: fprintf(fp, "Nat"); break;
  case KT_ZERO: fprintf(fp, "0"); break;
  case KT_SUCC:
    fprintf(fp, "(succ "); kt_fprint(fp, t->data.succ.pred); fprintf(fp, ")");
    break;
  case KT_PI:
    if (t->data.pi.implicit)
      fprintf(fp, "(Pi {%s : ", t->data.pi.name ? t->data.pi.name : "_");
    else
      fprintf(fp, "(Pi (%s : ", t->data.pi.name ? t->data.pi.name : "_");
    kt_fprint(fp, t->data.pi.domain);
    fprintf(fp, t->data.pi.implicit ? "} " : ") ");
    kt_fprint(fp, t->data.pi.codomain); fprintf(fp, ")");
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

  case KT_MAYBE:
    fprintf(fp, "(Maybe "); kt_fprint(fp, t->data.maybe.elem_type); fprintf(fp, ")");
    break;
  case KT_NOTHING:
    fprintf(fp, "nothing"); break;
  case KT_JUST:
    fprintf(fp, "(just "); kt_fprint(fp, t->data.just.value); fprintf(fp, ")");
    break;
  case KT_MAYBE_REC:
    fprintf(fp, "(maybe-rec ...)"); break;

  case KT_EMPTY:
    fprintf(fp, "Empty"); break;
  case KT_ABSURD:
    fprintf(fp, "(absurd ...)"); break;
  case KT_SUM_K:
    fprintf(fp, "(Sum "); kt_fprint(fp, t->data.sum_k.left_type);
    fprintf(fp, " "); kt_fprint(fp, t->data.sum_k.right_type);
    fprintf(fp, ")"); break;
  case KT_INL:
    fprintf(fp, "(inl "); kt_fprint(fp, t->data.inl.value); fprintf(fp, ")");
    break;
  case KT_INR:
    fprintf(fp, "(inr "); kt_fprint(fp, t->data.inr.value); fprintf(fp, ")");
    break;
  case KT_SUM_REC:
    fprintf(fp, "(case ...)"); break;
  case KT_INTERVAL: fprintf(fp, "I"); break;
  case KT_I0: fprintf(fp, "i0"); break;
  case KT_I1: fprintf(fp, "i1"); break;
  case KT_PATH:
    fprintf(fp, "(Path "); kt_fprint(fp, t->data.path.type);
    fprintf(fp, " "); kt_fprint(fp, t->data.path.a);
    fprintf(fp, " "); kt_fprint(fp, t->data.path.b); fprintf(fp, ")");
    break;
  case KT_PLAM:
    fprintf(fp, "(plam %s ", t->data.plam.name ? t->data.plam.name : "i");
    kt_fprint(fp, t->data.plam.body); fprintf(fp, ")");
    break;
  case KT_PAPP:
    fprintf(fp, "("); kt_fprint(fp, t->data.papp.path);
    fprintf(fp, " @ "); kt_fprint(fp, t->data.papp.arg); fprintf(fp, ")");
    break;
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
  case KT_NOTHING: case KT_NIL_K:
    return t;
  case KT_SUCC:
    return kt_succ(heap, meta_zonk(heap, mctx, t->data.succ.pred));
  case KT_PI: {
    kterm_t *r = kt_pi(heap, t->data.pi.name,
                 meta_zonk(heap, mctx, t->data.pi.domain),
                 meta_zonk(heap, mctx, t->data.pi.codomain));
    r->data.pi.implicit = t->data.pi.implicit;
    return r;
  }
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
  case KT_PROJ1:
    return kt_proj1(heap, meta_zonk(heap, mctx, t->data.proj.target));
  case KT_PROJ2:
    return kt_proj2(heap, meta_zonk(heap, mctx, t->data.proj.target));
  case KT_EMPTY:
    return t;
  case KT_NAT_REC: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_NAT_REC;
    r->data.nat_rec.motive = meta_zonk(heap, mctx, t->data.nat_rec.motive);
    r->data.nat_rec.zero_case = meta_zonk(heap, mctx, t->data.nat_rec.zero_case);
    r->data.nat_rec.succ_case = meta_zonk(heap, mctx, t->data.nat_rec.succ_case);
    r->data.nat_rec.scrutinee = meta_zonk(heap, mctx, t->data.nat_rec.scrutinee);
    return r;
  }
  case KT_BOOL_REC: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_BOOL_REC;
    r->data.bool_rec.motive = meta_zonk(heap, mctx, t->data.bool_rec.motive);
    r->data.bool_rec.true_case = meta_zonk(heap, mctx, t->data.bool_rec.true_case);
    r->data.bool_rec.false_case = meta_zonk(heap, mctx, t->data.bool_rec.false_case);
    r->data.bool_rec.scrutinee = meta_zonk(heap, mctx, t->data.bool_rec.scrutinee);
    return r;
  }
  case KT_LIST_REC: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_LIST_REC;
    r->data.list_rec.motive = meta_zonk(heap, mctx, t->data.list_rec.motive);
    r->data.list_rec.nil_case = meta_zonk(heap, mctx, t->data.list_rec.nil_case);
    r->data.list_rec.cons_case = meta_zonk(heap, mctx, t->data.list_rec.cons_case);
    r->data.list_rec.scrutinee = meta_zonk(heap, mctx, t->data.list_rec.scrutinee);
    return r;
  }
  case KT_MAYBE_REC: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_MAYBE_REC;
    r->data.maybe_rec.motive = meta_zonk(heap, mctx, t->data.maybe_rec.motive);
    r->data.maybe_rec.nothing_case = meta_zonk(heap, mctx, t->data.maybe_rec.nothing_case);
    r->data.maybe_rec.just_case = meta_zonk(heap, mctx, t->data.maybe_rec.just_case);
    r->data.maybe_rec.scrutinee = meta_zonk(heap, mctx, t->data.maybe_rec.scrutinee);
    return r;
  }
  case KT_SUM_REC: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_SUM_REC;
    r->data.sum_rec.motive = meta_zonk(heap, mctx, t->data.sum_rec.motive);
    r->data.sum_rec.left_case = meta_zonk(heap, mctx, t->data.sum_rec.left_case);
    r->data.sum_rec.right_case = meta_zonk(heap, mctx, t->data.sum_rec.right_case);
    r->data.sum_rec.scrutinee = meta_zonk(heap, mctx, t->data.sum_rec.scrutinee);
    return r;
  }
  case KT_J: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_J;
    r->data.j.motive = meta_zonk(heap, mctx, t->data.j.motive);
    r->data.j.base_case = meta_zonk(heap, mctx, t->data.j.base_case);
    r->data.j.type = meta_zonk(heap, mctx, t->data.j.type);
    r->data.j.a = meta_zonk(heap, mctx, t->data.j.a);
    r->data.j.b = meta_zonk(heap, mctx, t->data.j.b);
    r->data.j.proof = meta_zonk(heap, mctx, t->data.j.proof);
    return r;
  }
  case KT_SIGMA: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_SIGMA;
    r->data.sigma.name = t->data.sigma.name;
    r->data.sigma.fst_type = meta_zonk(heap, mctx, t->data.sigma.fst_type);
    r->data.sigma.snd_type = meta_zonk(heap, mctx, t->data.sigma.snd_type);
    return r;
  }
  case KT_PAIR: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_PAIR;
    r->data.pair.fst = meta_zonk(heap, mctx, t->data.pair.fst);
    r->data.pair.snd = meta_zonk(heap, mctx, t->data.pair.snd);
    return r;
  }
  case KT_ANNOT: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_ANNOT;
    r->data.annot.term = meta_zonk(heap, mctx, t->data.annot.term);
    r->data.annot.type = meta_zonk(heap, mctx, t->data.annot.type);
    return r;
  }
  case KT_INL: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_INL;
    r->data.inl.value = meta_zonk(heap, mctx, t->data.inl.value);
    r->data.inl.right_type = meta_zonk(heap, mctx, t->data.inl.right_type);
    return r;
  }
  case KT_INR: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_INR;
    r->data.inr.value = meta_zonk(heap, mctx, t->data.inr.value);
    r->data.inr.left_type = meta_zonk(heap, mctx, t->data.inr.left_type);
    return r;
  }
  case KT_JUST: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_JUST;
    r->data.just.value = meta_zonk(heap, mctx, t->data.just.value);
    return r;
  }
  case KT_MAYBE: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_MAYBE;
    r->data.maybe.elem_type = meta_zonk(heap, mctx, t->data.maybe.elem_type);
    return r;
  }
  case KT_SUM_K: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_SUM_K;
    r->data.sum_k.left_type = meta_zonk(heap, mctx, t->data.sum_k.left_type);
    r->data.sum_k.right_type = meta_zonk(heap, mctx, t->data.sum_k.right_type);
    return r;
  }
  case KT_ABSURD: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_ABSURD;
    r->data.absurd.target_type = meta_zonk(heap, mctx, t->data.absurd.target_type);
    return r;
  }
  case KT_LIST: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_LIST;
    r->data.list.elem_type = meta_zonk(heap, mctx, t->data.list.elem_type);
    return r;
  }
  case KT_CONS_K: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_CONS_K;
    r->data.cons_k.head = meta_zonk(heap, mctx, t->data.cons_k.head);
    r->data.cons_k.tail = meta_zonk(heap, mctx, t->data.cons_k.tail);
    return r;
  }
  case KT_LET: {
    kterm_t *r = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(r, 0, sizeof(*r));
    r->tag = KT_LET;
    r->data.let.name = t->data.let.name;
    r->data.let.value = meta_zonk(heap, mctx, t->data.let.value);
    r->data.let.body = meta_zonk(heap, mctx, t->data.let.body);
    return r;
  }
  case KT_INTERVAL: case KT_I0: case KT_I1:
    return t;
  case KT_PATH:
    return kt_path(heap, meta_zonk(heap, mctx, t->data.path.type),
                         meta_zonk(heap, mctx, t->data.path.a),
                         meta_zonk(heap, mctx, t->data.path.b));
  case KT_PLAM:
    return kt_plam(heap, t->data.plam.name,
                   meta_zonk(heap, mctx, t->data.plam.body));
  case KT_PAPP:
    return kt_papp(heap, meta_zonk(heap, mctx, t->data.papp.path),
                         meta_zonk(heap, mctx, t->data.papp.arg));
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
  case KT_CONST:
    return strcmp(na->data.constant.name, nb->data.constant.name) == 0;
  case KT_SORT:
    return na->data.sort.level == nb->data.sort.level;
  case KT_NAT: case KT_ZERO:
  case KT_BOOL: case KT_TRUE: case KT_FALSE:
  case KT_UNIT: case KT_STAR:
  case KT_NIL_K:
  case KT_NOTHING:
  case KT_EMPTY:
    return 1;
  case KT_SUM_K:
    return kt_unify(heap, ctx, mctx, na->data.sum_k.left_type, nb->data.sum_k.left_type) &&
           kt_unify(heap, ctx, mctx, na->data.sum_k.right_type, nb->data.sum_k.right_type);
  case KT_INL:
    return kt_unify(heap, ctx, mctx, na->data.inl.value, nb->data.inl.value);
  case KT_INR:
    return kt_unify(heap, ctx, mctx, na->data.inr.value, nb->data.inr.value);
  case KT_MAYBE:
    return kt_unify(heap, ctx, mctx, na->data.maybe.elem_type, nb->data.maybe.elem_type);
  case KT_JUST:
    return kt_unify(heap, ctx, mctx, na->data.just.value, nb->data.just.value);
  case KT_LIST:
    return kt_unify(heap, ctx, mctx, na->data.list.elem_type, nb->data.list.elem_type);
  case KT_CONS_K:
    return kt_unify(heap, ctx, mctx, na->data.cons_k.head, nb->data.cons_k.head) &&
           kt_unify(heap, ctx, mctx, na->data.cons_k.tail, nb->data.cons_k.tail);
  case KT_SUCC:
    return kt_unify(heap, ctx, mctx, na->data.succ.pred, nb->data.succ.pred);
  case KT_PI:
    return kt_unify(heap, ctx, mctx, na->data.pi.domain, nb->data.pi.domain) &&
           kt_unify(heap, kctx_extend(heap, ctx, na->data.pi.name,
                                      na->data.pi.domain, NULL),
                    mctx, na->data.pi.codomain, nb->data.pi.codomain);
  case KT_SIGMA:
    return kt_unify(heap, ctx, mctx, na->data.sigma.fst_type, nb->data.sigma.fst_type) &&
           kt_unify(heap, kctx_extend(heap, ctx, na->data.sigma.name,
                                      na->data.sigma.fst_type, NULL),
                    mctx, na->data.sigma.snd_type, nb->data.sigma.snd_type);
  case KT_LAM:
    return kt_unify(heap, kctx_extend(heap, ctx, na->data.lam.name,
                                      na->data.lam.domain, NULL),
                    mctx, na->data.lam.body, nb->data.lam.body);
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
  case KT_PROJ1:
    return kt_unify(heap, ctx, mctx, na->data.proj.target, nb->data.proj.target);
  case KT_PROJ2:
    return kt_unify(heap, ctx, mctx, na->data.proj.target, nb->data.proj.target);
  case KT_NAT_REC:
    return kt_unify(heap, ctx, mctx, na->data.nat_rec.motive, nb->data.nat_rec.motive) &&
           kt_unify(heap, ctx, mctx, na->data.nat_rec.zero_case, nb->data.nat_rec.zero_case) &&
           kt_unify(heap, ctx, mctx, na->data.nat_rec.succ_case, nb->data.nat_rec.succ_case) &&
           kt_unify(heap, ctx, mctx, na->data.nat_rec.scrutinee, nb->data.nat_rec.scrutinee);
  case KT_BOOL_REC:
    return kt_unify(heap, ctx, mctx, na->data.bool_rec.motive, nb->data.bool_rec.motive) &&
           kt_unify(heap, ctx, mctx, na->data.bool_rec.true_case, nb->data.bool_rec.true_case) &&
           kt_unify(heap, ctx, mctx, na->data.bool_rec.false_case, nb->data.bool_rec.false_case) &&
           kt_unify(heap, ctx, mctx, na->data.bool_rec.scrutinee, nb->data.bool_rec.scrutinee);
  case KT_LIST_REC:
    return kt_unify(heap, ctx, mctx, na->data.list_rec.motive, nb->data.list_rec.motive) &&
           kt_unify(heap, ctx, mctx, na->data.list_rec.nil_case, nb->data.list_rec.nil_case) &&
           kt_unify(heap, ctx, mctx, na->data.list_rec.cons_case, nb->data.list_rec.cons_case) &&
           kt_unify(heap, ctx, mctx, na->data.list_rec.scrutinee, nb->data.list_rec.scrutinee);
  case KT_MAYBE_REC:
    return kt_unify(heap, ctx, mctx, na->data.maybe_rec.motive, nb->data.maybe_rec.motive) &&
           kt_unify(heap, ctx, mctx, na->data.maybe_rec.nothing_case, nb->data.maybe_rec.nothing_case) &&
           kt_unify(heap, ctx, mctx, na->data.maybe_rec.just_case, nb->data.maybe_rec.just_case) &&
           kt_unify(heap, ctx, mctx, na->data.maybe_rec.scrutinee, nb->data.maybe_rec.scrutinee);
  case KT_SUM_REC:
    return kt_unify(heap, ctx, mctx, na->data.sum_rec.motive, nb->data.sum_rec.motive) &&
           kt_unify(heap, ctx, mctx, na->data.sum_rec.left_case, nb->data.sum_rec.left_case) &&
           kt_unify(heap, ctx, mctx, na->data.sum_rec.right_case, nb->data.sum_rec.right_case) &&
           kt_unify(heap, ctx, mctx, na->data.sum_rec.scrutinee, nb->data.sum_rec.scrutinee);
  case KT_J:
    return kt_unify(heap, ctx, mctx, na->data.j.motive, nb->data.j.motive) &&
           kt_unify(heap, ctx, mctx, na->data.j.base_case, nb->data.j.base_case) &&
           kt_unify(heap, ctx, mctx, na->data.j.a, nb->data.j.a) &&
           kt_unify(heap, ctx, mctx, na->data.j.b, nb->data.j.b) &&
           kt_unify(heap, ctx, mctx, na->data.j.proof, nb->data.j.proof);
  case KT_ABSURD:
    return kt_unify(heap, ctx, mctx, na->data.absurd.target_type, nb->data.absurd.target_type);
  case KT_INTERVAL: case KT_I0: case KT_I1:
    return 1;
  case KT_PATH:
    return kt_unify(heap, ctx, mctx, na->data.path.type, nb->data.path.type) &&
           kt_unify(heap, ctx, mctx, na->data.path.a, nb->data.path.a) &&
           kt_unify(heap, ctx, mctx, na->data.path.b, nb->data.path.b);
  case KT_PLAM:
    return kt_unify(heap, kctx_extend(heap, ctx, na->data.plam.name,
                                      kt_interval(heap), NULL),
                    mctx, na->data.plam.body, nb->data.plam.body);
  case KT_PAPP:
    return kt_unify(heap, ctx, mctx, na->data.papp.path, nb->data.papp.path) &&
           kt_unify(heap, ctx, mctx, na->data.papp.arg, nb->data.papp.arg);
  default:
    return 0;
  }
}


/* ---- global definitions ---- */

kdef_ctx_t *kdef_ctx_create(lizard_heap_t *heap) {
  kdef_ctx_t *d = (kdef_ctx_t *)lizard_heap_alloc(sizeof(kdef_ctx_t));
  d->defs = NULL;
  return d;
}

void kdef_add(lizard_heap_t *heap, kdef_ctx_t *dctx,
              const char *name, kterm_t *type, kterm_t *value) {
  kdef_t *def = (kdef_t *)lizard_heap_alloc(sizeof(kdef_t));
  def->name = name;
  def->type = type;
  def->value = value;
  def->next = dctx->defs;
  dctx->defs = def;
}

kdef_t *kdef_lookup(kdef_ctx_t *dctx, const char *name) {
  kdef_t *d;
  for (d = dctx->defs; d != NULL; d = d->next) {
    if (strcmp(d->name, name) == 0) return d;
  }
  return NULL;
}

/* ===================== user-defined inductive types ===================== */
/* Representation: the type former and each constructor are opaque constants
 * (KT_CONST) whose types are registered in the definition context; the
 * eliminator is a constant carrying an iota-reduction rule applied in kt_whnf.
 * Because everything reuses APP/CONST/PI, no new term tags are needed and
 * subst/shift/equal/zonk are unchanged.  The soundness surface is exactly two
 * things: the strict-positivity check (kind_declare) and the iota rule
 * (try_iota). */

#define KIND_MAX_SPINE 64

static kterm_t *mk_const(lizard_heap_t *heap, const char *name) {
  kterm_t *t = kt_alloc(heap, KT_CONST);
  t->data.constant.name = name;
  return t;
}

static const char *gen_name(lizard_heap_t *heap, char p, int n) {
  char *s = (char *)lizard_heap_alloc(12);
  sprintf(s, "%c%d", p, n);
  return s;
}

/* D p_1 .. p_nP where the param block sits `depth` binders above here
 * (p_k at de Bruijn index depth + (nP - k)). */
static kterm_t *ind_applied(lizard_heap_t *heap, const char *dname,
                            int nP, int depth) {
  kterm_t *acc = mk_const(heap, dname);
  int k;
  for (k = 1; k <= nP; k++)
    acc = kt_app(heap, acc, kt_var(heap, depth + (nP - k)));
  return acc;
}

/* Collect the spine of an application; args[0] is the leftmost argument. */
static kterm_t *gather_spine(kterm_t *t, kterm_t **args, int *n) {
  kterm_t *tmp[KIND_MAX_SPINE];
  int c = 0, i;
  while (t->tag == KT_APP && c < KIND_MAX_SPINE) {
    tmp[c++] = t->data.app.arg;
    t = t->data.app.fun;
  }
  for (i = 0; i < c; i++) args[i] = tmp[c - 1 - i];
  *n = c;
  return t;
}

kind_t *kind_find_rec(kind_ctx_t *kc, const char *name) {
  kind_t *k;
  if (kc == NULL) return NULL;
  for (k = kc->inds; k != NULL; k = k->next)
    if (strcmp(k->rec_name, name) == 0) return k;
  return NULL;
}

kind_t *kind_find_ctor(kind_ctx_t *kc, const char *name, int *index_out) {
  kind_t *k; int i;
  if (kc == NULL) return NULL;
  for (k = kc->inds; k != NULL; k = k->next)
    for (i = 0; i < k->n_ctors; i++)
      if (strcmp(k->ctors[i].name, name) == 0) {
        if (index_out) *index_out = i;
        return k;
      }
  return NULL;
}

/* Iota: D-rec p.. M m.. (c_i p.. a..) -> m_i a.. (D-rec p.. M m.. a_rec) ..
 * Returns the reduced (un-whnf'd) term, or NULL if `t` is not such a redex. */
static kterm_t *try_iota(lizard_heap_t *heap, kctx_t *ctx, kterm_t *t) {
  kind_ctx_t *kc;
  kterm_t *args[KIND_MAX_SPINE], *head, *scrut, *wscrut;
  kterm_t *cargs[KIND_MAX_SPINE], *chead, *rec_fn, *method, *result;
  kind_t *ind, *cind;
  int n, cn, ci, nP, nC, scrut_index, nargs, j, k;
  if (t->tag != KT_APP) return NULL;
  if (ctx == NULL || ctx->defs == NULL) return NULL;
  kc = (kind_ctx_t *)((kdef_ctx_t *)ctx->defs)->inds;
  if (kc == NULL) return NULL;
  head = gather_spine(t, args, &n);
  if (head->tag != KT_CONST) return NULL;
  ind = kind_find_rec(kc, head->data.constant.name);
  if (ind == NULL) return NULL;
  nP = ind->n_params; nC = ind->n_ctors;
  scrut_index = nP + 1 + nC;
  if (n < scrut_index + 1) return NULL;          /* under-applied: neutral */
  scrut = args[scrut_index];
  wscrut = kt_whnf(heap, ctx, scrut);
  chead = gather_spine(wscrut, cargs, &cn);
  if (chead->tag != KT_CONST) return NULL;       /* neutral scrutinee */
  cind = kind_find_ctor(kc, chead->data.constant.name, &ci);
  if (cind != ind) return NULL;                  /* not a ctor of this type */
  nargs = ind->ctors[ci].n_args;
  if (cn < nP + nargs) return NULL;              /* ill-formed; bail */
  method = args[nP + 1 + ci];
  rec_fn = head;
  for (k = 0; k < scrut_index; k++) rec_fn = kt_app(heap, rec_fn, args[k]);
  result = method;
  for (j = 0; j < nargs; j++)
    result = kt_app(heap, result, cargs[nP + j]);
  for (j = 0; j < nargs; j++)
    if (ind->ctors[ci].recursive[j])
      result = kt_app(heap, result, kt_app(heap, rec_fn, cargs[nP + j]));
  for (k = scrut_index + 1; k < n; k++)
    result = kt_app(heap, result, args[k]);
  return result;
}

/* Does the constant `name` occur anywhere in `t`?  Complete over the term
 * language (a missed occurrence would weaken the positivity check). */
static int const_occurs(kterm_t *t, const char *name) {
  if (t == NULL) return 0;
  switch (t->tag) {
  case KT_CONST: return strcmp(t->data.constant.name, name) == 0;
  case KT_PI:  return const_occurs(t->data.pi.domain, name) || const_occurs(t->data.pi.codomain, name);
  case KT_LAM: return const_occurs(t->data.lam.domain, name) || const_occurs(t->data.lam.body, name);
  case KT_APP: return const_occurs(t->data.app.fun, name) || const_occurs(t->data.app.arg, name);
  case KT_SIGMA: return const_occurs(t->data.sigma.fst_type, name) || const_occurs(t->data.sigma.snd_type, name);
  case KT_PAIR: return const_occurs(t->data.pair.fst, name) || const_occurs(t->data.pair.snd, name);
  case KT_PROJ1: case KT_PROJ2: return const_occurs(t->data.proj.target, name);
  case KT_SUCC: return const_occurs(t->data.succ.pred, name);
  case KT_ID: return const_occurs(t->data.id.type, name) || const_occurs(t->data.id.a, name) || const_occurs(t->data.id.b, name);
  case KT_REFL: return const_occurs(t->data.refl.value, name);
  case KT_NAT_REC: return const_occurs(t->data.nat_rec.motive, name) || const_occurs(t->data.nat_rec.zero_case, name)
                     || const_occurs(t->data.nat_rec.succ_case, name) || const_occurs(t->data.nat_rec.scrutinee, name);
  case KT_BOOL_REC: return const_occurs(t->data.bool_rec.motive, name) || const_occurs(t->data.bool_rec.true_case, name)
                     || const_occurs(t->data.bool_rec.false_case, name) || const_occurs(t->data.bool_rec.scrutinee, name);
  case KT_LIST_REC: return const_occurs(t->data.list_rec.motive, name) || const_occurs(t->data.list_rec.nil_case, name)
                     || const_occurs(t->data.list_rec.cons_case, name) || const_occurs(t->data.list_rec.scrutinee, name);
  case KT_MAYBE_REC: return const_occurs(t->data.maybe_rec.motive, name) || const_occurs(t->data.maybe_rec.nothing_case, name)
                     || const_occurs(t->data.maybe_rec.just_case, name) || const_occurs(t->data.maybe_rec.scrutinee, name);
  case KT_SUM_REC: return const_occurs(t->data.sum_rec.motive, name) || const_occurs(t->data.sum_rec.left_case, name)
                     || const_occurs(t->data.sum_rec.right_case, name) || const_occurs(t->data.sum_rec.scrutinee, name);
  case KT_J: return const_occurs(t->data.j.motive, name) || const_occurs(t->data.j.base_case, name)
                     || const_occurs(t->data.j.type, name) || const_occurs(t->data.j.a, name)
                     || const_occurs(t->data.j.b, name) || const_occurs(t->data.j.proof, name);
  case KT_ANNOT: return const_occurs(t->data.annot.term, name) || const_occurs(t->data.annot.type, name);
  case KT_LET: return const_occurs(t->data.let.value, name) || const_occurs(t->data.let.body, name);
  case KT_INL: return const_occurs(t->data.inl.value, name) || const_occurs(t->data.inl.right_type, name);
  case KT_INR: return const_occurs(t->data.inr.value, name) || const_occurs(t->data.inr.left_type, name);
  case KT_JUST: return const_occurs(t->data.just.value, name);
  case KT_MAYBE: return const_occurs(t->data.maybe.elem_type, name);
  case KT_SUM_K: return const_occurs(t->data.sum_k.left_type, name) || const_occurs(t->data.sum_k.right_type, name);
  case KT_ABSURD: return const_occurs(t->data.absurd.target_type, name);
  case KT_LIST: return const_occurs(t->data.list.elem_type, name);
  case KT_CONS_K: return const_occurs(t->data.cons_k.head, name) || const_occurs(t->data.cons_k.tail, name);
  case KT_INTERVAL: case KT_I0: case KT_I1: return 0;
  case KT_PATH: return const_occurs(t->data.path.type, name) || const_occurs(t->data.path.a, name) || const_occurs(t->data.path.b, name);
  case KT_PLAM: return const_occurs(t->data.plam.body, name);
  case KT_PAPP: return const_occurs(t->data.papp.path, name) || const_occurs(t->data.papp.arg, name);
  default: return 0;
  }
}

/* Classify a constructor argument type (in the parameter context [params]):
 *   0  non-recursive (does not mention the inductive)
 *   1  a strictly-positive recursive occurrence: exactly `Name p_1 .. p_nP`
 *  -1  an illegal occurrence (anything else mentioning Name)              */
static int classify_arg(kterm_t *T, const char *dname, int nP) {
  kterm_t *args[KIND_MAX_SPINE], *head;
  int n, k;
  if (!const_occurs(T, dname)) return 0;
  head = gather_spine(T, args, &n);
  if (head->tag != KT_CONST) return -1;
  if (strcmp(head->data.constant.name, dname) != 0) return -1;
  if (n != nP) return -1;
  for (k = 0; k < nP; k++) {
    if (args[k]->tag != KT_VAR) return -1;
    if (args[k]->data.var.index != nP - 1 - k) return -1;   /* p_{k+1} */
    if (const_occurs(args[k], dname)) return -1;
  }
  return 1;
}

/* Type former type: (p_1:P_1) .. (p_nP:P_nP) -> Sort_s */
kterm_t *kind_former_type(lizard_heap_t *heap, kind_decl_t *decl) {
  kterm_t *acc = kt_sort(heap, decl->sort_level);
  int k;
  for (k = decl->n_params; k >= 1; k--)
    acc = kt_pi(heap, decl->param_names[k - 1], decl->param_types[k - 1], acc);
  return acc;
}

/* Constructor c_i type: (params) -> (a_1:T_1) .. (a_nA:T_nA) -> D params */
static kterm_t *build_ctor_type(lizard_heap_t *heap, kind_decl_t *decl,
                                int i) {
  int nP = decl->n_params, nA = decl->ctor_nargs[i], j, k;
  kterm_t **T = decl->ctor_argtypes[i];
  kterm_t *acc = ind_applied(heap, decl->name, nP, nA);
  for (j = nA; j >= 1; j--) {
    kterm_t *dom = kt_shift(heap, T[j - 1], 0, j - 1);
    acc = kt_pi(heap, gen_name(heap, 'a', j), dom, acc);
  }
  for (k = nP; k >= 1; k--)
    acc = kt_pi(heap, decl->param_names[k - 1], decl->param_types[k - 1], acc);
  return acc;
}

/* Method type for c_i, in context [params, M, m_1..m_{i-1}] (i is 1-based):
 *   (a_1:T_1)..(a_nA:T_nA) -> (ih_l : M a_{j_l}).. -> M (c_i params a_1..a_nA)
 * with one ih per recursive argument, in argument order. */
static kterm_t *build_method(lizard_heap_t *heap, kind_decl_t *decl,
                             int idx /*0-based*/, int i /*1-based*/) {
  int nP = decl->n_params, nA = decl->ctor_nargs[idx], j, l, k, R = 0;
  int rec_pos[KIND_MAX_SPINE];
  kterm_t **T = decl->ctor_argtypes[idx];
  const int *recf = decl->ctor_recflags ? decl->ctor_recflags[idx] : NULL;
  kterm_t *capp, *acc;
  for (j = 1; j <= nA; j++) if (recf && recf[j - 1]) rec_pos[R++] = j;
  /* body: M (c_i p.. a..), context [base, a_1..a_nA, ih_1..ih_R] */
  capp = mk_const(heap, decl->ctor_names[idx]);
  for (k = 1; k <= nP; k++)
    capp = kt_app(heap, capp, kt_var(heap, (i + nA + R) + (nP - k)));
  for (j = 1; j <= nA; j++)
    capp = kt_app(heap, capp, kt_var(heap, (nA - j) + R));
  acc = kt_app(heap, kt_var(heap, (i - 1) + nA + R), capp);
  /* induction hypotheses ih_R..ih_1 */
  for (l = R; l >= 1; l--) {
    int jj = rec_pos[l - 1];
    kterm_t *ihdom = kt_app(heap, kt_var(heap, (i - 1) + nA + (l - 1)),
                                  kt_var(heap, (nA - jj) + (l - 1)));
    acc = kt_pi(heap, "ih", ihdom, acc);
  }
  /* arguments a_nA..a_1 */
  for (j = nA; j >= 1; j--) {
    kterm_t *dom = kt_shift(heap, T[j - 1], 0, i + j - 1);
    acc = kt_pi(heap, gen_name(heap, 'a', j), dom, acc);
  }
  return acc;
}

/* Recursor type:
 *  (params) -> (M : (x:D params)->Sort_L) -> (m_1:Meth_1)..(m_nC:Meth_nC)
 *           -> (x:D params) -> M x                                        */
static kterm_t *build_recursor_type(lizard_heap_t *heap,
                                    kind_decl_t *decl) {
  int nP = decl->n_params, nC = decl->n_ctors, i, k, L = decl->sort_level;
  kterm_t *acc, *motive;
  acc = kt_app(heap, kt_var(heap, nC + 1), kt_var(heap, 0));     /* M x */
  acc = kt_pi(heap, "x", ind_applied(heap, decl->name, nP, nC + 1), acc);
  for (i = nC; i >= 1; i--)
    acc = kt_pi(heap, gen_name(heap, 'm', i),
                build_method(heap, decl, i - 1, i), acc);
  motive = kt_pi(heap, "x", ind_applied(heap, decl->name, nP, 0), kt_sort(heap, L));
  acc = kt_pi(heap, "M", motive, acc);
  for (k = nP; k >= 1; k--)
    acc = kt_pi(heap, decl->param_names[k - 1], decl->param_types[k - 1], acc);
  return acc;
}

int kind_declare(lizard_heap_t *heap, kdef_ctx_t *dctx,
                 kind_decl_t *decl) {
  kctx_t *ctx;
  kind_t *ind;
  kterm_t *rec_type;
  int i, j, nC = decl->n_ctors, nP = decl->n_params;
  int **recflags;
  ctx = kctx_create(heap);
  ctx->defs = dctx;
  /* (1) strict positivity + recursion flags */
  recflags = (int **)lizard_heap_alloc(sizeof(int *) * (size_t)(nC ? nC : 1));
  for (i = 0; i < nC; i++) {
    int nA = decl->ctor_nargs[i];
    recflags[i] = (int *)lizard_heap_alloc(sizeof(int) * (size_t)(nA ? nA : 1));
    for (j = 0; j < nA; j++) {
      int c = classify_arg(decl->ctor_argtypes[i][j], decl->name, nP);
      if (c < 0) {
        fprintf(stderr, "data %s: constructor %s argument %d is not strictly "
                "positive (the type may only recur as `%s` applied to its "
                "parameters)\n", decl->name, decl->ctor_names[i], j + 1,
                decl->name);
        return 0;
      }
      recflags[i][j] = c;
    }
  }
  decl->ctor_recflags = recflags;  /* feed synthesis */
  /* (2) synthesize + sanity-check + register each constructor */
  for (i = 0; i < nC; i++) {
    kterm_t *cty = build_ctor_type(heap, decl, i);
    if (kt_infer(heap, ctx, cty) == NULL) {
      fprintf(stderr, "data %s: constructor %s has an ill-formed type\n",
              decl->name, decl->ctor_names[i]);
      return 0;
    }
    kdef_add(heap, dctx, decl->ctor_names[i], cty, NULL);
  }
  /* (3) synthesize + sanity-check + register the eliminator */
  rec_type = build_recursor_type(heap, decl);
  if (kt_infer(heap, ctx, rec_type) == NULL) {
    fprintf(stderr, "data %s: synthesized eliminator type is ill-formed\n",
            decl->name);
    return 0;
  }
  kdef_add(heap, dctx, decl->rec_name, rec_type, NULL);
  /* (4) register the signature for iota-reduction */
  ind = (kind_t *)lizard_heap_alloc(sizeof(kind_t));
  ind->name = decl->name;
  ind->rec_name = decl->rec_name;
  ind->n_params = nP;
  ind->n_ctors = nC;
  ind->ctors = (kind_ctor_t *)lizard_heap_alloc(sizeof(kind_ctor_t) * (size_t)(nC ? nC : 1));
  for (i = 0; i < nC; i++) {
    ind->ctors[i].name = decl->ctor_names[i];
    ind->ctors[i].n_args = decl->ctor_nargs[i];
    ind->ctors[i].recursive = recflags[i];
  }
  if (dctx->inds == NULL) dctx->inds = lizard_heap_alloc(sizeof(kind_ctx_t)), ((kind_ctx_t *)dctx->inds)->inds = NULL;
  ind->next = ((kind_ctx_t *)dctx->inds)->inds;
  ((kind_ctx_t *)dctx->inds)->inds = ind;
  return 1;
}
