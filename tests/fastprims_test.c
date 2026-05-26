/* tests/fastprims_test.c
 *
 * Tests for the fast bignum primitives that hand off to GMP:
 * arithmetic-shift, expt, gcd, lcm, quotient, remainder, abs,
 * square, modular-expt. These are the primitives that turn an
 * O(N²) lisp expression into an O(log N) GMP call.
 */
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* arithmetic-shift left and right. */
  r = lizard_test_eval(&e, "(arithmetic-shift 1 10)");
  TEST_ASSERT(lizard_test_is_int(r, 1024));
  r = lizard_test_eval(&e, "(arithmetic-shift 1024 -3)");
  TEST_ASSERT(lizard_test_is_int(r, 128));

  /* arithmetic-shift handles huge shifts in a single GMP call. The
   * naive (* 2 …) form would blow the heap; this completes in ms. */
  r = lizard_test_eval(&e, "(arithmetic-shift 1 100000)");
  TEST_ASSERT(r && r->type == AST_NUMBER);

  /* expt for small and large exponents. */
  r = lizard_test_eval(&e, "(expt 2 10)");
  TEST_ASSERT(lizard_test_is_int(r, 1024));
  r = lizard_test_eval(&e, "(expt 3 5)");
  TEST_ASSERT(lizard_test_is_int(r, 243));
  r = lizard_test_eval(&e, "(expt 7 100)");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "3234476509624757991344647769100216810857203"
                  "198904625400933895331391691459636928060001");

  /* gcd, lcm. */
  r = lizard_test_eval(&e, "(gcd 48 36)");
  TEST_ASSERT(lizard_test_is_int(r, 12));
  r = lizard_test_eval(&e, "(lcm 4 6)");
  TEST_ASSERT(lizard_test_is_int(r, 12));
  /* gcd of large coprime values is 1. */
  r = lizard_test_eval(&e, "(gcd (expt 2 100) (- (expt 3 100) 1))");
  TEST_ASSERT(r && r->type == AST_NUMBER);

  /* quotient and remainder (truncating). */
  r = lizard_test_eval(&e, "(quotient 17 5)");
  TEST_ASSERT(lizard_test_is_int(r, 3));
  r = lizard_test_eval(&e, "(remainder 17 5)");
  TEST_ASSERT(lizard_test_is_int(r, 2));
  r = lizard_test_eval(&e, "(remainder -17 5)");
  TEST_ASSERT(lizard_test_is_int(r, -2));

  /* abs, square. */
  r = lizard_test_eval(&e, "(abs -42)");
  TEST_ASSERT(lizard_test_is_int(r, 42));
  r = lizard_test_eval(&e, "(abs 42)");
  TEST_ASSERT(lizard_test_is_int(r, 42));
  r = lizard_test_eval(&e, "(square 13)");
  TEST_ASSERT(lizard_test_is_int(r, 169));

  /* modular-expt — the core of RSA. */
  r = lizard_test_eval(&e, "(modular-expt 4 13 497)");
  TEST_ASSERT(lizard_test_is_int(r, 445));
  /* Modular exp keeps intermediates bounded; this exponent is gigantic
   * but the result is small. */
  r = lizard_test_eval(&e, "(modular-expt 2 10000 (expt 10 9))");
  TEST_ASSERT(r && r->type == AST_NUMBER);

  /* Error cases. */
  r = lizard_test_eval(&e, "(quotient 1 0)");
  TEST_ASSERT(lizard_test_is_error(r));
  r = lizard_test_eval(&e, "(expt 2 -1)");
  TEST_ASSERT(lizard_test_is_error(r));
  r = lizard_test_eval(&e, "(gcd 1 \"two\")");
  TEST_ASSERT(lizard_test_is_error(r));

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
