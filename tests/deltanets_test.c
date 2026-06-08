/* deltanets_test.c — validates the Delta-Nets core reducer (Salvadori 2025).
 *
 * Section [1] asserts that translate -> reduce -> read-back matches the reference
 * normaliser on a fragment that INCLUDES function duplication, Church successor,
 * and Church ADDITION -- terms the bracket/croissant engine in opt_core could
 * not reduce (it hit uncovered same-index active pairs on `succ 2` already).
 * Delta-Nets has NO uncovered-pair case at all: every principal-principal pair
 * is covered by {annihilate, erase, commute}.
 *
 * Section [2] PRINTS (does not assert) the documented limitation: nonlinear
 * terms such as Church multiplication reduce cleanly but leave a cyclic net,
 * whose read-back needs the paper's non-interaction canonicalization rules plus
 * a global reduction order (not implemented here).  dn_normalize reports these
 * as cyclic rather than returning a wrong normal form.
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

/* Delta-Nets normal form must cover (not be cyclic) and match the reference */
static void dn_check(const char *name, lc_term *t) {
  long it = 0; int cyc = 0;
  lc_term *no = dn_normalize(t, &it, &cyc);
  lc_term *nr = lc_nf_ref(t);
  if (cyc || !no) { fails++; printf("    FAIL %-10s unexpectedly cyclic/null (interactions=%ld)\n", name, it); return; }
  if (lc_eq(no, nr)) printf("    ok   %-10s vs reference   interactions=%ld\n", name, it);
  else { fails++; printf("    FAIL %-10s dn=", name); pr(no); printf(" ref="); pr(nr); printf("\n"); }
}

/* documented limitation: report the cyclic-net cases (do NOT fail the suite) */
static void dn_gap(const char *name, lc_term *t) {
  long it = 0; int cyc = 0;
  lc_term *no = dn_normalize(t, &it, &cyc);
  if (cyc && !no) printf("    cyc  %-10s cyclic net -> needs canonicalization + global order (interactions=%ld)\n", name, it);
  else if (no)    printf("    note %-10s unexpectedly normalised (interactions=%ld)\n", name, it);
  else            printf("    note %-10s null without cycle flag (interactions=%ld)\n", name, it);
}

int main(void) {
  printf("[1] Delta-Nets core vs reference — validated fragment (function duplication, successor, ADDITION)\n");
  dn_check("I",          lc_lam(lc_var(0)));
  dn_check("I I",        lc_app(lc_lam(lc_var(0)), lc_lam(lc_var(0))));
  dn_check("church2",    church(2));
  dn_check("(\\g.gg)I",  lc_app(lc_lam(lc_app(lc_var(0), lc_var(0))), lc_lam(lc_var(0))));
  dn_check("KII",        lc_app(lc_app(lc_lam(lc_lam(lc_var(1))), lc_lam(lc_var(0))), lc_lam(lc_var(0))));
  dn_check("succ 2",     lc_app(c_succ(), church(2)));
  dn_check("add 2 3",    lc_app(lc_app(c_add(), church(2)), church(3)));

  printf("[2] documented limitation — nonlinear sharing leaves a cyclic net (needs canonicalization)\n");
  dn_gap("mul 2 3",      lc_app(lc_app(c_mul(), church(2)), church(3)));
  dn_gap("mul 3 3",      lc_app(lc_app(c_mul(), church(3)), church(3)));

  printf(fails ? "\n%d checks FAILED\n"
               : "\ndeltanets: core validated vs reference (incl. Church addition); no uncovered-pair case\n", fails);
  return fails ? 1 : 0;
}
