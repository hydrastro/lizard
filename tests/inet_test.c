/* inet_test.c — interaction-combinator runtime.
 *
 * Builds lambda terms directly through the C API, reduces them on the net,
 * and checks the integer read back from normal form.  Because the net reduces
 * by purely local graph rewriting, getting these right exercises the whole
 * machine: beta (CON~CON annihilation), sharing (DUP copying a function via
 * commutation), erasure of unused binders (ERA), and exact arithmetic agents.
 *
 * Church numeral N applied to succ = (lambda (n) (+ n 1)) and base 0 must
 * reduce to N — which forces the runtime to duplicate `succ` N times through
 * DUP/commute and run each copy.  That is the optimal-reduction story in
 * miniature, and the integer result is a hard correctness oracle.
 */
#include "test_harness.h"
#include "inet.h"
#include <gmp.h>
#include <string.h>

/* succ = (lambda (n) (+ n 1)); fresh tree each call (terms are owned). */
static inet_term_t *succ_fn(void) {
  return inet_lam("n", inet_op(INET_ADD, inet_var("n"), inet_num_si(1)));
}

/* Church numeral k = (lambda (f) (lambda (x) (f (f ... (f x))))). */
static inet_term_t *church(int k) {
  inet_term_t *body = inet_var("x");
  int i;
  for (i = 0; i < k; i++) body = inet_app(inet_var("f"), body);
  return inet_lam("f", inet_lam("x", body));
}

/* combinators (fresh tree each call) */
static inet_term_t *k_comb(void) {
  return inet_lam("a", inet_lam("b", inet_var("a")));
}
static inet_term_t *s_comb(void) {
  return inet_lam("f", inet_lam("g", inet_lam("x",
    inet_app(inet_app(inet_var("f"), inet_var("x")),
             inet_app(inet_var("g"), inet_var("x"))))));
}
static inet_term_t *plus_comb(void) {
  return inet_lam("m", inet_lam("n", inet_lam("f", inet_lam("x",
    inet_app(inet_app(inet_var("m"), inet_var("f")),
             inet_app(inet_app(inet_var("n"), inet_var("f")), inet_var("x")))))));
}
static inet_term_t *mul_comb(void) {
  return inet_lam("m", inet_lam("n", inet_lam("f",
    inet_app(inet_var("m"), inet_app(inet_var("n"), inet_var("f"))))));
}

/* readback: assert a term's normal form prints to an exact de Bruijn string. */
static int rb_eq(inet_term_t *t, const char *want) {
  char buf[4096];
  long inter;
  int st, ok;
  st = inet_readback(t, buf, sizeof(buf), &inter);
  inet_term_free(t);
  ok = (st == 1) && (strcmp(buf, want) == 0);
  if (!ok) {
    fprintf(stderr, "  readback: want \"%s\", status %d, got \"%s\"\n",
            want, st, (st == 1) ? buf : "<none>");
  }
  return ok;
}

/* readback boundary: a term whose bare normal form keeps residual compound
 * sharing must REFUSE (status != 1), never emit a wrong term. */
static int rb_refuses(inet_term_t *t) {
  char buf[4096];
  long inter;
  int st;
  st = inet_readback(t, buf, sizeof(buf), &inter);
  inet_term_free(t);
  if (st == 1) fprintf(stderr, "  readback: expected refusal, got \"%s\"\n", buf);
  return st != 1;
}

/* reduce a term to an integer; returns the status, fills out + interactions. */
static int run(inet_term_t *t, mpz_t out, long *inter) {
  int st = inet_normalize_int(t, out, inter);
  inet_term_free(t);
  return st;
}

/* convenience: assert a term reduces to a given small integer. */
static int eq_si(inet_term_t *t, long want, long *inter) {
  mpz_t out;
  int st, ok;
  mpz_init(out);
  st = run(t, out, inter);
  ok = (st == 1) && (mpz_cmp_si(out, want) == 0);
  if (!ok) {
    gmp_fprintf(stderr, "  expected %ld, status %d, got %Zd\n", want, st, out);
  }
  mpz_clear(out);
  return ok;
}

int main(void) {
  long inter;
  mpz_t out, expect;
  int k;

  /* ---- exact arithmetic in the net ---- */
  TEST_ASSERT(eq_si(inet_op(INET_ADD, inet_num_si(3), inet_num_si(4)), 7, &inter));
  TEST_ASSERT(eq_si(inet_op(INET_SUB, inet_num_si(10), inet_num_si(1)), 9, &inter));
  TEST_ASSERT(eq_si(inet_op(INET_MUL, inet_num_si(6), inet_num_si(7)), 42, &inter));
  /* nested: (6*3) + (10-1) = 27 */
  TEST_ASSERT(eq_si(
      inet_op(INET_ADD,
              inet_op(INET_MUL, inet_num_si(6), inet_num_si(3)),
              inet_op(INET_SUB, inet_num_si(10), inet_num_si(1))),
      27, &inter));

  /* ---- EXACT BIGNUM in the net (innovation over HVM2's 24-bit) ---- */
  mpz_init(out);
  mpz_init_set_str(expect, "1000000000000000000000000", 10); /* 10^24 */
  TEST_ASSERT(run(inet_op(INET_MUL, inet_num_str("1000000000000"),
                          inet_num_str("1000000000000")), out, &inter) == 1);
  TEST_ASSERT(mpz_cmp(out, expect) == 0);
  mpz_clear(expect);

  /* ---- combinators applied to numbers ---- */
  /* I = (lambda (x) x);  (I 42) = 42 */
  TEST_ASSERT(eq_si(inet_app(inet_lam("x", inet_var("x")), inet_num_si(42)),
                    42, &inter));
  /* K = (lambda (a) (lambda (b) a));  ((K 7) 99) = 7  (b unused -> ERA) */
  TEST_ASSERT(eq_si(
      inet_app(inet_app(inet_lam("a", inet_lam("b", inet_var("a"))),
                        inet_num_si(7)),
               inet_num_si(99)),
      7, &inter));
  /* a variable used twice forces a DUP: (lambda (x) (* x x)) 8 = 64 */
  TEST_ASSERT(eq_si(
      inet_app(inet_lam("x", inet_op(INET_MUL, inet_var("x"), inet_var("x"))),
               inet_num_si(8)),
      64, &inter));

  /* ---- Church numerals applied to succ and 0 (beta + sharing) ---- */
  /* church k (succ) 0 == k ; copies `succ` k times through DUP/commute */
  for (k = 0; k <= 6; k++) {
    inet_term_t *t = inet_app(inet_app(church(k), succ_fn()), inet_num_si(0));
    TEST_ASSERT(eq_si(t, k, &inter));
  }

  /* a slightly larger one to show it keeps working */
  TEST_ASSERT(eq_si(inet_app(inet_app(church(20), succ_fn()), inet_num_si(0)),
                    20, &inter));

  /* church 3 over succ, then double the result by sharing: (lam y (+ y y)) */
  TEST_ASSERT(eq_si(
      inet_app(inet_lam("y", inet_op(INET_ADD, inet_var("y"), inet_var("y"))),
               inet_app(inet_app(church(3), succ_fn()), inet_num_si(0))),
      6, &inter));

  /* ---- the net is consumed: almost no live nodes remain after readback ---- */
  TEST_ASSERT(eq_si(inet_app(inet_app(church(5), succ_fn()), inet_num_si(0)),
                    5, &inter));
  TEST_ASSERT(inet_live_nodes() < 8);

  /* ---- full lambda READBACK: reduce, then read the normal form back as a
   * de Bruijn term (#k = bound var, (f a) = app, (lam b) = abstraction) ---- */
  /* identity reads back to its own normal form */
  TEST_ASSERT(rb_eq(inet_lam("x", inet_var("x")), "(lam #0)"));
  /* K = the outer binder, used once; inner binder erased */
  TEST_ASSERT(rb_eq(k_comb(), "(lam (lam #1))"));
  /* Church numerals as TERMS: the duplicated f reads back to one variable */
  TEST_ASSERT(rb_eq(church(2), "(lam (lam (#1 (#1 #0))))"));
  TEST_ASSERT(rb_eq(church(3), "(lam (lam (#1 (#1 (#1 #0)))))"));
  /* a numeric normal form reads back as a decimal */
  TEST_ASSERT(rb_eq(inet_op(INET_ADD, inet_num_si(3), inet_num_si(4)), "7"));
  /* identity applied to identity reduces on the graph to the identity */
  TEST_ASSERT(rb_eq(inet_app(inet_lam("x", inet_var("x")),
                             inet_lam("y", inet_var("y"))), "(lam #0)"));
  /* S K K reduces (real combinatory reduction) and reads back as identity */
  TEST_ASSERT(rb_eq(inet_app(inet_app(s_comb(), k_comb()), k_comb()),
                    "(lam #0)"));
  /* Church addition: PLUS 2 3 reduces to Church 5 and reads back fully */
  TEST_ASSERT(rb_eq(inet_app(inet_app(plus_comb(), church(2)), church(3)),
                    "(lam (lam (#1 (#1 (#1 (#1 (#1 #0)))))))"));

  /* ---- readback BOUNDARY (honest): a bare normal form that keeps residual
   * sharing of a compound subterm refuses rather than emit a wrong term ---- */
  /* MUL 2 3 (bare) keeps DUP nodes not in active-pair position -> refuse */
  TEST_ASSERT(rb_refuses(inet_app(inet_app(mul_comb(), church(2)), church(3))));
  /* (2 2) self-application has the same shape -> refuse */
  TEST_ASSERT(rb_refuses(inet_app(church(2), church(2))));
  /* ...yet the SAME multiplication reduces to the right integer on demand */
  TEST_ASSERT(eq_si(
      inet_app(inet_app(inet_app(inet_app(mul_comb(), church(2)), church(3)),
                        succ_fn()),
               inet_num_si(0)),
      6, &inter));

  mpz_clear(out);
  TEST_RETURN();
}
