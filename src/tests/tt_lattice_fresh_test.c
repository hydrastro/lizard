/* tests/tt_lattice_fresh_test.c
 *
 * Tests for Phase L.3: dimension-creating quantification.
 *
 * The thesis claim "quantifying on a previous universe generates a
 * new universe" is realized as opt-in constructors `pi-fresh` and
 * `sigma-fresh` whose typing rule injects a fresh dimension into
 * the result universe.
 *
 * Coverage:
 *   - pi-fresh introduces a fresh dimension (distinct from ordinary Pi)
 *   - sigma-fresh same
 *   - Two pi-fresh in sequence get distinct dimensions
 *   - Nested pi-fresh accumulates dimensions
 *   - Cumulativity through fresh dimensions follows subset inclusion
 *
 * The fresh dimensions start at 1000 (counter base) and we test
 * only their distinctness and accumulation, not their exact values
 * (which depend on the per-session counter state — though within a
 * single test the counter resets to 1000 at process startup).
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* Old Pi: single level, no new dimension. */
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 0) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* pi-fresh: result is a U-set containing the original max plus
   * a fresh dimension. The fresh counter starts at 1000 per process. */
  r = lizard_test_eval(&e, "(infer (context) (pi-fresh 'x (U 0) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U-set 1 1000)");

  /* Next pi-fresh gets the next fresh dimension. */
  r = lizard_test_eval(&e, "(infer (context) (pi-fresh 'y (U 0) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U-set 1 1001)");

  /* sigma-fresh dimensions also come from the same counter. */
  r = lizard_test_eval(&e, "(infer (context) (sigma-fresh 'z (U 0) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U-set 1 1002)");

  /* Nested pi-fresh accumulates fresh dimensions. */
  r = lizard_test_eval(&e,
                       "(infer (context) "
                       "  (pi-fresh 'x (U 0) (pi-fresh 'y (U 0) (U 0))))");
  /* Inner pi-fresh produces dim 1003; outer produces dim 1004.
   * Both flow up through the U-max joins to a (U-set 1 1003 1004). */
  TEST_ASSERT_STR(lizard_test_format(r), "(U-set 1 1003 1004)");

  /* Cumulativity: (U 1) ≤ (U-set 1 1000) because {1} ⊆ {1, 1000} */
  r = lizard_test_eval(&e, "(universe-leq? (U 1) (U-set 1 1000))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  /* But (U-set 1 1000) ≰ (U 1) — fresh dim isn't in the singleton. */
  r = lizard_test_eval(&e, "(universe-leq? (U-set 1 1000) (U 1))");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");

  /* Two pi-fresh outputs are incomparable: each carries its own dim. */
  r = lizard_test_eval(&e,
                       "(universe-leq? (U-set 1 1000) (U-set 1 1001))");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");

  /* The non-fresh Pi is *not* affected — existing examples keep working.
   * Lizard's Pi rule is successor-of-max: Pi (U 2) (U 5) lives at
   * max(univ(U 2), univ(U 5)) = max(U 3, U 6) = U 6. */
  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 2) (U 5)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 6)");

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
