/* ic_parallel_test.c — wavefront reduction and the parallelism it exposes
 * (Phase 17, beyond the confluence foundation).
 *
 * All active pairs present at one moment are mutually disjoint, so a whole
 * frontier reduces as a batch.  Running in that wavefront mode records the
 * parallel DEPTH (number of rounds) and peak WIDTH (largest frontier); the
 * interaction count (WORK) is unchanged from sequential reduction.  work/depth
 * is the average parallelism a multicore reducer could exploit.
 *
 * For each term we first check that the wavefront result and work match the
 * sequential reducer (correctness), then print the profile.  Branching recursion
 * (fib) and balanced arithmetic expose high parallelism; tail recursion (gcd) and
 * left-nested arithmetic are essentially sequential.
 *
 * Build: cc -std=c89 -O2 -Isrc tests/ic_parallel_test.c src/ic.c -lgmp -o ic_parallel_test
 */
#include "ic.h"
#include <stdio.h>
#include <gmp.h>

static long fails = 0;

static void profile(const char *name, ic_term_t *seq, ic_term_t *par) {
  mpz_t a, b; long wseq = 0, wpar = 0, depth, peak; int sa, sb;
  mpz_init(a); mpz_init(b);
  ic_set_reduce_rounds(0); ic_set_reduce_fifo(0);
  sa = ic_normalize_int(seq, a, &wseq);                 /* sequential baseline */
  ic_set_reduce_rounds(1);
  sb = ic_normalize_int(par, b, &wpar);                 /* wavefront schedule  */
  depth = ic_reduce_rounds(); peak = ic_reduce_max_frontier();
  ic_set_reduce_rounds(0);
  if (sa != 1 || sb != 1 || mpz_cmp(a, b) != 0 || wseq != wpar) {
    fails++;
    gmp_printf("  FAIL %-18s seq=%Zd(work %ld)  wavefront=%Zd(work %ld)\n",
               name, a, wseq, b, wpar);
  } else {
    double avg = depth ? (double)wpar / (double)depth : 0.0;
    gmp_printf("  %-20s work=%-6ld depth=%-5ld peak=%-5ld avg-parallelism=%.2f\n",
               name, wpar, depth, peak, avg);
  }
  mpz_clear(a); mpz_clear(b);
  ic_term_free(seq); ic_term_free(par);
}

/* balanced sum of independent products: ((1*2)+(3*4)) + ((5*6)+(7*8)) ... */
static ic_term_t *wide(int leaves) {
  ic_term_t *acc = NULL;
  int i;
  for (i = 0; i < leaves; i++) {
    ic_term_t *prod = ic_op(IC_MUL, ic_num_si(i + 1), ic_num_si(i + 2));
    acc = acc ? ic_op(IC_ADD, acc, prod) : prod;
  }
  return acc;
}
/* left-nested increments: (((0+1)+1)+1)+... — inherently sequential */
static ic_term_t *deep(int n) {
  ic_term_t *acc = ic_num_si(0);
  int i;
  for (i = 0; i < n; i++) acc = ic_op(IC_ADD, acc, ic_num_si(1));
  return acc;
}

int main(void) {
  printf("term                   work=interactions  depth=rounds  peak=max frontier\n");
  printf("--------------------------------------------------------------------------\n");

  /* branching recursion: independent sub-calls -> high parallelism */
  profile("fib 10", ic_ref(IC_DEF_FIB, ic_num_si(10)), ic_ref(IC_DEF_FIB, ic_num_si(10)));
  profile("fib 15", ic_ref(IC_DEF_FIB, ic_num_si(15)), ic_ref(IC_DEF_FIB, ic_num_si(15)));
  profile("fib 18", ic_ref(IC_DEF_FIB, ic_num_si(18)), ic_ref(IC_DEF_FIB, ic_num_si(18)));

  /* tail / linear recursion: essentially sequential */
  profile("gcd(1071,462)",
          ic_ref(IC_DEF_GCD, ic_num_si((1071L << 16) | 462L)),
          ic_ref(IC_DEF_GCD, ic_num_si((1071L << 16) | 462L)));
  profile("sumto 60", ic_ref(IC_DEF_SUMTO, ic_num_si(60)), ic_ref(IC_DEF_SUMTO, ic_num_si(60)));
  profile("fact 10",  ic_ref(IC_DEF_FACT, ic_num_si(10)),  ic_ref(IC_DEF_FACT, ic_num_si(10)));

  /* arithmetic: balanced (parallel) vs left-nested (sequential) */
  profile("wide-arith (16)", wide(16), wide(16));
  profile("deep-arith (40)", deep(40), deep(40));

  printf("--------------------------------------------------------------------------\n");
  if (fails == 0)
    printf("OK: wavefront reduction matches sequential on result AND work for every term\n"
           "    (branching recursion and balanced arithmetic show the available parallelism)\n");
  else
    printf("%ld terms FAILED\n", fails);
  return fails ? 1 : 0;
}
