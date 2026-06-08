/* ic_sharing_test.c — what the labelled DUP fans actually share (optimality).
 *
 * The engine reduces with labelled interaction-net fans (the Lamping -> Asperti
 * -> HVM optimal-reduction lineage), NOT by eagerly copying work.  This test
 * measures it on two families as the duplication factor K grows:
 *
 *   value(K)  = (\x. x + x + ... [K times]) ((\y. ((((y*2)*2)*2)*2)*2) 1)
 *               the argument reduces to the VALUE 32; the five multiplications
 *               are the expensive part.
 *
 *   lambda(K) = (\g. (g 1) + ... [K applications]) (\x. ((((x*2)*2)*2)*2)*2)
 *               a LAMBDA is duplicated and applied K times; the multiplications
 *               live inside the duplicated body.
 *
 * Result (interactions as a function of K):
 *   - value(K):  the expensive work is done ONCE; each extra use adds only a
 *     fan-copy of the value plus one add -> a small, constant marginal cost.
 *     This is optimal sharing.
 *   - lambda(K): the fan commutes through the lambda and every copy re-reduces
 *     the body -> a large, constant marginal cost (the body is copied K times).
 *     This is sound but NOT work-optimal; closing it needs the bracket/croissant
 *     oracle for full Levy-optimality (the classic hard problem; HVM's boundary
 *     too).
 *
 * The test asserts: (a) all results are correct, (b) both marginal costs are
 * constant, and (c) value-sharing's marginal cost is strictly smaller than the
 * duplicated-lambda's -- i.e. values are shared and lambda bodies are not (yet).
 *
 * Build: cc -std=c89 -O2 -Isrc tests/ic_sharing_test.c src/ic.c -lgmp -o ic_sharing_test
 */
#include "ic.h"
#include <stdio.h>
#include <gmp.h>

static long fails = 0;

static long run(ic_term_t *t, mpz_t out) {
  long it = 0;
  ic_set_reduce_rounds(0); ic_set_reduce_fifo(0);
  ic_normalize_int(t, out, &it);
  ic_term_free(t);
  return it;
}

/* (\x. x + x + ... [k]) ((\y. y*2*2*2*2*2) 1) */
static ic_term_t *value_term(int k) {
  ic_term_t *y = ic_var("y"), *yb = y, *s, *x = ic_var("x");
  int i;
  for (i = 0; i < 5; i++) yb = ic_op(IC_MUL, yb, ic_num_si(2));
  s = x;
  for (i = 1; i < k; i++) s = ic_op(IC_ADD, s, ic_var("x"));
  return ic_app(ic_lam("x", s), ic_app(ic_lam("y", yb), ic_num_si(1)));
}
/* (\g. (g 1) + ... [k]) (\x. x*2*2*2*2*2) */
static ic_term_t *lambda_term(int k) {
  ic_term_t *x = ic_var("x"), *xb = x, *s;
  int i;
  for (i = 0; i < 5; i++) xb = ic_op(IC_MUL, xb, ic_num_si(2));
  s = ic_app(ic_var("g"), ic_num_si(1));
  for (i = 1; i < k; i++) s = ic_op(IC_ADD, s, ic_app(ic_var("g"), ic_num_si(1)));
  return ic_app(ic_lam("g", s), ic_lam("x", xb));
}

/* per-added-use marginal cost between K and 2K (added uses = K) */
static long marg(long it_lo, long it_hi, int k_lo) { return (it_hi - it_lo) / (long)k_lo; }

int main(void) {
  int ks[5] = {1, 2, 4, 8, 16};
  long v[5], l[5];
  long vm[4], lm[4];
  int j, expect;

  printf("== value(K): expensive arg reduces to 32, then is shared ==\n");
  printf("K    result   interactions  expected\n");
  for (j = 0; j < 5; j++) {
    mpz_t o; mpz_init(o); v[j] = run(value_term(ks[j]), o);
    expect = 32 * ks[j];
    if (mpz_cmp_si(o, expect) != 0) { fails++; gmp_printf("  FAIL value K=%d -> %Zd (want %d)\n", ks[j], o, expect); }
    gmp_printf("%-4d %-8Zd %-13ld %d\n", ks[j], o, v[j], expect);
    mpz_clear(o);
  }
  printf("\n== lambda(K): a lambda is duplicated and applied K times ==\n");
  printf("K    result   interactions  expected\n");
  for (j = 0; j < 5; j++) {
    mpz_t o; mpz_init(o); l[j] = run(lambda_term(ks[j]), o);
    expect = 32 * ks[j];
    if (mpz_cmp_si(o, expect) != 0) { fails++; gmp_printf("  FAIL lambda K=%d -> %Zd (want %d)\n", ks[j], o, expect); }
    gmp_printf("%-4d %-8Zd %-13ld %d\n", ks[j], o, l[j], expect);
    mpz_clear(o);
  }

  for (j = 0; j < 4; j++) { vm[j] = marg(v[j], v[j + 1], ks[j]); lm[j] = marg(l[j], l[j + 1], ks[j]); }
  printf("\nmarginal interactions per added use:\n");
  printf("  value :  %ld %ld %ld %ld   (constant & small  -> the value is shared, work done once)\n",
         vm[0], vm[1], vm[2], vm[3]);
  printf("  lambda:  %ld %ld %ld %ld   (constant & large  -> the body is copied per application)\n",
         lm[0], lm[1], lm[2], lm[3]);

  /* (b) both marginals constant */
  if (!(vm[0] == vm[1] && vm[1] == vm[2] && vm[2] == vm[3])) { fails++; printf("  FAIL: value marginal not constant\n"); }
  if (!(lm[0] == lm[1] && lm[1] == lm[2] && lm[2] == lm[3])) { fails++; printf("  FAIL: lambda marginal not constant\n"); }
  /* (c) value sharing strictly cheaper per use than copying a lambda body */
  if (!(vm[0] < lm[0])) { fails++; printf("  FAIL: value marginal not smaller than lambda marginal\n"); }

  printf("\n");
  if (fails == 0)
    printf("OK: values are shared optimally (work done once); duplicated lambda bodies are\n"
           "    copied (sound, not yet work-optimal) -- the bracket/croissant oracle is the gap.\n");
  else
    printf("%ld checks FAILED\n", fails);
  return fails ? 1 : 0;
}
