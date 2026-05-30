/* tests/tt_lattice_co_fresh_test.c
 *
 * Tests for Phase L.5: dimension-creating quantification in the
 * couniverse lattice.
 *
 * L.5 mirrors L.3 (pi-fresh / sigma-fresh) but the dimensions live
 * in the COUNIVERSE lattice. The fresh-dim counter is SHARED between
 * the two — every dimension-creating binding gets a globally unique
 * nat, and the result's AST kind (U-set vs Co-set) tells you which
 * lattice the dim belongs to.
 *
 * Coverage:
 *   - (Uco n) types successively as (Uco (n+1))
 *   - (Co-set ...) types as a couniverse element
 *   - co-pi-fresh produces a Co-max-of-Co-set with a fresh dim
 *   - Two co-pi-fresh in sequence get distinct dims
 *   - Sort discipline: co-pi-fresh REJECTS a universe-domain
 *   - The counter is shared with pi-fresh (we don't directly test
 *     specific values because counter state depends on test order;
 *     we test SHAPE)
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* Successor at the couniverse value level. */
  r = lizard_test_eval(&e, "(infer (context) (Uco 0))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Uco 1)");
  r = lizard_test_eval(&e, "(infer (context) (Uco 3))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Uco 4)");

  /* Co-set as a couniverse element. */
  r = lizard_test_eval(&e, "(infer (context) (Co-set 0 1))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Uco 0)");

  /* co-pi-fresh in fresh process — first call gets dim 1000.
   * Since this test file runs as its own process, we get a
   * deterministic counter starting at 1000. */
  r = lizard_test_eval(&e, "(infer (context) (co-pi-fresh 'x (Uco 0) (Uco 0)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(Co-max (Uco 1) (Co-set 1000))");

  /* Next call gets dim 1001. */
  r = lizard_test_eval(&e, "(infer (context) (co-pi-fresh 'y (Uco 0) (Uco 0)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(Co-max (Uco 1) (Co-set 1001))");

  /* co-sigma-fresh shares the counter. */
  r = lizard_test_eval(&e, "(infer (context) (co-sigma-fresh 'z (Uco 0) (Uco 0)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(Co-max (Uco 1) (Co-set 1002))");

  /* pi-fresh shares the SAME counter. */
  r = lizard_test_eval(&e, "(infer (context) (pi-fresh 'a (U 0) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U-set 1 1003)");

  /* And back to co. */
  r = lizard_test_eval(&e, "(infer (context) (co-pi-fresh 'b (Uco 0) (Uco 0)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(Co-max (Uco 1) (Co-set 1004))");

  /* Sort discipline: co-pi-fresh REJECTS a universe domain. */
  r = lizard_test_eval(&e, "(infer (context) (co-pi-fresh 'x (U 0) (Uco 0)))");
  /* The error is "Error: unknown AST node type in eval." because that
   * code is reused for type-errors via type_error(). */
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* Symmetric: pi-fresh would reject a couniverse domain. */
  r = lizard_test_eval(&e, "(infer (context) (pi-fresh 'x (Uco 0) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* Predicates */
  r = lizard_test_eval(&e, "(co-pi-fresh? (co-pi-fresh 'x (Uco 0) (Uco 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e, "(co-pi-fresh? (pi-fresh 'x (U 0) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
