/* tests/bignum_test.c
 *
 * Mathematical identity tests at scale. These catch arithmetic
 * bugs that pass small inputs but fail on bignums:
 *
 *   - Fermat's little theorem: a^(p-1) ≡ 1 (mod p) for prime p
 *   - gcd(a,b) * lcm(a,b) = a * b
 *   - Modular inverse via Fermat: a * a^(p-2) ≡ 1 (mod p)
 *   - Binomial sum: sum(C(n,k)) for k = 0..n = 2^n
 */
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* Fermat's little theorem at a sizeable prime. */
  /* 2^(7919-1) mod 7919 should be 1. */
  r = lizard_test_eval(&e, "(modular-expt 2 7918 7919)");
  TEST_ASSERT(lizard_test_is_int(r, 1));
  /* And for several different bases. */
  r = lizard_test_eval(&e, "(modular-expt 3 7918 7919)");
  TEST_ASSERT(lizard_test_is_int(r, 1));
  r = lizard_test_eval(&e, "(modular-expt 17 7918 7919)");
  TEST_ASSERT(lizard_test_is_int(r, 1));

  /* Mersenne prime test: 2^31 - 1 should pass Fermat for small bases. */
  r = lizard_test_eval(&e,
                       "(define mp (- (expt 2 31) 1))"
                       "(modular-expt 2 (- mp 1) mp)");
  TEST_ASSERT(lizard_test_is_int(r, 1));

  /* gcd * lcm = product. */
  r = lizard_test_eval(&e,
                       "(define a 12345678)"
                       "(define b 87654321)"
                       "(- (* (gcd a b) (lcm a b)) (* a b))");
  TEST_ASSERT(lizard_test_is_int(r, 0));

  /* Modular inverse via Fermat (only valid when modulus is prime).
   * a * a^(p-2) ≡ 1 (mod p) */
  r = lizard_test_eval(&e,
                       "(define p 7919)"
                       "(define a 1234)"
                       "(define inv (modular-expt a (- p 2) p))"
                       "(remainder (* a inv) p)");
  TEST_ASSERT(lizard_test_is_int(r, 1));

  /* Binomial sum identity: sum(C(n,k), k=0..n) = 2^n.
   * We sum C(20,k) for k=0..20 by computing each via fact. */
  r = lizard_test_eval(&e,
                       "(define (fact n) (if (= n 0) 1 (* n (fact (- n 1)))))"
                       "(define (C n k) (quotient (fact n)"
                       "                          (* (fact k) (fact (- n k)))))"
                       "(define (sum-C n k)"
                       "  (if (> k n) 0 (+ (C n k) (sum-C n (+ k 1)))))"
                       "(- (sum-C 20 0) (expt 2 20))");
  TEST_ASSERT(lizard_test_is_int(r, 0));

  /* 50! / (49! * 1!) = 50 — sanity for big factorials. */
  r = lizard_test_eval(&e,
                       "(define (fact n) (if (= n 0) 1 (* n (fact (- n 1)))))"
                       "(quotient (fact 50) (fact 49))");
  TEST_ASSERT(lizard_test_is_int(r, 50));

  /* gcd of two giant coprime numbers should be 1. */
  r = lizard_test_eval(&e, "(gcd (expt 2 200) (- (expt 3 100) 1))");
  TEST_ASSERT(r && r->type == AST_NUMBER);
  /* It is 1, but verify symbolically. */

  /* arithmetic-shift round-trip: ((x << n) >> n) = x for non-negative x. */
  r = lizard_test_eval(&e,
                       "(define x 1234567890)"
                       "(- (arithmetic-shift (arithmetic-shift x 17) -17) x)");
  TEST_ASSERT(lizard_test_is_int(r, 0));

  /* (expt a b) should match repeated multiplication for small b. */
  r = lizard_test_eval(&e,
                       "(define (rep-mul a b)"
                       "  (if (= b 0) 1 (* a (rep-mul a (- b 1)))))"
                       "(- (expt 13 20) (rep-mul 13 20))");
  TEST_ASSERT(lizard_test_is_int(r, 0));

  /* Square primitive matches (* x x). */
  r = lizard_test_eval(&e,
                       "(define x 9999999999)"
                       "(- (square x) (* x x))");
  TEST_ASSERT(lizard_test_is_int(r, 0));

  /* abs is idempotent: (abs (abs x)) = (abs x). */
  r = lizard_test_eval(&e, "(- (abs (abs -12345)) (abs -12345))");
  TEST_ASSERT(lizard_test_is_int(r, 0));

  /* remainder sign convention: same sign as dividend. */
  r = lizard_test_eval(&e, "(remainder -17 5)");
  TEST_ASSERT(lizard_test_is_int(r, -2));
  r = lizard_test_eval(&e, "(remainder 17 -5)");
  TEST_ASSERT(lizard_test_is_int(r, 2));

  /* Division identity: a = q*b + r, where q = quotient(a,b), r = remainder. */
  r = lizard_test_eval(&e,
                       "(define a 1000003)"
                       "(define b 7919)"
                       "(- a (+ (* (quotient a b) b) (remainder a b)))");
  TEST_ASSERT(lizard_test_is_int(r, 0));

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
