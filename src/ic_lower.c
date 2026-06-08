/* ic_lower.c — lower the core_term computational fragment to ic nets.
 * See ic_lower.h.  C89; depends only on ic.h and the C runtime (GMP via ic.h). */
#include "ic_lower.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ---- constructors ----------------------------------------------------- */
static core_t *calloc_node(core_kind_t k) {
  core_t *t = (core_t *)malloc(sizeof(core_t));   /* ownership-audit: allow */
  t->kind = k; t->idx = 0; t->op = 0; t->a = NULL; t->b = NULL;
  return t;
}
core_t *core_var(int i)              { core_t *t = calloc_node(CT_VAR); t->idx = i; return t; }
core_t *core_lam(core_t *body)       { core_t *t = calloc_node(CT_LAM); t->a = body; return t; }
core_t *core_app(core_t *f, core_t *a){ core_t *t = calloc_node(CT_APP); t->a = f; t->b = a; return t; }
core_t *core_pair(core_t *f, core_t *s){ core_t *t = calloc_node(CT_PAIR); t->a = f; t->b = s; return t; }
core_t *core_proj1(core_t *p)        { core_t *t = calloc_node(CT_PROJ1); t->a = p; return t; }
core_t *core_proj2(core_t *p)        { core_t *t = calloc_node(CT_PROJ2); t->a = p; return t; }
core_t *core_let(core_t *v, core_t *b){ core_t *t = calloc_node(CT_LET); t->a = v; t->b = b; return t; }
core_t *core_prim(int op, core_t *l, core_t *r) {
  core_t *t = calloc_node(CT_PRIM); t->op = op; t->a = l; t->b = r; return t;
}
core_t *core_lit_si(long v) {
  core_t *t = calloc_node(CT_LIT); mpz_init_set_si(t->num, v); return t;
}
core_t *core_lit_z(const mpz_t v) {
  core_t *t = calloc_node(CT_LIT); mpz_init_set(t->num, v); return t;
}
void core_free(core_t *t) {
  if (!t) return;
  core_free(t->a);
  core_free(t->b);
  if (t->kind == CT_LIT) mpz_clear(t->num);
  free(t);
}

/* ---- lowering --------------------------------------------------------- */
#define LOW_MAXDEPTH 4096

typedef struct {
  char names[LOW_MAXDEPTH][24];   /* de Bruijn binder name stack */
  int  depth;                     /* number of binders in scope  */
  int  fresh;                     /* fresh-name counter          */
  int  ok;                        /* cleared on overflow/malformed */
} LowCtx;

static void fresh_name(LowCtx *c, char *out) {
  sprintf(out, "#L%d", c->fresh++);
}

/* the two Sigma selectors: \a.\b.a and \a.\b.b, with fresh binder names. */
static ic_term_t *selector(LowCtx *c, int which) {
  char a[24], b[24];
  fresh_name(c, a);
  fresh_name(c, b);
  return ic_lam(a, ic_lam(b, ic_var(which == 1 ? a : b)));
}

static ic_term_t *lo(LowCtx *c, const core_t *t) {
  if (!t || !c->ok) { c->ok = 0; return ic_era(); }
  switch (t->kind) {
    case CT_VAR: {
      int slot = c->depth - 1 - t->idx;
      if (slot < 0 || slot >= c->depth) {        /* free / out of range */
        char nm[24];
        sprintf(nm, "#free%d", t->idx);
        return ic_var(nm);                        /* ic erases an unbound var */
      }
      return ic_var(c->names[slot]);
    }
    case CT_LAM: {
      char nm[24];
      ic_term_t *body;
      if (c->depth >= LOW_MAXDEPTH) { c->ok = 0; return ic_era(); }
      fresh_name(c, nm);
      strcpy(c->names[c->depth], nm);
      c->depth++;
      body = lo(c, t->a);
      c->depth--;
      return ic_lam(nm, body);
    }
    case CT_APP:
      return ic_app(lo(c, t->a), lo(c, t->b));
    case CT_LIT:
      return ic_num_z(t->num);
    case CT_PRIM:
      return ic_op(t->op, lo(c, t->a), lo(c, t->b));
    case CT_PAIR: {
      /* pair a b  ==>  \k. ((k a) b) */
      char k[24];
      ic_term_t *A = lo(c, t->a);
      ic_term_t *B = lo(c, t->b);
      fresh_name(c, k);
      return ic_lam(k, ic_app(ic_app(ic_var(k), A), B));
    }
    case CT_PROJ1:
      return ic_app(lo(c, t->a), selector(c, 1));
    case CT_PROJ2:
      return ic_app(lo(c, t->a), selector(c, 2));
    case CT_LET: {
      /* let _ = v in b  ==>  (\_. b) v ; v is in the OUTER scope */
      char nm[24];
      ic_term_t *V, *B;
      if (c->depth >= LOW_MAXDEPTH) { c->ok = 0; return ic_era(); }
      V = lo(c, t->a);                 /* value: outer scope, no new binder */
      fresh_name(c, nm);
      strcpy(c->names[c->depth], nm);
      c->depth++;
      B = lo(c, t->b);                 /* body: binds de Bruijn 0 */
      c->depth--;
      return ic_app(ic_lam(nm, B), V);
    }
  }
  c->ok = 0;
  return ic_era();
}

ic_term_t *ic_lower(const core_t *t) {
  LowCtx c;
  ic_term_t *r;
  c.depth = 0; c.fresh = 0; c.ok = 1;
  r = lo(&c, t);
  if (!c.ok) { ic_term_free(r); return NULL; }
  return r;
}
