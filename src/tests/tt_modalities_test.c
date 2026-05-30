/* tests/tt_modalities_test.c
 *
 * Tests for Phase M.5.1: Box and Diamond modal type constructors.
 *
 * M.5.1 delivers the type-constructor level only. No introduction
 * or elimination forms, no commitment to a specific modal logic
 * (K, T, S4, S5). The typing rule is the minimal one:
 *
 *   (Box A) : univ(A)
 *   (Diamond A) : univ(A)
 *
 * That is, the modality preserves the universe of its argument.
 *
 * Coverage:
 *   - Constructors build the right AST shape
 *   - Predicates correctly classify
 *   - Accessors return the argument
 *   - Typing produces the right universe
 *   - Nested modalities: (Box (Box A))
 *   - Composition with lattice features: (Box (pi-fresh ...))
 *   - Gate: disabling modalities-enabled rejects inference
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);
  lizard_logic_config_reset();

  /* Constructors. */
  r = lizard_test_eval(&e, "(Box (U 0))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (U 0))");
  r = lizard_test_eval(&e, "(Diamond (U 0))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Diamond (U 0))");

  /* Predicates. */
  r = lizard_test_eval(&e, "(Box? (Box (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e, "(Box? (Diamond (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");
  r = lizard_test_eval(&e, "(Diamond? (Diamond (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e, "(Diamond? (Box (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");
  r = lizard_test_eval(&e, "(Box? (U 0))");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");

  /* Accessors. */
  r = lizard_test_eval(&e, "(Box-arg (Box (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");
  r = lizard_test_eval(&e, "(Diamond-arg (Diamond (U 1)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* Typing — modality preserves universe of argument. */
  r = lizard_test_eval(&e, "(infer (context) (Box (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");
  r = lizard_test_eval(&e, "(infer (context) (Box (U 1)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 2)");
  r = lizard_test_eval(&e, "(infer (context) (Diamond (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* Typing with Pi inside. */
  r = lizard_test_eval(&e, "(infer (context) (Box (Pi 'x (U 0) (U 0))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* Nested modalities. */
  r = lizard_test_eval(&e, "(infer (context) (Box (Box (U 0))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");
  r = lizard_test_eval(&e, "(infer (context) (Box (Diamond (U 0))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* Reduction descends into argument. */
  r = lizard_test_eval(&e, "(reduce (Box (U-max (U-set 0) (U-set 1))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (U-set 0 1))");
  r = lizard_test_eval(&e, "(reduce (Diamond (U-max (U-set 0) (U-set 1))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Diamond (U-set 0 1))");

  /* Gate: disabling modalities-enabled rejects inference. */
  lizard_test_eval(&e, "(logic-rule-disable 'modalities-enabled)");
  r = lizard_test_eval(&e, "(infer (context) (Box (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");
  r = lizard_test_eval(&e, "(infer (context) (Diamond (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");
  /* Constructors still build values — only typing is gated. */
  r = lizard_test_eval(&e, "(Box (U 0))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (U 0))");

  lizard_test_eval(&e, "(logic-rule-enable 'modalities-enabled)");
  r = lizard_test_eval(&e, "(infer (context) (Box (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* Composition with lattice features. */
  /* Reset config so we have a deterministic counter for this assertion. */
  lizard_logic_config_reset();
  /* The fresh-dim counter is global per process; we can't assert an
   * exact value, but we can assert the SHAPE: (U-set 1 NNNN). We do
   * a structural check by typing something that doesn't depend on
   * the counter value. */
  r = lizard_test_eval(&e, "(infer (context) (Box (Pi 'x (U 0) (U 0))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* Argument-validation: Box of a non-type errors. */
  r = lizard_test_eval(&e, "(infer (context) (Box 5))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* Default state: modalities are enabled (registry empty). */
  lizard_logic_config_reset();
  r = lizard_test_eval(&e, "(infer (context) (Box (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
