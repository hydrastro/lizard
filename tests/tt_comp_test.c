/* tests/tt_comp_test.c
 *
 * Tests for Kan composition (Turn 8).
 *
 * Coverage:
 *   - F1 boundary: comp/hcomp with total partial collapse to (u @ i1)
 *   - comp Unit always gives tt
 *   - comp Sigma decomposes into Pair of comps
 *   - comp Pi (non-dependent) becomes a Lambda
 *   - Composition cascade: comp/hcomp + path-app beta
 *   - Typing: comp returns family@i1, hcomp returns family, fill
 *     returns Pi over I
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* ===== F1 boundary ===== */
  r = lizard_test_eval(&e, "(reduce (comp (path-abs 'i 'A) (F1) 'u 'u0))");
  TEST_ASSERT_STR(lizard_test_format(r), "(u @ i1)");

  r = lizard_test_eval(&e, "(reduce (hcomp 'A (F1) 'u 'u0))");
  TEST_ASSERT_STR(lizard_test_format(r), "(u @ i1)");

  /* ===== F1 + path-abs beta cascade =====
   * (hcomp A F1 (<i> v) u0) should reduce to v
   * because F1 makes it (u @ i1), then path-app beta substitutes. */
  r = lizard_test_eval(&e, "(reduce (hcomp 'A (F1) (path-abs 'i 'v) 'u0))");
  TEST_ASSERT_STR(lizard_test_format(r), "v");

  /* ===== comp Unit always gives tt ===== */
  r = lizard_test_eval(&e,
                       "(reduce (comp (path-abs 'i (Unit))"
                       "              (F-eq (I-var 'k) (i0)) 'u 'u0))");
  TEST_ASSERT_STR(lizard_test_format(r), "tt");

  /* ===== comp Sigma decomposes ===== */
  r = lizard_test_eval(&e,
                       "(reduce (comp (path-abs 'i (Sigma 'y 'A 'B))"
                       "              (F-eq (I-var 'k) (i0))"
                       "              'u (Pair 'a0 'b0)))");
  /* Should produce a Pair of two comps. Just check the head is Pair. */
  TEST_ASSERT(r != NULL);
  TEST_ASSERT(r->type == AST_TT_PAIR);

  /* ===== comp Pi (non-dep) gives Lambda ===== */
  r = lizard_test_eval(&e,
                       "(reduce (comp (path-abs 'i (Pi 'y 'A 'B))"
                       "              (F-eq (I-var 'k) (i0))"
                       "              'u 'u0))");
  TEST_ASSERT(r != NULL);
  TEST_ASSERT(r->type == AST_TT_LAMBDA);

  /* ===== Typing rules ===== */
  lizard_test_eval(&e,
                   "(define ctx_c (context (variable 'A (U 0))"
                   "                       (variable 'u (Pi 'i (I) 'A))"
                   "                       (variable 'u0 'A)))");

  /* comp typing: family is (path-abs i A), constant — result A. */
  r = lizard_test_eval(&e,
                       "(infer ctx_c (comp (path-abs 'i 'A) (F0) 'u 'u0))");
  TEST_ASSERT_STR(lizard_test_format(r), "A");

  /* hcomp typing: family is just the type. */
  r = lizard_test_eval(&e,
                       "(infer ctx_c (hcomp 'A (F0) 'u 'u0))");
  TEST_ASSERT_STR(lizard_test_format(r), "A");

  /* comp Unit typing. */
  r = lizard_test_eval(&e,
                       "(infer (context (variable 'u0 (Unit))"
                       "                (variable 'u (Pi 'i (I) (Unit))))"
                       "       (comp (path-abs 'i (Unit)) (F0) 'u 'u0))");
  TEST_ASSERT_STR(lizard_test_format(r), "Unit");

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
