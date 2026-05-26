/* tests/tt_lattice_test.c
 *
 * Tests for the universe lattice (Phase L.1): the meet operation
 * U-min and its lattice interactions with the existing U-max.
 *
 * Coverage:
 *   - Concrete meet: (U-min (U n) (U m)) -> (U min(n,m))
 *   - Idempotence: (U-min u u) -> u
 *   - Bottom annihilation: (U-min (U 0) u) -> (U 0)
 *     (this is the DUAL of U-max where 0 is identity)
 *   - Absorption laws (defining property of a lattice):
 *     - (U-min u (U-max u v)) -> u
 *     - (U-max u (U-min u v)) -> u
 *   - Lattice-aware universe ordering:
 *     - (U-min u v) <= u, <= v
 *     - w <= (U-min u v) iff w <= u AND w <= v
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* Concrete meet */
  r = lizard_test_eval(&e, "(reduce (U-min (U 2) (U 5)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 2)");

  r = lizard_test_eval(&e, "(reduce (U-min (U 5) (U 2)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 2)");

  /* Idempotence */
  r = lizard_test_eval(&e, "(reduce (U-min (U 3) (U 3)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 3)");

  r = lizard_test_eval(&e, "(reduce (U-min (U-var 'i) (U-var 'i)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U-var i)");

  /* Bottom annihilation: meet with bottom is bottom (DUAL of max) */
  r = lizard_test_eval(&e, "(reduce (U-min (U 0) (U 7)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");

  r = lizard_test_eval(&e, "(reduce (U-min (U 7) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");

  /* Compare: U-max with 0 is the OTHER arg */
  r = lizard_test_eval(&e, "(reduce (U-max (U 0) (U 7)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 7)");

  /* Absorption: u ∧ (u ∨ v) = u */
  r = lizard_test_eval(&e,
                       "(reduce (U-min (U-var 'u) "
                       "        (U-max (U-var 'u) (U-var 'v))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U-var u)");

  /* Absorption: u ∨ (u ∧ v) = u */
  r = lizard_test_eval(&e,
                       "(reduce (U-max (U-var 'u) "
                       "        (U-min (U-var 'u) (U-var 'v))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U-var u)");

  /* Absorption with reversed arg order */
  r = lizard_test_eval(&e,
                       "(reduce (U-min (U-max (U-var 'u) (U-var 'v)) "
                       "        (U-var 'u)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U-var u)");

  /* Lattice ordering: meet is below components */
  r = lizard_test_eval(&e,
                       "(universe-leq? (U-min (U-var 'u) (U-var 'v)) "
                       "               (U-var 'u))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  r = lizard_test_eval(&e,
                       "(universe-leq? (U-min (U-var 'u) (U-var 'v)) "
                       "               (U-var 'v))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  /* w ≤ meet iff w ≤ both */
  r = lizard_test_eval(&e,
                       "(universe-leq? (U 2) (U-min (U 5) (U 3)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  r = lizard_test_eval(&e,
                       "(universe-leq? (U 4) (U-min (U 5) (U 3)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
