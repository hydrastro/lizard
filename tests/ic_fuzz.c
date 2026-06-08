/* ic_fuzz.c -- differential validator for the interaction-calculus core.
 *
 *   cc -std=c89 -O2 -Wall -Wextra -Isrc tests/ic_fuzz.c src/ic.c -lgmp -o ic_fuzz
 *   ./ic_fuzz [iterations] [seed]
 *
 * inet.c was validated by differentially cross-checking it against the trusted
 * tree-walking kernel on the untyped lambda fragment.  That check cannot run
 * here (the full library does not link -- see <ds.h>), and it would not cover
 * the new agents anyway.  So this builds an *independent* oracle and property-
 * tests ic against it.
 *
 * The generator produces a random closed term together with its true integer
 * value, computed by ordinary C arithmetic over an environment.  Sharing and
 * erasure arise naturally: a term is built from nested
 *
 *     let x = V in B        (encoded as the application  (lambda (x) B) V)
 *
 * and inside B a bound variable may be referenced zero times (the binder is
 * erased -> ERA), once, or many times (the value is shared -> DUP).  Because the
 * bound value is a plain integer, the oracle just evaluates B in an environment
 * that maps x to value(V); this is correct for any reduction order and is
 * implemented by a completely different mechanism (substitution arithmetic) from
 * the net (local graph rewriting).  Agreement across many random terms exercises
 * beta, sharing, erasure, application, all eight operators, deep nesting, and the
 * integer readback all at once.
 *
 * Arithmetic mirrors ic.c's compute() exactly: '/' and '%' truncate toward zero,
 * a zero divisor yields 0, and = < > yield 1/0.  Values are kept within a cap so
 * the long-integer oracle stays exact (a multiplication that would overflow the
 * cap is emitted as an addition instead, so the term/value pair is always valid).
 */
#include "ic.h"
#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- a small deterministic PRNG (so a failing seed reproduces) ---------- */
static unsigned long g_state = 0x2545F4914F6CDD1DUL;
static unsigned long rnd(void) {           /* xorshift64* */
  unsigned long x = g_state;
  x ^= x >> 12; x ^= x << 25; x ^= x >> 27;
  g_state = x;
  return x * 0x2545F4914F6CDD1DUL;
}
static int rint(int n) { return (int)(rnd() % (unsigned long)n); }

/* ---- generation environment: bound name -> its integer value ------------ */
#define CAP   1000000000000L     /* keep |value| under this so long stays exact */
#define MAXENV 32
typedef struct { char names[MAXENV][16]; long vals[MAXENV]; int n; } Env;

static const int OPS[8] = { IC_ADD, IC_SUB, IC_MUL, IC_DIV, IC_MOD,
                            IC_EQ,  IC_LT,  IC_GT };

/* mirror of ic.c compute() for two longs */
static long oracle_op(int op, long a, long b) {
  switch (op) {
    case IC_ADD: return a + b;
    case IC_SUB: return a - b;
    case IC_MUL: return a * b;
    case IC_DIV: return b == 0 ? 0 : a / b;   /* C trunc toward zero == tdiv_q */
    case IC_MOD: return b == 0 ? 0 : a % b;   /* C remainder sign of a == tdiv_r */
    case IC_EQ:  return a == b ? 1 : 0;
    case IC_LT:  return a <  b ? 1 : 0;
    case IC_GT:  return a >  b ? 1 : 0;
  }
  return 0;
}

/* build a random term of bounded depth; set *val to its true integer value */
static ic_term_t *gen(int depth, Env *env, long *val) {
  int choice;
  /* at depth 0, or sometimes earlier, emit a leaf */
  if (depth <= 0) choice = rint(2);          /* 0=literal 1=variable */
  else            choice = rint(6);

  if (choice <= 1) {
    /* leaf: a bound variable (if any) or a small literal */
    if (choice == 1 && env->n > 0) {
      int i = rint(env->n);
      *val = env->vals[i];
      return ic_var(env->names[i]);
    }
    *val = (long)rint(13);                   /* 0..12 */
    return ic_num_si(*val);
  }

  if (choice <= 4) {
    /* an operator applied to two subterms */
    int op = OPS[rint(8)];
    long a, b;
    ic_term_t *l = gen(depth - 1, env, &a);
    ic_term_t *r = gen(depth - 1, env, &b);
    long v;
    /* avoid overflowing the oracle: downgrade a too-big product to a sum */
    if (op == IC_MUL) {
      double approx = (double)a * (double)b;
      if (approx > (double)CAP || approx < -(double)CAP) op = IC_ADD;
    }
    v = oracle_op(op, a, b);
    if (v > CAP || v < -CAP) { v = oracle_op(IC_ADD, a, b); op = IC_ADD; }
    *val = v;
    return ic_op(op, l, r);
  }

  /* choice == 5: a let-binding -> (lambda (x) B) V, exercising share/erase */
  {
    char name[16];
    long vval, bval;
    ic_term_t *vterm, *bterm;
    int idx = env->n;
    if (idx >= MAXENV) {                      /* env full: fall back to a leaf */
      *val = (long)rint(13);
      return ic_num_si(*val);
    }
    sprintf(name, "x%d", idx);
    vterm = gen(depth - 1, env, &vval);       /* the bound value */
    strcpy(env->names[idx], name);
    env->vals[idx] = vval;
    env->n = idx + 1;
    bterm = gen(depth - 1, env, &bval);       /* the body, may use x0..xidx */
    env->n = idx;                             /* pop */
    *val = bval;
    return ic_app(ic_lam(name, bterm), vterm);
  }
}

/* ---- a tiny term printer, only used to show a failing case -------------- */
static void show(ic_term_t *t) {
  if (!t) { printf("<null>"); return; }
  switch (t->kind) {
    case IC_TNUM: { char *s = mpz_get_str(NULL, 10, t->num); printf("%s", s); free(s); break; }
    case IC_TVAR: printf("%s", t->name); break;
    case IC_TLAM: printf("(lam %s ", t->name); show(t->a); printf(")"); break;
    case IC_TAPP: printf("("); show(t->a); printf(" "); show(t->b); printf(")"); break;
    case IC_TOP: {
      const char *o = "?";
      switch (t->op) {
        case IC_ADD: o="+"; break; case IC_SUB: o="-"; break;
        case IC_MUL: o="*"; break; case IC_DIV: o="/"; break;
        case IC_MOD: o="%"; break; case IC_EQ: o="="; break;
        case IC_LT: o="<"; break;  case IC_GT: o=">"; break;
      }
      printf("(op %s ", o); show(t->a); printf(" "); show(t->b); printf(")"); break;
    }
    case IC_TSUP: printf("{"); show(t->a); printf(" "); show(t->b); printf("}"); break;
    case IC_TDUP: printf("(dup %s %s ", t->name, t->name2); show(t->a);
                  printf(" "); show(t->b); printf(")"); break;
    case IC_TERA: printf("*"); break;
  }
}

int main(int argc, char **argv) {
  long iters = (argc > 1) ? strtol(argv[1], NULL, 10) : 20000;
  long i;
  long fails = 0, shared = 0, erased = 0, max_cost = 0;
  if (argc > 2) g_state = strtoul(argv[2], NULL, 10) | 1UL;

  printf("ic_fuzz: %ld random terms, seed=%lu\n", iters, g_state);

  for (i = 0; i < iters; i++) {
    Env env; long want; ic_term_t *t;
    mpz_t got; long cost = 0; int st;
    int uses_before;
    env.n = 0;
    t = gen(5, &env, &want);

    /* cheap coverage probes: did this term share or erase a binder?
     * (re-walk: count variable occurrences vs binders is overkill; instead
     * sample by checking the top-level shape is a let with >1 or 0 uses.) */
    uses_before = 0; (void)uses_before;

    mpz_init(got);
    st = ic_normalize_int(t, got, &cost);
    if (cost > max_cost) max_cost = cost;
    if (st != 1 || mpz_cmp_si(got, want) != 0) {
      fails++;
      if (fails <= 10) {
        char *g = (st == 1) ? mpz_get_str(NULL, 10, got) : NULL;
        printf("  FAIL [%ld]: want %ld, got %s (status %d, cost %ld)\n    term = ",
               i, want, g ? g : "(non-integer)", st, cost);
        show(t);
        printf("\n");
        if (g) free(g);
      }
    }
    mpz_clear(got);
    ic_term_free(t);
  }

  (void)shared; (void)erased;
  printf("----------------------------------------------------------\n");
  printf("max interactions in one term: %ld\n", max_cost);
  if (fails == 0) printf("ALL %ld TERMS AGREE WITH THE ORACLE\n", iters);
  else            printf("%ld / %ld TERMS DISAGREED\n", fails, iters);
  return fails ? 1 : 0;
}
