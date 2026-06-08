/* opt_core_test.c — validates the optimal-reduction core.
 *
 *  (1) the AG interaction engine obeys every rule (rule-level self test);
 *  (2) the reference normaliser computes Church arithmetic;
 *  (3) the encode/reduce/read-back pipeline matches the reference on a
 *      fragment that INCLUDES function duplication;
 *  (4) it then PRINTS (does not assert) the documented gap: terms that need
 *      the full bracket oracle hit uncovered same-index pairs.
 *
 * Build: cc <flags> -Isrc tests/opt_core_test.c src/opt_core.c -o opt_core_test
 */
#include "opt_core.h"
#include <stdio.h>

static int fails = 0;

/* Church helpers */
static lc_term *church(int n) { lc_term *b = lc_var(0); int i; for (i = 0; i < n; i++) b = lc_app(lc_var(1), b); return lc_lam(lc_lam(b)); }
static lc_term *c_succ(void) { return lc_lam(lc_lam(lc_lam( lc_app(lc_var(1), lc_app(lc_app(lc_var(2), lc_var(1)), lc_var(0))) ))); }
static lc_term *c_add(void) { return lc_lam(lc_lam(lc_lam(lc_lam( lc_app(lc_app(lc_var(3), lc_var(1)), lc_app(lc_app(lc_var(2), lc_var(1)), lc_var(0))) )))); }
static lc_term *c_mul(void) { return lc_lam(lc_lam(lc_lam( lc_app(lc_var(2), lc_app(lc_var(1), lc_var(0))) ))); }

static void pr(const lc_term *t) {
  if (t->tag == LC_VAR) printf("%d", t->idx);
  else if (t->tag == LC_LAM) { printf("\\."); pr(t->f); }
  else { printf("("); pr(t->f); printf(" "); pr(t->a); printf(")"); }
}

/* reference must equal expected */
static void ref_check(const char *name, lc_term *got, lc_term *want) {
  lc_term *g = lc_nf_ref(got), *w = lc_nf_ref(want);
  if (lc_eq(g, w)) printf("    ok   %s\n", name);
  else { fails++; printf("    FAIL %s: got ", name); pr(g); printf(" want "); pr(w); printf("\n"); }
}

/* optimal reducer must reduce, cover, and match the reference */
static void opt_check(const char *name, lc_term *t) {
  long it = 0; int unc = 0;
  lc_term *no = opt_normalize(t, &it, &unc);
  lc_term *nr = lc_nf_ref(t);
  if (unc || !no) { fails++; printf("    FAIL %-10s UNCOVERED (%d pairs, %ld interactions)\n", name, unc, it); return; }
  if (lc_eq(no, nr)) printf("    ok   %-10s vs reference  (interactions=%ld)\n", name, it);
  else { fails++; printf("    FAIL %-10s opt=", name); pr(no); printf(" ref="); pr(nr); printf("\n"); }
}

/* documented gap: PRINT status only (these need the full bracket oracle) */
static void gap_report(const char *name, lc_term *t) {
  long it = 0; int unc = 0;
  lc_term *no = opt_normalize(t, &it, &unc);
  if (unc || !no) printf("    gap  %-10s needs full oracle (uncovered pairs=%d)\n", name, unc);
  else printf("    note %-10s unexpectedly covered (interactions=%ld)\n", name, it);
}

int main(void) {
  int eng;
  printf("[1] AG interaction engine — rule-level self test\n");
  eng = opt_engine_selftest();
  if (eng == 0) printf("    all engine rules validated\n"); else { fails += eng; printf("    %d engine rule checks FAILED\n", eng); }

  printf("[2] reference normal-order normaliser (ground truth)\n");
  ref_check("I I = I", lc_app(lc_lam(lc_var(0)), lc_lam(lc_var(0))), lc_lam(lc_var(0)));
  ref_check("succ 2 = 3", lc_app(c_succ(), church(2)), church(3));
  ref_check("add 2 3 = 5", lc_app(lc_app(c_add(), church(2)), church(3)), church(5));
  ref_check("mul 3 3 = 9", lc_app(lc_app(c_mul(), church(3)), church(3)), church(9));

  printf("[3] optimal reducer vs reference — validated fragment (incl. duplication)\n");
  opt_check("I",          lc_lam(lc_var(0)));
  opt_check("I I",        lc_app(lc_lam(lc_var(0)), lc_lam(lc_var(0))));
  opt_check("church2",    church(2));
  opt_check("(\\g.gg)I",  lc_app(lc_lam(lc_app(lc_var(0), lc_var(0))), lc_lam(lc_var(0))));
  opt_check("KII",        lc_app(lc_app(lc_lam(lc_lam(lc_var(1))), lc_lam(lc_var(0))), lc_lam(lc_var(0))));

  printf("[4] documented gap — needs the full bracket oracle (Lamping/Asperti-Guerrini)\n");
  gap_report("succ 2",   lc_app(c_succ(), church(2)));
  gap_report("add 2 3",  lc_app(lc_app(c_add(), church(2)), church(3)));
  gap_report("mul 2 3",  lc_app(lc_app(c_mul(), church(2)), church(3)));

  printf(fails ? "\n%d checks FAILED\n" : "\nopt_core: engine + reference + fragment all validated\n", fails);
  return fails ? 1 : 0;
}
