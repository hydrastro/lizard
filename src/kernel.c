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
