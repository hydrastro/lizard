/* tests/tt_lattice_set_test.c
 *
 * Tests for Phase L.2: multi-dimensional universe indices.
 *
 * Universes are now indexed by *finite sets of natural numbers*,
 * not single integers. The partial order is subset inclusion;
 * join is set union; meet is set intersection. Crucially this
 * gives genuinely INCOMPARABLE elements: (U-set 0 1) and
 * (U-set 0 2) are both extensions of (U-set 0) but neither
 * is below the other.
 *
 * Coverage:
 *   - Construction (sorting and dedup)
 *   - Empty set as lattice bottom
 *   - Join as set union
 *   - Meet as set intersection
 *   - Subset ordering, including incomparable elements
 *   - Mixed (U n) and (U-set ...) — auto-lifting
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* Construction sorts and dedups */
  r = lizard_test_eval(&e, "(U-set 2 0 1 0 2)");
  TEST_ASSERT_STR(lizard_test_format(r), "(U-set 0 1 2)");

  /* Empty set */
  r = lizard_test_eval(&e, "(U-set)");
  TEST_ASSERT_STR(lizard_test_format(r), "(U-set)");

  /* Join = union */
  r = lizard_test_eval(&e, "(reduce (U-max (U-set 0 1) (U-set 0 2)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U-set 0 1 2)");

  r = lizard_test_eval(&e, "(reduce (U-max (U-set 0 1 2) (U-set)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U-set 0 1 2)");

  /* Meet = intersection */
  r = lizard_test_eval(&e, "(reduce (U-min (U-set 0 1 2) (U-set 0 2 3)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U-set 0 2)");

  /* Meet with disjoint = empty */
  r = lizard_test_eval(&e, "(reduce (U-min (U-set 0 1) (U-set 2 3)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U-set)");

  /* Meet with empty = empty (annihilation) */
  r = lizard_test_eval(&e, "(reduce (U-min (U-set 0 1 2) (U-set)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U-set)");

  /* Subset ordering — equal */
  r = lizard_test_eval(&e, "(universe-leq? (U-set 0 1) (U-set 0 1))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  /* Subset ordering — strict */
  r = lizard_test_eval(&e, "(universe-leq? (U-set 0 1) (U-set 0 1 2))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  /* INCOMPARABLE — the lattice property */
  r = lizard_test_eval(&e, "(universe-leq? (U-set 0 1) (U-set 0 2))");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");

  r = lizard_test_eval(&e, "(universe-leq? (U-set 0 2) (U-set 0 1))");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");

  /* Both ≤ join — diamond shape */
  r = lizard_test_eval(&e, "(universe-leq? (U-set 0 1) (U-set 0 1 2))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e, "(universe-leq? (U-set 0 2) (U-set 0 1 2))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  /* Empty set is bottom — ≤ everything */
  r = lizard_test_eval(&e, "(universe-leq? (U-set) (U-set 0 1 2))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  /* Mixed (U n) and (U-set ...) — auto-lifts */
  r = lizard_test_eval(&e, "(reduce (U-max (U 3) (U-set 0 2)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U-set 0 2 3)");

  r = lizard_test_eval(&e, "(reduce (U-min (U 2) (U-set 0 2 4)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U-set 2)");

  /* (U n) as universe — ≤ comparison with U-set */
  r = lizard_test_eval(&e, "(universe-leq? (U 1) (U-set 0 1 2))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
