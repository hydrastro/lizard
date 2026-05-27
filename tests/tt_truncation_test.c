/* tests/tt_truncation_test.c
 *
 * Tests for Phase H.2 — propositional truncation.
 *
 * What this turn ships:
 *   1. Type former (Trunc A) at the same universe as A.
 *   2. Constructor (trunc-intro e) of type (Trunc A) when e : A.
 *   3. Recursor (trunc-rec C cm cs e) of type C, when:
 *        - C is a type
 *        - cm has type (Pi _ A C) where e : (Trunc A)
 *        - cs has at least the outer Pi shape Pi _:C ...
 *        - e is Trunc-typed
 *   4. Primary computation rule:
 *        (trunc-rec C cm cs (trunc-intro x))  ⟶  (@ cm x)
 *      The rule is deterministic: no other LHS pattern matches.
 *
 * Honest gap: cs's full propositionality shape (Π x:C. Π y:C. Path C x y)
 * isn't verified — only that cs's type starts with a Pi over C. A
 * future turn ("propositionality coherence") can tighten the check.
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);
  lizard_logic_config_reset();

  /* === Type former === */

  r = lizard_test_eval(&e, "(infer (context) (Trunc (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  r = lizard_test_eval(&e, "(infer (context) (Trunc (U 5)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 6)");

  /* Trunc of a non-type rejected. */
  r = lizard_test_eval(&e, "(infer (context) (Trunc 'x))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* === Constructor === */

  /* In an empty context, trunc-intro of a universe works since
   * the universe is a value of the next universe up. */
  r = lizard_test_eval(&e, "(infer (context) (trunc-intro (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Trunc (U 1))");

  /* With a variable in context. */
  lizard_test_eval(&e, "(define ctx1 (context-extend (context) (variable 'x (U 0))))");
  r = lizard_test_eval(&e, "(infer ctx1 (trunc-intro 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Trunc (U 0))");

  /* === Recursor === */

  /* Build the canonical setup:
   *   A : (U 0)
   *   C : (U 0)
   *   cm : Pi _ A C
   *   cs : Pi x C (Pi y C (Path C x y))
   *   e  : Trunc A
   */
  lizard_test_eval(&e,
    "(define ctx2 "
    "  (context-extend "
    "    (context-extend "
    "      (context-extend "
    "        (context-extend "
    "          (context-extend (context) "
    "            (variable 'A (U 0))) "
    "          (variable 'C (U 0))) "
    "        (variable 'cm (Pi '_ 'A 'C))) "
    "      (variable 'cs (Pi 'x 'C (Pi 'y 'C (Path 'C 'x 'y))))) "
    "    (variable 'e (Trunc 'A))))");

  /* Recursor types as the motive C. */
  r = lizard_test_eval(&e, "(infer ctx2 (trunc-rec 'C 'cm 'cs 'e))");
  TEST_ASSERT_STR(lizard_test_format(r), "C");

  /* === Primary computation rule === */

  /* Beta on a literal trunc-intro: reduces to (@ cm x). */
  r = lizard_test_eval(&e,
    "(reduce (trunc-rec 'C 'cm 'cs (trunc-intro 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(@ cm x)");

  /* === Determinism === */

  /* Variable scrutinee: no LHS pattern matches, recursor stays. */
  r = lizard_test_eval(&e, "(reduce (trunc-rec 'C 'cm 'cs 'e))");
  TEST_ASSERT_STR(lizard_test_format(r), "(trunc-rec C cm cs e)");

  /* Recursor scrutinee (not a trunc-intro): no match. */
  r = lizard_test_eval(&e,
    "(reduce (trunc-rec 'C 'cm 'cs (trunc-rec 'D 'cm2 'cs2 'e)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(trunc-rec C cm cs (trunc-rec D cm2 cs2 e))");

  /* Beta fires inside compound terms. */
  r = lizard_test_eval(&e,
    "(reduce (trunc-rec 'C 'cm 'cs (trunc-intro (trunc-intro 'x))))");
  /* Inner trunc-intro doesn't reduce (not at a recursor); outer beta
   * fires; result is (@ cm (trunc-intro x)). */
  TEST_ASSERT_STR(lizard_test_format(r), "(@ cm (trunc-intro x))");

  /* === Error cases === */

  /* Scrutinee not Trunc-typed: rejected. */
  lizard_test_eval(&e,
    "(define ctxBad (context-extend (context) (variable 'e2 (U 0))))");
  r = lizard_test_eval(&e, "(infer ctxBad (trunc-rec (U 0) 'cm 'cs 'e2))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* === Predicates and accessors === */

  r = lizard_test_eval(&e, "(Trunc? (Trunc (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e, "(Trunc? (U 0))");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");

  r = lizard_test_eval(&e, "(trunc-intro? (trunc-intro 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e, "(trunc-rec? (trunc-rec 'C 'cm 'cs 'e))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  r = lizard_test_eval(&e, "(Trunc-arg (Trunc (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");
  r = lizard_test_eval(&e, "(trunc-intro-body (trunc-intro 'foo))");
  TEST_ASSERT_STR(lizard_test_format(r), "foo");
  r = lizard_test_eval(&e, "(trunc-rec-motive (trunc-rec 'A 'b 'c 'd))");
  TEST_ASSERT_STR(lizard_test_format(r), "A");
  r = lizard_test_eval(&e, "(trunc-rec-scrutinee (trunc-rec 'A 'b 'c 'd))");
  TEST_ASSERT_STR(lizard_test_format(r), "d");

  lizard_logic_config_reset();
  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
