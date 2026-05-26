/* tests/tt_lattice_co_test.c
 *
 * Tests for Phase L.4: the dual (couniverse) lattice.
 *
 * Universes (U-set) and couniverses (Co-set) live in SEPARATE
 * lattices. They share the same dim-space (natural numbers) but
 * the AST kind distinguishes them. Mixing a U-set with a Co-set
 * under Co-max or U-max stays unreduced — it's a structural marker
 * that the two sorts can't be combined.
 *
 * Coverage:
 *   - Construction (sort + dedup, same as U-set)
 *   - Co-max as set union
 *   - Co-min as set intersection
 *   - Lattice axioms: idempotence, absorption
 *   - Couniverse ordering via subset
 *   - Cross-lattice operations stay separate
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* Construction sorts and dedups. */
  r = lizard_test_eval(&e, "(Co-set 2 0 1 0 2)");
  TEST_ASSERT_STR(lizard_test_format(r), "(Co-set 0 1 2)");

  /* Empty Co-set */
  r = lizard_test_eval(&e, "(Co-set)");
  TEST_ASSERT_STR(lizard_test_format(r), "(Co-set)");

  /* Co-max = set union */
  r = lizard_test_eval(&e, "(reduce (Co-max (Co-set 0 1) (Co-set 0 2)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Co-set 0 1 2)");

  /* Co-min = set intersection */
  r = lizard_test_eval(&e, "(reduce (Co-min (Co-set 0 1 2) (Co-set 0 2 3)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Co-set 0 2)");

  /* Intersection with empty */
  r = lizard_test_eval(&e, "(reduce (Co-min (Co-set 0 1 2) (Co-set)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Co-set)");

  /* Idempotence */
  r = lizard_test_eval(&e, "(reduce (Co-max (Co-set 0 1) (Co-set 0 1)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Co-set 0 1)");
  r = lizard_test_eval(&e, "(reduce (Co-min (Co-set 0 1) (Co-set 0 1)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Co-set 0 1)");

  /* Absorption: u ∧ (u ∨ v) = u */
  r = lizard_test_eval(&e,
                       "(reduce (Co-min (Co-set 0) "
                       "                (Co-max (Co-set 0) (Co-set 1))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Co-set 0)");

  /* Absorption: u ∨ (u ∧ v) = u */
  r = lizard_test_eval(&e,
                       "(reduce (Co-max (Co-set 0) "
                       "                (Co-min (Co-set 0) (Co-set 1))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Co-set 0)");

  /* Couniverse ordering */
  r = lizard_test_eval(&e, "(couniverse-leq? (Co-set 0 1) (Co-set 0 1 2))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  /* Incomparable in the lattice */
  r = lizard_test_eval(&e, "(couniverse-leq? (Co-set 0 1) (Co-set 0 2))");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");
  r = lizard_test_eval(&e, "(couniverse-leq? (Co-set 0 2) (Co-set 0 1))");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");

  /* Empty Co-set is the bottom */
  r = lizard_test_eval(&e, "(couniverse-leq? (Co-set) (Co-set 0 1 2))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  /* CROSS-LATTICE: mixing U-set with Co-set under Co-max stays
   * unreduced — the two lattices don't blend. */
  r = lizard_test_eval(&e, "(reduce (Co-max (Co-set 0 1) (U-set 0 1)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(Co-max (Co-set 0 1) (U-set 0 1))");

  /* couniverse-leq? on mixed input returns 'unknown */
  r = lizard_test_eval(&e, "(couniverse-leq? (Co-set 0) (U-set 0))");
  TEST_ASSERT_STR(lizard_test_format(r), "unknown");

  /* And U-max with a Co-set arg also stays unreduced */
  r = lizard_test_eval(&e, "(reduce (U-max (U-set 0 1) (Co-set 0 1)))");
  /* the U-set-aware rule sees the Co-set isn't a U-set, falls through.
   * U-max non-numeric, non-set inputs become a no-op. */
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(U-max (U-set 0 1) (Co-set 0 1))");

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
