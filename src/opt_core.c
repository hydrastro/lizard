/* opt_core.c — see opt_core.h. Standalone (libc only). */
#include "opt_core.h"
#include <stdio.h>
#include <stdlib.h>

/* ===================== lambda terms + reference normaliser ===================== */

static lc_term *mk(lc_tag t) {
  lc_term *x = (lc_term *)calloc((size_t)1, sizeof(lc_term));
  if (!x) { fprintf(stderr, "oom\n"); exit(2); }
  x->tag = t; return x;
}
lc_term *lc_var(int i) { lc_term *x = mk(LC_VAR); x->idx = i; return x; }
lc_term *lc_lam(lc_term *b) { lc_term *x = mk(LC_LAM); x->f = b; return x; }
lc_term *lc_app(lc_term *f, lc_term *a) { lc_term *x = mk(LC_APP); x->f = f; x->a = a; return x; }

static lc_term *cp(const lc_term *t) {
  if (!t) return 0;
  if (t->tag == LC_VAR) return lc_var(t->idx);
  if (t->tag == LC_LAM) return lc_lam(cp(t->f));
  return lc_app(cp(t->f), cp(t->a));
}
static lc_term *sh(int d, int c, const lc_term *t) {
  if (t->tag == LC_VAR) return lc_var(t->idx >= c ? t->idx + d : t->idx);
  if (t->tag == LC_LAM) return lc_lam(sh(d, c + 1, t->f));
  return lc_app(sh(d, c, t->f), sh(d, c, t->a));
}
static lc_term *subst(int j, const lc_term *s, const lc_term *t) {
  if (t->tag == LC_VAR) return t->idx == j ? cp(s) : lc_var(t->idx);
  if (t->tag == LC_LAM) { lc_term *s1 = sh(1, 0, s); return lc_lam(subst(j + 1, s1, t->f)); }
  return lc_app(subst(j, s, t->f), subst(j, s, t->a));
}
static lc_term *beta(const lc_term *body, const lc_term *arg) {
  lc_term *s1 = sh(1, 0, arg);
  lc_term *r = subst(0, s1, body);
  return sh(-1, 0, r);
}
static long g_rsteps;
static lc_term *whnf(lc_term *t) {
  while (t->tag == LC_APP) {
    lc_term *f = whnf(t->f);
    if (f->tag == LC_LAM) { if (++g_rsteps > 2000000L) return t; t = beta(f->f, t->a); }
    else { t->f = f; break; }
  }
  return t;
}
static lc_term *nf(lc_term *t) {
  t = whnf(t);
  if (t->tag == LC_LAM) return lc_lam(nf(t->f));
  if (t->tag == LC_APP) return lc_app(nf(t->f), nf(t->a));
  return t;
}
lc_term *lc_nf_ref(const lc_term *t) { g_rsteps = 0; return nf(cp(t)); }
int lc_eq(const lc_term *a, const lc_term *b) {
  if (a->tag != b->tag) return 0;
  if (a->tag == LC_VAR) return a->idx == b->idx;
  if (a->tag == LC_LAM) return lc_eq(a->f, b->f);
  return lc_eq(a->f, b->f) && lc_eq(a->a, b->a);
}

/* ===================== AG interaction net engine ===================== */

enum { K_ERA, K_CRO, K_BRA, K_LAM, K_APP, K_FAN, K_ROOT };
static int arity(int k) { return (k == K_ERA || k == K_ROOT) ? 0 : ((k == K_CRO || k == K_BRA) ? 1 : 2); }

#define MAXA 300000
static int g_kind[MAXA], g_idx[MAXA], g_dead[MAXA];
static int g_lt[MAXA * 3], g_lp[MAXA * 3];
static int g_n;
static long g_inter;
static int g_uncovered;

static int newa(int k, int i) {
  int a = g_n++, p;
  if (a >= MAXA) { fprintf(stderr, "arena overflow\n"); exit(2); }
  g_kind[a] = k; g_idx[a] = i; g_dead[a] = 0;
  for (p = 0; p < 3; p++) { g_lt[a * 3 + p] = -1; g_lp[a * 3 + p] = -1; }
  return a;
}
static void wire(int a, int pa, int b, int pb) {
  g_lt[a * 3 + pa] = b; g_lp[a * 3 + pa] = pb;
  g_lt[b * 3 + pb] = a; g_lp[b * 3 + pb] = pa;
}
static void jn(int a, int pa, int b, int pb) { if (a < 0 || b < 0) return; wire(a, pa, b, pb); }
static int tg(int a, int p) { return g_lt[a * 3 + p]; }
static int tpp(int a, int p) { return g_lp[a * 3 + p]; }

/* one interaction on active pair (a.0 <-> b.0); returns 1 if a covered rule applied, 0 if uncovered */
static int interact(int a, int b) {
  int ka = g_kind[a], kb = g_kind[b], ia = g_idx[a], ib = g_idx[b];
  if (ka == K_ERA || kb == K_ERA) {
    int e = (ka == K_ERA) ? a : b, o = (ka == K_ERA) ? b : a, k, n = arity(g_kind[o]);
    for (k = 1; k <= n; k++) { int ne = newa(K_ERA, g_idx[e]); jn(ne, 0, tg(o, k), tpp(o, k)); }
    g_dead[a] = g_dead[b] = 1; g_inter++; return 1;
  }
  if ((ka == K_LAM && kb == K_APP) || (ka == K_APP && kb == K_LAM)) {
    jn(tg(a, 1), tpp(a, 1), tg(b, 1), tpp(b, 1));
    jn(tg(a, 2), tpp(a, 2), tg(b, 2), tpp(b, 2));
    g_dead[a] = g_dead[b] = 1; g_inter++; return 1;
  }
  if (ka == K_FAN && kb == K_FAN && ia == ib) {
    jn(tg(a, 1), tpp(a, 1), tg(b, 1), tpp(b, 1));
    jn(tg(a, 2), tpp(a, 2), tg(b, 2), tpp(b, 2));
    g_dead[a] = g_dead[b] = 1; g_inter++; return 1;
  }
  if (ka == K_CRO && kb == K_CRO && ia == ib) { jn(tg(a, 1), tpp(a, 1), tg(b, 1), tpp(b, 1)); g_dead[a] = g_dead[b] = 1; g_inter++; return 1; }
  if (ka == K_BRA && kb == K_BRA && ia == ib) { jn(tg(a, 1), tpp(a, 1), tg(b, 1), tpp(b, 1)); g_dead[a] = g_dead[b] = 1; g_inter++; return 1; }
  if (ka == K_FAN || kb == K_FAN) {
    int f = (ka == K_FAN) ? a : b, o = (ka == K_FAN) ? b : a, fi, oj, k, n, b1, b2;
    if (g_kind[o] == K_FAN) { if (g_idx[f] > g_idx[o]) { int t = f; f = o; o = t; } }
    fi = g_idx[f]; oj = g_idx[o]; n = arity(g_kind[o]);
    b1 = newa(g_kind[o], oj); b2 = newa(g_kind[o], oj);
    jn(b1, 0, tg(f, 1), tpp(f, 1)); jn(b2, 0, tg(f, 2), tpp(f, 2));
    for (k = 1; k <= n; k++) { int fk = newa(K_FAN, fi); jn(fk, 0, tg(o, k), tpp(o, k)); wire(fk, 1, b1, k); wire(fk, 2, b2, k); }
    g_dead[a] = g_dead[b] = 1; g_inter++; return 1;
  }
  if (ka == K_CRO || kb == K_CRO || ka == K_BRA || kb == K_BRA) {
    int c = (ka == K_CRO || ka == K_BRA) ? a : b, o = (ka == K_CRO || ka == K_BRA) ? b : a;
    int ck = g_kind[c], ci = g_idx[c], oj = g_idx[o], k, n = arity(g_kind[o]), d = (ck == K_BRA) ? 1 : -1, b1;
    if (!(ci < oj)) { g_uncovered++; return 0; }
    b1 = newa(g_kind[o], oj + d); jn(b1, 0, tg(c, 1), tpp(c, 1));
    for (k = 1; k <= n; k++) { int c2 = newa(ck, ci); jn(c2, 0, tg(o, k), tpp(o, k)); wire(c2, 1, b1, k); }
    g_dead[a] = g_dead[b] = 1; g_inter++; return 1;
  }
  g_uncovered++; return 0;
}
static void reduce_all(void) {
  int progress = 1; long steps = 0;
  while (progress) {
    int a; progress = 0;
    for (a = 0; a < g_n; a++) {
      int b;
      if (g_dead[a] || g_kind[a] == K_ROOT) continue;
      b = tg(a, 0);
      if (b < 0 || g_dead[b] || g_kind[b] == K_ROOT) continue;
      if (tpp(a, 0) == 0 && tg(b, 0) == a && tpp(b, 0) == 0 && a < b) {
        if (interact(a, b)) { progress = 1; if (++steps > 4000000L) return; }
      }
    }
  }
}

/* ===================== name-based encoding + read-back ===================== */

#define MAXW 300000
static int w_a[MAXW], w_p[MAXW], w_cnt[MAXW];
static int g_wn;
static int neww(void) { int w = g_wn++; if (w >= MAXW) { fprintf(stderr, "wire overflow\n"); exit(2); } w_cnt[w] = 0; return w; }
static void att(int w, int a, int p) {
  if (w_cnt[w] == 0) { w_a[w] = a; w_p[w] = p; w_cnt[w] = 1; }
  else { wire(w_a[w], w_p[w], a, p); w_cnt[w] = 2; }
}

#define MAXDEPTH 80
#define MAXOCC 100000
static int occ_w[MAXDEPTH][MAXOCC], occ_n[MAXDEPTH], blevel[MAXDEPTH];

/* returns the wire id of the OUT edge of the encoded term */
static int enc(const lc_term *t, int level, int depth) {
  if (t->tag == LC_VAR) {
    int bd = depth - 1 - t->idx, bl = blevel[bd], cur = level;
    if (cur == bl) { int w = neww(); occ_w[bd][occ_n[bd]++] = w; return w; }
    {
      int wu = neww(), wb = neww(), i, prev_a = -1, prev_p = -1;
      for (i = bl; i < cur; i++) {
        int c = newa(K_CRO, i);
        if (i == bl) att(wb, c, 0); else wire(c, 0, prev_a, prev_p);
        prev_a = c; prev_p = 1;
      }
      att(wu, prev_a, prev_p);
      occ_w[bd][occ_n[bd]++] = wb;
      return wu;
    }
  }
  if (t->tag == LC_LAM) {
    int Ln = newa(K_LAM, level), bd = depth, wb, k;
    blevel[bd] = level; occ_n[bd] = 0;
    wb = enc(t->f, level, depth + 1); att(wb, Ln, 2);     /* body, same level */
    k = occ_n[bd];
    if (k == 0) { int e = newa(K_ERA, level); wire(Ln, 1, e, 0); }
    else if (k == 1) { att(occ_w[bd][0], Ln, 1); }
    else {
      int cur = occ_w[bd][0], i;
      for (i = 1; i < k; i++) { int F = newa(K_FAN, level); att(cur, F, 1); att(occ_w[bd][i], F, 2); cur = neww(); att(cur, F, 0); }
      att(cur, Ln, 1);
    }
    { int wo = neww(); att(wo, Ln, 0); return wo; }
  }
  {
    int An = newa(K_APP, level), wf, wa, wo;
    wf = enc(t->f, level, depth);     att(wf, An, 0);     /* function */
    wa = enc(t->a, level + 1, depth); att(wa, An, 1);     /* argument boxed (level+1) */
    wo = neww(); att(wo, An, 2);
    return wo;
  }
}

static int rb_d[MAXA];
static lc_term *readback(int a, int p, int depth) {
  for (;;) {
    if (a < 0 || g_dead[a]) return lc_var(900);
    if (g_kind[a] == K_ROOT) { int na = tg(a, 0), npp = tpp(a, 0); a = na; p = npp; continue; }
    if (g_kind[a] == K_CRO || g_kind[a] == K_BRA) { int np = (p == 0) ? 1 : 0, na = tg(a, np), npp = tpp(a, np); a = na; p = npp; continue; }
    if (g_kind[a] == K_FAN) { int np = (p == 0) ? 1 : 0, na = tg(a, np), npp = tpp(a, np); a = na; p = npp; continue; }
    break;
  }
  if (g_kind[a] == K_LAM) {
    if (p == 1) return lc_var(depth - 1 - rb_d[a]);
    rb_d[a] = depth;
    return lc_lam(readback(tg(a, 2), tpp(a, 2), depth + 1));
  }
  if (g_kind[a] == K_APP) {
    if (p == 2) { lc_term *f = readback(tg(a, 0), tpp(a, 0), depth); lc_term *x = readback(tg(a, 1), tpp(a, 1), depth); return lc_app(f, x); }
    return lc_var(901);
  }
  return lc_var(902);
}

lc_term *opt_normalize(const lc_term *t, long *interactions, int *uncovered) {
  int rw, R, d;
  g_n = 0; g_inter = 0; g_uncovered = 0; g_wn = 0;
  for (d = 0; d < MAXDEPTH; d++) occ_n[d] = 0;
  rw = enc(t, 0, 0);
  R = newa(K_ROOT, 0); att(rw, R, 0);
  reduce_all();
  if (interactions) *interactions = g_inter;
  if (uncovered) *uncovered = g_uncovered;
  if (g_uncovered) return 0;
  return readback(tg(R, 0), tpp(R, 0), 0);
}

/* ===================== rule-level engine self-test ===================== */

static int st_fails;
static void expect(int cond, const char *msg) {
  if (!cond) { st_fails++; printf("    FAIL %s\n", msg); }
  else printf("    ok   %s\n", msg);
}
int opt_engine_selftest(void) {
  st_fails = 0;
  /* beta: LAM_0 <-> APP_0 wires binder<->arg, body<->result */
  {
    int L, Ap, b1, b2, a1, a2;
    g_n = 0; g_inter = 0; g_uncovered = 0;
    L = newa(K_LAM, 0); Ap = newa(K_APP, 0);
    b1 = newa(K_CRO, 11); b2 = newa(K_CRO, 12); a1 = newa(K_CRO, 13); a2 = newa(K_CRO, 14);
    wire(L, 1, b1, 1); wire(L, 2, b2, 1); wire(Ap, 1, a1, 1); wire(Ap, 2, a2, 1); wire(L, 0, Ap, 0);
    expect(interact(L, Ap) == 1, "beta covered");
    expect(tg(b1, 1) == a1 && tg(b2, 1) == a2, "beta wires binder<->arg, body<->result");
  }
  /* fan FAN_0 duplicates LAM_1 -> two LAM_1, fans pushed onto aux */
  {
    int F, L, o1, o2, bnd, bod, c1, c2, fb;
    g_n = 0;
    F = newa(K_FAN, 0); L = newa(K_LAM, 1);
    o1 = newa(K_CRO, 21); o2 = newa(K_CRO, 22); bnd = newa(K_CRO, 23); bod = newa(K_CRO, 24);
    wire(F, 1, o1, 1); wire(F, 2, o2, 1); wire(L, 1, bnd, 1); wire(L, 2, bod, 1); wire(F, 0, L, 0);
    expect(interact(F, L) == 1, "fan x lambda (i<j) covered");
    c1 = tg(o1, 1); c2 = tg(o2, 1); fb = tg(bnd, 1);
    expect(c1 >= 0 && g_kind[c1] == K_LAM && g_idx[c1] == 1, "fan duplicates lambda: copy is LAM_1");
    expect(c2 >= 0 && g_kind[c2] == K_LAM && c2 != c1, "fan duplicates lambda: two distinct copies");
    expect(fb >= 0 && g_kind[fb] == K_FAN && g_idx[fb] == 0, "fan pushed onto lambda binder aux");
  }
  /* bracket increments, croissant decrements */
  {
    int B, L, o, c;
    g_n = 0; B = newa(K_BRA, 0); L = newa(K_LAM, 1);
    o = newa(K_CRO, 31); newa(K_CRO, 32); newa(K_CRO, 33);
    wire(B, 1, o, 1); wire(L, 1, 3, 1); wire(L, 2, 4, 1); wire(B, 0, L, 0);
    expect(interact(B, L) == 1, "bracket x lambda covered");
    c = tg(o, 1); expect(c >= 0 && g_kind[c] == K_LAM && g_idx[c] == 2, "bracket increments LAM_1 -> LAM_2");
  }
  {
    int C, L, o, c;
    g_n = 0; C = newa(K_CRO, 0); L = newa(K_LAM, 2);
    o = newa(K_CRO, 41); newa(K_CRO, 42); newa(K_CRO, 43);
    wire(C, 1, o, 1); wire(L, 1, 3, 1); wire(L, 2, 4, 1); wire(C, 0, L, 0);
    expect(interact(C, L) == 1, "croissant x lambda covered");
    c = tg(o, 1); expect(c >= 0 && g_kind[c] == K_LAM && g_idx[c] == 1, "croissant decrements LAM_2 -> LAM_1");
  }
  /* fan-fan same index annihilate, straight wiring */
  {
    int F1, F2, x1, x2, y1, y2;
    g_n = 0; F1 = newa(K_FAN, 5); F2 = newa(K_FAN, 5);
    x1 = newa(K_CRO, 51); x2 = newa(K_CRO, 52); y1 = newa(K_CRO, 53); y2 = newa(K_CRO, 54);
    wire(F1, 1, x1, 1); wire(F1, 2, x2, 1); wire(F2, 1, y1, 1); wire(F2, 2, y2, 1); wire(F1, 0, F2, 0);
    expect(interact(F1, F2) == 1, "fan-fan same index covered");
    expect(tg(x1, 1) == y1 && tg(x2, 1) == y2, "fan-fan annihilate wires straight through");
  }
  /* eraser erases + propagates */
  {
    int E, L, bnd, bod, e1, e2;
    g_n = 0; E = newa(K_ERA, 0); L = newa(K_LAM, 3); bnd = newa(K_CRO, 61); bod = newa(K_CRO, 62);
    wire(L, 1, bnd, 1); wire(L, 2, bod, 1); wire(E, 0, L, 0);
    expect(interact(E, L) == 1, "eraser x lambda covered");
    e1 = tg(bnd, 1); e2 = tg(bod, 1);
    expect(e1 >= 0 && g_kind[e1] == K_ERA && e2 >= 0 && g_kind[e2] == K_ERA, "eraser propagates to both aux");
  }
  return st_fails;
}
