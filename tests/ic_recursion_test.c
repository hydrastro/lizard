/* ic_recursion_test.c — recursion-as-cycles in the interaction net.
 *
 * A recursive definition is represented as a self-referential graph: a (ref D a)
 * node whose unfolding (build_def_body) contains further (ref D ...) nodes.  The
 * cycle is unfolded lazily — a ref fires only once its argument has reduced to a
 * concrete number — and terminates because the definition branches on that
 * number, so the base case introduces no further ref.  This is the discipline an
 * interaction-net runtime (HVM) uses for its top-level definitions; the branch in
 * build_def_body plays the role of pattern matching on the constructor.
 *
 * Here we drive several classic recursive functions through the net and check the
 * results against straightforward C oracles, including a bignum factorial that
 * overflows a C long (so the GMP path is genuinely exercised).
 *
 * Build (standalone): cc -std=c89 -O2 -Isrc -Iinclude tests/ic_recursion_test.c \
 *                        src/ic.c -lgmp -o ic_recursion_test
 */
#include "ic.h"
#include <stdio.h>
#include <string.h>
#include <gmp.h>

static long fails = 0;
static long checks = 0;

/* run definition `def` on integer argument `arg`, return the net's result */
static long run_si(int def, long arg) {
  ic_term_t *t = ic_ref(def, ic_num_si(arg));
  mpz_t out; long it = 0, v; int st;
  mpz_init(out);
  st = ic_normalize_int(t, out, &it);
  v = (st == 1) ? mpz_get_si(out) : -999999L;
  mpz_clear(out);
  ic_term_free(t);
  return v;
}

static void expect(const char *what, long got, long want) {
  checks++;
  if (got != want) {
    fails++;
    printf("  FAIL %s: net=%ld expected=%ld\n", what, got, want);
  }
}

/* oracles */
static long o_fact(long n)  { long r = 1; while (n > 1) r *= n--; return r; }
static long o_sumto(long n) { return n * (n + 1) / 2; }
static long o_pow2(long n)  { long r = 1; while (n-- > 0) r *= 2; return r; }
static long o_fib(long n) {
  long a = 0, b = 1, i;
  if (n < 2) return n;
  for (i = 2; i <= n; i++) { long t = a + b; a = b; b = t; }
  return b;
}
static long o_gcd(long a, long b) { while (b) { long t = a % b; a = b; b = t; } return a; }

int main(void) {
  long n;
  char label[64];

  /* factorial: fact n = n=0 ? 1 : n * fact (n-1) */
  for (n = 0; n <= 12; n++) {
    sprintf(label, "fact %ld", n);
    expect(label, run_si(IC_DEF_FACT, n), o_fact(n));
  }

  /* triangular: sumto n = n=0 ? 0 : n + sumto (n-1) */
  for (n = 0; n <= 300; n += 13) {
    sprintf(label, "sumto %ld", n);
    expect(label, run_si(IC_DEF_SUMTO, n), o_sumto(n));
  }

  /* fibonacci: two recursive calls per step — a *branching* unfolding.  Naive
   * fib is exponential (~2*fib(n) ref-firings), so we keep n modest. */
  for (n = 0; n <= 22; n++) {
    sprintf(label, "fib %ld", n);
    expect(label, run_si(IC_DEF_FIB, n), o_fib(n));
  }

  /* pow2 */
  for (n = 0; n <= 24; n++) {
    sprintf(label, "pow2 %ld", n);
    expect(label, run_si(IC_DEF_POW2, n), o_pow2(n));
  }

  /* gcd: tail recursion (the call is in tail position), pair packed as a<<16|b */
  {
    long pr[][2] = { {48,36}, {1071,462}, {17,5}, {100,100}, {81,27}, {1000,1} };
    int i;
    for (i = 0; i < 6; i++) {
      long a = pr[i][0], b = pr[i][1];
      sprintf(label, "gcd(%ld,%ld)", a, b);
      expect(label, run_si(IC_DEF_GCD, (a << 16) | b), o_gcd(a, b));
    }
  }

  /* a bignum factorial: 25! overflows a 64-bit long, so this checks the GMP path */
  {
    ic_term_t *t = ic_ref(IC_DEF_FACT, ic_num_si(25));
    mpz_t out, want; long it = 0;
    mpz_init(out); mpz_init_set_str(want, "15511210043330985984000000", 10);
    checks++;
    if (ic_normalize_int(t, out, &it) != 1 || mpz_cmp(out, want) != 0) {
      fails++; gmp_printf("  FAIL fact 25: net=%Zd\n", out);
    }
    mpz_clear(out); mpz_clear(want); ic_term_free(t);
  }

  if (fails == 0)
    printf("ALL %ld RECURSION CHECKS PASSED (recursion-as-cycles)\n", checks);
  else
    printf("%ld/%ld recursion checks FAILED\n", fails, checks);
  return fails ? 1 : 0;
}
