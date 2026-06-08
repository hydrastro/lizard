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
kterm_t *kt_equiv(lizard_heap_t *heap, kterm_t *a, kterm_t *b) {
  kterm_t *t = kt_alloc(heap, KT_EQUIV);
  t->data.equiv.a = a; t->data.equiv.b = b;
  return t;
}
kterm_t *kt_ua(lizard_heap_t *heap, kterm_t *equiv) {
  kterm_t *t = kt_alloc(heap, KT_UA);
  t->data.ua.equiv = equiv;
  return t;
}

kterm_t *kt_transp(lizard_heap_t *heap, kterm_t *line, kterm_t *base) {
  kterm_t *t = kt_alloc(heap, KT_TRANSP);
  t->data.transp.line = line;
  t->data.transp.base = base;
  return t;
}

kterm_t *kt_ineg(lizard_heap_t *heap, kterm_t *arg) {
  kterm_t *t = kt_alloc(heap, KT_INEG);
  t->data.ineg.arg = arg;
  return t;
}

kterm_t *kt_imeet(lizard_heap_t *heap, kterm_t *left, kterm_t *right) {
  kterm_t *t = kt_alloc(heap, KT_IMEET);
  t->data.imeet.left = left;
  t->data.imeet.right = right;
  return t;
}

kterm_t *kt_ijoin(lizard_heap_t *heap, kterm_t *left, kterm_t *right) {
  kterm_t *t = kt_alloc(heap, KT_IJOIN);
  t->data.ijoin.left = left;
  t->data.ijoin.right = right;
  return t;
}

kterm_t *kt_id_equiv(lizard_heap_t *heap, kterm_t *type) {
  kterm_t *t = kt_alloc(heap, KT_ID_EQUIV);
  t->data.id_equiv.type = type;
  return t;
}

kterm_t *kt_hcomp(lizard_heap_t *heap, kterm_t *type, kterm_t *base) {
  kterm_t *t = kt_alloc(heap, KT_HCOMP);
  t->data.hcomp.type = type;
  t->data.hcomp.base = base;
  return t;
}

kterm_t *kt_cofib(lizard_heap_t *heap, kterm_t *var, kterm_t *endpoint) {
  kterm_t *t = kt_alloc(heap, KT_COFIB);
  t->data.cofib.var = var;
  t->data.cofib.endpoint = endpoint;
  return t;
}

kterm_t *kt_cofib_or(lizard_heap_t *heap, kterm_t *cof1, kterm_t *cof2) {
  kterm_t *t = kt_alloc(heap, KT_COFIB_OR);
  t->data.cofib_or.cof1 = cof1;
  t->data.cofib_or.cof2 = cof2;
  return t;
}

kterm_t *kt_cofib_forall(lizard_heap_t *heap, kterm_t *body) {
  kterm_t *t = kt_alloc(heap, KT_COFIB_FORALL);
  t->data.cofib_forall.body = body;
  return t;
}

kterm_t *kt_hcomp1(lizard_heap_t *heap, kterm_t *type, kterm_t *cof, kterm_t *partial, kterm_t *base) {
  kterm_t *t = kt_alloc(heap, KT_HCOMP1);
  t->data.hcomp1.type = type;
  t->data.hcomp1.cof = cof;
  t->data.hcomp1.partial = partial;
  t->data.hcomp1.base = base;
  return t;
}

kterm_t *kt_comp(lizard_heap_t *heap, kterm_t *line, kterm_t *cof, kterm_t *partial, kterm_t *base) {
  kterm_t *t = kt_alloc(heap, KT_COMP);
  t->data.comp.line = line;
  t->data.comp.cof = cof;
  t->data.comp.partial = partial;
  t->data.comp.base = base;
  return t;
}

kterm_t *kt_comp2(lizard_heap_t *heap, kterm_t *line, kterm_t *cof1, kterm_t *partial1,
                  kterm_t *cof2, kterm_t *partial2, kterm_t *base) {
  kterm_t *t = kt_alloc(heap, KT_COMP2);
  t->data.comp2.line = line;
  t->data.comp2.cof1 = cof1;
  t->data.comp2.partial1 = partial1;
  t->data.comp2.cof2 = cof2;
  t->data.comp2.partial2 = partial2;
  t->data.comp2.base = base;
  return t;
}

kterm_t *kt_hcomp2(lizard_heap_t *heap, kterm_t *type, kterm_t *cof1, kterm_t *partial1,
                   kterm_t *cof2, kterm_t *partial2, kterm_t *base) {
  kterm_t *t = kt_alloc(heap, KT_HCOMP2);
  t->data.hcomp2.type = type;
  t->data.hcomp2.cof1 = cof1;
  t->data.hcomp2.partial1 = partial1;
  t->data.hcomp2.cof2 = cof2;
  t->data.hcomp2.partial2 = partial2;
  t->data.hcomp2.base = base;
  return t;
}

kterm_t *kt_glue(lizard_heap_t *heap, kterm_t *base_type, kterm_t *cof, kterm_t *glue_type, kterm_t *equiv) {
  kterm_t *t = kt_alloc(heap, KT_GLUE);
  t->data.glue.base_type = base_type;
  t->data.glue.cof = cof;
  t->data.glue.glue_type = glue_type;
  t->data.glue.equiv = equiv;
  return t;
}

kterm_t *kt_partial(lizard_heap_t *heap, kterm_t *cof, kterm_t *type) {
  kterm_t *t = kt_alloc(heap, KT_PARTIAL);
  t->data.partial.cof = cof;
  t->data.partial.type = type;
  return t;
}

kterm_t *kt_psys(lizard_heap_t *heap, kterm_t *cof, kterm_t *type, kterm_t *elem) {
  kterm_t *t = kt_alloc(heap, KT_PSYS);
  t->data.psys.cof = cof;
  t->data.psys.type = type;
  t->data.psys.elem = elem;
  return t;
}

kterm_t *kt_equiv_fun(lizard_heap_t *heap, kterm_t *equiv) {
  kterm_t *t = kt_alloc(heap, KT_EQUIV_FUN);
  t->data.equiv_fun.equiv = equiv;
  return t;
}

kterm_t *kt_equiv_inv(lizard_heap_t *heap, kterm_t *equiv) {
  kterm_t *t = kt_alloc(heap, KT_EQUIV_INV);
  t->data.equiv_inv.equiv = equiv;
  return t;
}

kterm_t *kt_mk_equiv(lizard_heap_t *heap, kterm_t *glue_type, kterm_t *base_type,
                     kterm_t *fwd, kterm_t *inv, kterm_t *eta, kterm_t *eps) {
  kterm_t *t = kt_alloc(heap, KT_MK_EQUIV);
  t->data.mk_equiv.glue_type = glue_type;
  t->data.mk_equiv.base_type = base_type;
  t->data.mk_equiv.fwd = fwd;
  t->data.mk_equiv.inv = inv;
  t->data.mk_equiv.eta = eta;
  t->data.mk_equiv.eps = eps;
  return t;
}

kterm_t *kt_equiv_eta(lizard_heap_t *heap, kterm_t *equiv) {
  kterm_t *t = kt_alloc(heap, KT_EQUIV_ETA);
  t->data.equiv_eta.equiv = equiv;
  return t;
}

kterm_t *kt_equiv_eps(lizard_heap_t *heap, kterm_t *equiv) {
  kterm_t *t = kt_alloc(heap, KT_EQUIV_EPS);
  t->data.equiv_eps.equiv = equiv;
  return t;
}

kterm_t *kt_unglue(lizard_heap_t *heap, kterm_t *base_type, kterm_t *cof, kterm_t *glue_type,
                   kterm_t *equiv, kterm_t *arg) {
  kterm_t *t = kt_alloc(heap, KT_UNGLUE);
  t->data.unglue.base_type = base_type;
  t->data.unglue.cof = cof;
  t->data.unglue.glue_type = glue_type;
  t->data.unglue.equiv = equiv;
  t->data.unglue.arg = arg;
  return t;
}

kterm_t *kt_glue_elem(lizard_heap_t *heap, kterm_t *base_type, kterm_t *cof, kterm_t *glue_type,
                      kterm_t *equiv, kterm_t *member, kterm_t *base) {
  kterm_t *t = kt_alloc(heap, KT_GLUE_ELEM);
  t->data.glue_elem.base_type = base_type;
  t->data.glue_elem.cof = cof;
  t->data.glue_elem.glue_type = glue_type;
  t->data.glue_elem.equiv = equiv;
  t->data.glue_elem.member = member;
  t->data.glue_elem.base = base;
  return t;
}

kterm_t *kt_gtransp(lizard_heap_t *heap, kterm_t *base_type, kterm_t *cof, kterm_t *glue_type,
                    kterm_t *equiv, kterm_t *base) {
  kterm_t *t = kt_alloc(heap, KT_GTRANSP);
  t->data.gtransp.base_type = base_type;
  t->data.gtransp.cof = cof;
  t->data.gtransp.glue_type = glue_type;
  t->data.gtransp.equiv = equiv;
  t->data.gtransp.base = base;
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

/* kctx_restrict — restrict a context by a face equation (the de Bruijn variable at absolute index
 * `vidx` in `ctx` is set to the interval endpoint `ep`).  This produces the context Γ, φ used to
 * type a PARTIAL element / partial section: under the face, the variable genuinely equals ep, which
 * (after whnf) refines hypotheses whose types mention it — e.g. a Glue type (cofib v ep) collapses to
 * its T-component, so a glue element becomes a T-element on the face.  Only entries MORE RECENT than
 * vidx can reference it: entry at position j (j < vidx) sees the variable at index (vidx-1-j) within
 * its own type's scope, so we substitute that index by ep there; entries at j >= vidx are unchanged.
 * SOUNDNESS: this is only ever used to type a term that is provably consumed on the face (comp's
 * partial, a glue member), so imposing the face is justified; the restricted context is never used to
 * type something that escapes the face. */
static kctx_t *kctx_restrict(lizard_heap_t *heap, kctx_t *ctx, int vidx, kterm_t *ep) {
  kctx_entry_t *src = ctx->entries;
  kctx_entry_t *out_head = NULL, *out_tail = NULL;
  kctx_t *nc;
  int j = 0;
  /* Restrict the context by the face equation (the variable at index vidx equals the endpoint ep) by
   * marking that variable as a DEFINITION (value = ep) rather than substituting.  kt_whnf already
   * reduces a KT_VAR to its entry's value, so wherever the face variable appears — in a later
   * hypothesis's type (e.g. a Glue type (cofib v i1) collapsing to its T-component) or in the partial
   * leg's body — it reduces to ep on demand.  Crucially this keeps EVERY de Bruijn index stable: no
   * entry is dropped and no term is shifted, so the term checked in this context needs NO kt_subst and
   * stays perfectly aligned (the previous subst-and-restrict scheme decremented the body's indices
   * above vidx while the context kept them, mis-resolving a leg that directly consumes a refined glue
   * variable — the homogeneous-Glue-composition blocker).  Endpoints are closed, so no shifting of ep
   * is needed at any depth. */
  for (; src != NULL; src = src->next, j++) {
    kctx_entry_t *ne = (kctx_entry_t *)lizard_heap_alloc(sizeof(kctx_entry_t));
    ne->name = src->name;
    ne->type = src->type;
    ne->value = (j == vidx) ? ep : src->value;   /* the face var becomes a definition = ep */
    ne->next = NULL;
    if (out_head == NULL) { out_head = ne; out_tail = ne; }
    else { out_tail->next = ne; out_tail = ne; }
  }
  nc = (kctx_t *)lizard_heap_alloc(sizeof(kctx_t));
  nc->entries = out_head; nc->depth = ctx->depth; nc->heap = heap; nc->defs = ctx->defs;
  return nc;
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
  case KT_EQUIV:
    return kt_equiv(heap, kt_shift(heap, t->data.equiv.a, cutoff, delta),
                          kt_shift(heap, t->data.equiv.b, cutoff, delta));
  case KT_UA:
    return kt_ua(heap, kt_shift(heap, t->data.ua.equiv, cutoff, delta));
  case KT_TRANSP:
    return kt_transp(heap, kt_shift(heap, t->data.transp.line, cutoff, delta),
                           kt_shift(heap, t->data.transp.base, cutoff, delta));
  case KT_INEG:
    return kt_ineg(heap, kt_shift(heap, t->data.ineg.arg, cutoff, delta));
  case KT_IMEET:
    return kt_imeet(heap, kt_shift(heap, t->data.imeet.left, cutoff, delta),
                          kt_shift(heap, t->data.imeet.right, cutoff, delta));
  case KT_IJOIN:
    return kt_ijoin(heap, kt_shift(heap, t->data.ijoin.left, cutoff, delta),
                          kt_shift(heap, t->data.ijoin.right, cutoff, delta));
  case KT_ID_EQUIV:
    return kt_id_equiv(heap, kt_shift(heap, t->data.id_equiv.type, cutoff, delta));
  case KT_HCOMP:
    return kt_hcomp(heap, kt_shift(heap, t->data.hcomp.type, cutoff, delta),
                          kt_shift(heap, t->data.hcomp.base, cutoff, delta));
  case KT_COFIB:
    return kt_cofib(heap, kt_shift(heap, t->data.cofib.var, cutoff, delta),
                          kt_shift(heap, t->data.cofib.endpoint, cutoff, delta));
  case KT_COFIB_OR:
    return kt_cofib_or(heap, kt_shift(heap, t->data.cofib_or.cof1, cutoff, delta),
                             kt_shift(heap, t->data.cofib_or.cof2, cutoff, delta));
  case KT_COFIB_FORALL:
    return kt_cofib_forall(heap, kt_shift(heap, t->data.cofib_forall.body, cutoff + 1, delta));
  case KT_HCOMP1:
    return kt_hcomp1(heap, kt_shift(heap, t->data.hcomp1.type, cutoff, delta),
                           kt_shift(heap, t->data.hcomp1.cof, cutoff, delta),
                           kt_shift(heap, t->data.hcomp1.partial, cutoff, delta),
                           kt_shift(heap, t->data.hcomp1.base, cutoff, delta));
  case KT_COMP:
    return kt_comp(heap, kt_shift(heap, t->data.comp.line, cutoff, delta),
                         kt_shift(heap, t->data.comp.cof, cutoff, delta),
                         kt_shift(heap, t->data.comp.partial, cutoff, delta),
                         kt_shift(heap, t->data.comp.base, cutoff, delta));
  case KT_COMP2:
    return kt_comp2(heap, kt_shift(heap, t->data.comp2.line, cutoff, delta),
                          kt_shift(heap, t->data.comp2.cof1, cutoff, delta),
                          kt_shift(heap, t->data.comp2.partial1, cutoff, delta),
                          kt_shift(heap, t->data.comp2.cof2, cutoff, delta),
                          kt_shift(heap, t->data.comp2.partial2, cutoff, delta),
                          kt_shift(heap, t->data.comp2.base, cutoff, delta));
  case KT_HCOMP2:
    return kt_hcomp2(heap, kt_shift(heap, t->data.hcomp2.type, cutoff, delta),
                           kt_shift(heap, t->data.hcomp2.cof1, cutoff, delta),
                           kt_shift(heap, t->data.hcomp2.partial1, cutoff, delta),
                           kt_shift(heap, t->data.hcomp2.cof2, cutoff, delta),
                           kt_shift(heap, t->data.hcomp2.partial2, cutoff, delta),
                           kt_shift(heap, t->data.hcomp2.base, cutoff, delta));
  case KT_GLUE:
    return kt_glue(heap, kt_shift(heap, t->data.glue.base_type, cutoff, delta),
                         kt_shift(heap, t->data.glue.cof, cutoff, delta),
                         kt_shift(heap, t->data.glue.glue_type, cutoff, delta),
                         kt_shift(heap, t->data.glue.equiv, cutoff, delta));
  case KT_PARTIAL:
    return kt_partial(heap, kt_shift(heap, t->data.partial.cof, cutoff, delta),
                            kt_shift(heap, t->data.partial.type, cutoff, delta));
  case KT_PSYS:
    return kt_psys(heap, kt_shift(heap, t->data.psys.cof, cutoff, delta),
                         kt_shift(heap, t->data.psys.type, cutoff, delta),
                         kt_shift(heap, t->data.psys.elem, cutoff, delta));
  case KT_EQUIV_FUN:
    return kt_equiv_fun(heap, kt_shift(heap, t->data.equiv_fun.equiv, cutoff, delta));
  case KT_EQUIV_INV:
    return kt_equiv_inv(heap, kt_shift(heap, t->data.equiv_inv.equiv, cutoff, delta));
  case KT_MK_EQUIV:
    return kt_mk_equiv(heap, kt_shift(heap, t->data.mk_equiv.glue_type, cutoff, delta),
                             kt_shift(heap, t->data.mk_equiv.base_type, cutoff, delta),
                             kt_shift(heap, t->data.mk_equiv.fwd, cutoff, delta),
                             kt_shift(heap, t->data.mk_equiv.inv, cutoff, delta),
                             kt_shift(heap, t->data.mk_equiv.eta, cutoff, delta),
                             kt_shift(heap, t->data.mk_equiv.eps, cutoff, delta));
  case KT_EQUIV_ETA:
    return kt_equiv_eta(heap, kt_shift(heap, t->data.equiv_eta.equiv, cutoff, delta));
  case KT_EQUIV_EPS:
    return kt_equiv_eps(heap, kt_shift(heap, t->data.equiv_eps.equiv, cutoff, delta));
  case KT_UNGLUE:
    return kt_unglue(heap, kt_shift(heap, t->data.unglue.base_type, cutoff, delta),
                           kt_shift(heap, t->data.unglue.cof, cutoff, delta),
                           kt_shift(heap, t->data.unglue.glue_type, cutoff, delta),
                           kt_shift(heap, t->data.unglue.equiv, cutoff, delta),
                           kt_shift(heap, t->data.unglue.arg, cutoff, delta));
  case KT_GLUE_ELEM:
    return kt_glue_elem(heap, kt_shift(heap, t->data.glue_elem.base_type, cutoff, delta),
                              kt_shift(heap, t->data.glue_elem.cof, cutoff, delta),
                              kt_shift(heap, t->data.glue_elem.glue_type, cutoff, delta),
                              kt_shift(heap, t->data.glue_elem.equiv, cutoff, delta),
                              kt_shift(heap, t->data.glue_elem.member, cutoff, delta),
                              kt_shift(heap, t->data.glue_elem.base, cutoff, delta));
  case KT_GTRANSP:
    return kt_gtransp(heap, kt_shift(heap, t->data.gtransp.base_type, cutoff, delta),
                            kt_shift(heap, t->data.gtransp.cof, cutoff, delta),
                            kt_shift(heap, t->data.gtransp.glue_type, cutoff, delta),
                            kt_shift(heap, t->data.gtransp.equiv, cutoff, delta),
                            kt_shift(heap, t->data.gtransp.base, cutoff, delta));
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
  case KT_EQUIV:
    return kt_equiv(heap, kt_subst(heap, t->data.equiv.a, n, s),
                          kt_subst(heap, t->data.equiv.b, n, s));
  case KT_UA:
    return kt_ua(heap, kt_subst(heap, t->data.ua.equiv, n, s));
  case KT_TRANSP:
    return kt_transp(heap, kt_subst(heap, t->data.transp.line, n, s),
                           kt_subst(heap, t->data.transp.base, n, s));
  case KT_INEG:
    return kt_ineg(heap, kt_subst(heap, t->data.ineg.arg, n, s));
  case KT_IMEET:
    return kt_imeet(heap, kt_subst(heap, t->data.imeet.left, n, s),
                          kt_subst(heap, t->data.imeet.right, n, s));
  case KT_IJOIN:
    return kt_ijoin(heap, kt_subst(heap, t->data.ijoin.left, n, s),
                          kt_subst(heap, t->data.ijoin.right, n, s));
  case KT_ID_EQUIV:
    return kt_id_equiv(heap, kt_subst(heap, t->data.id_equiv.type, n, s));
  case KT_HCOMP:
    return kt_hcomp(heap, kt_subst(heap, t->data.hcomp.type, n, s),
                          kt_subst(heap, t->data.hcomp.base, n, s));
  case KT_COFIB:
    return kt_cofib(heap, kt_subst(heap, t->data.cofib.var, n, s),
                          kt_subst(heap, t->data.cofib.endpoint, n, s));
  case KT_COFIB_OR:
    return kt_cofib_or(heap, kt_subst(heap, t->data.cofib_or.cof1, n, s),
                             kt_subst(heap, t->data.cofib_or.cof2, n, s));
  case KT_COFIB_FORALL:
    return kt_cofib_forall(heap, kt_subst(heap, t->data.cofib_forall.body, n + 1, s));
  case KT_HCOMP1:
    return kt_hcomp1(heap, kt_subst(heap, t->data.hcomp1.type, n, s),
                           kt_subst(heap, t->data.hcomp1.cof, n, s),
                           kt_subst(heap, t->data.hcomp1.partial, n, s),
                           kt_subst(heap, t->data.hcomp1.base, n, s));
  case KT_COMP:
    return kt_comp(heap, kt_subst(heap, t->data.comp.line, n, s),
                         kt_subst(heap, t->data.comp.cof, n, s),
                         kt_subst(heap, t->data.comp.partial, n, s),
                         kt_subst(heap, t->data.comp.base, n, s));
  case KT_COMP2:
    return kt_comp2(heap, kt_subst(heap, t->data.comp2.line, n, s),
                          kt_subst(heap, t->data.comp2.cof1, n, s),
                          kt_subst(heap, t->data.comp2.partial1, n, s),
                          kt_subst(heap, t->data.comp2.cof2, n, s),
                          kt_subst(heap, t->data.comp2.partial2, n, s),
                          kt_subst(heap, t->data.comp2.base, n, s));
  case KT_HCOMP2:
    return kt_hcomp2(heap, kt_subst(heap, t->data.hcomp2.type, n, s),
                           kt_subst(heap, t->data.hcomp2.cof1, n, s),
                           kt_subst(heap, t->data.hcomp2.partial1, n, s),
                           kt_subst(heap, t->data.hcomp2.cof2, n, s),
                           kt_subst(heap, t->data.hcomp2.partial2, n, s),
                           kt_subst(heap, t->data.hcomp2.base, n, s));
  case KT_GLUE:
    return kt_glue(heap, kt_subst(heap, t->data.glue.base_type, n, s),
                         kt_subst(heap, t->data.glue.cof, n, s),
                         kt_subst(heap, t->data.glue.glue_type, n, s),
                         kt_subst(heap, t->data.glue.equiv, n, s));
  case KT_PARTIAL:
    return kt_partial(heap, kt_subst(heap, t->data.partial.cof, n, s),
                            kt_subst(heap, t->data.partial.type, n, s));
  case KT_PSYS:
    return kt_psys(heap, kt_subst(heap, t->data.psys.cof, n, s),
                         kt_subst(heap, t->data.psys.type, n, s),
                         kt_subst(heap, t->data.psys.elem, n, s));
  case KT_EQUIV_FUN:
    return kt_equiv_fun(heap, kt_subst(heap, t->data.equiv_fun.equiv, n, s));
  case KT_EQUIV_INV:
    return kt_equiv_inv(heap, kt_subst(heap, t->data.equiv_inv.equiv, n, s));
  case KT_MK_EQUIV:
    return kt_mk_equiv(heap, kt_subst(heap, t->data.mk_equiv.glue_type, n, s),
                             kt_subst(heap, t->data.mk_equiv.base_type, n, s),
                             kt_subst(heap, t->data.mk_equiv.fwd, n, s),
                             kt_subst(heap, t->data.mk_equiv.inv, n, s),
                             kt_subst(heap, t->data.mk_equiv.eta, n, s),
                             kt_subst(heap, t->data.mk_equiv.eps, n, s));
  case KT_EQUIV_ETA:
    return kt_equiv_eta(heap, kt_subst(heap, t->data.equiv_eta.equiv, n, s));
  case KT_EQUIV_EPS:
    return kt_equiv_eps(heap, kt_subst(heap, t->data.equiv_eps.equiv, n, s));
  case KT_UNGLUE:
    return kt_unglue(heap, kt_subst(heap, t->data.unglue.base_type, n, s),
                           kt_subst(heap, t->data.unglue.cof, n, s),
                           kt_subst(heap, t->data.unglue.glue_type, n, s),
                           kt_subst(heap, t->data.unglue.equiv, n, s),
                           kt_subst(heap, t->data.unglue.arg, n, s));
  case KT_GLUE_ELEM:
    return kt_glue_elem(heap, kt_subst(heap, t->data.glue_elem.base_type, n, s),
                              kt_subst(heap, t->data.glue_elem.cof, n, s),
                              kt_subst(heap, t->data.glue_elem.glue_type, n, s),
                              kt_subst(heap, t->data.glue_elem.equiv, n, s),
                              kt_subst(heap, t->data.glue_elem.member, n, s),
                              kt_subst(heap, t->data.glue_elem.base, n, s));
  case KT_GTRANSP:
    return kt_gtransp(heap, kt_subst(heap, t->data.gtransp.base_type, n, s),
                            kt_subst(heap, t->data.gtransp.cof, n, s),
                            kt_subst(heap, t->data.gtransp.glue_type, n, s),
                            kt_subst(heap, t->data.gtransp.equiv, n, s),
                            kt_subst(heap, t->data.gtransp.base, n, s));
  default:
    return t;
  }
}

/* ---- weak head normal form ---- */

/* forward decl: iota-reduction for user inductives (defined below) */
static kterm_t *try_iota(lizard_heap_t *heap, kctx_t *ctx, kterm_t *t);

/* Cofibration decision, understanding both (cofib r b) and (cofib-or c1 c2).  A disjunction HOLDS if
 * either disjunct holds, and is EMPTY only if BOTH disjuncts are empty (recursively).  These are used
 * by hcomp/comp to reduce a system over a face that is a disjunction, exactly as for a single face.
 * The cofibration argument must already be whnf'd by the caller. */
static int cofib_decided_held(lizard_heap_t *heap, kctx_t *ctx, kterm_t *c) {
  if (c->tag == KT_COFIB) {
    kterm_t *r = kt_whnf(heap, ctx, c->data.cofib.var);
    kterm_t *b = kt_whnf(heap, ctx, c->data.cofib.endpoint);
    return kt_equal(heap, ctx, r, b);
  }
  if (c->tag == KT_COFIB_OR) {
    kterm_t *c1 = kt_whnf(heap, ctx, c->data.cofib_or.cof1);
    kterm_t *c2 = kt_whnf(heap, ctx, c->data.cofib_or.cof2);
    return cofib_decided_held(heap, ctx, c1) || cofib_decided_held(heap, ctx, c2);
  }
  return 0;
}
static int cofib_decided_empty(lizard_heap_t *heap, kctx_t *ctx, kterm_t *c) {
  if (c->tag == KT_COFIB) {
    kterm_t *r = kt_whnf(heap, ctx, c->data.cofib.var);
    kterm_t *b = kt_whnf(heap, ctx, c->data.cofib.endpoint);
    return (r->tag == KT_I0 && b->tag == KT_I1) || (r->tag == KT_I1 && b->tag == KT_I0);
  }
  if (c->tag == KT_COFIB_OR) {
    kterm_t *c1 = kt_whnf(heap, ctx, c->data.cofib_or.cof1);
    kterm_t *c2 = kt_whnf(heap, ctx, c->data.cofib_or.cof2);
    return cofib_decided_empty(heap, ctx, c1) && cofib_decided_empty(heap, ctx, c2);
  }
  return 0;
}

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
  case KT_TRANSP: {
    /* transp line base : (line @ i1).  The ONLY sound computation rule here is the
     * constant-line rule: if the type line does not vary over the interval — i.e.
     * its body at i0 is definitionally equal to its body at i1 — then transport is
     * the identity and transp reduces to base.  (Transport along a constant path
     * does nothing.)  This is type-preserving because base : line@i0 and, when the
     * line is constant, line@i0 ≡ line@i1, so base also inhabits the result type.
     * A NON-constant line stays NEUTRAL: full transport across a varying line needs
     * Glue/comp, which is deliberately not provided — so we never compute a wrong
     * value, we just decline to reduce. */
    kterm_t *line = kt_whnf(heap, ctx, t->data.transp.line);
    kterm_t *base;
    if (line->tag == KT_PLAM) {
      /* Reduce the line body to weak head normal form UNDER the interval binder, so the structural
       * rules below dispatch on the body's actual head — seeing through a Glue type (or any other
       * redex) that only reduces to a Sigma/Sum/Pi.  This is meaning-preserving: whnf does not
       * change the type, it only exposes the head.  In particular an EMPTY-FACE Glue line
       * <i>(Glue A(i) (cofib i0 i1) T e) has body reducing to A(i), so transport over it now
       * delegates to the bare A(i)-line transport (the Glue is absent on an empty face). */
      kctx_t *ictx = kctx_extend(heap, ctx, line->data.plam.name, kt_interval(heap), NULL);
      kterm_t *rbody = kt_whnf(heap, ictx, line->data.plam.body);
      if (rbody != line->data.plam.body)
        line = kt_plam(heap, line->data.plam.name, rbody);
      {
      kterm_t *at_i0 = kt_subst(heap, line->data.plam.body, 0, kt_i0(heap));
      kterm_t *at_i1 = kt_subst(heap, line->data.plam.body, 0, kt_i1(heap));
      if (kt_equal(heap, ctx, at_i0, at_i1)) {
        /* constant line: transport is the identity */
        return kt_whnf(heap, ctx, t->data.transp.base);
      }
      }
      /* STRUCTURAL RULE — non-dependent Sigma: transport at a product is componentwise.
       * If the line is <i> (Sigma (_ : A(i)) B(i)) where B does NOT depend on the first
       * component (a NON-dependent product), then
       *   transp <i>(Sigma A(i) B(i)) (a, b)  =  ( transp <i>A(i) a , transp <i>B(i) b ).
       * This needs no Glue: the two component sub-lines are obtained by reusing the same
       * interval binder.  Type-preserving: transp <i>A(i) a : A(i1) and transp <i>B(i) b :
       * B(i1), so the pair inhabits Sigma A(i1) B(i1) = line@i1.  Restricted to the
       * non-dependent case so that B's sub-line is well-formed without dependency surgery;
       * a dependent Sigma stays NEUTRAL. */
      if (line->data.plam.body->tag == KT_SIGMA) {
        kterm_t *sig = line->data.plam.body;
        kterm_t *snd = sig->data.sigma.snd_type;
        /* non-dependence test: the pair binder (de Bruijn index 0 inside snd_type) does
         * not occur — equivalently, substituting it with a dummy leaves snd_type unchanged.
         * We detect it by checking that snd_type is invariant under shifting index 0, using
         * the existing kt_equal on the i0/i1 specialisations of a candidate B-line below.
         * Concretely: form B's line by lowering snd_type out of the pair binder via subst
         * with a placeholder; if snd has no index-0 occurrence this is exact. */
        kterm_t *wbase = kt_whnf(heap, ctx, t->data.transp.base);
        if (wbase->tag == KT_PAIR) {
          /* Build A-line = <i> fst_type.  fst_type is already under just the interval
           * binder, so the plam wrapper restores it directly. */
          kterm_t *a_line = kt_plam(heap, line->data.plam.name, sig->data.sigma.fst_type);
          /* Build B-line = <i> (snd_type with the pair binder removed).  For a non-dependent
           * Sigma, snd_type does not reference index 0 (the pair var); substituting index 0
           * with i0 (an arbitrary closed term of no consequence here) and then it is the
           * body under the interval binder.  We guard non-dependence by requiring that this
           * substitution is a no-op up to kt_equal at both endpoints. */
          kterm_t *snd_lowered = kt_subst(heap, snd, 0, kt_i0(heap));
          kterm_t *snd_check   = kt_subst(heap, snd, 0, kt_i1(heap));
          if (kt_equal(heap, ctx, snd_lowered, snd_check)) {
            /* non-dependent: B(i) does not vary with the pair var, so snd_lowered = B(i) */
            kterm_t *b_line = kt_plam(heap, line->data.plam.name, snd_lowered);
            kterm_t *new_fst = kt_transp(heap, a_line, wbase->data.pair.fst);
            kterm_t *new_snd = kt_transp(heap, b_line, wbase->data.pair.snd);
            kterm_t *res = kt_alloc(heap, KT_PAIR);
            res->data.pair.fst = new_fst;
            res->data.pair.snd = new_snd;
            return kt_whnf(heap, ctx, res);
          }
          /* DEPENDENT Sigma: B(i, x) genuinely depends on the first component x.  Transport is still
           * componentwise in the first slot, but the second component must be transported along a
           * line of B-types whose x-argument follows the FIRST component as it moves — the transport
           * FILLER q(i) = transp <k>A(k /\ i) a (a path in A(i) from a at i0 to a' at i1, using the
           * interval meet).  Then:
           *   transp <i>(Sigma (x:A(i)) B(i,x)) (a,b) = ( transp <i>A(i) a , transp <i>B(i,q(i)) b ).
           * Type-preserving: q(i) : A(i) with q(i0)=a, q(i1)=a' (the meet boundaries k/\i0=i0,
           * k/\i1=k make these definitional), so the B-line <i>B(i,q(i)) runs from B(i0,a) (where b
           * lives) to B(i1,a') (where the second component must land), and the pair inhabits
           * Sigma (x:A(i1)) B(i1,x).  This delegates entirely to the proven transp + the sound meet;
           * no Glue, no comp correction. */
          {
            /* a_line = <i>A(i) ; fst_type is under just the interval binder. */
            kterm_t *new_fst = kt_transp(heap, a_line, wbase->data.pair.fst);
            /* Build the filler-as-function-of-i and the B-line under the interval binder.
             * Contexts: ambient = ctx; under the i-binder, index 0 = i.
             *  - a (the pair's first component) is at ambient, lift past the i-binder: shift +1 @ 0.
             *  - filler q-line body, under (i, k): <k> A(k /\ i).  fst_type lives under the i-binder
             *    (index 0 = i); to put its interval argument at (k /\ i) under (i,k): shift fst_type
             *    by +1 at cutoff 1 (lift i and ambient past the new k-binder, keep index 0 as the
             *    placeholder), then substitute index 0 with (imeet k i) = (imeet #0 #1). */
            kterm_t *a_lift_i  = kt_shift(heap, wbase->data.pair.fst, 0, 1);   /* a under <i> */
            kterm_t *fst_shift = kt_shift(heap, sig->data.sigma.fst_type, 1, 1); /* A under <i,k>, idx0=placeholder */
            kterm_t *meet_ki   = kt_imeet(heap, kt_var(heap, 0), kt_var(heap, 1)); /* k /\ i under <i,k> */
            kterm_t *fill_body = kt_subst(heap, fst_shift, 0, meet_ki);        /* A(k /\ i) under <i,k> */
            kterm_t *q_line    = kt_plam(heap, "k", fill_body);                /* <k>A(k/\i), under <i> */
            kterm_t *q_of_i    = kt_transp(heap, q_line, a_lift_i);            /* q(i) : A(i), under <i> */
            /* B-line body = snd_type[x := q(i)].  snd_type under (i, x): idx0=x, idx1=i.  q_of_i is
             * under <i> (idx0=i); substituting it for x lowers i to idx0, matching <i>. */
            kterm_t *b_line_body = kt_subst(heap, snd, 0, q_of_i);
            kterm_t *b_line = kt_plam(heap, line->data.plam.name, b_line_body); /* <i>B(i,q(i)) */
            kterm_t *new_snd = kt_transp(heap, b_line, wbase->data.pair.snd);
            kterm_t *res = kt_alloc(heap, KT_PAIR);
            res->data.pair.fst = new_fst;
            res->data.pair.snd = new_snd;
            return kt_whnf(heap, ctx, res);
          }
        }
      }
      /* STRUCTURAL RULE — Path-type-line: transport over a line of PATH types is the genuine
       * disjunction-system Kan composition, expressed with comp2.
       *   transp <i>(Path A(i) u(i) v(i)) p
       *     =  <j> comp2 <i>A(i) [(cofib j i0) -> <i>u(i)] [(cofib j i1) -> <i>v(i)] (p@j).
       * The result is a path in A(i1) from u(i1) to v(i1): at j=i0 the first face is held so comp2
       * gives u(i1); at j=i1 the second is held giving v(i1); in the interior both faces are empty so
       * comp2 is transp <i>A(i) (p@j) — and for an abstract A the whole thing stays a well-typed
       * neutral path whose endpoints nonetheless compute.  Type-preserving: comp2's typing checks the
       * sections u(i),v(i):A(i) and the face compatibilities (which hold by the path-endpoint rule
       * p@i0=u(i0), p@i1=v(i0)), and yields A(i1) under the j-binder, so the plam inhabits
       * Path A(i1) u(i1) v(i1) = line@i1.  This delegates to the proven comp2 brick; no value is
       * guessed (the interior is neutral when A is abstract). */
      if (line->data.plam.body->tag == KT_PATH) {
        kterm_t *pth = line->data.plam.body;       /* Path A(i) u(i) v(i), under <i> (idx0 = i) */
        kterm_t *wp = kt_whnf(heap, ctx, t->data.transp.base);
        /* A-line <i>A(i): the path's carrier is under <i>; rebuild the plam directly. */
        kterm_t *a_line = kt_plam(heap, line->data.plam.name, pth->data.path.type);
        /* partials and base lifted under the OUTER j-binder.  Under <j,i>: i=idx0, j=idx1, amb=idx2+.
         * The path's subterms live under <i> (i=idx0, amb=idx1+); lift their ambient past the new
         * j-binder with shift(.,1,1) (keep idx0=i, push idx1+ to idx2+).  The base p lives at ambient;
         * lift it past the j-binder with shift(.,0,1) and apply it to j = idx0-under-<j>. */
        kterm_t *u_lift = kt_shift(heap, pth->data.path.a, 1, 1);   /* u(i) under <j,i> */
        kterm_t *v_lift = kt_shift(heap, pth->data.path.b, 1, 1);   /* v(i) under <j,i> */
        kterm_t *a_lift = kt_shift(heap, pth->data.path.type, 1, 1);/* A(i) under <j,i> */
        kterm_t *a_line_j = kt_plam(heap, line->data.plam.name, a_lift); /* <i>A(i) under <j> */
        kterm_t *p1 = kt_plam(heap, line->data.plam.name, u_lift);  /* <i>u(i) under <j> */
        kterm_t *p2 = kt_plam(heap, line->data.plam.name, v_lift);  /* <i>v(i) under <j> */
        kterm_t *cof1 = kt_cofib(heap, kt_var(heap, 0), kt_i0(heap)); /* (cofib j i0) */
        kterm_t *cof2 = kt_cofib(heap, kt_var(heap, 0), kt_i1(heap)); /* (cofib j i1) */
        kterm_t *p_lift = kt_shift(heap, wp, 0, 1);                 /* p under <j> */
        kterm_t *base_j = kt_papp(heap, p_lift, kt_var(heap, 0));   /* p@j */
        kterm_t *body = kt_comp2(heap, a_line_j, cof1, p1, cof2, p2, base_j);
        kterm_t *res = kt_plam(heap, "j", body);
        (void)a_line;
        return kt_whnf(heap, ctx, res);
      }
      /* STRUCTURAL RULE — Sum (coproduct): transport pushes inside the constructor.
       *   transp <i>(A(i) + B(i)) (inl a) = inl (transp <i>A(i) a)
       *   transp <i>(A(i) + B(i)) (inr b) = inr (transp <i>B(i) b)
       * A Sum is always NON-dependent (no binder in A + B), so no dependency guard is
       * needed.  Type-preserving: inl a : A(i0)+B(i0), and inl (transp <i>A(i) a) :
       * A(i1)+B(i1) = line@i1.  Needs no Glue. */
      if (line->data.plam.body->tag == KT_SUM_K) {
        kterm_t *sum = line->data.plam.body;
        kterm_t *wbase = kt_whnf(heap, ctx, t->data.transp.base);
        if (wbase->tag == KT_INL) {
          kterm_t *a_line = kt_plam(heap, line->data.plam.name, sum->data.sum_k.left_type);
          kterm_t *r = kt_alloc(heap, KT_INL);
          r->data.inl.value = kt_transp(heap, a_line, wbase->data.inl.value);
          /* the right_type at the i1 endpoint */
          r->data.inl.right_type = kt_subst(heap, sum->data.sum_k.right_type, 0, kt_i1(heap));
          return kt_whnf(heap, ctx, r);
        }
        if (wbase->tag == KT_INR) {
          kterm_t *b_line = kt_plam(heap, line->data.plam.name, sum->data.sum_k.right_type);
          kterm_t *r = kt_alloc(heap, KT_INR);
          r->data.inr.value = kt_transp(heap, b_line, wbase->data.inr.value);
          r->data.inr.left_type = kt_subst(heap, sum->data.sum_k.left_type, 0, kt_i1(heap));
          return kt_whnf(heap, ctx, r);
        }
      }
      /* STRUCTURAL RULE — Pi with a CONSTANT DOMAIN: transport acts on the codomain only.
       *   transp <i>(Pi (x:A) B(i)) f = lam (x:A). transp <i>B(i) (f x)   [A constant in i]
       * This is the slice of Pi-transport that needs NO interval reversal: because the domain
       * A does not vary, an argument x : A(i1) = A(i0) feeds f directly (no backward transport).
       * Restricted to (1) constant domain and (2) non-dependent codomain (B not depending on x),
       * so the codomain sub-line is well-formed without dependency surgery.  Type-preserving:
       * (f x) : B(i0), transp <i>B(i) (f x) : B(i1), so the lam : Pi (x:A) B(i1) = line@i1.
       * The full varying-domain Pi (which needs interval negation, absent from this kernel) stays
       * NEUTRAL and is named as roadmap. */
      if (line->data.plam.body->tag == KT_PI) {
        kterm_t *pi = line->data.plam.body;
        kterm_t *dom = pi->data.pi.domain;
        kterm_t *cod = pi->data.pi.codomain;
        /* domain constant in the interval? compare its i0 and i1 specialisations.
         * dom lives under the interval binder (index 0 = i), so substitute i0/i1. */
        kterm_t *dom_i0 = kt_subst(heap, dom, 0, kt_i0(heap));
        kterm_t *dom_i1 = kt_subst(heap, dom, 0, kt_i1(heap));
        int dom_constant = kt_equal(heap, ctx, dom_i0, dom_i1);
        /* codomain non-dependent in x? cod lives under (interval binder, then x binder):
         * index 0 = x, index 1 = i.  Non-dependence in x: substituting index 0 is a no-op. */
        kterm_t *cod_lo = kt_subst(heap, cod, 0, kt_i0(heap));
        kterm_t *cod_ck = kt_subst(heap, cod, 0, kt_i1(heap));
        if (kt_equal(heap, ctx, cod_lo, cod_ck)) {
          /* cod_lo is now B(i) free of the x-binder, under the interval binder.
           * The result lambda binds x : dom@i1 (the i1 endpoint of the domain).
           * The codomain sub-line is <i> B(i), and the argument fed to f is x transported
           * BACKWARD along the domain: transp <i>(dom[i:=~i]) x.  When the domain is
           * constant, dom[i:=~i] is also constant and that backward transport reduces to x,
           * recovering the constant-domain rule; when the domain varies, interval negation
           * supplies the reversal.  Type-preserving:
           *   x : dom@i1; transp <i>(dom[~i]) x : (dom[~i])@i1 = dom@i0; (f .) : B(i0);
           *   transp <i>B(i) (f .) : B(i1); so lam (x:dom@i1). . : Pi (x:dom@i1) B(i1) = line@i1.
           * Restricted to a non-dependent codomain so the B-line is well-formed. */
          kterm_t *b_line = kt_plam(heap, line->data.plam.name, cod_lo);
          kterm_t *f_shifted = kt_shift(heap, t->data.transp.base, 0, 1); /* under the new lam */
          kterm_t *arg_for_f;
          if (dom_constant) {
            /* constant domain: feed x directly (backward transport is the identity) */
            arg_for_f = kt_var(heap, 0);
          } else {
            /* varying domain: reverse the domain line via interval negation and transport x back.
             * The reversed domain body is dom with the interval var (index 0 inside the plam)
             * replaced by its negation.  Build it by substituting index 0 := ~(var 0):
             * but we must keep the binder, so construct <i> (dom[i := ~i]).  We do this by
             * substituting the bound interval occurrence with its negation under a fresh plam. */
            kterm_t *dom_shifted = kt_shift(heap, dom, 1, 1);  /* make room for the reversal binder */
            kterm_t *neg_i = kt_ineg(heap, kt_var(heap, 0));
            kterm_t *dom_rev_body = kt_subst(heap, dom_shifted, 0, neg_i); /* dom[i := ~i] */
            kterm_t *rev_line = kt_plam(heap, line->data.plam.name, dom_rev_body);
            kterm_t *rev_line_s = kt_shift(heap, rev_line, 0, 1); /* under the new lam */
            arg_for_f = kt_transp(heap, rev_line_s, kt_var(heap, 0)); /* transp <i>dom[~i] x */
          }
          {
            kterm_t *applied = kt_app(heap, f_shifted, arg_for_f);  /* (f arg) */
            kterm_t *b_line_s = kt_shift(heap, b_line, 0, 1);       /* B-line under the lam */
            kterm_t *transported = kt_transp(heap, b_line_s, applied);
            kterm_t *res = kt_alloc(heap, KT_LAM);
            res->data.lam.name = pi->data.pi.name;
            res->data.lam.domain = dom_i1;  /* the result lambda binds x : dom@i1 */
            res->data.lam.body = transported;
            return kt_whnf(heap, ctx, res);
          }
        }
      }
    }
    /* GLUE TRANSP RULE — the held-face (regularity) case.  When the line is
     *   <i> (Glue A(i) (cofib r b) T(i) e(i))
     * and the cofibration's face HOLDS THROUGHOUT (r ≡ b definitionally, independent of i),
     * the Glue boundary rule gives Glue ... ≡ T(i) at every point, so the Glue line is
     * definitionally the line <i> T(i).  Transport along it therefore reduces to transport
     * along <i> T(i) — which the existing transp machinery handles.  This needs NO inverse,
     * NO hcomp correction, and NO fabrication of the (opaque) equivalence's components.
     * Type-preserving: the Glue line equals the T line definitionally, and transp over equal
     * lines is equal; the T-line transp is already sound.
     * The general Glue transp (varying/empty face, requiring the equivalence inverse + an
     * hcomp boundary correction) stays NEUTRAL — the named final frontier. */
    if (line->tag == KT_PLAM && line->data.plam.body->tag == KT_GLUE) {
      kterm_t *gl = line->data.plam.body;
      kterm_t *cof = kt_whnf(heap, ctx, gl->data.glue.cof);
      if (cof->tag == KT_COFIB) {
        kterm_t *r = kt_whnf(heap, ctx, cof->data.cofib.var);
        kterm_t *b = kt_whnf(heap, ctx, cof->data.cofib.endpoint);
        /* face held throughout: r ≡ b and both are interval ENDPOINTS (so the face does not
         * depend on the transported interval variable — a constant, always-true cofibration) */
        if (kt_equal(heap, ctx, r, b) &&
            (r->tag == KT_I0 || r->tag == KT_I1) &&
            (b->tag == KT_I0 || b->tag == KT_I1)) {
          /* rewrite to transport along the underlying type line <i> T(i) */
          kterm_t *t_line = kt_plam(heap, line->data.plam.name, gl->data.glue.glue_type);
          return kt_whnf(heap, ctx, kt_transp(heap, t_line, t->data.transp.base));
        }
        /* GENERAL GLUE TRANSPORT (the univalence computation) — when the face does NOT depend on the
         * transported interval variable (it is constant in i, the standard CCHM precondition), build
         *   transp <i>(Glue A(i) phi T(i) e(i)) g0 = glue A(i1) phi T(i1) e(i1) t1 a1,  where
         *   t1 = transp <i>T(i) g0   (the T-component, a partial element relevant on phi), and
         *   a1 = comp <i>A(i) [phi -> <i> equiv-fun(e(i), transp <k>T(k/\i) g0)] (unglue .. g0).
         * The glue's coherence (equiv-fun e(i1)) t1 == a1 holds because a1 restricted to phi reduces
         * (held-face i-varying comp) to exactly equiv-fun(e(i1), t1).  The partial leg and t1 are
         * PARTIAL elements (well-typed only on phi); comp's and glue's typing now check them under the
         * face restriction, so the whole result type-checks.  On an undecided phi the result is a
         * well-typed glue element whose components are neutral; substituting phi to an endpoint makes
         * it compute (off phi -> the bare A-line transport, on phi -> t1).  The face being constant in
         * i means r,b do not reference the line binder: shifting them out by -1 at cutoff 0 is exact. */
        {
          kterm_t *Abody = gl->data.glue.base_type;   /* A(i) under <i> */
          kterm_t *Tbody = gl->data.glue.glue_type;   /* T(i) under <i> */
          kterm_t *Ebody = gl->data.glue.equiv;       /* e(i) under <i> */
          /* face must be constant in i: r,b have no occurrence of index 0.  Detect by checking the
           * i0- and i1-specialisations agree (no dependence on the binder). */
          kterm_t *r_i0 = kt_subst(heap, cof->data.cofib.var, 0, kt_i0(heap));
          kterm_t *r_i1 = kt_subst(heap, cof->data.cofib.var, 0, kt_i1(heap));
          kterm_t *b_i0 = kt_subst(heap, cof->data.cofib.endpoint, 0, kt_i0(heap));
          kterm_t *b_i1 = kt_subst(heap, cof->data.cofib.endpoint, 0, kt_i1(heap));
          if (kt_equal(heap, ctx, r_i0, r_i1) && kt_equal(heap, ctx, b_i0, b_i1)) {
            kterm_t *g0 = t->data.transp.base;
            const char *nm = line->data.plam.name;
            /* phi with the binder removed (constant in i): shift r,b down past the gone binder */
            kterm_t *rC = kt_shift(heap, cof->data.cofib.var, 0, -1);
            kterm_t *bC = kt_shift(heap, cof->data.cofib.endpoint, 0, -1);
            kterm_t *phiC = kt_cofib(heap, rC, bC);                 /* phi in ctx */
            /* endpoint specialisations (in ctx) */
            kterm_t *A_i1 = kt_subst(heap, Abody, 0, kt_i1(heap));
            kterm_t *T_i1 = kt_subst(heap, Tbody, 0, kt_i1(heap));
            kterm_t *E_i1 = kt_subst(heap, Ebody, 0, kt_i1(heap));
            kterm_t *A_i0 = kt_subst(heap, Abody, 0, kt_i0(heap));
            kterm_t *T_i0 = kt_subst(heap, Tbody, 0, kt_i0(heap));
            kterm_t *E_i0 = kt_subst(heap, Ebody, 0, kt_i0(heap));
            /* lines (in ctx) */
            kterm_t *A_line = kt_plam(heap, nm, Abody);             /* <i>A(i) */
            kterm_t *T_line = kt_plam(heap, nm, Tbody);             /* <i>T(i) */
            kterm_t *t1 = kt_transp(heap, T_line, g0);              /* transp <i>T(i) g0 : T(i1) */
            /* the comp partial body, under <i>:  equiv-fun(e(i), transp <k>T(k/\i) g0) : A(i) on phi.
             * Under <i,k>: e(i),T,g0 lifted past the k-binder (shift +1 @ cutoff1 for i-scope terms,
             * +1 @ cutoff0 then the meet for the filler). */
            kterm_t *Tbody_ik = kt_shift(heap, Tbody, 1, 1);       /* T under <i,k> */
            kterm_t *meet_ki = kt_imeet(heap, kt_var(heap, 0), kt_var(heap, 1)); /* k/\i */
            kterm_t *T_kfill_body = kt_subst(heap, Tbody_ik, 0, meet_ki);        /* T(k/\i) under <i,k> */
            kterm_t *T_kfill = kt_plam(heap, "k", T_kfill_body);   /* <k>T(k/\i) under <i> */
            kterm_t *g0_i = kt_shift(heap, g0, 0, 1);              /* g0 under <i> */
            kterm_t *fillT = kt_transp(heap, T_kfill, g0_i);       /* transp<k>T(k/\i) g0 : T(i) on phi, under <i> */
            kterm_t *efun_i = kt_equiv_fun(heap, Ebody);           /* equiv-fun e(i) under <i> */
            kterm_t *part_body = kt_app(heap, efun_i, fillT);      /* equiv-fun(e(i), fillT) under <i> */
            kterm_t *partial = kt_plam(heap, nm, part_body);       /* <i> .. */
            /* base of the comp: unglue at i0 of g0 (in ctx) */
            kterm_t *r0s = kt_subst(heap, cof->data.cofib.var, 0, kt_i0(heap));
            kterm_t *b0s = kt_subst(heap, cof->data.cofib.endpoint, 0, kt_i0(heap));
            kterm_t *phi0 = kt_cofib(heap, kt_shift(heap, r0s, 0, -1), kt_shift(heap, b0s, 0, -1));
            kterm_t *ug = kt_unglue(heap, A_i0, phi0, T_i0, E_i0, g0);  /* unglue .. g0 : A(i0) */
            kterm_t *a1 = kt_comp(heap, A_line, phiC, partial, ug);     /* comp .. : A(i1) */
            kterm_t *res = kt_glue_elem(heap, A_i1, phiC, T_i1, E_i1, t1, a1);
            (void)r; (void)b;
            return kt_whnf(heap, ctx, res);
          }
        }
      }
    }
    /* otherwise neutral (correctly typed, but not reduced) */
    base = kt_whnf(heap, ctx, t->data.transp.base);
    if (line != t->data.transp.line || base != t->data.transp.base)
      return kt_transp(heap, line, base);
    return t;
  }
  case KT_INEG: {
    /* interval negation: ~i0 = i1, ~i1 = i0, ~(~r) = r (involution); else neutral.
     * These rules are total and finite and only rearrange interval endpoints. */
    kterm_t *r = kt_whnf(heap, ctx, t->data.ineg.arg);
    if (r->tag == KT_I0) return kt_i1(heap);
    if (r->tag == KT_I1) return kt_i0(heap);
    if (r->tag == KT_INEG) return kt_whnf(heap, ctx, r->data.ineg.arg);  /* ~~r = r */
    if (r != t->data.ineg.arg) return kt_ineg(heap, r);
    return t;
  }
  case KT_IMEET: {
    /* interval meet (min) — the distributive-lattice laws on endpoints:
     *   i0 /\ x = i0,  x /\ i0 = i0  (i0 is bottom: meet with bottom is bottom)
     *   i1 /\ x = x,   x /\ i1 = x   (i1 is top: meet with top is the other)
     *   x /\ x = x                   (idempotent)
     * else neutral.  Total and finite; only rearranges interval endpoints, never invents one. */
    kterm_t *l = kt_whnf(heap, ctx, t->data.imeet.left);
    kterm_t *r = kt_whnf(heap, ctx, t->data.imeet.right);
    if (l->tag == KT_I0 || r->tag == KT_I0) return kt_i0(heap);   /* bottom absorbs */
    if (l->tag == KT_I1) return r;                                 /* top is identity */
    if (r->tag == KT_I1) return l;
    if (kt_equal(heap, ctx, l, r)) return l;                       /* idempotent */
    if (l != t->data.imeet.left || r != t->data.imeet.right) return kt_imeet(heap, l, r);
    return t;
  }
  case KT_IJOIN: {
    /* interval join (max) — dual of meet:
     *   i1 \/ x = i1,  x \/ i1 = i1  (i1 is top: join with top is top)
     *   i0 \/ x = x,   x \/ i0 = x   (i0 is bottom: join with bottom is the other)
     *   x \/ x = x                   (idempotent)
     * else neutral.  Total and finite; only rearranges interval endpoints. */
    kterm_t *l = kt_whnf(heap, ctx, t->data.ijoin.left);
    kterm_t *r = kt_whnf(heap, ctx, t->data.ijoin.right);
    if (l->tag == KT_I1 || r->tag == KT_I1) return kt_i1(heap);   /* top absorbs */
    if (l->tag == KT_I0) return r;                                 /* bottom is identity */
    if (r->tag == KT_I0) return l;
    if (kt_equal(heap, ctx, l, r)) return l;                       /* idempotent */
    if (l != t->data.ijoin.left || r != t->data.ijoin.right) return kt_ijoin(heap, l, r);
    return t;
  }
  case KT_ID_EQUIV: {
    /* id-equiv is a value (the identity equivalence); reduce its type argument only. */
    kterm_t *a = kt_whnf(heap, ctx, t->data.id_equiv.type);
    if (a != t->data.id_equiv.type) return kt_id_equiv(heap, a);
    return t;
  }
  case KT_HCOMP:
    /* empty-system homogeneous composition is the base: hcomp A u0 = u0. */
    return kt_whnf(heap, ctx, t->data.hcomp.base);
  case KT_COFIB: {
    /* a cofibration reduces its components; it is a constraint marker (no further computation). */
    kterm_t *r = kt_whnf(heap, ctx, t->data.cofib.var);
    kterm_t *b = kt_whnf(heap, ctx, t->data.cofib.endpoint);
    if (r != t->data.cofib.var || b != t->data.cofib.endpoint) return kt_cofib(heap, r, b);
    return t;
  }
  case KT_COFIB_OR: {
    /* disjunction of cofibrations.  HELD if either operand is held (its (cofib r b) has r==b); EMPTY
     * if BOTH operands are empty (each has distinct constant endpoints).  Otherwise reduce the
     * operands and stay neutral.  This is the cofibration-lattice join; whnf only normalises here —
     * decisions are read off by the consumers (comp/hcomp), exactly as for a single (cofib ..). */
    kterm_t *c1 = kt_whnf(heap, ctx, t->data.cofib_or.cof1);
    kterm_t *c2 = kt_whnf(heap, ctx, t->data.cofib_or.cof2);
    if (c1 != t->data.cofib_or.cof1 || c2 != t->data.cofib_or.cof2) return kt_cofib_or(heap, c1, c2);
    return t;
  }
  case KT_COFIB_FORALL: {
    /* forall-face ∀i.phi(i).  We normalise the body under its binder and CONSERVATIVELY collapse it to
     * a decided face only when certain (soundness over completeness):
     *   - if phi does NOT mention the bound i (lift-back test) and the lowered phi is HELD  -> held;
     *   - if phi is reflexive (cofib r r) for syntactically equal r                          -> held;
     *   - if phi at i:=i0 is decided-EMPTY, or phi at i:=i1 is decided-EMPTY                  -> empty
     *       (a face empty at an endpoint cannot hold for ALL i);
     *   - otherwise stay a (normalised) cofib-forall, neutral.
     * Collapsing to a decided (cofib i1 i1)/(cofib i0 i1) lets consumers read the decision off. */
    kctx_t *ictx = kctx_extend(heap, ctx, "i", kt_interval(heap), NULL);
    kterm_t *body = kt_whnf(heap, ictx, t->data.cofib_forall.body);
    /* constant-in-i held: lower across the binder, require lift-back equality, then decide */
    {
      kterm_t *low = kt_shift(heap, body, 0, -1);
      if (kt_equal(heap, ictx, kt_shift(heap, low, 0, 1), body)) {
        kterm_t *lw = kt_whnf(heap, ctx, low);
        if (lw->tag == KT_COFIB) {
          kterm_t *r = kt_whnf(heap, ctx, lw->data.cofib.var);
          kterm_t *b = kt_whnf(heap, ctx, lw->data.cofib.endpoint);
          if (kt_equal(heap, ctx, r, b)) return kt_cofib(heap, kt_i1(heap), kt_i1(heap)); /* held */
          if ((r->tag == KT_I0 && b->tag == KT_I1) || (r->tag == KT_I1 && b->tag == KT_I0))
            return kt_cofib(heap, kt_i0(heap), kt_i1(heap)); /* empty */
        }
      }
    }
    /* reflexive face (cofib r r): held for all i */
    if (body->tag == KT_COFIB && kt_equal(heap, ictx, body->data.cofib.var, body->data.cofib.endpoint))
      return kt_cofib(heap, kt_i1(heap), kt_i1(heap));
    /* empty at an endpoint => fails for some i => the forall is empty */
    if (body->tag == KT_COFIB) {
      kterm_t *b0 = kt_whnf(heap, ctx, kt_subst(heap, body, 0, kt_i0(heap)));
      kterm_t *b1 = kt_whnf(heap, ctx, kt_subst(heap, body, 0, kt_i1(heap)));
      if (b0->tag == KT_COFIB) {
        kterm_t *r = kt_whnf(heap, ctx, b0->data.cofib.var), *e = kt_whnf(heap, ctx, b0->data.cofib.endpoint);
        if ((r->tag == KT_I0 && e->tag == KT_I1) || (r->tag == KT_I1 && e->tag == KT_I0))
          return kt_cofib(heap, kt_i0(heap), kt_i1(heap)); /* empty */
      }
      if (b1->tag == KT_COFIB) {
        kterm_t *r = kt_whnf(heap, ctx, b1->data.cofib.var), *e = kt_whnf(heap, ctx, b1->data.cofib.endpoint);
        if ((r->tag == KT_I0 && e->tag == KT_I1) || (r->tag == KT_I1 && e->tag == KT_I0))
          return kt_cofib(heap, kt_i0(heap), kt_i1(heap)); /* empty */
      }
    }
    if (body != t->data.cofib_forall.body) return kt_cofib_forall(heap, body);
    return t;
  }
  case KT_HCOMP1: {
    /* single-face homogeneous composition along (r = b):
     *   - on the face (r reduces to b): result is the partial element u (typing forced u=u0 here);
     *   - off the face (r reduces to the OTHER endpoint, an empty face): result is the base u0;
     *   - else neutral.  Every branch returns a term already of type A, so type-preserving. */
    kterm_t *cof = kt_whnf(heap, ctx, t->data.hcomp1.cof);
    if (cof->tag == KT_COFIB || cof->tag == KT_COFIB_OR) {
      /* face holds (for a disjunction: either disjunct holds): result is the partial */
      if (cofib_decided_held(heap, ctx, cof))
        return kt_whnf(heap, ctx, t->data.hcomp1.partial);
      /* empty face (for a disjunction: both disjuncts empty): result is the base */
      if (cofib_decided_empty(heap, ctx, cof))
        return kt_whnf(heap, ctx, t->data.hcomp1.base);
    }
    return t;  /* neutral */
  }
  case KT_HCOMP2: {
    /* two-face homogeneous composition along (r1=b1) ∨ (r2=b2):
     *   - face1 holds (r1=b1): result is u1;
     *   - else face2 holds (r2=b2): result is u2;
     *   - else BOTH faces empty (distinct endpoints each): result is the base u0;
     *   - else neutral.
     * Type-preserving: every branch returns u1/u2/u0, each : A.  The elif order (face1
     * before face2) is sound because the typing OVERLAP check forces u1=u2 wherever both
     * faces hold, so choosing u1 over u2 on the overlap is choosing a definitionally equal
     * term. */
    kterm_t *c1 = kt_whnf(heap, ctx, t->data.hcomp2.cof1);
    kterm_t *c2 = kt_whnf(heap, ctx, t->data.hcomp2.cof2);
    int f1_holds = 0, f1_empty = 0, f2_holds = 0, f2_empty = 0;
    if (c1->tag == KT_COFIB) {
      kterm_t *r = kt_whnf(heap, ctx, c1->data.cofib.var);
      kterm_t *b = kt_whnf(heap, ctx, c1->data.cofib.endpoint);
      if (kt_equal(heap, ctx, r, b)) f1_holds = 1;
      else if ((r->tag == KT_I0 && b->tag == KT_I1) || (r->tag == KT_I1 && b->tag == KT_I0)) f1_empty = 1;
    }
    if (c2->tag == KT_COFIB) {
      kterm_t *r = kt_whnf(heap, ctx, c2->data.cofib.var);
      kterm_t *b = kt_whnf(heap, ctx, c2->data.cofib.endpoint);
      if (kt_equal(heap, ctx, r, b)) f2_holds = 1;
      else if ((r->tag == KT_I0 && b->tag == KT_I1) || (r->tag == KT_I1 && b->tag == KT_I0)) f2_empty = 1;
    }
    if (f1_holds) return kt_whnf(heap, ctx, t->data.hcomp2.partial1);
    if (f2_holds) return kt_whnf(heap, ctx, t->data.hcomp2.partial2);
    if (f1_empty && f2_empty) return kt_whnf(heap, ctx, t->data.hcomp2.base);
    return t;  /* neutral */
  }
  case KT_GLUE: {
    /* Glue boundary rules:
     *   - on the face (r=b): (Glue A cof T e) = T;
     *   - on an EMPTY face (r,b distinct endpoints i0/i1): (Glue A cof T e) = A
     *     (when the face never holds, the glue contributes nothing and the type is A);
     *   - else neutral.
     * Both are defining CCHM equations and type-preserving (T and A share the Glue's sort).
     * The empty-face boundary makes transp across an empty-face Glue line reduce — via the
     * existing transport tier — to ordinary transport in A, with no new transp code. */
    kterm_t *cof = kt_whnf(heap, ctx, t->data.glue.cof);
    if (cof->tag == KT_COFIB) {
      kterm_t *r = kt_whnf(heap, ctx, cof->data.cofib.var);
      kterm_t *b = kt_whnf(heap, ctx, cof->data.cofib.endpoint);
      if (kt_equal(heap, ctx, r, b))
        return kt_whnf(heap, ctx, t->data.glue.glue_type);  /* on the face: T */
      if ((r->tag == KT_I0 && b->tag == KT_I1) || (r->tag == KT_I1 && b->tag == KT_I0))
        return kt_whnf(heap, ctx, t->data.glue.base_type);  /* empty face: A */
    }
    return t;  /* neutral Glue */
  }
  case KT_PARTIAL: {
    /* Partial (cofib r b) A — the type of partial elements of A on the face:
     *   - on the face (r=b): a partial element on a true face is a total element, so = A;
     *   - on an EMPTY face (r,b distinct endpoints): the only partial element is the trivial one,
     *     so = Unit;
     *   - else neutral.
     * Type-preserving: A and Unit are both types in the ambient sort, and the boundary equations are
     * the defining CCHM facts for Partial. */
    kterm_t *cof = kt_whnf(heap, ctx, t->data.partial.cof);
    if (cof->tag == KT_COFIB) {
      kterm_t *r = kt_whnf(heap, ctx, cof->data.cofib.var);
      kterm_t *b = kt_whnf(heap, ctx, cof->data.cofib.endpoint);
      if (kt_equal(heap, ctx, r, b))
        return kt_whnf(heap, ctx, t->data.partial.type);   /* on the face: A */
      if ((r->tag == KT_I0 && b->tag == KT_I1) || (r->tag == KT_I1 && b->tag == KT_I0)) {
        kterm_t *u = kt_alloc(heap, KT_UNIT);
        return u;                                            /* empty face: Unit */
      }
    }
    return t;  /* neutral Partial */
  }
  case KT_PSYS: {
    /* psys (cofib r b) A a — a single-face system [phi -> a] : Partial (cofib r b) A.
     *   - on the face (r=b): the partial element on a true face is its value, so = a;
     *   - on an EMPTY face (r,b distinct endpoints): Partial = Unit, so = * (the trivial element);
     *   - else neutral.
     * Type-preserving: on the face a : A and Partial = A; on an empty face * : Unit and Partial = Unit. */
    kterm_t *cof = kt_whnf(heap, ctx, t->data.psys.cof);
    if (cof->tag == KT_COFIB) {
      kterm_t *r = kt_whnf(heap, ctx, cof->data.cofib.var);
      kterm_t *b = kt_whnf(heap, ctx, cof->data.cofib.endpoint);
      if (kt_equal(heap, ctx, r, b))
        return kt_whnf(heap, ctx, t->data.psys.elem);       /* on the face: the value a */
      if ((r->tag == KT_I0 && b->tag == KT_I1) || (r->tag == KT_I1 && b->tag == KT_I0)) {
        kterm_t *st = kt_alloc(heap, KT_STAR);
        return st;                                           /* empty face: * : Unit */
      }
    }
    return t;  /* neutral psys */
  }
  case KT_EQUIV_FUN: {
    /* equiv-fun (id-equiv A) = lam (x:A).x;  equiv-fun (mk-equiv _ _ f _ _ _) = f (beta).
     * A general/opaque equivalence stays neutral. */
    kterm_t *e = kt_whnf(heap, ctx, t->data.equiv_fun.equiv);
    if (e->tag == KT_ID_EQUIV) {
      kterm_t *lam = kt_alloc(heap, KT_LAM);
      lam->data.lam.name = "x";
      lam->data.lam.domain = e->data.id_equiv.type;
      lam->data.lam.body = kt_var(heap, 0);
      return lam;
    }
    if (e->tag == KT_MK_EQUIV) return kt_whnf(heap, ctx, e->data.mk_equiv.fwd);
    if (e != t->data.equiv_fun.equiv) return kt_equiv_fun(heap, e);
    return t;
  }
  case KT_EQUIV_INV: {
    /* equiv-inv (id-equiv A) = lam (x:A).x;  equiv-inv (mk-equiv _ _ _ g _ _) = g (beta).
     * A general/opaque equivalence stays neutral. */
    kterm_t *e = kt_whnf(heap, ctx, t->data.equiv_inv.equiv);
    if (e->tag == KT_ID_EQUIV) {
      kterm_t *lam = kt_alloc(heap, KT_LAM);
      lam->data.lam.name = "x";
      lam->data.lam.domain = e->data.id_equiv.type;
      lam->data.lam.body = kt_var(heap, 0);
      return lam;
    }
    if (e->tag == KT_MK_EQUIV) return kt_whnf(heap, ctx, e->data.mk_equiv.inv);
    if (e != t->data.equiv_inv.equiv) return kt_equiv_inv(heap, e);
    return t;
  }
  case KT_MK_EQUIV: {
    /* mk-equiv is a value (a packaged equivalence); reduce its components on demand. */
    kterm_t *f2 = kt_whnf(heap, ctx, t->data.mk_equiv.fwd);
    if (f2 != t->data.mk_equiv.fwd)
      return kt_mk_equiv(heap, t->data.mk_equiv.glue_type, t->data.mk_equiv.base_type,
                               f2, t->data.mk_equiv.inv, t->data.mk_equiv.eta, t->data.mk_equiv.eps);
    return t;
  }
  case KT_EQUIV_ETA: {
    /* equiv-eta (mk-equiv _ _ _ _ eta _) = eta (beta).  For id-equiv, eta is refl at the identity:
     * (x:A). refl x — but to keep the value simple and sound we return the stored proof for mk-equiv
     * and stay neutral for id-equiv (its eta is a propositional refl family, left abstract). */
    kterm_t *e = kt_whnf(heap, ctx, t->data.equiv_eta.equiv);
    if (e->tag == KT_MK_EQUIV) return kt_whnf(heap, ctx, e->data.mk_equiv.eta);
    if (e != t->data.equiv_eta.equiv) return kt_equiv_eta(heap, e);
    return t;
  }
  case KT_EQUIV_EPS: {
    /* equiv-eps (mk-equiv _ _ _ _ _ eps) = eps (beta).  Neutral otherwise. */
    kterm_t *e = kt_whnf(heap, ctx, t->data.equiv_eps.equiv);
    if (e->tag == KT_MK_EQUIV) return kt_whnf(heap, ctx, e->data.mk_equiv.eps);
    if (e != t->data.equiv_eps.equiv) return kt_equiv_eps(heap, e);
    return t;
  }
  case KT_UNGLUE: {
    /* unglue A (cofib r b) T e g : A.
     *   - off the face (r,b distinct endpoints): g is already in A, unglue g = g;
     *   - on the face (r=b): unglue g = (equiv-fun e) g  (apply the forward map to land in A);
     *     this reduces further only when equiv-fun reduces (e.g. e = id-equiv);
     *   - else neutral.
     * Type-preserving: off-face g : A; on-face (equiv-fun e) g : A since equiv-fun e : T -> A. */
    kterm_t *cof = kt_whnf(heap, ctx, t->data.unglue.cof);
    if (cof->tag == KT_COFIB) {
      kterm_t *r = kt_whnf(heap, ctx, cof->data.cofib.var);
      kterm_t *b = kt_whnf(heap, ctx, cof->data.cofib.endpoint);
      if (kt_equal(heap, ctx, r, b)) {
        /* on the face: apply the equivalence's forward map */
        kterm_t *fn = kt_equiv_fun(heap, t->data.unglue.equiv);
        return kt_whnf(heap, ctx, kt_app(heap, fn, t->data.unglue.arg));
      }
      if ((r->tag == KT_I0 && b->tag == KT_I1) || (r->tag == KT_I1 && b->tag == KT_I0))
        return kt_whnf(heap, ctx, t->data.unglue.arg);  /* off the face: identity */
    }
    /* beta: unglue (glue A cof T e u a) = a  (the A-component) — independent of the face, since the
     * glue's coherence guarantees a = (equiv-fun e) u on the face, matching the on-face unglue. */
    {
      kterm_t *g = kt_whnf(heap, ctx, t->data.unglue.arg);
      if (g->tag == KT_GLUE_ELEM)
        return kt_whnf(heap, ctx, g->data.glue_elem.base);
    }
    return t;  /* neutral */
  }
  case KT_GLUE_ELEM: {
    /* glue A (cofib r b) T e u a : Glue A (cofib r b) T e — the Glue introduction.
     *   - off the face (r,b distinct endpoints): the Glue type is just A, so glue = a;
     *   - on the face (r=b): the Glue type is T, so glue = u (and unglue then gives (equiv-fun e) u,
     *     which equals a by the coherence checked at typing);
     *   - else neutral. */
    kterm_t *cof = kt_whnf(heap, ctx, t->data.glue_elem.cof);
    if (cof->tag == KT_COFIB) {
      kterm_t *r = kt_whnf(heap, ctx, cof->data.cofib.var);
      kterm_t *b = kt_whnf(heap, ctx, cof->data.cofib.endpoint);
      if (kt_equal(heap, ctx, r, b))
        return kt_whnf(heap, ctx, t->data.glue_elem.member);   /* on the face: the T-component */
      if ((r->tag == KT_I0 && b->tag == KT_I1) || (r->tag == KT_I1 && b->tag == KT_I0))
        return kt_whnf(heap, ctx, t->data.glue_elem.base);     /* off the face: the A-component */
    }
    return t;  /* neutral */
  }
  case KT_GTRANSP: {
    /* Glue transport — the SOUND fragment.  Transport reduces to the base g0 whenever the
     * gluing equivalence's forward map is DEFINITIONALLY the identity, because then f(g0) = g0
     * and the CCHM correction hcomp is trivial.  This covers:
     *   - e = id-equiv (the identity-equivalence constructor), and
     *   - e = mk-equiv A A f g .. with f reducing to the identity lambda (an identity-packaged
     *     equivalence) — regularity extended from the constructor to any definitional identity.
     * For a general equivalence with a non-identity forward map and a proper (partial) face, the
     * correction requires comp composed with the section coherence eps; that term's definitional
     * correctness is not captured by a single typecheck, so gtransp stays NEUTRAL there — it never
     * guesses an inhabitant. */
    kterm_t *e = kt_whnf(heap, ctx, t->data.gtransp.equiv);
    if (e->tag == KT_ID_EQUIV)
      return kt_whnf(heap, ctx, t->data.gtransp.base);
    /* identity-packaged: forward map reduces to lam (x:_). x  (body = var 0) */
    {
      kterm_t *fwd = kt_whnf(heap, ctx, kt_equiv_fun(heap, t->data.gtransp.equiv));
      if (fwd->tag == KT_LAM) {
        kterm_t *body = kt_whnf(heap, ctx, fwd->data.lam.body);
        if (body->tag == KT_VAR && body->data.var.index == 0)
          return kt_whnf(heap, ctx, t->data.gtransp.base);
      }
    }
    /* EMPTY-FACE REGULARITY (sound for ANY equivalence): when the cofibration's face is empty
     * (the two interval endpoints are distinct, i0 vs i1), the Glue type degenerates to its base
     * A by the empty-face boundary rule — the gluing equivalence is absent there.  Since gtransp's
     * base type A is a single (constant-in-i) type, transport over the constant A-line is the
     * identity, so the result is exactly the base g0.  Crucially this holds REGARDLESS of e: the
     * equivalence never enters, so no coherence is needed and nothing is guessed.  This extends
     * regularity from the identity-equivalence case to a general equivalence on an empty face. */
    {
      kterm_t *cof = kt_whnf(heap, ctx, t->data.gtransp.cof);
      if (cof->tag == KT_COFIB) {
        kterm_t *r = kt_whnf(heap, ctx, cof->data.cofib.var);
        kterm_t *b = kt_whnf(heap, ctx, cof->data.cofib.endpoint);
        if ((r->tag == KT_I0 && b->tag == KT_I1) || (r->tag == KT_I1 && b->tag == KT_I0))
          return kt_whnf(heap, ctx, t->data.gtransp.base);
        /* HELD-FACE CORRECTION (sound only when f∘inv is DEFINITIONALLY the identity): when the
         * face is held (r=b), the Glue equals T, and the constant-line transport of g0:A landing
         * back in A is f(inv(g0)) — pull g0 into T via the inverse, transport over the constant
         * T-line (identity), push back via the forward map.  This term is always type-correct
         * (: A).  We only REDUCE it, however, when f(inv(g0)) reduces to g0 definitionally — i.e.
         * when the section coherence is definitional (eps = refl up to computation).  Then the
         * held-face value is unambiguously g0, with no propositional slack and no guessing: we use
         * only f and inv (both carried by the kernel), and the result is forced.  When f∘inv does
         * NOT collapse definitionally, the value is correct only up to the eps path, which is not a
         * definitional equality — so gtransp stays NEUTRAL there rather than committing a value the
         * coherence would only justify propositionally. */
        if (kt_equal(heap, ctx, r, b)) {
          kterm_t *fwd = kt_equiv_fun(heap, t->data.gtransp.equiv);
          kterm_t *inv = kt_equiv_inv(heap, t->data.gtransp.equiv);
          kterm_t *corrected = kt_app(heap, fwd, kt_app(heap, inv, t->data.gtransp.base));
          kterm_t *cw = kt_whnf(heap, ctx, corrected);
          if (kt_equal(heap, ctx, cw, t->data.gtransp.base))
            return kt_whnf(heap, ctx, t->data.gtransp.base);
          /* otherwise: f∘inv is not definitionally the identity — stay neutral (do not commit
           * a value justified only up to the eps path). */
        }
      }
    }
    return t;  /* neutral for a general equivalence on an unresolved / non-collapsing face */
  }
  case KT_COMP: {
    /* comp line (cofib r b) u u0 — heterogeneous composition along a type line A(i).
     * GENERALISES transp and hcomp1, reducing to whichever applies; the genuinely
     * heterogeneous case (varying line + nonempty face) stays NEUTRAL — never guessed.
     *   - EMPTY face (r,b distinct endpoints): no partial constraint, so comp over the line is
     *     exactly transport: comp = transp line u0  (delegates to the proven transp rule;
     *     covers both a constant line — transp reduces to u0 — and a structural varying line).
     *   - CONSTANT line (body at i0 ≡ body at i1) with the face NOT empty: the heterogeneous
     *     composition degenerates to homogeneous composition in the single fibre A:
     *     comp = hcomp1 A (cofib r b) u u0  (delegates to the proven hcomp1 rule).
     *   - otherwise NEUTRAL. */
    kterm_t *cline = kt_whnf(heap, ctx, t->data.comp.line);
    if (cline->tag == KT_PLAM) {
      kterm_t *ccof = kt_whnf(heap, ctx, t->data.comp.cof);
      int empty = 0;
      if (ccof->tag == KT_COFIB) {
        kterm_t *r = kt_whnf(heap, ctx, ccof->data.cofib.var);
        kterm_t *b = kt_whnf(heap, ctx, ccof->data.cofib.endpoint);
        if ((r->tag == KT_I0 && b->tag == KT_I1) || (r->tag == KT_I1 && b->tag == KT_I0))
          empty = 1;
      }
      if (empty) {
        /* empty face: comp is transport of the base along the line */
        return kt_whnf(heap, ctx, kt_transp(heap, cline, t->data.comp.base));
      }
      /* i-VARYING partial, HELD face.  When the partial is itself a path-abstraction <i>u(i) (a
       * genuine i-varying family, not a plain term) and the face is HELD (r ≡ b, always true), the
       * composite equals the partial at the i1 end: comp <i>A(i) [held -> <i>u(i)] u0 = u(i1).
       * Soundness: on a true face the result must equal the partial throughout, so it is the partial
       * line evaluated at i1; u(i1) = (partial line)@i1 : A(i1) by the line's own typing, and the
       * compatibility u(i0) ≡ u0 (checked by comp's typing) makes the i0 boundary agree with the
       * base.  This is the correct CCHM held-face behavior for an i-varying partial and is the forced
       * fragment of the genuine i-varying comp; the undecided-face case (the real disjunction-system
       * Kan composition) stays NEUTRAL below. */
      if (t->data.comp.partial->tag == KT_PLAM) {
        kterm_t *wcof2 = kt_whnf(heap, ctx, t->data.comp.cof);
        if (wcof2->tag == KT_COFIB) {
          kterm_t *rr = kt_whnf(heap, ctx, wcof2->data.cofib.var);
          kterm_t *bb = kt_whnf(heap, ctx, wcof2->data.cofib.endpoint);
          if (kt_equal(heap, ctx, rr, bb)) {
            /* partial line @ i1 = substitute i1 for the line's bound interval variable */
            return kt_whnf(heap, ctx,
              kt_subst(heap, t->data.comp.partial->data.plam.body, 0, kt_i1(heap)));
          }
        }
      }
      {
        /* constant-line test: body at i0 vs i1 (reduce under the interval binder first) */
        kctx_t *ictx = kctx_extend(heap, ctx, cline->data.plam.name, kt_interval(heap), NULL);
        kterm_t *rbody = kt_whnf(heap, ictx, cline->data.plam.body);
        kterm_t *at_i0 = kt_subst(heap, rbody, 0, kt_i0(heap));
        kterm_t *at_i1 = kt_subst(heap, rbody, 0, kt_i1(heap));
        if (kt_equal(heap, ctx, at_i0, at_i1)) {
          /* constant line.  If the fibre is a GLUE type with an UNDECIDED gluing cofibration, this is
           * the homogeneous Glue-composition (the hardest CCHM operation).  Build the CCHM result
           *   res = glue A psi T e t1 a1,  t1 = comp <k>T [phi -> u] g0  (T-component, partial on psi),
           *     a1 = comp2 <k>A [phi -> unglue u] [psi -> equiv-fun e (Tfill(k))] (unglue g0),
           *   Tfill(k) = comp2 <l>T [phi -> u] [(cofib k i0) -> g0] g0   (the composition filler).
           * In this single-cofibration representation the partial u and base u0 are i0-given (plain
           * terms), so u is constant along the composition direction and the damping i/\l is the
           * identity here.  The glue introduction's coherence ((equiv-fun e) t1 == a1 on psi) holds
           * because a1 restricted to psi reduces (held-face comp2) to equiv-fun e (Tfill@i1) =
           * equiv-fun e t1; the overlap between a1's phi-leg (unglue u = equiv-fun e u on psi) and
           * psi-leg agrees because Tfill = u on phi.  All of t1, a1, and res TYPE-CHECK (verified at
           * the surface) via the face-restricted typing; on an undecided psi the result is a
           * well-typed glue element with neutral components, and on a DECIDED psi the Glue type itself
           * whnf's to A or T first (so this branch only fires for genuinely-undecided psi). */
          kterm_t *fib = kt_whnf(heap, ctx, at_i1);
          if (fib->tag == KT_GLUE) {
            kterm_t *gcof = kt_whnf(heap, ctx, fib->data.glue.cof);
            int gdecided = 0;
            if (gcof->tag == KT_COFIB) {
              kterm_t *gr = kt_whnf(heap, ctx, gcof->data.cofib.var);
              kterm_t *gb = kt_whnf(heap, ctx, gcof->data.cofib.endpoint);
              if (kt_equal(heap, ctx, gr, gb)) gdecided = 1;
              else if ((gr->tag == KT_I0 && gb->tag == KT_I1) || (gr->tag == KT_I1 && gb->tag == KT_I0)) gdecided = 1;
            }
            if (!gdecided) {
              kterm_t *GA = fib->data.glue.base_type;
              kterm_t *Gpsi = fib->data.glue.cof;
              kterm_t *GT = fib->data.glue.glue_type;
              kterm_t *Ge = fib->data.glue.equiv;
              kterm_t *phi = t->data.comp.cof;
              kterm_t *u0 = t->data.comp.base;
              kterm_t *upart = t->data.comp.partial;   /* i0-given partial (plain term or <i>-line) */
              /* the partial value as a plain term (strip a leading plam if present) */
              kterm_t *uval = (upart->tag == KT_PLAM) ? upart->data.plam.body : upart;
              /* under a fresh composition binder k (index 0): shift the ctx-level pieces up by 1 */
              kterm_t *GA1 = kt_shift(heap, GA, 0, 1);
              kterm_t *Gpsi1 = kt_shift(heap, Gpsi, 0, 1);
              kterm_t *GT1 = kt_shift(heap, GT, 0, 1);
              kterm_t *Ge1 = kt_shift(heap, Ge, 0, 1);
              kterm_t *phi1 = kt_shift(heap, phi, 0, 1);
              kterm_t *u0_1 = kt_shift(heap, u0, 0, 1);
              kterm_t *uval1 = kt_shift(heap, uval, 0, 1);   /* u under <k> (constant in k) */
              /* t1 = comp <k>GT [phi -> <k>u] g0 */
              kterm_t *Tline = kt_plam(heap, "k", GT1);
              kterm_t *t1 = kt_comp(heap, Tline, phi, kt_plam(heap, "k", uval1), u0);
              /* Tfill(k) under <k>: comp2 <l>GT [phi -> <l>u] [(cofib k i0) -> <l>g0] g0, with k=index0
               * and l=index0-under-its-own-binder.  Build under <k> then <l>. */
              {
                kterm_t *GT2 = kt_shift(heap, GT, 0, 2);   /* GT under <k,l> */
                kterm_t *uval2 = kt_shift(heap, uval, 0, 2);
                kterm_t *u0_2 = kt_shift(heap, u0, 0, 2);
                kterm_t *phi2 = kt_shift(heap, phi, 0, 2);
                kterm_t *clampcof = kt_cofib(heap, kt_var(heap, 1), kt_i0(heap)); /* (cofib k i0), k=index1 under <k,l> */
                kterm_t *Tfill_k = kt_comp2(heap, kt_plam(heap, "l", GT2),
                                            phi2, kt_plam(heap, "l", uval2),
                                            clampcof, kt_plam(heap, "l", u0_2),
                                            u0_1);                         /* : GT, under <k> */
                kterm_t *efun1 = kt_equiv_fun(heap, Ge1);
                kterm_t *psileg = kt_app(heap, efun1, Tfill_k);            /* equiv-fun e (Tfill(k)), under <k> */
                kterm_t *Aline = kt_plam(heap, "k", GA1);
                kterm_t *ungl_u = kt_unglue(heap, GA1, Gpsi1, GT1, Ge1, uval1);  /* unglue u, under <k> */
                kterm_t *ungl_g0 = kt_unglue(heap, GA, Gpsi, GT, Ge, u0);        /* unglue g0, in ctx */
                kterm_t *a1 = kt_comp2(heap, Aline,
                                       phi1, kt_plam(heap, "k", ungl_u),
                                       Gpsi1, kt_plam(heap, "k", psileg),
                                       ungl_g0);                          /* : GA */
                kterm_t *res = kt_glue_elem(heap, GA, Gpsi, GT, Ge, t1, a1);
                return kt_whnf(heap, ctx, res);
              }
            }
          }
          /* constant line, non-Glue fibre: degenerate to homogeneous composition in the fibre */
          return kt_whnf(heap, ctx, kt_hcomp1(heap, at_i1, t->data.comp.cof,
                                                    t->data.comp.partial, t->data.comp.base));
        }
        /* VARYING line, nonempty face — the heterogeneous correction.  In this single-cofibration
         * representation the partial u and the base u0 are both given at the i0 end (the typing
         * checks u : A(i0) and u0 : A(i0)).  Heterogeneous composition transports BOTH to the i1
         * fibre along the line and composes them homogeneously there:
         *     comp = hcomp1 A(i1) (cofib r b) (transp line u) (transp line u0).
         * Soundness: transp line u, transp line u0 : A(i1) (delegating to the proven transp), and
         * the hcomp1 compatibility is preserved — if u ≡ u0 on the face (required by comp's typing),
         * then applying the same transp gives transp line u ≡ transp line u0 on the face, so the
         * hcomp1 compatibility holds and the homogeneous composition is well-formed.  This agrees
         * with both degenerate treads: on a constant line transp is the identity (recovering hcomp1
         * A φ u u0), and on an empty face hcomp1 reduces to its base = transp line u0 (recovering the
         * empty-face tread).  Because the partial is i0-given (not an i-varying family), the CCHM
         * forward-filler subtlety does not arise — the whole-line transp is exactly right here. */
        {
          /* HETEROGENEOUS comp over a GLUE line (the type varies in i): if the line is
           *   <i>(Glue A(i) phi T(i) e(i))  with phi CONSTANT in i and the gluing cofibration
           * UNDECIDED, build the CCHM result rather than the degenerate hcomp1.  The recipe (verified
           * at the surface) reuses the connection-line filler as CCHM "pres":
           *   res = glue A(i1) phi T(i1) e(i1) t1 a1,
           *   t1 = comp <i>T(i) [phi -> <i>u] g0           (u, g0 are T-elements on phi),
           *   Tfill(k) = comp2 <l>T(k/\l) [phi -> <l>u] [(cofib k i0) -> <l>g0] g0   (the pres filler),
           *   a1 = comp2 <k>A(k) [psi(=phi here is the comp face) ...]   -- two faces: the comp's
           *        system face (cof) and the gluing face phi:
           *   a1 = comp2 <k>A(k) [cof -> <k>unglue u] [phi -> <k>equiv-fun(e(k), Tfill(k))] (unglue g0).
           * The line bodies A(i),T(i),e(i) reference the comp binder (index 0); we re-abstract them
           * under fresh binders.  phi is required constant in i: we read it at i (index 0) and shift
           * it down to ctx level, taking this branch only when that shift is well-defined (phi does
           * not mention i), which the gluing-decision test below enforces structurally. */
          kterm_t *body = kt_whnf(heap, ictx, cline->data.plam.body);   /* Glue A(i) phi T(i) e(i), i=index0 */
          if (body->tag == KT_GLUE) {
            /* phi must be constant in i: detect by lowering it across the binder (shift cutoff 0 by -1).
             * If phi mentions i this lowering corrupts it; we additionally require the lowered phi to be
             * a cofibration whose decision is UNDECIDED.  kt_shift(.,0,-1) is the inverse of lifting. */
            kterm_t *phi_i = body->data.glue.cof;            /* phi at i-level */
            kterm_t *phi0 = kt_shift(heap, phi_i, 0, -1);    /* phi lowered to ctx level (valid iff const in i) */
            kterm_t *gcofw = kt_whnf(heap, ctx, phi0);
            int gdecided = 0, gconst = 1;
            /* constant-in-i check: phi0 lifted back must equal phi_i */
            if (!kt_equal(heap, ictx, kt_shift(heap, phi0, 0, 1), phi_i)) gconst = 0;
            if (gcofw->tag == KT_COFIB) {
              kterm_t *gr = kt_whnf(heap, ctx, gcofw->data.cofib.var);
              kterm_t *gb = kt_whnf(heap, ctx, gcofw->data.cofib.endpoint);
              if (kt_equal(heap, ctx, gr, gb)) gdecided = 1;
              else if ((gr->tag == KT_I0 && gb->tag == KT_I1) || (gr->tag == KT_I1 && gb->tag == KT_I0)) gdecided = 1;
            } else {
              gdecided = 1;  /* not a plain cofibration we can decide: be conservative, do not fire */
            }
            if (!gconst) {
              /* phi VARIES in i.  Use the forall-face delta = (forall i. phi(i)).  If delta is
               * decided-HELD, the gluing structure is stable along the WHOLE line, so Glue A(i) phi
               * T(i) e(i) = T(i) throughout and the composition is just the composition in the T line:
               *   comp <i>(Glue ..) [psi -> u] g0  =  comp <i>T(i) [psi -> u] g0    (sound and exact).
               * (If delta is decided-EMPTY or undecided we do NOT fire — the genuinely-hard moving-face
               * case needs the equivalence lemma; we leave it neutral, which is sound.) */
              kterm_t *delta = kt_whnf(heap, ctx, kt_cofib_forall(heap, kt_plam(heap, "i", phi_i)));
              int dheld = 0;
              if (delta->tag == KT_COFIB) {
                kterm_t *dr = kt_whnf(heap, ctx, delta->data.cofib.var);
                kterm_t *db = kt_whnf(heap, ctx, delta->data.cofib.endpoint);
                if (kt_equal(heap, ctx, dr, db)) dheld = 1;
              }
              if (dheld) {
                kterm_t *Tline_d = kt_plam(heap, "i", body->data.glue.glue_type); /* <i>T(i) */
                return kt_whnf(heap, ctx, kt_comp(heap, Tline_d, t->data.comp.cof,
                                                  t->data.comp.partial, t->data.comp.base));
              }
            }
            if (gconst && !gdecided) {
              kterm_t *A_i = body->data.glue.base_type;   /* A(i), i=index0 */
              kterm_t *T_i = body->data.glue.glue_type;   /* T(i) */
              kterm_t *e_i = body->data.glue.equiv;       /* e(i) */
              kterm_t *u0 = t->data.comp.base;            /* g0 : Glue(0), in ctx */
              kterm_t *upart = t->data.comp.partial;
              kterm_t *uval = (upart->tag == KT_PLAM) ? upart->data.plam.body : upart;  /* u : Glue(0)-ish, in ctx */
              /* lines at i1: substitute the binder := i1 */
              kterm_t *A1 = kt_subst(heap, A_i, 0, kt_i1(heap));
              kterm_t *T1 = kt_subst(heap, T_i, 0, kt_i1(heap));
              kterm_t *e1 = kt_subst(heap, e_i, 0, kt_i1(heap));
              /* t1 = comp <i>T(i) [phi -> <i>u] g0.  Tline = <i>T(i) is exactly plam over T_i.
               * The partial <i>u: u is i0-given (ctx level); under the i-binder it is constant, so lift it. */
              kterm_t *uval_i = kt_shift(heap, uval, 0, 1);   /* u under <i> */
              kterm_t *Tline_i = kt_plam(heap, "i", T_i);
              kterm_t *t1 = kt_comp(heap, Tline_i, phi0, kt_plam(heap, "i", uval_i), u0);
              {
                /* Build a1 under a fresh binder k (index 0). */
                kterm_t *A_k = A_i;            /* A(k): same body, binder reused as k */
                kterm_t *T_k = T_i;
                kterm_t *e_k = e_i;
                kterm_t *phi1 = kt_shift(heap, phi0, 0, 1);   /* phi under <k> (const) */
                kterm_t *uval_k = kt_shift(heap, uval, 0, 1); /* u under <k> */
                kterm_t *u0_k = kt_shift(heap, u0, 0, 1);     /* g0 under <k> */
                /* Tfill(k) under <k>: comp2 <l>T(k/\l) [phi -> <l>u] [(cofib k i0) -> <l>g0] g0.
                 * Under <k,l>: k=index1, l=index0.  T(k/\l): take T_i and substitute its binder (the
                 * single interval var) by (imeet k l).  T_i references index0 as its type variable; under
                 * <k,l> we want that variable to become (imeet k l) where k=1,l=0. */
                kterm_t *kmeetl = kt_imeet(heap, kt_var(heap, 1), kt_var(heap, 0));  /* k /\ l under <k,l> */
                /* T(k/\l): substitute T_i's binder(index0) with kmeetl, after lifting T_i's other vars by 1
                 * (T_i lives one binder deep already, so under <k,l> its free ctx vars are +2; build by
                 * shifting T_i up by 2 then substituting index? simpler: T_i has exactly index0 as binder var
                 * and ctx vars >=1. Under <k,l> ctx vars must be +2 and the binder var -> kmeetl. */
                kterm_t *T_i_lift = kt_shift(heap, T_i, 1, 2);    /* lift ctx vars (>=1) by 2, keep index0 */
                kterm_t *T_kl = kt_subst(heap, T_i_lift, 0, kmeetl);  /* binder := k/\l */
                kterm_t *uval_kl = kt_shift(heap, uval, 0, 2);   /* u under <k,l> */
                kterm_t *u0_kl = kt_shift(heap, u0, 0, 2);       /* g0 under <k,l> */
                kterm_t *phi_kl = kt_shift(heap, phi0, 0, 2);    /* phi under <k,l> */
                kterm_t *clampcof = kt_cofib(heap, kt_var(heap, 1), kt_i0(heap));  /* (cofib k i0), k=index1 */
                kterm_t *Tfill_k = kt_comp2(heap, kt_plam(heap, "l", T_kl),
                                            phi_kl, kt_plam(heap, "l", uval_kl),
                                            clampcof, kt_plam(heap, "l", u0_kl),
                                            u0_k);                       /* : T(k), under <k> */
                kterm_t *efun_k = kt_equiv_fun(heap, e_k);
                kterm_t *psileg = kt_app(heap, efun_k, Tfill_k);          /* equiv-fun(e(k), Tfill(k)), under <k> */
                kterm_t *Aline_k = kt_plam(heap, "k", A_k);
                kterm_t *ungl_u_k = kt_unglue(heap, A_k, kt_shift(heap, body->data.glue.cof,0,0), T_k, e_k, uval_k);
                /* unglue u under <k>: needs Glue components at k.  body's components are at i==k (index0). */
                kterm_t *ungl_u = kt_unglue(heap, A_k, body->data.glue.cof, T_k, e_k, uval_k);
                kterm_t *ungl_g0 = kt_unglue(heap, kt_subst(heap,A_i,0,kt_i0(heap)),
                                             phi0, kt_subst(heap,T_i,0,kt_i0(heap)),
                                             kt_subst(heap,e_i,0,kt_i0(heap)), u0);   /* unglue g0 : A(0), in ctx */
                kterm_t *cof1 = t->data.comp.cof;            /* the comp's system face (in ctx) */
                kterm_t *cof1_k = kt_shift(heap, cof1, 0, 1);
                kterm_t *a1 = kt_comp2(heap, Aline_k,
                                       cof1_k, kt_plam(heap, "k", ungl_u),
                                       phi1, kt_plam(heap, "k", psileg),
                                       ungl_g0);                          /* : A(i1) */
                (void)ungl_u_k;
                {
                  kterm_t *res = kt_glue_elem(heap, A1, phi0, T1, e1, t1, a1);
                  return kt_whnf(heap, ctx, res);
                }
              }
            }
          }
        }
        {
          /* non-Glue varying line: transport both endpoints and compose homogeneously in the fibre */
          kterm_t *tu  = kt_transp(heap, cline, t->data.comp.partial);
          kterm_t *tu0 = kt_transp(heap, cline, t->data.comp.base);
          return kt_whnf(heap, ctx, kt_hcomp1(heap, at_i1, t->data.comp.cof, tu, tu0));
        }
      }
    }
    return t;  /* neutral only if the line is not a path-abstraction */
  }
  case KT_COMP2: {
    /* comp2 line (cofib r1 b1) u1 (cofib r2 b2) u2 u0 — two-face composition along a type line A(i)
     * with i-VARYING line partials u1 = <i>u1(i), u2 = <i>u2(i).  Sound boundary behavior:
     *   - a DECIDED held face (rk ≡ bk): the result is that face's partial line at i1, uk(i1)
     *     (on a true face the composite equals the partial at the top of the i-direction);
     *   - an EMPTY system (both faces are empty, rk,bk distinct endpoints): no constraint, so it is
     *     transport of the base, transp line u0;
     *   - otherwise NEUTRAL (abstract A and/or undecided faces — the genuine fill is recovered only
     *     when a face becomes decided under substitution, which is exactly how a Path-line's
     *     endpoints compute).  Face1 is tried first; on overlap (both decided) the typing has already
     *     checked u1(i1) ≡ u2(i1), so either branch is correct. */
    kterm_t *cline = kt_whnf(heap, ctx, t->data.comp2.line);
    if (cline->tag == KT_PLAM) {
      kterm_t *c1 = kt_whnf(heap, ctx, t->data.comp2.cof1);
      kterm_t *c2 = kt_whnf(heap, ctx, t->data.comp2.cof2);
      int held1 = 0, held2 = 0, empty1 = 0, empty2 = 0;
      if (c1->tag == KT_COFIB) {
        kterm_t *r = kt_whnf(heap, ctx, c1->data.cofib.var);
        kterm_t *b = kt_whnf(heap, ctx, c1->data.cofib.endpoint);
        if (kt_equal(heap, ctx, r, b)) held1 = 1;
        else if ((r->tag == KT_I0 && b->tag == KT_I1) || (r->tag == KT_I1 && b->tag == KT_I0)) empty1 = 1;
      }
      if (c2->tag == KT_COFIB) {
        kterm_t *r = kt_whnf(heap, ctx, c2->data.cofib.var);
        kterm_t *b = kt_whnf(heap, ctx, c2->data.cofib.endpoint);
        if (kt_equal(heap, ctx, r, b)) held2 = 1;
        else if ((r->tag == KT_I0 && b->tag == KT_I1) || (r->tag == KT_I1 && b->tag == KT_I0)) empty2 = 1;
      }
      /* a decided held face: the partial line (must be a path-abstraction) evaluated at i1 */
      if (held1 && t->data.comp2.partial1->tag == KT_PLAM) {
        return kt_whnf(heap, ctx, kt_subst(heap, t->data.comp2.partial1->data.plam.body, 0, kt_i1(heap)));
      }
      if (held2 && t->data.comp2.partial2->tag == KT_PLAM) {
        return kt_whnf(heap, ctx, kt_subst(heap, t->data.comp2.partial2->data.plam.body, 0, kt_i1(heap)));
      }
      /* both faces empty: the system is empty, so comp2 is transport of the base */
      if (empty1 && empty2) {
        return kt_whnf(heap, ctx, kt_transp(heap, cline, t->data.comp2.base));
      }
      /* exactly ONE face empty: the empty face contributes no constraint, so comp2 drops to the
       * single-face comp on the other face.  This is the equation the composition FILLER relies on
       * (a filler is a comp2 whose clamp face becomes empty at the i1 end, and must there equal the
       * underlying single-face composite).  Sound: an empty cofibration is never satisfied, so the
       * two-face system and the one-face system have the same (empty) restriction on that face. */
      if (empty1 && !empty2) {
        return kt_whnf(heap, ctx, kt_comp(heap, cline, t->data.comp2.cof2,
                                          t->data.comp2.partial2, t->data.comp2.base));
      }
      if (empty2 && !empty1) {
        return kt_whnf(heap, ctx, kt_comp(heap, cline, t->data.comp2.cof1,
                                          t->data.comp2.partial1, t->data.comp2.base));
      }
    }
    return t;  /* neutral: undecided faces / abstract line — the genuine fill is the named frontier */
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
  case KT_EQUIV:
    return kt_equal(heap, ctx, na->data.equiv.a, nb->data.equiv.a)
        && kt_equal(heap, ctx, na->data.equiv.b, nb->data.equiv.b);
  case KT_UA:
    return kt_equal(heap, ctx, na->data.ua.equiv, nb->data.ua.equiv);
  case KT_TRANSP:
    return kt_equal(heap, ctx, na->data.transp.line, nb->data.transp.line)
        && kt_equal(heap, ctx, na->data.transp.base, nb->data.transp.base);
  case KT_INEG:
    return kt_equal(heap, ctx, na->data.ineg.arg, nb->data.ineg.arg);
  case KT_IMEET:
    return kt_equal(heap, ctx, na->data.imeet.left, nb->data.imeet.left)
        && kt_equal(heap, ctx, na->data.imeet.right, nb->data.imeet.right);
  case KT_IJOIN:
    return kt_equal(heap, ctx, na->data.ijoin.left, nb->data.ijoin.left)
        && kt_equal(heap, ctx, na->data.ijoin.right, nb->data.ijoin.right);
  case KT_ID_EQUIV:
    return kt_equal(heap, ctx, na->data.id_equiv.type, nb->data.id_equiv.type);
  case KT_HCOMP:
    return kt_equal(heap, ctx, na->data.hcomp.type, nb->data.hcomp.type)
        && kt_equal(heap, ctx, na->data.hcomp.base, nb->data.hcomp.base);
  case KT_COFIB:
    return kt_equal(heap, ctx, na->data.cofib.var, nb->data.cofib.var)
        && kt_equal(heap, ctx, na->data.cofib.endpoint, nb->data.cofib.endpoint);
  case KT_COFIB_OR:
    return kt_equal(heap, ctx, na->data.cofib_or.cof1, nb->data.cofib_or.cof1)
        && kt_equal(heap, ctx, na->data.cofib_or.cof2, nb->data.cofib_or.cof2);
  case KT_COFIB_FORALL:
    return kt_equal(heap, ctx, na->data.cofib_forall.body, nb->data.cofib_forall.body);
  case KT_HCOMP1:
    return kt_equal(heap, ctx, na->data.hcomp1.type, nb->data.hcomp1.type)
        && kt_equal(heap, ctx, na->data.hcomp1.cof, nb->data.hcomp1.cof)
        && kt_equal(heap, ctx, na->data.hcomp1.partial, nb->data.hcomp1.partial)
        && kt_equal(heap, ctx, na->data.hcomp1.base, nb->data.hcomp1.base);
  case KT_COMP:
    return kt_equal(heap, ctx, na->data.comp.line, nb->data.comp.line)
        && kt_equal(heap, ctx, na->data.comp.cof, nb->data.comp.cof)
        && kt_equal(heap, ctx, na->data.comp.partial, nb->data.comp.partial)
        && kt_equal(heap, ctx, na->data.comp.base, nb->data.comp.base);
  case KT_COMP2:
    return kt_equal(heap, ctx, na->data.comp2.line, nb->data.comp2.line)
        && kt_equal(heap, ctx, na->data.comp2.cof1, nb->data.comp2.cof1)
        && kt_equal(heap, ctx, na->data.comp2.partial1, nb->data.comp2.partial1)
        && kt_equal(heap, ctx, na->data.comp2.cof2, nb->data.comp2.cof2)
        && kt_equal(heap, ctx, na->data.comp2.partial2, nb->data.comp2.partial2)
        && kt_equal(heap, ctx, na->data.comp2.base, nb->data.comp2.base);
  case KT_HCOMP2:
    return kt_equal(heap, ctx, na->data.hcomp2.type, nb->data.hcomp2.type)
        && kt_equal(heap, ctx, na->data.hcomp2.cof1, nb->data.hcomp2.cof1)
        && kt_equal(heap, ctx, na->data.hcomp2.partial1, nb->data.hcomp2.partial1)
        && kt_equal(heap, ctx, na->data.hcomp2.cof2, nb->data.hcomp2.cof2)
        && kt_equal(heap, ctx, na->data.hcomp2.partial2, nb->data.hcomp2.partial2)
        && kt_equal(heap, ctx, na->data.hcomp2.base, nb->data.hcomp2.base);
  case KT_GLUE:
    return kt_equal(heap, ctx, na->data.glue.base_type, nb->data.glue.base_type)
        && kt_equal(heap, ctx, na->data.glue.cof, nb->data.glue.cof)
        && kt_equal(heap, ctx, na->data.glue.glue_type, nb->data.glue.glue_type)
        && kt_equal(heap, ctx, na->data.glue.equiv, nb->data.glue.equiv);
  case KT_PARTIAL:
    return kt_equal(heap, ctx, na->data.partial.cof, nb->data.partial.cof)
        && kt_equal(heap, ctx, na->data.partial.type, nb->data.partial.type);
  case KT_PSYS:
    return kt_equal(heap, ctx, na->data.psys.cof, nb->data.psys.cof)
        && kt_equal(heap, ctx, na->data.psys.type, nb->data.psys.type)
        && kt_equal(heap, ctx, na->data.psys.elem, nb->data.psys.elem);
  case KT_EQUIV_FUN:
    return kt_equal(heap, ctx, na->data.equiv_fun.equiv, nb->data.equiv_fun.equiv);
  case KT_EQUIV_INV:
    return kt_equal(heap, ctx, na->data.equiv_inv.equiv, nb->data.equiv_inv.equiv);
  case KT_MK_EQUIV:
    return kt_equal(heap, ctx, na->data.mk_equiv.glue_type, nb->data.mk_equiv.glue_type)
        && kt_equal(heap, ctx, na->data.mk_equiv.base_type, nb->data.mk_equiv.base_type)
        && kt_equal(heap, ctx, na->data.mk_equiv.fwd, nb->data.mk_equiv.fwd)
        && kt_equal(heap, ctx, na->data.mk_equiv.inv, nb->data.mk_equiv.inv)
        && kt_equal(heap, ctx, na->data.mk_equiv.eta, nb->data.mk_equiv.eta)
        && kt_equal(heap, ctx, na->data.mk_equiv.eps, nb->data.mk_equiv.eps);
  case KT_EQUIV_ETA:
    return kt_equal(heap, ctx, na->data.equiv_eta.equiv, nb->data.equiv_eta.equiv);
  case KT_EQUIV_EPS:
    return kt_equal(heap, ctx, na->data.equiv_eps.equiv, nb->data.equiv_eps.equiv);
  case KT_UNGLUE:
    return kt_equal(heap, ctx, na->data.unglue.base_type, nb->data.unglue.base_type)
        && kt_equal(heap, ctx, na->data.unglue.cof, nb->data.unglue.cof)
        && kt_equal(heap, ctx, na->data.unglue.glue_type, nb->data.unglue.glue_type)
        && kt_equal(heap, ctx, na->data.unglue.equiv, nb->data.unglue.equiv)
        && kt_equal(heap, ctx, na->data.unglue.arg, nb->data.unglue.arg);
  case KT_GLUE_ELEM:
    return kt_equal(heap, ctx, na->data.glue_elem.base_type, nb->data.glue_elem.base_type)
        && kt_equal(heap, ctx, na->data.glue_elem.cof, nb->data.glue_elem.cof)
        && kt_equal(heap, ctx, na->data.glue_elem.glue_type, nb->data.glue_elem.glue_type)
        && kt_equal(heap, ctx, na->data.glue_elem.equiv, nb->data.glue_elem.equiv)
        && kt_equal(heap, ctx, na->data.glue_elem.member, nb->data.glue_elem.member)
        && kt_equal(heap, ctx, na->data.glue_elem.base, nb->data.glue_elem.base);
  case KT_GTRANSP:
    return kt_equal(heap, ctx, na->data.gtransp.base_type, nb->data.gtransp.base_type)
        && kt_equal(heap, ctx, na->data.gtransp.cof, nb->data.gtransp.cof)
        && kt_equal(heap, ctx, na->data.gtransp.glue_type, nb->data.gtransp.glue_type)
        && kt_equal(heap, ctx, na->data.gtransp.equiv, nb->data.gtransp.equiv)
        && kt_equal(heap, ctx, na->data.gtransp.base, nb->data.gtransp.base);
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
  case KT_EQUIV: {
    /* (Equiv A B) : Sort(max la lb)  when A : Sort(la), B : Sort(lb).
     * The type of equivalences between two types, living in the larger universe. */
    kterm_t *sa = kt_infer(heap, ctx, t->data.equiv.a);
    kterm_t *sb;
    int la, lb;
    if (sa == NULL) return NULL;
    sa = kt_whnf(heap, ctx, sa);
    if (sa->tag != KT_SORT) return NULL;
    sb = kt_infer(heap, ctx, t->data.equiv.b);
    if (sb == NULL) return NULL;
    sb = kt_whnf(heap, ctx, sb);
    if (sb->tag != KT_SORT) return NULL;
    la = sa->data.sort.level;
    lb = sb->data.sort.level;
    return kt_sort(heap, la > lb ? la : lb);
  }
  case KT_UA: {
    /* ua e : (Id (Sort n) A B)  when e : (Equiv A B), A,B : Sort(n).
     * Univalence at the typing level: an equivalence yields an identity between
     * the two types IN THE UNIVERSE.  (This rule types univalence; full
     * computational univalence — transport across ua reducing through Glue — is
     * NOT provided here and is not claimed.) */
    kterm_t *et = kt_infer(heap, ctx, t->data.ua.equiv);
    kterm_t *A, *B, *sa;
    if (et == NULL) return NULL;
    et = kt_whnf(heap, ctx, et);
    if (et->tag != KT_EQUIV) return NULL;       /* the argument must be an equivalence */
    A = et->data.equiv.a;
    B = et->data.equiv.b;
    sa = kt_infer(heap, ctx, A);
    if (sa == NULL) return NULL;
    sa = kt_whnf(heap, ctx, sa);
    if (sa->tag != KT_SORT) return NULL;
    /* result: Id (Sort la) A B — a path between the two types in their universe */
    return kt_id(heap, kt_sort(heap, sa->data.sort.level), A, B);
  }
  case KT_TRANSP: {
    /* transp line base : (line @ i1)   when
     *   line = <i> A(i)  is a path-abstraction whose body A(i) is a Sort for i : I, and
     *   base : (line @ i0)  i.e. base inhabits the type at the i0 endpoint.
     * The result is the type at the i1 endpoint.  This is the standard CCHM/cubical-Agda
     * transp TYPING rule.  (Computation is handled in kt_whnf: the constant-line case
     * reduces to base; a varying line stays neutral.  Full transport across a varying line
     * via Glue/comp is NOT provided — only typed.) */
    kterm_t *line = t->data.transp.line;
    kterm_t *wline, *body_sort, *at_i0, *at_i1, *rbody;
    kctx_t *e;
    if (line == NULL) return NULL;
    wline = kt_whnf(heap, ctx, line);
    if (wline->tag != KT_PLAM) return NULL;          /* the line must be a path-abstraction */
    /* the body, under i : I, must be a type (a Sort) */
    e = kctx_extend(heap, ctx, wline->data.plam.name, kt_interval(heap), NULL);
    /* Reduce the body to whnf under the interval binder, mirroring the computation rule, so the
     * endpoint types resolve through a Glue type (or any other redex) in the line body.  Without
     * this, an empty-face Glue line — whose body reduces to a bare A(i) — would have its raw Glue
     * endpoints fed to kt_check, conservatively rejecting a well-typed transport.  whnf is
     * meaning-preserving, so this only ACCEPTS more well-typed terms; it never accepts a bad one. */
    rbody = kt_whnf(heap, e, wline->data.plam.body);
    body_sort = kt_infer(heap, e, rbody);
    if (body_sort == NULL) return NULL;
    body_sort = kt_whnf(heap, e, body_sort);
    if (body_sort->tag != KT_SORT) return NULL;      /* A(i) must be a Sort */
    /* base must inhabit the type at i0 */
    at_i0 = kt_subst(heap, rbody, 0, kt_i0(heap));
    if (kt_check(heap, ctx, t->data.transp.base, at_i0) != KERNEL_OK) return NULL;
    /* result type is the body at i1 */
    at_i1 = kt_subst(heap, rbody, 0, kt_i1(heap));
    return at_i1;
  }
  case KT_INEG: {
    /* ~r : I  when  r : I.  Interval negation stays within the interval. */
    kterm_t *rt = kt_infer(heap, ctx, t->data.ineg.arg);
    if (rt == NULL) return NULL;
    rt = kt_whnf(heap, ctx, rt);
    if (rt->tag != KT_INTERVAL) return NULL;
    return kt_interval(heap);
  }
  case KT_IMEET: {
    /* r /\ s : I  when  r : I and s : I.  Meet stays within the interval. */
    kterm_t *lt = kt_infer(heap, ctx, t->data.imeet.left);
    kterm_t *rt2 = kt_infer(heap, ctx, t->data.imeet.right);
    if (lt == NULL || rt2 == NULL) return NULL;
    lt = kt_whnf(heap, ctx, lt);
    rt2 = kt_whnf(heap, ctx, rt2);
    if (lt->tag != KT_INTERVAL || rt2->tag != KT_INTERVAL) return NULL;
    return kt_interval(heap);
  }
  case KT_IJOIN: {
    /* r \/ s : I  when  r : I and s : I.  Join stays within the interval. */
    kterm_t *lt = kt_infer(heap, ctx, t->data.ijoin.left);
    kterm_t *rt2 = kt_infer(heap, ctx, t->data.ijoin.right);
    if (lt == NULL || rt2 == NULL) return NULL;
    lt = kt_whnf(heap, ctx, lt);
    rt2 = kt_whnf(heap, ctx, rt2);
    if (lt->tag != KT_INTERVAL || rt2->tag != KT_INTERVAL) return NULL;
    return kt_interval(heap);
  }
  case KT_ID_EQUIV: {
    /* (id-equiv A) : (Equiv A A)  when  A : Sort n.  The identity equivalence. */
    kterm_t *at = kt_infer(heap, ctx, t->data.id_equiv.type);
    if (at == NULL) return NULL;
    at = kt_whnf(heap, ctx, at);
    if (at->tag != KT_SORT) return NULL;
    return kt_equiv(heap, t->data.id_equiv.type, t->data.id_equiv.type);
  }
  case KT_HCOMP: {
    /* hcomp A u0 : A  when  A : Sort n  and  u0 : A.  Empty-system composition.
     * Type-preserving because it reduces to u0 : A. */
    kterm_t *at = kt_infer(heap, ctx, t->data.hcomp.type);
    if (at == NULL) return NULL;
    at = kt_whnf(heap, ctx, at);
    if (at->tag != KT_SORT) return NULL;
    if (kt_check(heap, ctx, t->data.hcomp.base, t->data.hcomp.type) != KERNEL_OK) return NULL;
    return t->data.hcomp.type;
  }
  case KT_COFIB: {
    /* (cofib r b) : I-ish constraint marker; well-formed when r : I and b : I.
     * We give it the interval as its "type" (it is consumed by hcomp1, not eliminated). */
    kterm_t *rt = kt_infer(heap, ctx, t->data.cofib.var);
    kterm_t *bt;
    if (rt == NULL) return NULL;
    rt = kt_whnf(heap, ctx, rt);
    if (rt->tag != KT_INTERVAL) return NULL;
    bt = kt_infer(heap, ctx, t->data.cofib.endpoint);
    if (bt == NULL) return NULL;
    bt = kt_whnf(heap, ctx, bt);
    if (bt->tag != KT_INTERVAL) return NULL;
    return kt_interval(heap);
  }
  case KT_COFIB_OR: {
    /* (cofib-or c1 c2) : well-formed when both operands are themselves cofibrations — structurally a
     * (cofib ..) or a nested (cofib-or ..) — each well-formed.  We require the structural shape (not
     * merely an interval-typed term) so the former cannot be fed a bare interval point.  Nominal type:
     * the interval, like (cofib ..); it is a constraint marker consumed by comp/hcomp, not eliminated. */
    kterm_t *c1 = kt_whnf(heap, ctx, t->data.cofib_or.cof1);
    kterm_t *c2 = kt_whnf(heap, ctx, t->data.cofib_or.cof2);
    if (c1->tag != KT_COFIB && c1->tag != KT_COFIB_OR) return NULL;
    if (c2->tag != KT_COFIB && c2->tag != KT_COFIB_OR) return NULL;
    if (kt_infer(heap, ctx, c1) == NULL) return NULL;
    if (kt_infer(heap, ctx, c2) == NULL) return NULL;
    return kt_interval(heap);
  }
  case KT_COFIB_FORALL: {
    /* (cofib-forall <i> phi) : well-formed when phi(i) is a cofibration under i : I.  We extend the
     * context with the interval binder and require the body to be a (cofib ..) or (cofib-or ..) /
     * nested forall that infers (to the interval).  Nominal type: the interval (constraint marker). */
    kctx_t *ictx = kctx_extend(heap, ctx, "i", kt_interval(heap), NULL);
    kterm_t *bw = kt_whnf(heap, ictx, t->data.cofib_forall.body);
    if (bw->tag != KT_COFIB && bw->tag != KT_COFIB_OR && bw->tag != KT_COFIB_FORALL) return NULL;
    if (kt_infer(heap, ictx, t->data.cofib_forall.body) == NULL) return NULL;
    return kt_interval(heap);
  }
  case KT_HCOMP1: {
    /* hcomp1 A (cofib r b) u u0 : A  when  A : Sort n, the cofibration is well-formed,
     * u : A, u0 : A, AND the compatibility condition holds: when the face (r=b) holds
     * definitionally, the partial element u must equal the base u0.  This compatibility
     * check is the soundness heart — it blocks returning a u that disagrees with u0 on
     * the face.  Off/neutral faces impose no extra constraint. */
    kterm_t *at, *cof;
    if (kt_infer(heap, ctx, t->data.hcomp1.cof) == NULL) return NULL;  /* cofibration well-formed */
    at = kt_infer(heap, ctx, t->data.hcomp1.type);
    if (at == NULL) return NULL;
    at = kt_whnf(heap, ctx, at);
    if (at->tag != KT_SORT) return NULL;
    if (kt_check(heap, ctx, t->data.hcomp1.partial, t->data.hcomp1.type) != KERNEL_OK) return NULL;
    if (kt_check(heap, ctx, t->data.hcomp1.base, t->data.hcomp1.type) != KERNEL_OK) return NULL;
    cof = kt_whnf(heap, ctx, t->data.hcomp1.cof);
    if (cof->tag == KT_COFIB || cof->tag == KT_COFIB_OR) {
      /* COMPATIBILITY: if the face holds (for a disjunction: either disjunct holds), u must equal u0 */
      if (cofib_decided_held(heap, ctx, cof)) {
        if (!kt_equal(heap, ctx, t->data.hcomp1.partial, t->data.hcomp1.base)) return NULL;
      }
    }
    return t->data.hcomp1.type;
  }
  case KT_HCOMP2: {
    /* hcomp2 A (cofib r1 b1) u1 (cofib r2 b2) u2 u0 : A  when  A : Sort n, both cofibrations
     * well-formed, u1:A, u2:A, u0:A, AND the OVERLAP COMPATIBILITY LATTICE holds:
     *   (C1) if face1 holds: u1 = u0   (partial1 agrees with the base on its face)
     *   (C2) if face2 holds: u2 = u0   (partial2 agrees with the base on its face)
     *   (C3) if BOTH faces hold: u1 = u2   (THE OVERLAP — the new soundness-critical check)
     * C3 is what makes the elif reduction order sound: wherever both faces hold, u1 and u2 are
     * forced definitionally equal, so the computation's choice of u1 over u2 is consistent. */
    kterm_t *at, *c1, *c2;
    int f1 = 0, f2 = 0;
    if (kt_infer(heap, ctx, t->data.hcomp2.cof1) == NULL) return NULL;
    if (kt_infer(heap, ctx, t->data.hcomp2.cof2) == NULL) return NULL;
    at = kt_infer(heap, ctx, t->data.hcomp2.type);
    if (at == NULL) return NULL;
    at = kt_whnf(heap, ctx, at);
    if (at->tag != KT_SORT) return NULL;
    if (kt_check(heap, ctx, t->data.hcomp2.partial1, t->data.hcomp2.type) != KERNEL_OK) return NULL;
    if (kt_check(heap, ctx, t->data.hcomp2.partial2, t->data.hcomp2.type) != KERNEL_OK) return NULL;
    if (kt_check(heap, ctx, t->data.hcomp2.base, t->data.hcomp2.type) != KERNEL_OK) return NULL;
    c1 = kt_whnf(heap, ctx, t->data.hcomp2.cof1);
    c2 = kt_whnf(heap, ctx, t->data.hcomp2.cof2);
    if (c1->tag == KT_COFIB &&
        kt_equal(heap, ctx, kt_whnf(heap, ctx, c1->data.cofib.var),
                            kt_whnf(heap, ctx, c1->data.cofib.endpoint))) f1 = 1;
    if (c2->tag == KT_COFIB &&
        kt_equal(heap, ctx, kt_whnf(heap, ctx, c2->data.cofib.var),
                            kt_whnf(heap, ctx, c2->data.cofib.endpoint))) f2 = 1;
    /* C1 */
    if (f1 && !kt_equal(heap, ctx, t->data.hcomp2.partial1, t->data.hcomp2.base)) return NULL;
    /* C2 */
    if (f2 && !kt_equal(heap, ctx, t->data.hcomp2.partial2, t->data.hcomp2.base)) return NULL;
    /* C3 — the overlap compatibility */
    if (f1 && f2 && !kt_equal(heap, ctx, t->data.hcomp2.partial1, t->data.hcomp2.partial2)) return NULL;
    return t->data.hcomp2.type;
  }
  case KT_GLUE: {
    /* Glue A (cofib r b) T e : Sort n  when  A : Sort n, the cofibration is well-formed,
     * T : Sort n, and e : (Equiv T A).  Type-preserving boundary: on the face it reduces to
     * T : Sort n = the result sort. */
    kterm_t *at, *tt, *et, *want;
    if (kt_infer(heap, ctx, t->data.glue.cof) == NULL) return NULL;
    at = kt_infer(heap, ctx, t->data.glue.base_type);
    if (at == NULL) return NULL;
    at = kt_whnf(heap, ctx, at);
    if (at->tag != KT_SORT) return NULL;
    tt = kt_infer(heap, ctx, t->data.glue.glue_type);
    if (tt == NULL) return NULL;
    tt = kt_whnf(heap, ctx, tt);
    if (tt->tag != KT_SORT) return NULL;
    /* e : Equiv T A */
    et = kt_infer(heap, ctx, t->data.glue.equiv);
    if (et == NULL) return NULL;
    want = kt_equiv(heap, t->data.glue.glue_type, t->data.glue.base_type);
    if (!kt_equal(heap, ctx, et, want)) return NULL;
    /* result sort: the base type's sort (A and T share the universe level here) */
    return at;
  }
  case KT_PARTIAL: {
    /* Partial (cofib r b) A : Sort n  when the cofibration is well-formed and A : Sort n.
     * Type-preserving boundary: on the face it reduces to A : Sort n, on an empty face to
     * Unit : Sort 0 — both inhabit the result sort. */
    kterm_t *at;
    if (kt_infer(heap, ctx, t->data.partial.cof) == NULL) return NULL;
    at = kt_infer(heap, ctx, t->data.partial.type);
    if (at == NULL) return NULL;
    at = kt_whnf(heap, ctx, at);
    if (at->tag != KT_SORT) return NULL;
    return at;                                /* : Sort n */
  }
  case KT_PSYS: {
    /* psys (cofib r b) A a : (Partial (cofib r b) A)   when the Partial type is well-formed and the
     * element a : A holds ON the face.  Since a is a PARTIAL element (relevant only where the face
     * holds), its typing uses the face-restricted context: for a face (cofib v ep) with v a variable
     * we check a : A with v := ep imposed; on an EMPTY face there is no obligation on a (Partial =
     * Unit); on a held/other face we check a : A directly.  Result: the Partial type. */
    kterm_t *partial_ty, *c0;
    partial_ty = kt_partial(heap, t->data.psys.cof, t->data.psys.type);
    if (kt_infer(heap, ctx, partial_ty) == NULL) return NULL;     /* Partial well-formed */
    c0 = kt_whnf(heap, ctx, t->data.psys.cof);
    if (c0->tag == KT_COFIB) {
      kterm_t *r = kt_whnf(heap, ctx, c0->data.cofib.var);
      kterm_t *b = kt_whnf(heap, ctx, c0->data.cofib.endpoint);
      int empty = (r->tag == KT_I0 && b->tag == KT_I1) || (r->tag == KT_I1 && b->tag == KT_I0);
      if (!empty) {
        kctx_t *cR = ctx;
        kterm_t *et;
        if (r->tag == KT_VAR) cR = kctx_restrict(heap, ctx, r->data.var.index, b);
        et = kt_infer(heap, cR, t->data.psys.elem);
        if (et == NULL) return NULL;
        if (!kt_equal(heap, cR, et, t->data.psys.type)) return NULL;  /* a : A on the face */
      }
    }
    return partial_ty;                                            /* : Partial (cofib r b) A */
  }
  case KT_EQUIV_FUN: {
    /* equiv-fun e : (Pi (_ : T) A)  when  e : (Equiv T A). */
    kterm_t *et;
    et = kt_infer(heap, ctx, t->data.equiv_fun.equiv);
    if (et == NULL) return NULL;
    et = kt_whnf(heap, ctx, et);
    if (et->tag != KT_EQUIV) return NULL;
    {
      /* build (Pi (_ : T) A); codomain A must be shifted under the new binder */
      kterm_t *pi = kt_alloc(heap, KT_PI);
      pi->data.pi.name = "_";
      pi->data.pi.domain = et->data.equiv.a;                       /* T */
      pi->data.pi.codomain = kt_shift(heap, et->data.equiv.b, 0, 1); /* A under the binder */
      pi->data.pi.implicit = 0;
      return pi;
    }
  }
  case KT_EQUIV_INV: {
    /* equiv-inv e : (Pi (_ : A) T)  when  e : (Equiv T A) — the inverse direction. */
    kterm_t *et;
    et = kt_infer(heap, ctx, t->data.equiv_inv.equiv);
    if (et == NULL) return NULL;
    et = kt_whnf(heap, ctx, et);
    if (et->tag != KT_EQUIV) return NULL;
    {
      kterm_t *pi = kt_alloc(heap, KT_PI);
      pi->data.pi.name = "_";
      pi->data.pi.domain = et->data.equiv.b;                       /* A */
      pi->data.pi.codomain = kt_shift(heap, et->data.equiv.a, 0, 1); /* T under the binder */
      pi->data.pi.implicit = 0;
      return pi;
    }
  }
  case KT_MK_EQUIV: {
    /* mk-equiv T A f g eta eps : (Equiv T A)  WHEN
     *   f   : (Pi (_:T) A)
     *   g   : (Pi (_:A) T)
     *   eta : (Pi (x:T) Id T (g (f x)) x)        [retraction]
     *   eps : (Pi (y:A) Id A (f (g y)) y)        [section]
     * Requiring all four typed components at typing time means a mk-equiv is a genuine
     * quasi-equivalence — this is what makes a general transp across it sound. */
    kterm_t *T = t->data.mk_equiv.glue_type;
    kterm_t *A = t->data.mk_equiv.base_type;
    kterm_t *Ts, *As, *want_f, *want_g, *want_eta, *want_eps;
    kterm_t *gfx, *fgy, *idT, *idA;
    /* T, A must be types */
    { kterm_t *kt = kt_infer(heap, ctx, T); if (kt == NULL) return NULL;
      kt = kt_whnf(heap, ctx, kt); if (kt->tag != KT_SORT) return NULL; }
    { kterm_t *ka = kt_infer(heap, ctx, A); if (ka == NULL) return NULL;
      ka = kt_whnf(heap, ctx, ka); if (ka->tag != KT_SORT) return NULL; }
    /* f : Pi (_:T) A */
    want_f = kt_alloc(heap, KT_PI); want_f->data.pi.name = "_";
    want_f->data.pi.domain = T; want_f->data.pi.codomain = kt_shift(heap, A, 0, 1);
    if (kt_check(heap, ctx, t->data.mk_equiv.fwd, want_f) != KERNEL_OK) return NULL;
    /* g : Pi (_:A) T */
    want_g = kt_alloc(heap, KT_PI); want_g->data.pi.name = "_";
    want_g->data.pi.domain = A; want_g->data.pi.codomain = kt_shift(heap, T, 0, 1);
    if (kt_check(heap, ctx, t->data.mk_equiv.inv, want_g) != KERNEL_OK) return NULL;
    /* eta : Pi (x:T) Id T (g (f x)) x.  Under the binder x:T (index 0), T,A,f,g shift +1. */
    Ts = kt_shift(heap, T, 0, 1); As = kt_shift(heap, A, 0, 1);
    { kterm_t *fs = kt_shift(heap, t->data.mk_equiv.fwd, 0, 1);
      kterm_t *gs = kt_shift(heap, t->data.mk_equiv.inv, 0, 1);
      kterm_t *fx = kt_app(heap, fs, kt_var(heap, 0));
      gfx = kt_app(heap, gs, fx);
      idT = kt_id(heap, Ts, gfx, kt_var(heap, 0));
      want_eta = kt_alloc(heap, KT_PI); want_eta->data.pi.name = "x";
      want_eta->data.pi.domain = T; want_eta->data.pi.codomain = idT;
      if (kt_check(heap, ctx, t->data.mk_equiv.eta, want_eta) != KERNEL_OK) return NULL;
    }
    /* eps : Pi (y:A) Id A (f (g y)) y.  Under y:A (index 0). */
    { kterm_t *fs = kt_shift(heap, t->data.mk_equiv.fwd, 0, 1);
      kterm_t *gs = kt_shift(heap, t->data.mk_equiv.inv, 0, 1);
      kterm_t *gy = kt_app(heap, gs, kt_var(heap, 0));
      fgy = kt_app(heap, fs, gy);
      idA = kt_id(heap, As, fgy, kt_var(heap, 0));
      want_eps = kt_alloc(heap, KT_PI); want_eps->data.pi.name = "y";
      want_eps->data.pi.domain = A; want_eps->data.pi.codomain = idA;
      if (kt_check(heap, ctx, t->data.mk_equiv.eps, want_eps) != KERNEL_OK) return NULL;
    }
    return kt_equiv(heap, T, A);
  }
  case KT_EQUIV_ETA: {
    /* equiv-eta e : (Pi (x:T) Id T ((equiv-inv e) ((equiv-fun e) x)) x)  when e : Equiv T A. */
    kterm_t *et = kt_infer(heap, ctx, t->data.equiv_eta.equiv);
    kterm_t *T, *Ts, *f, *g, *fx, *gfx, *pi;
    if (et == NULL) return NULL;
    et = kt_whnf(heap, ctx, et);
    if (et->tag != KT_EQUIV) return NULL;
    T = et->data.equiv.a;
    Ts = kt_shift(heap, T, 0, 1);
    f = kt_shift(heap, kt_equiv_fun(heap, t->data.equiv_eta.equiv), 0, 1);
    g = kt_shift(heap, kt_equiv_inv(heap, t->data.equiv_eta.equiv), 0, 1);
    fx = kt_app(heap, f, kt_var(heap, 0));
    gfx = kt_app(heap, g, fx);
    pi = kt_alloc(heap, KT_PI); pi->data.pi.name = "x";
    pi->data.pi.domain = T; pi->data.pi.codomain = kt_id(heap, Ts, gfx, kt_var(heap, 0));
    return pi;
  }
  case KT_EQUIV_EPS: {
    /* equiv-eps e : (Pi (y:A) Id A ((equiv-fun e) ((equiv-inv e) y)) y)  when e : Equiv T A. */
    kterm_t *et = kt_infer(heap, ctx, t->data.equiv_eps.equiv);
    kterm_t *A, *As, *f, *g, *gy, *fgy, *pi;
    if (et == NULL) return NULL;
    et = kt_whnf(heap, ctx, et);
    if (et->tag != KT_EQUIV) return NULL;
    A = et->data.equiv.b;
    As = kt_shift(heap, A, 0, 1);
    f = kt_shift(heap, kt_equiv_fun(heap, t->data.equiv_eps.equiv), 0, 1);
    g = kt_shift(heap, kt_equiv_inv(heap, t->data.equiv_eps.equiv), 0, 1);
    gy = kt_app(heap, g, kt_var(heap, 0));
    fgy = kt_app(heap, f, gy);
    pi = kt_alloc(heap, KT_PI); pi->data.pi.name = "y";
    pi->data.pi.domain = A; pi->data.pi.codomain = kt_id(heap, As, fgy, kt_var(heap, 0));
    return pi;
  }
  case KT_UNGLUE: {
    /* unglue A (cofib r b) T e g : A  when the Glue data is well-formed and
     * g : (Glue A (cofib r b) T e).  Result is A.  (We re-check the Glue is well-typed and
     * that g inhabits it.) */
    kterm_t *glue_ty, *gt;
    if (kt_infer(heap, ctx, t->data.unglue.cof) == NULL) return NULL;
    glue_ty = kt_glue(heap, t->data.unglue.base_type, t->data.unglue.cof,
                            t->data.unglue.glue_type, t->data.unglue.equiv);
    if (kt_infer(heap, ctx, glue_ty) == NULL) return NULL;   /* Glue well-formed */
    gt = kt_infer(heap, ctx, t->data.unglue.arg);
    if (gt == NULL) return NULL;
    if (!kt_equal(heap, ctx, gt, glue_ty)) return NULL;       /* g : Glue ... */
    return t->data.unglue.base_type;                          /* : A */
  }
  case KT_GTRANSP: {
    /* gtransp A (cofib r b) T e g0 : A  when the Glue data is well-formed and g0 : A (the base
     * inhabitant of the base type A).  Result is A.  (Like transp at a Glue line landing in the
     * base type; the id-equiv reduction returns g0 : A, so type-preserving.) */
    kterm_t *glue_ty, *bt;
    if (kt_infer(heap, ctx, t->data.gtransp.cof) == NULL) return NULL;
    glue_ty = kt_glue(heap, t->data.gtransp.base_type, t->data.gtransp.cof,
                            t->data.gtransp.glue_type, t->data.gtransp.equiv);
    if (kt_infer(heap, ctx, glue_ty) == NULL) return NULL;    /* Glue well-formed */
    bt = kt_infer(heap, ctx, t->data.gtransp.base);
    if (bt == NULL) return NULL;
    if (!kt_equal(heap, ctx, bt, t->data.gtransp.base_type)) return NULL;  /* g0 : A */
    return t->data.gtransp.base_type;                         /* : A */
  }
  case KT_GLUE_ELEM: {
    /* glue A (cofib r b) T e u a : Glue A (cofib r b) T e   when
     *   the Glue type is well-formed (A,T : Sort, e : Equiv T A),
     *   a : A (the base component),
     *   u : T (the T-component, relevant on the face), and
     *   the glue COHERENCE holds ON THE FACE: (equiv-fun e) u == a.
     * The result type is the Glue type.  Type-preservation: off-face glue = a : A and the Glue is
     * A there; on-face glue = u : T and the Glue is T there; the coherence ((equiv-fun e) u == a on
     * the face) is exactly the condition for unglue to be well-defined (unglue glue = (equiv-fun e) u
     * = a on the face).  The coherence is checked UNDER the face: for a face (cofib v ep) with v a de
     * Bruijn variable we restrict by substituting v := ep in both sides before comparing, so an
     * undecided face is handled by checking the equation holds once the face is imposed (this is what
     * lets the Glue-transport result, whose base is a comp that only reduces on the face, type-check). */
    kterm_t *glue_ty, *at, *ut, *cof, *fn, *fu;
    if (kt_infer(heap, ctx, t->data.glue_elem.cof) == NULL) return NULL;
    glue_ty = kt_glue(heap, t->data.glue_elem.base_type, t->data.glue_elem.cof,
                            t->data.glue_elem.glue_type, t->data.glue_elem.equiv);
    if (kt_infer(heap, ctx, glue_ty) == NULL) return NULL;        /* Glue well-formed */
    at = kt_infer(heap, ctx, t->data.glue_elem.base);
    if (at == NULL) return NULL;
    if (!kt_equal(heap, ctx, at, t->data.glue_elem.base_type)) return NULL;   /* a : A */
    /* the member u : T is a PARTIAL element — relevant only ON the face (where Glue = T), so it need
     * only type-check under the face restriction.  When φ = (cofib v ep) with v a variable we check
     * u : T in the restricted context (v := ep), which refines hypotheses (e.g. a Glue element used
     * inside u becomes a T-element).  This is what lets the Glue-transport member t1 = transp T g0
     * (which needs g0 : T on the face) type-check. */
    {
      kctx_t *cR = ctx;
      kterm_t *c0 = kt_whnf(heap, ctx, t->data.glue_elem.cof);
      if (c0->tag == KT_COFIB && c0->data.cofib.var->tag == KT_VAR) {
        cR = kctx_restrict(heap, ctx, c0->data.cofib.var->data.var.index, c0->data.cofib.endpoint);
      }
      ut = kt_infer(heap, cR, t->data.glue_elem.member);
      if (ut == NULL) return NULL;
      if (!kt_equal(heap, cR, ut, t->data.glue_elem.glue_type)) return NULL;  /* u : T (under the face) */
    }
    /* coherence on the face: (equiv-fun e) u == a, restricted to the face */
    fn = kt_equiv_fun(heap, t->data.glue_elem.equiv);
    fu = kt_app(heap, fn, t->data.glue_elem.member);             /* (equiv-fun e) u */
    cof = kt_whnf(heap, ctx, t->data.glue_elem.cof);
    {
      kterm_t *lhs = fu, *rhs = t->data.glue_elem.base;
      if (cof->tag == KT_COFIB) {
        kterm_t *v = kt_whnf(heap, ctx, cof->data.cofib.var);
        kterm_t *bb = kt_whnf(heap, ctx, cof->data.cofib.endpoint);
        int empty = (v->tag == KT_I0 && bb->tag == KT_I1) || (v->tag == KT_I1 && bb->tag == KT_I0);
        if (empty) {
          /* off the face there is no coherence constraint */
        } else {
          if (v->tag == KT_VAR) {
            lhs = kt_subst(heap, fu, v->data.var.index, cof->data.cofib.endpoint);
            rhs = kt_subst(heap, t->data.glue_elem.base, v->data.var.index, cof->data.cofib.endpoint);
          }
          if (!kt_equal(heap, ctx, lhs, rhs)) return NULL;       /* coherence */
        }
      }
    }
    return glue_ty;                                              /* : Glue A (cofib r b) T e */
  }
  case KT_COMP: {
    /* comp line (cofib r b) u u0 : A(i1)   when
     *   line = <i> A(i)  is a path-abstraction whose body is a Sort for i : I,
     *   the cofibration is well-formed,
     *   u0 : A(i0)  (the base inhabits the i0 endpoint), and
     *   on the face, the partial u agrees with u0 at i0 (the hcomp1 compatibility, lifted to i0).
     * The result type is A(i1).  Mirrors transp typing (reducing the line body under the binder so
     * endpoints resolve through any redex), plus the partial-compatibility check from hcomp1. */
    kterm_t *cline = t->data.comp.line;
    kterm_t *wline, *body_sort, *rbody, *at_i0, *at_i1, *ccof;
    kctx_t *e;
    if (cline == NULL) return NULL;
    if (kt_infer(heap, ctx, t->data.comp.cof) == NULL) return NULL;
    wline = kt_whnf(heap, ctx, cline);
    if (wline->tag != KT_PLAM) return NULL;
    e = kctx_extend(heap, ctx, wline->data.plam.name, kt_interval(heap), NULL);
    rbody = kt_whnf(heap, e, wline->data.plam.body);
    body_sort = kt_infer(heap, e, rbody);
    if (body_sort == NULL) return NULL;
    body_sort = kt_whnf(heap, e, body_sort);
    if (body_sort->tag != KT_SORT) return NULL;          /* A(i) must be a Sort */
    at_i0 = kt_subst(heap, rbody, 0, kt_i0(heap));
    at_i1 = kt_subst(heap, rbody, 0, kt_i1(heap));
    /* base u0 : A(i0) */
    if (kt_check(heap, ctx, t->data.comp.base, at_i0) != KERNEL_OK) return NULL;
    /* partial: either a plain term : A(i0) (the i0-given representation), OR a genuine i-varying
     * line <i>u(i) with u(i) : A(i) under the binder.  Dispatch on the syntactic form so existing
     * plain-term uses are unchanged while the i-varying partial is also accepted. */
    if (t->data.comp.partial->tag == KT_PLAM) {
      /* line partial: check u(i) : A(i).  This is a PARTIAL SECTION — it need only be well-typed
       * ON the face φ (off φ it is never used, since comp reduces to the partial only on a held face).
       * So when φ = (cofib v ep) with v a de Bruijn variable, we check the section in the context
       * RESTRICTED to the face (v := ep), which refines hypotheses whose types mention v — e.g. a Glue
       * type collapses to its T-component there, so a glue element becomes a T-element and the
       * Glue-transport correction's filler type-checks.  When φ is not of that form we check as a
       * total section (the previous behavior).  This is the partial-section typing the undecided-face
       * Glue transport needs, and it is sound because the partial is only consumed on φ. */
      kterm_t *u_body = t->data.comp.partial->data.plam.body;
      kterm_t *u_at_i0;
      kterm_t *ccof0 = kt_whnf(heap, ctx, t->data.comp.cof);
      kctx_t *eR = e;            /* section-checking context, possibly face-restricted */
      kctx_t *cR = ctx;          /* base-checking context, possibly face-restricted */
      if (ccof0->tag == KT_COFIB && ccof0->data.cofib.var->tag == KT_VAR) {
        int vidx = ccof0->data.cofib.var->data.var.index;
        kterm_t *ep = ccof0->data.cofib.endpoint;
        /* restrict by marking the face variable as a definition = ep.  No substitution into the term:
         * the variable reduces to ep on demand via whnf, so indices stay aligned (the face var sits at
         * vidx in ctx and vidx+1 in the interval-extended context e). */
        eR = kctx_restrict(heap, e, vidx + 1, ep);
        cR = kctx_restrict(heap, ctx, vidx, ep);
      }
      if (kt_check(heap, eR, u_body, rbody) != KERNEL_OK) return NULL;
      u_at_i0 = kt_subst(heap, u_body, 0, kt_i0(heap));
      if (kt_check(heap, cR, u_at_i0, at_i0) != KERNEL_OK) return NULL;
      ccof = kt_whnf(heap, ctx, t->data.comp.cof);
      if (ccof->tag == KT_COFIB &&
          kt_equal(heap, ctx, kt_whnf(heap, ctx, ccof->data.cofib.var),
                              kt_whnf(heap, ctx, ccof->data.cofib.endpoint))) {
        /* on a held face the partial's base value must agree with u0 (the compatibility) */
        if (!kt_equal(heap, ctx, u_at_i0, t->data.comp.base)) return NULL;
      }
      return at_i1;                                      /* : A(i1) */
    }
    /* partial u : A(i0) as well (it must agree with u0 at the i0 end); and when the face holds,
     * u must be definitionally equal to u0 (the hcomp1 compatibility) */
    if (kt_check(heap, ctx, t->data.comp.partial, at_i0) != KERNEL_OK) return NULL;
    ccof = kt_whnf(heap, ctx, t->data.comp.cof);
    if (ccof->tag == KT_COFIB &&
        kt_equal(heap, ctx, kt_whnf(heap, ctx, ccof->data.cofib.var),
                            kt_whnf(heap, ctx, ccof->data.cofib.endpoint))) {
      if (!kt_equal(heap, ctx, t->data.comp.partial, t->data.comp.base)) return NULL;
    }
    return at_i1;                                        /* : A(i1) */
  }
  case KT_COMP2: {
    /* comp2 line (cofib r1 b1) u1 (cofib r2 b2) u2 u0 : A(i1)   when
     *   line = <i>A(i) is a path-abstraction whose body is a Sort for i : I,
     *   both cofibrations are well-formed,
     *   u0 : A(i0),
     *   each partial uk is a path-abstraction <i>uk(i) with uk(i) : A(i) under the binder,
     *   the compatibility uk(i0) ≡ u0 holds UNDER the assumption that face k is satisfied (so the
     *     base agrees with the partial's base on the face — e.g. for a Path-line where the base is
     *     p@j and face1 is j=i0, this is u(i0) ≡ p@i0, which holds by the path's endpoint rule), and
     *   the overlap is coherent: under both faces, u1(i1) ≡ u2(i1).
     * The result type is A(i1). */
    kterm_t *cline = t->data.comp2.line;
    kterm_t *wline, *body_sort, *rbody, *at_i0, *at_i1;
    kterm_t *p1, *p2, *c1, *c2;
    kctx_t *e;
    if (cline == NULL) return NULL;
    if (kt_infer(heap, ctx, t->data.comp2.cof1) == NULL) return NULL;
    if (kt_infer(heap, ctx, t->data.comp2.cof2) == NULL) return NULL;
    wline = kt_whnf(heap, ctx, cline);
    if (wline->tag != KT_PLAM) return NULL;
    e = kctx_extend(heap, ctx, wline->data.plam.name, kt_interval(heap), NULL);
    rbody = kt_whnf(heap, e, wline->data.plam.body);
    body_sort = kt_infer(heap, e, rbody);
    if (body_sort == NULL) return NULL;
    body_sort = kt_whnf(heap, e, body_sort);
    if (body_sort->tag != KT_SORT) return NULL;          /* A(i) must be a Sort */
    at_i0 = kt_subst(heap, rbody, 0, kt_i0(heap));
    at_i1 = kt_subst(heap, rbody, 0, kt_i1(heap));
    if (kt_check(heap, ctx, t->data.comp2.base, at_i0) != KERNEL_OK) return NULL;  /* u0 : A(i0) */
    p1 = t->data.comp2.partial1;
    p2 = t->data.comp2.partial2;
    if (p1->tag != KT_PLAM || p2->tag != KT_PLAM) return NULL;   /* partials are i-lines */
    c1 = kt_whnf(heap, ctx, t->data.comp2.cof1);
    c2 = kt_whnf(heap, ctx, t->data.comp2.cof2);
    /* each partial is a PARTIAL SECTION: uk(i) : A(i) need only hold ON face k (off it, the partial is
     * never used — comp2 reduces to a partial only on its held face).  So we check each section in the
     * context RESTRICTED to its face (the de Bruijn variable defining face k set to its endpoint),
     * exactly as comp does — this is what lets the Glue-composition legs (unglue u, and
     * equiv-fun e (filler), each well-typed only on its face) type-check.  When face k is not of the
     * form (cofib v ep) with v a variable, the section is checked totally (the previous behavior). */
    {
      kctx_t *e1 = e, *e2 = e;
      int skip1 = 0, skip2 = 0;
      if (c1->tag == KT_COFIB && c1->data.cofib.var->tag == KT_VAR) {
        e1 = kctx_restrict(heap, e, c1->data.cofib.var->data.var.index + 1, c1->data.cofib.endpoint);
      }
      if (c2->tag == KT_COFIB && c2->data.cofib.var->tag == KT_VAR) {
        e2 = kctx_restrict(heap, e, c2->data.cofib.var->data.var.index + 1, c2->data.cofib.endpoint);
      }
      /* An EMPTY face imposes no constraint and its partial is vacuous, so do not type-check that
       * partial section: it need not (and for a non-constant line, cannot) have the section type away
       * from the face.  Decided-empty faces are detected via the cofibration helper. */
      skip1 = cofib_decided_empty(heap, ctx, c1);
      skip2 = cofib_decided_empty(heap, ctx, c2);
      if (!skip1 && kt_check(heap, e1, p1->data.plam.body, rbody) != KERNEL_OK) return NULL;
      if (!skip2 && kt_check(heap, e2, p2->data.plam.body, rbody) != KERNEL_OK) return NULL;
    }
    /* OVERLAP COHERENCE (essential for a two-system composition): where BOTH faces hold, the two
     * partials must agree — u1(i) ≡ u2(i) on (face1 ∧ face2).  We impose both face equations by
     * marking both face variables as definitions in the context (stacking the restrictions); no term
     * substitution, so indices stay aligned.  Skipped if either face is empty (the overlap is empty). */
    {
      int empty1 = 0, empty2 = 0;
      kctx_t *eo = e;
      if (c1->tag == KT_COFIB) {
        kterm_t *v = kt_whnf(heap, ctx, c1->data.cofib.var);
        kterm_t *bb = kt_whnf(heap, ctx, c1->data.cofib.endpoint);
        empty1 = (v->tag == KT_I0 && bb->tag == KT_I1) || (v->tag == KT_I1 && bb->tag == KT_I0);
        if (!empty1 && v->tag == KT_VAR)
          eo = kctx_restrict(heap, eo, v->data.var.index + 1, c1->data.cofib.endpoint);
      }
      if (c2->tag == KT_COFIB) {
        kterm_t *v = kt_whnf(heap, ctx, c2->data.cofib.var);
        kterm_t *bb = kt_whnf(heap, ctx, c2->data.cofib.endpoint);
        empty2 = (v->tag == KT_I0 && bb->tag == KT_I1) || (v->tag == KT_I1 && bb->tag == KT_I0);
        if (!empty2 && v->tag == KT_VAR)
          eo = kctx_restrict(heap, eo, v->data.var.index + 1, c2->data.cofib.endpoint);
      }
      if (!empty1 && !empty2) {
        if (!kt_equal(heap, eo, p1->data.plam.body, p2->data.plam.body)) return NULL;
      }
    }
    /* face-restricted compatibility: on each non-empty face k, the partial's base value uk(i0) must
     * agree with the comp2 base u0.  We compare under the restricted context (face var marked = ep),
     * with no term substitution. */
    {
      kterm_t *u1_at_i0 = kt_subst(heap, p1->data.plam.body, 0, kt_i0(heap));
      kterm_t *u2_at_i0 = kt_subst(heap, p2->data.plam.body, 0, kt_i0(heap));
      /* On a non-empty face k, the partial's base value uk(i0) must agree with the comp2 base u0.  We
       * compare them in the context RESTRICTED to face k (the face variable marked = its endpoint),
       * so both sides reduce on the face; no term substitution, indices stay aligned.  (For a Path-
       * line: on j=i0 the held face makes u(i0)=p@i0 by the path-endpoint rule; the other face is
       * empty and skipped.) */
      if (c1->tag == KT_COFIB) {
        kterm_t *v = kt_whnf(heap, ctx, c1->data.cofib.var);
        kterm_t *b = kt_whnf(heap, ctx, c1->data.cofib.endpoint);
        int empty = (v->tag == KT_I0 && b->tag == KT_I1) || (v->tag == KT_I1 && b->tag == KT_I0);
        if (!empty) {
          kctx_t *cr = ctx;
          if (v->tag == KT_VAR) cr = kctx_restrict(heap, ctx, v->data.var.index, c1->data.cofib.endpoint);
          if (!kt_equal(heap, cr, u1_at_i0, t->data.comp2.base)) return NULL;
        }
      }
      if (c2->tag == KT_COFIB) {
        kterm_t *v = kt_whnf(heap, ctx, c2->data.cofib.var);
        kterm_t *b = kt_whnf(heap, ctx, c2->data.cofib.endpoint);
        int empty = (v->tag == KT_I0 && b->tag == KT_I1) || (v->tag == KT_I1 && b->tag == KT_I0);
        if (!empty) {
          kctx_t *cr = ctx;
          if (v->tag == KT_VAR) cr = kctx_restrict(heap, ctx, v->data.var.index, c2->data.cofib.endpoint);
          if (!kt_equal(heap, cr, u2_at_i0, t->data.comp2.base)) return NULL;
        }
      }
    }
    return at_i1;                                        /* : A(i1) */
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
  kterm_t *inferred;
  /* Bidirectional rule for dependent pairs.  A pair's INFERRED type is always a NON-dependent
   * Sigma (its second component's type is inferred independently of the first), so a dependent
   * pair could never be checked against a dependent Sigma by infer-then-compare.  When the
   * expected type is a Sigma, check the components against it directly: fst : A, and snd : B[x:=fst].
   * This is the standard checking rule for Sigma introduction and is sound — it verifies exactly the
   * typing premises of the pair at the expected type, substituting the actual first component into
   * the (possibly dependent) second-component type. */
  if (term != NULL && term->tag == KT_PAIR) {
    kterm_t *wexp = kt_whnf(heap, ctx, expected_type);
    if (wexp->tag == KT_SIGMA) {
      kterm_t *snd_ty;
      if (kt_check(heap, ctx, term->data.pair.fst, wexp->data.sigma.fst_type) != KERNEL_OK)
        return KERNEL_TYPE_ERROR;
      snd_ty = kt_subst(heap, wexp->data.sigma.snd_type, 0, term->data.pair.fst);
      if (kt_check(heap, ctx, term->data.pair.snd, snd_ty) != KERNEL_OK)
        return KERNEL_TYPE_ERROR;
      return KERNEL_OK;
    }
  }
  inferred = kt_infer(heap, ctx, term);
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
  case KT_EQUIV:
    fprintf(fp, "(Equiv "); kt_fprint(fp, t->data.equiv.a);
    fprintf(fp, " "); kt_fprint(fp, t->data.equiv.b); fprintf(fp, ")");
    break;
  case KT_UA:
    fprintf(fp, "(ua "); kt_fprint(fp, t->data.ua.equiv); fprintf(fp, ")");
    break;
  case KT_TRANSP:
    fprintf(fp, "(transp "); kt_fprint(fp, t->data.transp.line);
    fprintf(fp, " "); kt_fprint(fp, t->data.transp.base); fprintf(fp, ")");
    break;
  case KT_INEG:
    fprintf(fp, "(~ "); kt_fprint(fp, t->data.ineg.arg); fprintf(fp, ")");
    break;
  case KT_IMEET:
    fprintf(fp, "(imeet "); kt_fprint(fp, t->data.imeet.left);
    fprintf(fp, " "); kt_fprint(fp, t->data.imeet.right); fprintf(fp, ")");
    break;
  case KT_IJOIN:
    fprintf(fp, "(ijoin "); kt_fprint(fp, t->data.ijoin.left);
    fprintf(fp, " "); kt_fprint(fp, t->data.ijoin.right); fprintf(fp, ")");
    break;
  case KT_ID_EQUIV:
    fprintf(fp, "(id-equiv "); kt_fprint(fp, t->data.id_equiv.type); fprintf(fp, ")");
    break;
  case KT_HCOMP:
    fprintf(fp, "(hcomp "); kt_fprint(fp, t->data.hcomp.type);
    fprintf(fp, " "); kt_fprint(fp, t->data.hcomp.base); fprintf(fp, ")");
    break;
  case KT_COFIB:
    fprintf(fp, "(cofib "); kt_fprint(fp, t->data.cofib.var);
    fprintf(fp, " "); kt_fprint(fp, t->data.cofib.endpoint); fprintf(fp, ")");
    break;
  case KT_COFIB_OR:
    fprintf(fp, "(cofib-or "); kt_fprint(fp, t->data.cofib_or.cof1);
    fprintf(fp, " "); kt_fprint(fp, t->data.cofib_or.cof2); fprintf(fp, ")");
    break;
  case KT_COFIB_FORALL:
    fprintf(fp, "(cofib-forall "); kt_fprint(fp, t->data.cofib_forall.body); fprintf(fp, ")");
    break;
  case KT_HCOMP1:
    fprintf(fp, "(hcomp1 "); kt_fprint(fp, t->data.hcomp1.type);
    fprintf(fp, " "); kt_fprint(fp, t->data.hcomp1.cof);
    fprintf(fp, " "); kt_fprint(fp, t->data.hcomp1.partial);
    fprintf(fp, " "); kt_fprint(fp, t->data.hcomp1.base); fprintf(fp, ")");
    break;
  case KT_COMP:
    fprintf(fp, "(comp "); kt_fprint(fp, t->data.comp.line);
    fprintf(fp, " "); kt_fprint(fp, t->data.comp.cof);
    fprintf(fp, " "); kt_fprint(fp, t->data.comp.partial);
    fprintf(fp, " "); kt_fprint(fp, t->data.comp.base); fprintf(fp, ")");
    break;
  case KT_COMP2:
    fprintf(fp, "(comp2 "); kt_fprint(fp, t->data.comp2.line);
    fprintf(fp, " "); kt_fprint(fp, t->data.comp2.cof1);
    fprintf(fp, " "); kt_fprint(fp, t->data.comp2.partial1);
    fprintf(fp, " "); kt_fprint(fp, t->data.comp2.cof2);
    fprintf(fp, " "); kt_fprint(fp, t->data.comp2.partial2);
    fprintf(fp, " "); kt_fprint(fp, t->data.comp2.base); fprintf(fp, ")");
    break;
  case KT_HCOMP2:
    fprintf(fp, "(hcomp2 "); kt_fprint(fp, t->data.hcomp2.type);
    fprintf(fp, " "); kt_fprint(fp, t->data.hcomp2.cof1);
    fprintf(fp, " "); kt_fprint(fp, t->data.hcomp2.partial1);
    fprintf(fp, " "); kt_fprint(fp, t->data.hcomp2.cof2);
    fprintf(fp, " "); kt_fprint(fp, t->data.hcomp2.partial2);
    fprintf(fp, " "); kt_fprint(fp, t->data.hcomp2.base); fprintf(fp, ")");
    break;
  case KT_GLUE:
    fprintf(fp, "(Glue "); kt_fprint(fp, t->data.glue.base_type);
    fprintf(fp, " "); kt_fprint(fp, t->data.glue.cof);
    fprintf(fp, " "); kt_fprint(fp, t->data.glue.glue_type);
    fprintf(fp, " "); kt_fprint(fp, t->data.glue.equiv); fprintf(fp, ")");
    break;
  case KT_PARTIAL:
    fprintf(fp, "(Partial "); kt_fprint(fp, t->data.partial.cof);
    fprintf(fp, " "); kt_fprint(fp, t->data.partial.type); fprintf(fp, ")");
    break;
  case KT_PSYS:
    fprintf(fp, "(psys "); kt_fprint(fp, t->data.psys.cof);
    fprintf(fp, " "); kt_fprint(fp, t->data.psys.type);
    fprintf(fp, " "); kt_fprint(fp, t->data.psys.elem); fprintf(fp, ")");
    break;
  case KT_EQUIV_FUN:
    fprintf(fp, "(equiv-fun "); kt_fprint(fp, t->data.equiv_fun.equiv); fprintf(fp, ")");
    break;
  case KT_EQUIV_INV:
    fprintf(fp, "(equiv-inv "); kt_fprint(fp, t->data.equiv_inv.equiv); fprintf(fp, ")");
    break;
  case KT_MK_EQUIV:
    fprintf(fp, "(mk-equiv "); kt_fprint(fp, t->data.mk_equiv.glue_type);
    fprintf(fp, " "); kt_fprint(fp, t->data.mk_equiv.base_type);
    fprintf(fp, " "); kt_fprint(fp, t->data.mk_equiv.fwd);
    fprintf(fp, " "); kt_fprint(fp, t->data.mk_equiv.inv);
    fprintf(fp, " "); kt_fprint(fp, t->data.mk_equiv.eta);
    fprintf(fp, " "); kt_fprint(fp, t->data.mk_equiv.eps); fprintf(fp, ")");
    break;
  case KT_EQUIV_ETA:
    fprintf(fp, "(equiv-eta "); kt_fprint(fp, t->data.equiv_eta.equiv); fprintf(fp, ")");
    break;
  case KT_EQUIV_EPS:
    fprintf(fp, "(equiv-eps "); kt_fprint(fp, t->data.equiv_eps.equiv); fprintf(fp, ")");
    break;
  case KT_UNGLUE:
    fprintf(fp, "(unglue "); kt_fprint(fp, t->data.unglue.base_type);
    fprintf(fp, " "); kt_fprint(fp, t->data.unglue.cof);
    fprintf(fp, " "); kt_fprint(fp, t->data.unglue.glue_type);
    fprintf(fp, " "); kt_fprint(fp, t->data.unglue.equiv);
    fprintf(fp, " "); kt_fprint(fp, t->data.unglue.arg); fprintf(fp, ")");
    break;
  case KT_GLUE_ELEM:
    fprintf(fp, "(glue "); kt_fprint(fp, t->data.glue_elem.base_type);
    fprintf(fp, " "); kt_fprint(fp, t->data.glue_elem.cof);
    fprintf(fp, " "); kt_fprint(fp, t->data.glue_elem.glue_type);
    fprintf(fp, " "); kt_fprint(fp, t->data.glue_elem.equiv);
    fprintf(fp, " "); kt_fprint(fp, t->data.glue_elem.member);
    fprintf(fp, " "); kt_fprint(fp, t->data.glue_elem.base); fprintf(fp, ")");
    break;
  case KT_GTRANSP:
    fprintf(fp, "(gtransp "); kt_fprint(fp, t->data.gtransp.base_type);
    fprintf(fp, " "); kt_fprint(fp, t->data.gtransp.cof);
    fprintf(fp, " "); kt_fprint(fp, t->data.gtransp.glue_type);
    fprintf(fp, " "); kt_fprint(fp, t->data.gtransp.equiv);
    fprintf(fp, " "); kt_fprint(fp, t->data.gtransp.base); fprintf(fp, ")");
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
  case KT_EQUIV:
    return kt_equiv(heap, meta_zonk(heap, mctx, t->data.equiv.a),
                          meta_zonk(heap, mctx, t->data.equiv.b));
  case KT_UA:
    return kt_ua(heap, meta_zonk(heap, mctx, t->data.ua.equiv));
  case KT_TRANSP:
    return kt_transp(heap, meta_zonk(heap, mctx, t->data.transp.line),
                           meta_zonk(heap, mctx, t->data.transp.base));
  case KT_INEG:
    return kt_ineg(heap, meta_zonk(heap, mctx, t->data.ineg.arg));
  case KT_IMEET:
    return kt_imeet(heap, meta_zonk(heap, mctx, t->data.imeet.left),
                          meta_zonk(heap, mctx, t->data.imeet.right));
  case KT_IJOIN:
    return kt_ijoin(heap, meta_zonk(heap, mctx, t->data.ijoin.left),
                          meta_zonk(heap, mctx, t->data.ijoin.right));
  case KT_ID_EQUIV:
    return kt_id_equiv(heap, meta_zonk(heap, mctx, t->data.id_equiv.type));
  case KT_HCOMP:
    return kt_hcomp(heap, meta_zonk(heap, mctx, t->data.hcomp.type),
                          meta_zonk(heap, mctx, t->data.hcomp.base));
  case KT_COFIB:
    return kt_cofib(heap, meta_zonk(heap, mctx, t->data.cofib.var),
                          meta_zonk(heap, mctx, t->data.cofib.endpoint));
  case KT_COFIB_OR:
    return kt_cofib_or(heap, meta_zonk(heap, mctx, t->data.cofib_or.cof1),
                             meta_zonk(heap, mctx, t->data.cofib_or.cof2));
  case KT_COFIB_FORALL:
    return kt_cofib_forall(heap, meta_zonk(heap, mctx, t->data.cofib_forall.body));
  case KT_HCOMP1:
    return kt_hcomp1(heap, meta_zonk(heap, mctx, t->data.hcomp1.type),
                           meta_zonk(heap, mctx, t->data.hcomp1.cof),
                           meta_zonk(heap, mctx, t->data.hcomp1.partial),
                           meta_zonk(heap, mctx, t->data.hcomp1.base));
  case KT_COMP:
    return kt_comp(heap, meta_zonk(heap, mctx, t->data.comp.line),
                         meta_zonk(heap, mctx, t->data.comp.cof),
                         meta_zonk(heap, mctx, t->data.comp.partial),
                         meta_zonk(heap, mctx, t->data.comp.base));
  case KT_COMP2:
    return kt_comp2(heap, meta_zonk(heap, mctx, t->data.comp2.line),
                          meta_zonk(heap, mctx, t->data.comp2.cof1),
                          meta_zonk(heap, mctx, t->data.comp2.partial1),
                          meta_zonk(heap, mctx, t->data.comp2.cof2),
                          meta_zonk(heap, mctx, t->data.comp2.partial2),
                          meta_zonk(heap, mctx, t->data.comp2.base));
  case KT_HCOMP2:
    return kt_hcomp2(heap, meta_zonk(heap, mctx, t->data.hcomp2.type),
                           meta_zonk(heap, mctx, t->data.hcomp2.cof1),
                           meta_zonk(heap, mctx, t->data.hcomp2.partial1),
                           meta_zonk(heap, mctx, t->data.hcomp2.cof2),
                           meta_zonk(heap, mctx, t->data.hcomp2.partial2),
                           meta_zonk(heap, mctx, t->data.hcomp2.base));
  case KT_GLUE:
    return kt_glue(heap, meta_zonk(heap, mctx, t->data.glue.base_type),
                         meta_zonk(heap, mctx, t->data.glue.cof),
                         meta_zonk(heap, mctx, t->data.glue.glue_type),
                         meta_zonk(heap, mctx, t->data.glue.equiv));
  case KT_PARTIAL:
    return kt_partial(heap, meta_zonk(heap, mctx, t->data.partial.cof),
                            meta_zonk(heap, mctx, t->data.partial.type));
  case KT_PSYS:
    return kt_psys(heap, meta_zonk(heap, mctx, t->data.psys.cof),
                         meta_zonk(heap, mctx, t->data.psys.type),
                         meta_zonk(heap, mctx, t->data.psys.elem));
  case KT_EQUIV_FUN:
    return kt_equiv_fun(heap, meta_zonk(heap, mctx, t->data.equiv_fun.equiv));
  case KT_EQUIV_INV:
    return kt_equiv_inv(heap, meta_zonk(heap, mctx, t->data.equiv_inv.equiv));
  case KT_MK_EQUIV:
    return kt_mk_equiv(heap, meta_zonk(heap, mctx, t->data.mk_equiv.glue_type),
                             meta_zonk(heap, mctx, t->data.mk_equiv.base_type),
                             meta_zonk(heap, mctx, t->data.mk_equiv.fwd),
                             meta_zonk(heap, mctx, t->data.mk_equiv.inv),
                             meta_zonk(heap, mctx, t->data.mk_equiv.eta),
                             meta_zonk(heap, mctx, t->data.mk_equiv.eps));
  case KT_EQUIV_ETA:
    return kt_equiv_eta(heap, meta_zonk(heap, mctx, t->data.equiv_eta.equiv));
  case KT_EQUIV_EPS:
    return kt_equiv_eps(heap, meta_zonk(heap, mctx, t->data.equiv_eps.equiv));
  case KT_UNGLUE:
    return kt_unglue(heap, meta_zonk(heap, mctx, t->data.unglue.base_type),
                           meta_zonk(heap, mctx, t->data.unglue.cof),
                           meta_zonk(heap, mctx, t->data.unglue.glue_type),
                           meta_zonk(heap, mctx, t->data.unglue.equiv),
                           meta_zonk(heap, mctx, t->data.unglue.arg));
  case KT_GLUE_ELEM:
    return kt_glue_elem(heap, meta_zonk(heap, mctx, t->data.glue_elem.base_type),
                              meta_zonk(heap, mctx, t->data.glue_elem.cof),
                              meta_zonk(heap, mctx, t->data.glue_elem.glue_type),
                              meta_zonk(heap, mctx, t->data.glue_elem.equiv),
                              meta_zonk(heap, mctx, t->data.glue_elem.member),
                              meta_zonk(heap, mctx, t->data.glue_elem.base));
  case KT_GTRANSP:
    return kt_gtransp(heap, meta_zonk(heap, mctx, t->data.gtransp.base_type),
                            meta_zonk(heap, mctx, t->data.gtransp.cof),
                            meta_zonk(heap, mctx, t->data.gtransp.glue_type),
                            meta_zonk(heap, mctx, t->data.gtransp.equiv),
                            meta_zonk(heap, mctx, t->data.gtransp.base));
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
  case KT_EQUIV:
    return kt_unify(heap, ctx, mctx, na->data.equiv.a, nb->data.equiv.a) &&
           kt_unify(heap, ctx, mctx, na->data.equiv.b, nb->data.equiv.b);
  case KT_UA:
    return kt_unify(heap, ctx, mctx, na->data.ua.equiv, nb->data.ua.equiv);
  case KT_TRANSP:
    return kt_unify(heap, ctx, mctx, na->data.transp.line, nb->data.transp.line) &&
           kt_unify(heap, ctx, mctx, na->data.transp.base, nb->data.transp.base);
  case KT_INEG:
    return kt_unify(heap, ctx, mctx, na->data.ineg.arg, nb->data.ineg.arg);
  case KT_IMEET:
    return kt_unify(heap, ctx, mctx, na->data.imeet.left, nb->data.imeet.left) &&
           kt_unify(heap, ctx, mctx, na->data.imeet.right, nb->data.imeet.right);
  case KT_IJOIN:
    return kt_unify(heap, ctx, mctx, na->data.ijoin.left, nb->data.ijoin.left) &&
           kt_unify(heap, ctx, mctx, na->data.ijoin.right, nb->data.ijoin.right);
  case KT_ID_EQUIV:
    return kt_unify(heap, ctx, mctx, na->data.id_equiv.type, nb->data.id_equiv.type);
  case KT_HCOMP:
    return kt_unify(heap, ctx, mctx, na->data.hcomp.type, nb->data.hcomp.type) &&
           kt_unify(heap, ctx, mctx, na->data.hcomp.base, nb->data.hcomp.base);
  case KT_COFIB:
    return kt_unify(heap, ctx, mctx, na->data.cofib.var, nb->data.cofib.var) &&
           kt_unify(heap, ctx, mctx, na->data.cofib.endpoint, nb->data.cofib.endpoint);
  case KT_COFIB_OR:
    return kt_unify(heap, ctx, mctx, na->data.cofib_or.cof1, nb->data.cofib_or.cof1) &&
           kt_unify(heap, ctx, mctx, na->data.cofib_or.cof2, nb->data.cofib_or.cof2);
  case KT_COFIB_FORALL:
    return kt_unify(heap, ctx, mctx, na->data.cofib_forall.body, nb->data.cofib_forall.body);
  case KT_HCOMP1:
    return kt_unify(heap, ctx, mctx, na->data.hcomp1.type, nb->data.hcomp1.type) &&
           kt_unify(heap, ctx, mctx, na->data.hcomp1.cof, nb->data.hcomp1.cof) &&
           kt_unify(heap, ctx, mctx, na->data.hcomp1.partial, nb->data.hcomp1.partial) &&
           kt_unify(heap, ctx, mctx, na->data.hcomp1.base, nb->data.hcomp1.base);
  case KT_COMP:
    return kt_unify(heap, ctx, mctx, na->data.comp.line, nb->data.comp.line) &&
           kt_unify(heap, ctx, mctx, na->data.comp.cof, nb->data.comp.cof) &&
           kt_unify(heap, ctx, mctx, na->data.comp.partial, nb->data.comp.partial) &&
           kt_unify(heap, ctx, mctx, na->data.comp.base, nb->data.comp.base);
  case KT_COMP2:
    return kt_unify(heap, ctx, mctx, na->data.comp2.line, nb->data.comp2.line) &&
           kt_unify(heap, ctx, mctx, na->data.comp2.cof1, nb->data.comp2.cof1) &&
           kt_unify(heap, ctx, mctx, na->data.comp2.partial1, nb->data.comp2.partial1) &&
           kt_unify(heap, ctx, mctx, na->data.comp2.cof2, nb->data.comp2.cof2) &&
           kt_unify(heap, ctx, mctx, na->data.comp2.partial2, nb->data.comp2.partial2) &&
           kt_unify(heap, ctx, mctx, na->data.comp2.base, nb->data.comp2.base);
  case KT_HCOMP2:
    return kt_unify(heap, ctx, mctx, na->data.hcomp2.type, nb->data.hcomp2.type) &&
           kt_unify(heap, ctx, mctx, na->data.hcomp2.cof1, nb->data.hcomp2.cof1) &&
           kt_unify(heap, ctx, mctx, na->data.hcomp2.partial1, nb->data.hcomp2.partial1) &&
           kt_unify(heap, ctx, mctx, na->data.hcomp2.cof2, nb->data.hcomp2.cof2) &&
           kt_unify(heap, ctx, mctx, na->data.hcomp2.partial2, nb->data.hcomp2.partial2) &&
           kt_unify(heap, ctx, mctx, na->data.hcomp2.base, nb->data.hcomp2.base);
  case KT_GLUE:
    return kt_unify(heap, ctx, mctx, na->data.glue.base_type, nb->data.glue.base_type) &&
           kt_unify(heap, ctx, mctx, na->data.glue.cof, nb->data.glue.cof) &&
           kt_unify(heap, ctx, mctx, na->data.glue.glue_type, nb->data.glue.glue_type) &&
           kt_unify(heap, ctx, mctx, na->data.glue.equiv, nb->data.glue.equiv);
  case KT_PARTIAL:
    return kt_unify(heap, ctx, mctx, na->data.partial.cof, nb->data.partial.cof) &&
           kt_unify(heap, ctx, mctx, na->data.partial.type, nb->data.partial.type);
  case KT_PSYS:
    return kt_unify(heap, ctx, mctx, na->data.psys.cof, nb->data.psys.cof) &&
           kt_unify(heap, ctx, mctx, na->data.psys.type, nb->data.psys.type) &&
           kt_unify(heap, ctx, mctx, na->data.psys.elem, nb->data.psys.elem);
  case KT_EQUIV_FUN:
    return kt_unify(heap, ctx, mctx, na->data.equiv_fun.equiv, nb->data.equiv_fun.equiv);
  case KT_EQUIV_INV:
    return kt_unify(heap, ctx, mctx, na->data.equiv_inv.equiv, nb->data.equiv_inv.equiv);
  case KT_MK_EQUIV:
    return kt_unify(heap, ctx, mctx, na->data.mk_equiv.glue_type, nb->data.mk_equiv.glue_type) &&
           kt_unify(heap, ctx, mctx, na->data.mk_equiv.base_type, nb->data.mk_equiv.base_type) &&
           kt_unify(heap, ctx, mctx, na->data.mk_equiv.fwd, nb->data.mk_equiv.fwd) &&
           kt_unify(heap, ctx, mctx, na->data.mk_equiv.inv, nb->data.mk_equiv.inv) &&
           kt_unify(heap, ctx, mctx, na->data.mk_equiv.eta, nb->data.mk_equiv.eta) &&
           kt_unify(heap, ctx, mctx, na->data.mk_equiv.eps, nb->data.mk_equiv.eps);
  case KT_EQUIV_ETA:
    return kt_unify(heap, ctx, mctx, na->data.equiv_eta.equiv, nb->data.equiv_eta.equiv);
  case KT_EQUIV_EPS:
    return kt_unify(heap, ctx, mctx, na->data.equiv_eps.equiv, nb->data.equiv_eps.equiv);
  case KT_UNGLUE:
    return kt_unify(heap, ctx, mctx, na->data.unglue.base_type, nb->data.unglue.base_type) &&
           kt_unify(heap, ctx, mctx, na->data.unglue.cof, nb->data.unglue.cof) &&
           kt_unify(heap, ctx, mctx, na->data.unglue.glue_type, nb->data.unglue.glue_type) &&
           kt_unify(heap, ctx, mctx, na->data.unglue.equiv, nb->data.unglue.equiv) &&
           kt_unify(heap, ctx, mctx, na->data.unglue.arg, nb->data.unglue.arg);
  case KT_GLUE_ELEM:
    return kt_unify(heap, ctx, mctx, na->data.glue_elem.base_type, nb->data.glue_elem.base_type) &&
           kt_unify(heap, ctx, mctx, na->data.glue_elem.cof, nb->data.glue_elem.cof) &&
           kt_unify(heap, ctx, mctx, na->data.glue_elem.glue_type, nb->data.glue_elem.glue_type) &&
           kt_unify(heap, ctx, mctx, na->data.glue_elem.equiv, nb->data.glue_elem.equiv) &&
           kt_unify(heap, ctx, mctx, na->data.glue_elem.member, nb->data.glue_elem.member) &&
           kt_unify(heap, ctx, mctx, na->data.glue_elem.base, nb->data.glue_elem.base);
  case KT_GTRANSP:
    return kt_unify(heap, ctx, mctx, na->data.gtransp.base_type, nb->data.gtransp.base_type) &&
           kt_unify(heap, ctx, mctx, na->data.gtransp.cof, nb->data.gtransp.cof) &&
           kt_unify(heap, ctx, mctx, na->data.gtransp.glue_type, nb->data.gtransp.glue_type) &&
           kt_unify(heap, ctx, mctx, na->data.gtransp.equiv, nb->data.gtransp.equiv) &&
           kt_unify(heap, ctx, mctx, na->data.gtransp.base, nb->data.gtransp.base);
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
  case KT_EQUIV: return const_occurs(t->data.equiv.a, name) || const_occurs(t->data.equiv.b, name);
  case KT_UA: return const_occurs(t->data.ua.equiv, name);
  case KT_TRANSP: return const_occurs(t->data.transp.line, name) || const_occurs(t->data.transp.base, name);
  case KT_INEG: return const_occurs(t->data.ineg.arg, name);
  case KT_IMEET: return const_occurs(t->data.imeet.left, name) || const_occurs(t->data.imeet.right, name);
  case KT_IJOIN: return const_occurs(t->data.ijoin.left, name) || const_occurs(t->data.ijoin.right, name);
  case KT_ID_EQUIV: return const_occurs(t->data.id_equiv.type, name);
  case KT_HCOMP: return const_occurs(t->data.hcomp.type, name) || const_occurs(t->data.hcomp.base, name);
  case KT_COFIB: return const_occurs(t->data.cofib.var, name) || const_occurs(t->data.cofib.endpoint, name);
  case KT_COFIB_OR: return const_occurs(t->data.cofib_or.cof1, name) || const_occurs(t->data.cofib_or.cof2, name);
  case KT_COFIB_FORALL: return const_occurs(t->data.cofib_forall.body, name);
  case KT_HCOMP1: return const_occurs(t->data.hcomp1.type, name) || const_occurs(t->data.hcomp1.cof, name)
              || const_occurs(t->data.hcomp1.partial, name) || const_occurs(t->data.hcomp1.base, name);
  case KT_COMP: return const_occurs(t->data.comp.line, name) || const_occurs(t->data.comp.cof, name)
              || const_occurs(t->data.comp.partial, name) || const_occurs(t->data.comp.base, name);
  case KT_COMP2: return const_occurs(t->data.comp2.line, name) || const_occurs(t->data.comp2.cof1, name)
              || const_occurs(t->data.comp2.partial1, name) || const_occurs(t->data.comp2.cof2, name)
              || const_occurs(t->data.comp2.partial2, name) || const_occurs(t->data.comp2.base, name);
  case KT_HCOMP2: return const_occurs(t->data.hcomp2.type, name) || const_occurs(t->data.hcomp2.cof1, name)
              || const_occurs(t->data.hcomp2.partial1, name) || const_occurs(t->data.hcomp2.cof2, name)
              || const_occurs(t->data.hcomp2.partial2, name) || const_occurs(t->data.hcomp2.base, name);
  case KT_GLUE: return const_occurs(t->data.glue.base_type, name) || const_occurs(t->data.glue.cof, name)
              || const_occurs(t->data.glue.glue_type, name) || const_occurs(t->data.glue.equiv, name);
  case KT_PARTIAL: return const_occurs(t->data.partial.cof, name) || const_occurs(t->data.partial.type, name);
  case KT_PSYS: return const_occurs(t->data.psys.cof, name) || const_occurs(t->data.psys.type, name)
              || const_occurs(t->data.psys.elem, name);
  case KT_EQUIV_FUN: return const_occurs(t->data.equiv_fun.equiv, name);
  case KT_EQUIV_INV: return const_occurs(t->data.equiv_inv.equiv, name);
  case KT_MK_EQUIV: return const_occurs(t->data.mk_equiv.glue_type, name) || const_occurs(t->data.mk_equiv.base_type, name)
              || const_occurs(t->data.mk_equiv.fwd, name) || const_occurs(t->data.mk_equiv.inv, name)
              || const_occurs(t->data.mk_equiv.eta, name) || const_occurs(t->data.mk_equiv.eps, name);
  case KT_EQUIV_ETA: return const_occurs(t->data.equiv_eta.equiv, name);
  case KT_EQUIV_EPS: return const_occurs(t->data.equiv_eps.equiv, name);
  case KT_UNGLUE: return const_occurs(t->data.unglue.base_type, name) || const_occurs(t->data.unglue.cof, name)
              || const_occurs(t->data.unglue.glue_type, name) || const_occurs(t->data.unglue.equiv, name)
              || const_occurs(t->data.unglue.arg, name);
  case KT_GLUE_ELEM: return const_occurs(t->data.glue_elem.base_type, name) || const_occurs(t->data.glue_elem.cof, name)
              || const_occurs(t->data.glue_elem.glue_type, name) || const_occurs(t->data.glue_elem.equiv, name)
              || const_occurs(t->data.glue_elem.member, name) || const_occurs(t->data.glue_elem.base, name);
  case KT_GTRANSP: return const_occurs(t->data.gtransp.base_type, name) || const_occurs(t->data.gtransp.cof, name)
              || const_occurs(t->data.gtransp.glue_type, name) || const_occurs(t->data.gtransp.equiv, name)
              || const_occurs(t->data.gtransp.base, name);
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
