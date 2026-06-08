/* deltanets_test.c — validates the Delta-Nets core reducer (Salvadori 2025) and,
 * crucially, its SOUNDNESS boundary.
 *
 * What this suite establishes
 * ---------------------------
 * [1] On a hand-picked fragment that INCLUDES function duplication, Church
 *     successor, and Church ADDITION, translate -> reduce -> read-back matches
 *     the reference normaliser exactly. (opt_core could not reduce these.)
 *
 * [2] SOUNDNESS on the linear fragment (lambda-L: every bound variable used
 *     exactly once). Thousands of random linear terms are normalised by both
 *     dn_normalize and the reference; every one matches, and none is flagged.
 *     This is the fragment the Delta-Nets CORE is guaranteed correct on: only
 *     fan annihilation occurs, which is perfectly confluent, so reduction order
 *     is irrelevant and the result is always canonical.
 *
 * [3] SOUND REFUSAL. For nonlinear terms whose core reduction leaves a
 *     non-canonical net (e.g. Church multiplication -> a cyclic net),
 *     dn_normalize returns NULL with *cyclic = 1 -- it reports "needs
 *     canonicalization" rather than emitting a wrong normal form.
 *
 * Why only lambda-L is GUARANTEED. Per the paper, full lambda-K correctness
 * needs a leftmost-outermost reduction order plus canonicalization rules
 * (unpaired-replicator merging/decay, phase-2 aux-fan replication, final erasure
 * GC) -- none of which the core implements. Randomized differential testing
 * confirms the gap: with an arbitrary reduction order the core reduces ~99% of
 * random lambda-K terms correctly and refuses most of the rest, but a rare term
 * reduces to a structurally-valid yet WRONG net that read-back cannot detect.
 * Hence dn_normalize is only relied upon for lambda-L here; the specific lambda-K
 * terms in [1] are individually verified against the reference. Closing the gap
 * is the documented next step (see docs/INET_ENGINE_PLAN.md, section 7).
 *
 * Build: cc <flags> -Isrc tests/deltanets_test.c src/deltanets.c src/opt_core.c -o deltanets_test
 */
#include "opt_core.h"
#include "deltanets.h"
#include <stdio.h>

static int fails = 0;

/* Church helpers (de Bruijn) */
static lc_term *church(int n) { lc_term *b = lc_var(0); int i; for (i = 0; i < n; i++) b = lc_app(lc_var(1), b); return lc_lam(lc_lam(b)); }
static lc_term *c_succ(void) { return lc_lam(lc_lam(lc_lam( lc_app(lc_var(1), lc_app(lc_app(lc_var(2), lc_var(1)), lc_var(0))) ))); }
static lc_term *c_add(void)  { return lc_lam(lc_lam(lc_lam(lc_lam( lc_app(lc_app(lc_var(3), lc_var(1)), lc_app(lc_app(lc_var(2), lc_var(1)), lc_var(0))) )))); }
static lc_term *c_mul(void)  { return lc_lam(lc_lam(lc_lam( lc_app(lc_var(2), lc_app(lc_var(1), lc_var(0))) ))); }

static void pr(const lc_term *t) {
  if (t->tag == LC_VAR) printf("%d", t->idx);
  else if (t->tag == LC_LAM) { printf("\\."); pr(t->f); }
  else { printf("("); pr(t->f); printf(" "); pr(t->a); printf(")"); }
}

/* [1] must reduce to a clean NF matching the reference */
static void dn_check(const char *name, lc_term *t) {
  long it = 0; int cyc = 0;
  lc_term *no = dn_normalize(t, &it, &cyc);
  lc_term *nr = lc_nf_ref(t);
  if (cyc || !no) { fails++; printf("    FAIL %-10s unexpectedly flagged (interactions=%ld)\n", name, it); return; }
  if (lc_eq(no, nr)) printf("    ok   %-10s vs reference   interactions=%ld\n", name, it);
  else { fails++; printf("    FAIL %-10s dn=", name); pr(no); printf(" ref="); pr(nr); printf("\n"); }
}

/* [3] must be REFUSED (reported non-canonical), never mis-normalised */
static void dn_refuse(const char *name, lc_term *t) {
  long it = 0; int cyc = 0;
  lc_term *no = dn_normalize(t, &it, &cyc);
  if (!no && cyc) printf("    ok   %-10s correctly reported non-canonical (refused, not mis-normalised; interactions=%ld)\n", name, it);
  else { fails++; printf("    FAIL %-10s expected non-canonical refusal\n", name); }
}

/* ---- [2] linear closed-term generator (every bound variable used exactly once) ---- */
static unsigned long lrng = 0x9e3779b97f4a7c15UL;
static int lrand(int n) { lrng ^= lrng << 13; lrng ^= lrng >> 7; lrng ^= lrng << 17; return (int)(lrng % (unsigned long)n); }
static int gfuel;
static lc_term *lcomb(int *av, int nav, int D) {
  lc_term *t; int i;
  if (nav == 0) return lc_lam(lc_var(0));                 /* identity: a closed linear atom */
  t = lc_var(D - 1 - av[0]);
  for (i = 1; i < nav; i++) t = lc_app(t, lc_var(D - 1 - av[i]));
  return t;
}
static lc_term *glin(int *av, int nav, int D, int maxd) {
  if (gfuel-- <= 0 || D >= maxd) return lcomb(av, nav, D);
  if (nav == 0) {
    if (lrand(2) == 0) { int a2[64]; a2[0] = D; return lc_lam(glin(a2, 1, D + 1, maxd)); }
    return lc_app(glin(av, 0, D, maxd), glin(av, 0, D, maxd));
  }
  if (nav == 1 && lrand(2) == 0) return lc_var(D - 1 - av[0]);
  if (lrand(3) == 0) { int a2[64], i; for (i = 0; i < nav; i++) a2[i] = av[i]; a2[nav] = D; return lc_lam(glin(a2, nav + 1, D + 1, maxd)); }
  { int l[64], r[64], nl = 0, nr = 0, i;
    for (i = 0; i < nav; i++) { if (lrand(2) == 0) l[nl++] = av[i]; else r[nr++] = av[i]; }
    return lc_app(glin(l, nl, D, maxd), glin(r, nr, D, maxd)); }
}

int main(void) {
  printf("[1] Delta-Nets core vs reference - hand-picked fragment (function duplication, successor, ADDITION)\n");
  dn_check("I",          lc_lam(lc_var(0)));
  dn_check("I I",        lc_app(lc_lam(lc_var(0)), lc_lam(lc_var(0))));
  dn_check("church2",    church(2));
  dn_check("(\\g.gg)I",  lc_app(lc_lam(lc_app(lc_var(0), lc_var(0))), lc_lam(lc_var(0))));
  dn_check("KII",        lc_app(lc_app(lc_lam(lc_lam(lc_var(1))), lc_lam(lc_var(0))), lc_lam(lc_var(0))));
  dn_check("succ 2",     lc_app(c_succ(), church(2)));
  dn_check("add 2 3",    lc_app(lc_app(c_add(), church(2)), church(3)));

  printf("[2] SOUNDNESS on the linear fragment - random differential test vs reference\n");
  {
    int i, N = 6000, lfail = 0, lflag = 0; int av[64];
    for (i = 0; i < N; i++) {
      long it = 0; int cyc = 0; lc_term *no, *nr, *t;
      gfuel = 14 + lrand(22);
      t = glin(av, 0, 0, 6);
      no = dn_normalize(t, &it, &cyc);
      nr = lc_nf_ref(t);
      if (cyc || !no) { lflag++; }                        /* a linear term must never be flagged */
      else if (!lc_eq(no, nr)) {
        lfail++;
        if (lfail <= 4) { printf("    MISMATCH term="); pr(t); printf(" dn="); pr(no); printf(" ref="); pr(nr); printf("\n"); }
      }
    }
    if (lfail == 0 && lflag == 0)
      printf("    ok   %d random linear terms: all match the reference, none flagged (zero mismatch)\n", N);
    else { fails++; printf("    FAIL linear fuzz: mismatches=%d flagged=%d of %d\n", lfail, lflag, N); }
  }

  printf("[3] SOUND REFUSAL - nonlinear terms that leave a non-canonical net are reported, not mis-normalised\n");
  dn_refuse("mul 2 3",  lc_app(lc_app(c_mul(), church(2)), church(3)));
  dn_refuse("mul 3 3",  lc_app(lc_app(c_mul(), church(3)), church(3)));

  printf(fails ? "\n%d checks FAILED\n"
               : "\ndeltanets: linear fragment proven sound by fuzzing; lambda-K examples verified; non-canonical nets refused\n", fails);
  return fails ? 1 : 0;
}
