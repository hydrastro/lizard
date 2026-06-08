/* ic_lower_test.c — lowering the core IR (kterm computational fragment) to nets.
 *
 *   cc -std=c89 -O2 -Wall -Wextra -Itests -Isrc \
 *      tests/ic_lower_test.c src/ic_lower.c src/ic.c -lgmp -o ic_lower_test
 *
 * Two parts.  First, curated terms built straight through the core constructors
 * (standing in for elaborator output) with known integer results — covering
 * Sigma intro/elim (pair/proj), de Bruijn resolution under nested binders,
 * sharing of a *compound* value (a pair duplicated and projected twice), and
 * erasure.  Second, a differential fuzz: random core terms generated together
 * with their true integer value, lowered, reduced on the net, and checked.
 */
#include "test_harness.h"
#include "ic_lower.h"
#include "ic.h"
#include <gmp.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* lower, reduce, return the integer normal form. consumes the core term. */
static long eval(core_t *t, int *ok) {
  ic_term_t *e = ic_lower(t);
  mpz_t out; long inter = 0; int st; long v = 0;
  core_free(t);
  if (!e) { *ok = 0; return 0; }
  mpz_init(out);
  st = ic_normalize_int(e, out, &inter);
  *ok = (st == 1);
  if (st == 1) v = mpz_get_si(out);
  mpz_clear(out);
  ic_term_free(e);
  return v;
}

/* shorthands */
#define LIT(n)   core_lit_si(n)
#define VAR(i)   core_var(i)
#define ADD(a,b) core_prim(IC_ADD,(a),(b))
#define MUL(a,b) core_prim(IC_MUL,(a),(b))

static void curated(void) {
  int ok;

  /* Sigma elimination */
  TEST_ASSERT_EQ(eval(core_proj1(core_pair(LIT(3), LIT(4))), &ok), 3); TEST_ASSERT(ok);
  TEST_ASSERT_EQ(eval(core_proj2(core_pair(LIT(3), LIT(4))), &ok), 4); TEST_ASSERT(ok);

  /* an operator over two projections of two separate pairs */
  TEST_ASSERT_EQ(
    eval(ADD(core_proj1(core_pair(LIT(10), LIT(20))),
             core_proj2(core_pair(LIT(10), LIT(20)))), &ok), 30); TEST_ASSERT(ok);

  /* a SHARED pair: bind it once, duplicate it, project each copy.
   * let p = (10,20) in (pi1 p) + (pi2 p)  ->  30
   * (the pair value flows through a sharing DUP — the compound-constructor case) */
  TEST_ASSERT_EQ(
    eval(core_let(core_pair(LIT(10), LIT(20)),
                  ADD(core_proj1(VAR(0)), core_proj2(VAR(0)))), &ok), 30); TEST_ASSERT(ok);

  /* nested pairs: pi1 (pi2 (1, (2, 3)))  ->  2 */
  TEST_ASSERT_EQ(
    eval(core_proj1(core_proj2(core_pair(LIT(1), core_pair(LIT(2), LIT(3))))), &ok), 2);
  TEST_ASSERT(ok);

  /* a let-bound projection, then shared: let x = pi1 (7,8) in x*x -> 49 */
  TEST_ASSERT_EQ(
    eval(core_let(core_proj1(core_pair(LIT(7), LIT(8))), MUL(VAR(0), VAR(0))), &ok), 49);
  TEST_ASSERT(ok);

  /* swap-by-projection: pi1 (let p=(1,2) in (pi2 p, pi1 p)) -> 2 */
  TEST_ASSERT_EQ(
    eval(core_proj1(core_let(core_pair(LIT(1), LIT(2)),
                             core_pair(core_proj2(VAR(0)), core_proj1(VAR(0))))), &ok), 2);
  TEST_ASSERT(ok);

  /* plain beta + arithmetic */
  TEST_ASSERT_EQ(eval(core_app(core_lam(VAR(0)), LIT(5)), &ok), 5); TEST_ASSERT(ok);
  TEST_ASSERT_EQ(eval(core_app(core_lam(ADD(VAR(0), VAR(0))), LIT(9)), &ok), 18); TEST_ASSERT(ok);

  /* de Bruijn under two binders: K = \.\. #1 ; (K 1 2) -> 1, ((\.\.#0) 1 2) -> 2 */
  TEST_ASSERT_EQ(
    eval(core_app(core_app(core_lam(core_lam(VAR(1))), LIT(1)), LIT(2)), &ok), 1); TEST_ASSERT(ok);
  TEST_ASSERT_EQ(
    eval(core_app(core_app(core_lam(core_lam(VAR(0))), LIT(1)), LIT(2)), &ok), 2); TEST_ASSERT(ok);
}

/* ---- differential fuzz over the core IR -------------------------------- */
#define CAP 1000000000000L
static unsigned long g_state = 0x9E3779B97F4A7C15UL;
static unsigned long rnd(void) {
  unsigned long x = g_state;
  x ^= x >> 12; x ^= x << 25; x ^= x >> 27; g_state = x;
  return x * 0x2545F4914F6CDD1DUL;
}
static int rint_(int n) { return (int)(rnd() % (unsigned long)n); }

static const int OPS[8] = { IC_ADD, IC_SUB, IC_MUL, IC_DIV, IC_MOD, IC_EQ, IC_LT, IC_GT };
static long oop(int op, long a, long b) {
  switch (op) {
    case IC_ADD: return a + b;  case IC_SUB: return a - b;  case IC_MUL: return a * b;
    case IC_DIV: return b ? a / b : 0; case IC_MOD: return b ? a % b : 0;
    case IC_EQ: return a == b; case IC_LT: return a < b; case IC_GT: return a > b;
  }
  return 0;
}

/* env: integer-valued de Bruijn binders only (every gen binder is integer). */
typedef struct { long v[64]; int n; } IEnv;

static core_t *genI(int depth, IEnv *e, long *val) {
  int ch = (depth <= 0) ? rint_(e->n > 0 ? 2 : 1) : rint_(6);
  switch (ch) {
    case 0: { long n = (long)rint_(13); *val = n; return LIT(n); }
    case 1: { int j; if (e->n == 0) { long n = (long)rint_(13); *val = n; return LIT(n); }
              j = rint_(e->n); *val = e->v[j]; return VAR(e->n - 1 - j); }
    case 2: case 3: {  /* prim */
      long a, b, v; int op = OPS[rint_(8)];
      core_t *l = genI(depth - 1, e, &a);
      core_t *r = genI(depth - 1, e, &b);
      if (op == IC_MUL) { double ap = (double)a * (double)b;
        if (ap > (double)CAP || ap < -(double)CAP) op = IC_ADD; }
      v = oop(op, a, b);
      if (v > CAP || v < -CAP) { op = IC_ADD; v = a + b; }
      *val = v; return core_prim(op, l, r);
    }
    case 4: {  /* projection of an immediate pair (the non-chosen side is erased) */
      long a, b; int which = rint_(2);
      core_t *l = genI(depth - 1, e, &a);
      core_t *r = genI(depth - 1, e, &b);
      core_t *p = core_pair(l, r);
      *val = which ? a : b;
      return which ? core_proj1(p) : core_proj2(p);
    }
    default: {  /* let _ = V in B   (B may use the new integer binder) */
      long vv, bv;
      core_t *V, *B;
      if (e->n >= 64) { long n = (long)rint_(13); *val = n; return LIT(n); }
      V = genI(depth - 1, e, &vv);
      e->v[e->n++] = vv;            /* push */
      B = genI(depth - 1, e, &bv);
      e->n--;                       /* pop */
      *val = bv;
      return core_let(V, B);
    }
  }
}

static int fuzz(long iters) {
  long i, fails = 0;
  for (i = 0; i < iters; i++) {
    IEnv e; long want; int ok; long got;
    e.n = 0;
    got = eval(genI(5, &e, &want), &ok);   /* eval consumes the term */
    if (!ok || got != want) {
      if (++fails <= 8)
        printf("  fuzz FAIL [%ld]: want %ld got %ld (ok=%d)\n", i, want, got, ok);
    }
  }
  if (fails) printf("  lowering fuzz: %ld/%ld disagreed\n", fails, iters);
  else       printf("  lowering fuzz: all %ld random core terms agree\n", iters);
  return fails == 0;
}

int main(int argc, char **argv) {
  long iters = (argc > 1) ? strtol(argv[1], NULL, 10) : 20000;
  if (argc > 2) g_state = strtoul(argv[2], NULL, 10) | 1UL;
  curated();
  TEST_ASSERT(fuzz(iters));
  TEST_RETURN();
}
