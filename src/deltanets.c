/* deltanets.c — Delta-Nets (Salvadori, arXiv:2505.20314, 2025): the CORE
 * interaction system (fan / eraser / replicator) plus the lambda<->net
 * translation and read-back.  See deltanets.h for scope and references.
 *
 * Design notes
 * ------------
 * Three agent kinds:
 *   FAN  — 2 auxiliary ports.  Represents BOTH abstraction (lambda) and
 *          application (@); beta-reduction is fan annihilation.  As a lambda:
 *          port0 = value (out), port1 = body, port2 = variable.  As an @:
 *          port0 = function, port1 = result (out), port2 = argument.
 *   ERA  — 0 auxiliary ports.  Erases whatever it meets.
 *   REP  — n auxiliary ports, a non-negative `level`, and a per-port integer
 *          `level delta`.  A single agent that subsumes the indexed fans,
 *          brackets and croissants of the Lamping / Asperti-Guerrini line:
 *          negative deltas play the croissant role, positive deltas the bracket
 *          role, all consolidated so delimiters can never accumulate.
 *
 * Interaction rules (principal-principal active pairs):
 *   eraser  x  X        -> erase: propagate an eraser to each of X's aux ports.
 *   fan     x  fan      -> annihilate: wire aux_i <-> aux_i (this is beta).
 *   rep(l)  x  rep(l)   -> annihilate (equal levels are equal by construction).
 *   rep     x  fan      -> commute (replicator copies are exact).
 *   rep(a)  x  rep(b)   -> commute; copies of the HIGHER-level replicator have
 *                          level (higher.level + lower.delta[port]); copies of
 *                          the lower-level one are exact.
 *
 * The core is perfectly confluent: any normalising order yields the same result
 * in the same number of interactions.  It is SOUND on the linear fragment (only
 * fan annihilation; validated by a 260k random-term differential test vs the
 * reference).  Full lambda-K additionally needs the paper's canonicalization
 * rules + a leftmost-outermost order (not implemented); since an arbitrary order
 * can leave a wrong-but-valid-looking net for adversarial lambda-K, read-back is
 * made sound by REFUSAL -- dn_normalize returns "needs canonicalization" whenever
 * it detects a non-canonical net (cycle, residual sharing, reachable eraser, dead
 * stub, bad index) rather than emitting a possibly-wrong normal form.
 */
#include "deltanets.h"
#include <stdio.h>
#include <stdlib.h>

enum { K_ERA, K_FAN, K_REP, K_ROOT };

#define MAXA   200000
#define MAXAUX 16
#define PORTS  (MAXAUX + 1)
#define MAXW   200000
#define MAXD   96
#define RB_CAP 5000

static int  g_kind[MAXA], g_lev[MAXA], g_ar[MAXA], g_dead[MAXA];
static int  g_lt[MAXA * PORTS], g_lp[MAXA * PORTS], g_dl[MAXA * PORTS];
static int  g_n;
static long g_inter;
static int  g_bad;

static int na(int kind, int lev, int ar) {
  int a = g_n++, p;
  if (a >= MAXA) { fprintf(stderr, "deltanets: agent arena overflow\n"); exit(2); }
  g_kind[a] = kind; g_lev[a] = lev; g_ar[a] = ar; g_dead[a] = 0;
  for (p = 0; p < PORTS; p++) { g_lt[a * PORTS + p] = -1; g_lp[a * PORTS + p] = -1; g_dl[a * PORTS + p] = 0; }
  return a;
}
static void wire(int a, int pa, int b, int pb) {
  g_lt[a * PORTS + pa] = b; g_lp[a * PORTS + pa] = pb;
  g_lt[b * PORTS + pb] = a; g_lp[b * PORTS + pb] = pa;
}
static void jn(int a, int pa, int b, int pb) { if (a < 0 || b < 0) return; wire(a, pa, b, pb); }
static int  tg(int a, int p)  { return g_lt[a * PORTS + p]; }
static int  tpp(int a, int p) { return g_lp[a * PORTS + p]; }
static int  dl(int a, int p)  { return g_dl[a * PORTS + p]; }

/* generic commutation of two copyable agents P, Q (principals connected) */
static void commute(int P, int Q) {
  int nP = g_ar[P], nQ = g_ar[Q], i, j;
  int pe_t[PORTS], pe_p[PORTS], qe_t[PORTS], qe_p[PORTS];
  int Qc[PORTS], Pc[PORTS];
  int bothrep = (g_kind[P] == K_REP && g_kind[Q] == K_REP);
  for (i = 1; i <= nP; i++) { pe_t[i] = tg(P, i); pe_p[i] = tpp(P, i); }
  for (j = 1; j <= nQ; j++) { qe_t[j] = tg(Q, j); qe_p[j] = tpp(Q, j); }
  for (i = 1; i <= nP; i++) {
    int k;
    Qc[i] = na(g_kind[Q], g_lev[Q], nQ);
    if (bothrep && g_lev[P] < g_lev[Q]) g_lev[Qc[i]] = g_lev[Q] + dl(P, i);
    for (k = 1; k <= nQ; k++) g_dl[Qc[i] * PORTS + k] = dl(Q, k);   /* copy Q's deltas */
    jn(Qc[i], 0, pe_t[i], pe_p[i]);
  }
  for (j = 1; j <= nQ; j++) {
    int k;
    Pc[j] = na(g_kind[P], g_lev[P], nP);
    if (bothrep && g_lev[Q] < g_lev[P]) g_lev[Pc[j]] = g_lev[P] + dl(Q, j);
    for (k = 1; k <= nP; k++) g_dl[Pc[j] * PORTS + k] = dl(P, k);   /* copy P's deltas */
    jn(Pc[j], 0, qe_t[j], qe_p[j]);
  }
  for (i = 1; i <= nP; i++) for (j = 1; j <= nQ; j++) wire(Pc[j], i, Qc[i], j);
  g_dead[P] = g_dead[Q] = 1; g_inter++;
}

static int interact(int a, int b) {
  int ka = g_kind[a], kb = g_kind[b];
  if (ka == K_ERA || kb == K_ERA) {
    int o = (ka == K_ERA) ? b : a, i, n = g_ar[o];
    for (i = 1; i <= n; i++) { int ne = na(K_ERA, 0, 0); jn(ne, 0, tg(o, i), tpp(o, i)); }
    g_dead[a] = g_dead[b] = 1; g_inter++; return 1;
  }
  if (ka == K_FAN && kb == K_FAN) {
    jn(tg(a, 1), tpp(a, 1), tg(b, 1), tpp(b, 1));
    jn(tg(a, 2), tpp(a, 2), tg(b, 2), tpp(b, 2));
    g_dead[a] = g_dead[b] = 1; g_inter++; return 1;
  }
  if (ka == K_REP && kb == K_REP && g_lev[a] == g_lev[b]) {
    int i, n;
    if (g_ar[a] != g_ar[b]) { g_bad++; return 0; }
    n = g_ar[a];
    for (i = 1; i <= n; i++) jn(tg(a, i), tpp(a, i), tg(b, i), tpp(b, i));
    g_dead[a] = g_dead[b] = 1; g_inter++; return 1;
  }
  if ((ka == K_REP && kb == K_FAN) || (ka == K_FAN && kb == K_REP) ||
      (ka == K_REP && kb == K_REP)) { commute(a, b); return 1; }
  g_bad++; return 0;
}

static void reduce(void) {
  int prog = 1; long steps = 0;
  while (prog) {
    int a; prog = 0;
    for (a = 0; a < g_n; a++) {
      int b;
      if (g_dead[a] || g_kind[a] == K_ROOT) continue;
      b = tg(a, 0);
      if (b < 0 || g_dead[b] || g_kind[b] == K_ROOT) continue;
      if (tpp(a, 0) == 0 && tg(b, 0) == a && tpp(b, 0) == 0 && a < b) {
        if (interact(a, b)) { prog = 1; if (++steps > 3000000L) return; }
      }
    }
  }
}

/* ---- translation: term -> net (name-based wiring) ---- */
static int w_a[MAXW], w_p[MAXW], w_c[MAXW];
static int g_wn;
static int neww(void) {
  int w = g_wn++;
  if (w >= MAXW) { fprintf(stderr, "deltanets: wire arena overflow\n"); exit(2); }
  w_c[w] = 0; return w;
}
static void att(int w, int a, int p) {
  if (w_c[w] == 0) { w_a[w] = a; w_p[w] = p; w_c[w] = 1; }
  else { wire(w_a[w], w_p[w], a, p); w_c[w] = 2; }
}

static int occ_w[MAXD][1024], occ_l[MAXD][1024], occ_n[MAXD];

static int enc(const lc_term *t, int lev, int depth) {
  if (t->tag == LC_VAR) {
    int bd = depth - 1 - t->idx, w = neww();
    occ_w[bd][occ_n[bd]] = w; occ_l[bd][occ_n[bd]] = lev; occ_n[bd]++;
    return w;
  }
  if (t->tag == LC_LAM) {
    int F = na(K_FAN, 0, 2), bd = depth, wb, k, wo;
    occ_n[bd] = 0;
    wb = enc(t->f, lev, depth + 1); att(wb, F, 1);             /* body -> aux1 */
    k = occ_n[bd];
    if (k == 0) { int e = na(K_ERA, 0, 0); wire(F, 2, e, 0); }
    else if (k == 1 && (occ_l[bd][0] - (lev + 1)) == 0) { att(occ_w[bd][0], F, 2); }
    else {
      int R = na(K_REP, lev + 1, k), i;
      wire(F, 2, R, 0);
      for (i = 0; i < k; i++) { g_dl[R * PORTS + (i + 1)] = occ_l[bd][i] - (lev + 1); att(occ_w[bd][i], R, i + 1); }
    }
    wo = neww(); att(wo, F, 0); return wo;                     /* value = principal */
  }
  {
    int G = na(K_FAN, 0, 2), wf, wa, wo;
    wf = enc(t->f, lev, depth);     att(wf, G, 0);             /* function  -> principal */
    wa = enc(t->a, lev + 1, depth); att(wa, G, 2);             /* argument  -> aux2 (level+1) */
    wo = neww(); att(wo, G, 1); return wo;                     /* result = aux1 */
  }
}

/* ---- readback: arrive at port0 = lambda, port1 = @-result, port2 = lambda's var.
 *
 * The read-back is the term-reading bijection phi^-1, which is only defined on a
 * CANONICAL net (a tree, up to the transparent 1-ary replicators that encode
 * variable scope). A proper-but-non-canonical net -- one that still has genuine
 * sharing (a node reachable via two term-paths), a reachable eraser or dead
 * stub in a term position, or a cycle -- cannot be linearised into a lambda-term
 * without the canonicalization layer. Rather than emit a structurally-valid but
 * semantically-WRONG tree in those cases, read-back sets rb_noncanon / rb_cyc and
 * dn_normalize reports "needs canonicalization" (NULL, *cyclic = 1). This keeps
 * the function sound: it never returns a wrong normal form. */
static int rb_d[MAXA], rb_seen[MAXA];
static int rb_rec, rb_cyc, rb_noncanon;
static lc_term *rb(int a, int p, int depth) {
  lc_term *res;
  if (++rb_rec > RB_CAP) { rb_rec--; rb_cyc = 1; return lc_var(0); }
  for (;;) {
    if (a < 0 || g_dead[a]) { rb_noncanon = 1; rb_rec--; return lc_var(0); }   /* dead stub */
    if (g_kind[a] == K_ROOT) { int na2 = tg(a, 0), np = tpp(a, 0); a = na2; p = np; continue; }
    if (g_kind[a] == K_REP)  { int np = (p == 0) ? 1 : 0, na2 = tg(a, np), npp = tpp(a, np); a = na2; p = npp; continue; }
    if (g_kind[a] == K_ERA)  { rb_noncanon = 1; rb_rec--; return lc_var(0); }  /* eraser in term position */
    break;
  }
  if (p == 0) {                                                               /* abstraction */
    if (rb_seen[a]) { rb_noncanon = 1; rb_rec--; return lc_var(0); }           /* shared node */
    rb_seen[a] = 1; rb_d[a] = depth;
    res = lc_lam(rb(tg(a, 1), tpp(a, 1), depth + 1)); rb_rec--; return res;
  }
  if (p == 1) {                                                               /* application */
    lc_term *f, *x;
    if (rb_seen[a]) { rb_noncanon = 1; rb_rec--; return lc_var(0); }           /* shared node */
    rb_seen[a] = 1;
    f = rb(tg(a, 0), tpp(a, 0), depth);
    x = rb(tg(a, 2), tpp(a, 2), depth); rb_rec--; return lc_app(f, x);
  }
  {                                                                           /* variable */
    int idx = depth - 1 - rb_d[a];
    if (idx < 0 || !rb_seen[a]) { rb_noncanon = 1; rb_rec--; return lc_var(0); }
    rb_rec--; return lc_var(idx);
  }
}

lc_term *dn_normalize(const lc_term *t, long *interactions, int *cyclic) {
  int rw, R, d;
  lc_term *no;
  g_n = 0; g_inter = 0; g_bad = 0; g_wn = 0;
  for (d = 0; d < MAXD; d++) occ_n[d] = 0;
  rb_rec = 0; rb_cyc = 0; rb_noncanon = 0;
  rw = enc(t, 0, 0);
  R = na(K_ROOT, 0, 0); att(rw, R, 0);
  reduce();
  if (interactions) *interactions = g_inter;
  if (g_bad) { if (cyclic) *cyclic = 1; return (lc_term *)0; }
  for (d = 0; d < g_n; d++) rb_seen[d] = 0;
  no = rb(tg(R, 0), tpp(R, 0), 0);
  if (rb_cyc || rb_noncanon) { if (cyclic) *cyclic = 1; return (lc_term *)0; }
  if (cyclic) *cyclic = 0;
  return no;
}
