/* ic_confluence_test.c — empirical strong confluence (Phase 17 foundation).
 *
 * Interaction nets have the one-step diamond property: reduction is a set of
 * local graph rewrites, so the normal form — and, notably, the NUMBER of
 * interactions taken to reach it — is independent of the order in which active
 * pairs are reduced.  That order-independence is exactly what makes parallel
 * scheduling sound: independent redexes can be fired concurrently without
 * changing the result.  This test reduces a spread of terms (recursion, exact
 * arithmetic, and binder-sharing that exercises DUP) under both the default LIFO
 * order and a FIFO order, and checks that BOTH the result and the interaction
 * count match.  True multithreading is the remaining Phase 17 work; this pins
 * the property it relies on.
 *
 * Build (standalone):
 *   cc -std=c89 -O2 -Isrc tests/ic_confluence_test.c src/ic.c -lgmp -o ic_confluence_test
 */
#include "ic.h"
#include <stdio.h>
#include <string.h>
#include <gmp.h>

static long fails = 0, checks = 0;

/* reduce t under both orders; require identical result AND interaction count */
static void confluent(const char *name, ic_term_t *lifo, ic_term_t *fifo) {
  mpz_t a, b; long ia = 0, ib = 0; int sa, sb;
  checks++;
  mpz_init(a); mpz_init(b);
  ic_set_reduce_fifo(0); sa = ic_normalize_int(lifo, a, &ia);
  ic_set_reduce_fifo(1); sb = ic_normalize_int(fifo, b, &ib);
  ic_set_reduce_fifo(0);                       /* restore default */
  if (sa != 1 || sb != 1 || mpz_cmp(a, b) != 0 || ia != ib) {
    fails++;
    gmp_printf("  FAIL %s: LIFO=%Zd(%ld) FIFO=%Zd(%ld)\n", name, a, ia, b, ib);
  }
  mpz_clear(a); mpz_clear(b);
  ic_term_free(lifo); ic_term_free(fifo);
}

/* builders for a couple of sharing-heavy expressions */
static ic_term_t *dbl(long v) {     /* (\x. x + x) v  -- the binder is used twice -> DUP */
  return ic_app(ic_lam("x", ic_op(IC_ADD, ic_var("x"), ic_var("x"))), ic_num_si(v));
}
static ic_term_t *sq(long v) {      /* (\x. x * x) v */
  return ic_app(ic_lam("x", ic_op(IC_MUL, ic_var("x"), ic_var("x"))), ic_num_si(v));
}
static ic_term_t *poly(long v) {    /* (\x. (x+1)*(x+x)) v -- x used three times */
  return ic_app(ic_lam("x", ic_op(IC_MUL,
                                  ic_op(IC_ADD, ic_var("x"), ic_num_si(1)),
                                  ic_op(IC_ADD, ic_var("x"), ic_var("x")))),
                ic_num_si(v));
}

int main(void) {
  long n;
  char label[64];

  /* recursion-as-cycles under both orders */
  for (n = 0; n <= 10; n++) {
    sprintf(label, "fact %ld", n);
    confluent(label, ic_ref(IC_DEF_FACT, ic_num_si(n)), ic_ref(IC_DEF_FACT, ic_num_si(n)));
  }
  for (n = 0; n <= 18; n++) {
    sprintf(label, "fib %ld", n);
    confluent(label, ic_ref(IC_DEF_FIB, ic_num_si(n)), ic_ref(IC_DEF_FIB, ic_num_si(n)));
  }
  confluent("sumto 100", ic_ref(IC_DEF_SUMTO, ic_num_si(100)), ic_ref(IC_DEF_SUMTO, ic_num_si(100)));
  confluent("pow2 16",   ic_ref(IC_DEF_POW2, ic_num_si(16)),   ic_ref(IC_DEF_POW2, ic_num_si(16)));
  confluent("gcd(1071,462)",
            ic_ref(IC_DEF_GCD, ic_num_si((1071L << 16) | 462L)),
            ic_ref(IC_DEF_GCD, ic_num_si((1071L << 16) | 462L)));

  /* binder-sharing (DUP) and arithmetic */
  for (n = 0; n <= 6; n++) {
    sprintf(label, "(\\x.x+x) %ld", n * 7);
    confluent(label, dbl(n * 7), dbl(n * 7));
    sprintf(label, "(\\x.x*x) %ld", n * 5);
    confluent(label, sq(n * 5), sq(n * 5));
    sprintf(label, "(\\x.(x+1)*(x+x)) %ld", n * 3);
    confluent(label, poly(n * 3), poly(n * 3));
  }

  if (fails == 0)
    printf("ALL %ld TERMS CONFLUENT (LIFO and FIFO agree on result AND interaction count)\n", checks);
  else
    printf("%ld/%ld confluence checks FAILED\n", fails, checks);
  return fails ? 1 : 0;
}
