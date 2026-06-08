/* ic_test.c — the interaction-calculus core (the four agents).
 *
 * Exercises ic.c the way inet_test.c exercises inet.c, plus the piece inet.c
 * does not have: the SUPERPOSITION agent and the DUP/SUP duality.
 *
 * Two oracles are used.  Integer normal forms are checked against an exact
 * arithmetic answer (a hard oracle — beta, sharing through DUP, erasure, and
 * GMP arithmetic all have to be right to land on it).  Superposition normal
 * forms are checked against their textual readback, which pins down the label
 * semantics: matching dup/sup labels COLLAPSE, equal-label superpositions stay
 * CORRELATED, and distinct-label superpositions form the OUTER PRODUCT.
 */
#include "test_harness.h"
#include "ic.h"
#include <gmp.h>
#include <string.h>

/* reduce a term to an integer normal form and compare. consumes t. */
static long norm_int(ic_term_t *t, int *ok) {
  mpz_t out;
  long inter = 0;
  int st;
  long v = 0;
  mpz_init(out);
  st = ic_normalize_int(t, out, &inter);
  *ok = (st == 1);
  if (st == 1) v = mpz_get_si(out);
  mpz_clear(out);
  ic_term_free(t);
  return v;
}

/* reduce a term and compare its textual readback. consumes t. returns 1 on match. */
static int rb_eq(ic_term_t *t, const char *want) {
  char buf[4096];
  long inter = 0;
  int st, ok;
  st = ic_readback(t, buf, sizeof buf, &inter);
  ok = (st == 1) && (strcmp(buf, want) == 0);
  if (!ok)
    fprintf(stderr, "  readback: want \"%s\", status %d, got \"%s\"\n",
            want, st, (st == 1 ? buf : "(n/a)"));
  ic_term_free(t);
  return ok;
}

/* parse helper: aborts the test loudly if the source does not parse. */
static ic_term_t *P(const char *src) {
  char err[256];
  ic_term_t *t = ic_parse(src, err, sizeof err);
  if (!t) fprintf(stderr, "  PARSE FAIL: %s : %s\n", src, err);
  return t;
}

int main(void) {
  int ok;

  /* ---- exact arithmetic ------------------------------------------------ */
  TEST_ASSERT_EQ(norm_int(P("(op + 3 4)"), &ok), 7);            TEST_ASSERT(ok);
  TEST_ASSERT_EQ(norm_int(P("(op * 12 (op + 3 4))"), &ok), 84); TEST_ASSERT(ok);
  TEST_ASSERT_EQ(norm_int(P("(op - 100 1)"), &ok), 99);         TEST_ASSERT(ok);
  TEST_ASSERT_EQ(norm_int(P("(op * 123 123)"), &ok), 15129);    TEST_ASSERT(ok);

  /* ---- lambda / application: beta, sharing, erasure -------------------- */
  /* identity */
  TEST_ASSERT_EQ(norm_int(P("((lam x x) 99)"), &ok), 99);       TEST_ASSERT(ok);
  /* x used three times -> two binder DUPs, all copies summed */
  TEST_ASSERT_EQ(norm_int(P("((lam x (op + x (op + x x))) 7)"), &ok), 21); TEST_ASSERT(ok);
  /* composition of doubling */
  TEST_ASSERT_EQ(
    norm_int(P("((lam x (op + x x)) ((lam x (op + x x)) 10))"), &ok), 40); TEST_ASSERT(ok);
  /* K erases its second argument */
  TEST_ASSERT_EQ(norm_int(P("(((lam a (lam b a)) 5) 9)"), &ok), 5); TEST_ASSERT(ok);
  /* S K K 5 = 5 : the classic shared-application stress test */
  TEST_ASSERT_EQ(norm_int(P(
    "((((lam f (lam g (lam x ((f x) (g x))))) (lam a (lam b a)))"
    " (lam c (lam d c))) 5)"), &ok), 5); TEST_ASSERT(ok);

  /* ---- the C constructor API (not just the parser) --------------------- */
  {
    /* (lambda (x) (+ x x)) applied to 21 -> 42, built by hand */
    ic_term_t *dbl = ic_lam("x", ic_op(IC_ADD, ic_var("x"), ic_var("x")));
    ic_term_t *e   = ic_app(dbl, ic_num_si(21));
    TEST_ASSERT_EQ(norm_int(e, &ok), 42); TEST_ASSERT(ok);
  }
  {
    /* a superposition built through the API reads back faithfully */
    ic_term_t *s = ic_sup(0, ic_num_si(2), ic_num_si(3));
    TEST_ASSERT(rb_eq(s, "{2 3}"));
  }

  /* ---- superposition: bare pairs read back -----------------------------*/
  TEST_ASSERT(rb_eq(P("{2 3}"), "{2 3}"));
  TEST_ASSERT(rb_eq(P("{10 {20 30}}"), "{10 {20 30}}"));
  TEST_ASSERT(rb_eq(P("{:1 2 3}"), "{2 3}"));    /* label is internal */

  /* ---- a computation distributes over a superposition (search) -------- */
  TEST_ASSERT(rb_eq(P("(op + 1 {2 3})"), "{3 4}"));
  TEST_ASSERT(rb_eq(P("(op * {2 3} 10)"), "{20 30}"));
  /* lambda using its argument twice, fed a superposition: copies keep the
   * argument's label, so they correlate -> {2+2, 3+3} */
  TEST_ASSERT(rb_eq(P("((lam x (op + x x)) {2 3})"), "{4 6}"));

  /* ---- DUP/SUP duality and label semantics ----------------------------*/
  /* matching labels collapse: x<-10, y<-20 -> 30 */
  TEST_ASSERT_EQ(norm_int(P("(dup :7 x y {:7 10 20} (op + x y))"), &ok), 30);
  TEST_ASSERT(ok);
  /* same (default) label = one shared choice, correlated/zip */
  TEST_ASSERT(rb_eq(P("(op + {10 20} {100 200})"), "{110 220}"));
  /* different labels = independent choices, outer product */
  TEST_ASSERT(rb_eq(P("(op + {:1 10 20} {:2 100 200})"),
                    "{{110 210} {120 220}}"));

  /* ---- Sigma: first-class pairs and projections ----------------------- */
  /* projection fires against the constructor */
  TEST_ASSERT(rb_eq(P("(fst (pair 3 4))"), "3"));
  TEST_ASSERT(rb_eq(P("(snd (pair 3 4))"), "4"));
  /* nested pairs */
  TEST_ASSERT(rb_eq(P("(fst (fst (pair (pair 1 2) 3)))"), "1"));
  /* a pair value reads back as (pair a b) */
  TEST_ASSERT(rb_eq(P("(pair (op + 1 2) (op * 5 5))"), "(pair 3 25)"));
  /* a shared pair: bound once, used twice -> a DUP duplicates it, then each
   * copy is projected (PAIR~DUP commute, then FST/SND ~ PAIR) */
  TEST_ASSERT(rb_eq(P("((lam p (op + (fst p) (snd p))) (pair 10 20))"), "30"));
  /* a projection distributes over a superposition of pairs (FST ~ SUP) */
  TEST_ASSERT(rb_eq(P("(fst {(pair 1 2) (pair 3 4)})"), "{1 3}"));

  /* ---- transport (Id-by-observation); reflexive transport is the identity -- */
  TEST_ASSERT(rb_eq(P("(transp 5)"), "5"));                       /* base */
  TEST_ASSERT(rb_eq(P("(transp (pair 3 4))"), "(pair 3 4)"));     /* Σ componentwise */
  TEST_ASSERT(rb_eq(P("(fst (transp (pair 3 4)))"), "3"));
  TEST_ASSERT(rb_eq(P("((transp (lam x (op + x 1))) 5)"), "6"));  /* Π pointwise */
  TEST_ASSERT(rb_eq(P("((transp (lam x (op + x x))) 5)"), "10")); /* Π + shared binder */
  TEST_ASSERT(rb_eq(P("(transp {1 2})"), "{1 2}"));               /* over a superposition */
  TEST_ASSERT(rb_eq(P("(transp (transp (pair 7 (op * 2 4))))"), "(pair 7 8)"));

  TEST_RETURN();
}
