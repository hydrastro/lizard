/* tests/tt_hit_test.c
 *
 * Tests for Phase H.1: HIT scaffolding.
 *
 * H.1 provides the *data structures* for higher inductive types:
 * declarations are first-class values, get registered in a process-
 * local registry, and can be looked up by name. Constructor and path
 * records are themselves AST nodes that compose into declarations.
 *
 * H.1 deliberately does NOT provide computation rules for HIT
 * instances — applying a recursor, computing on a path constructor,
 * comp interaction — those require per-HIT or per-shape work that
 * is the open research problem. This test file covers what H.1
 * actually delivers: declaration, registration, lookup, structural
 * equality, and basic typing.
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* Reset registry so test ordering doesn't matter. */
  lizard_tt_hit_registry_reset();

  /* Bare records (no registration side effect). */
  r = lizard_test_eval(&e, "(HIT-constructor 'base)");
  TEST_ASSERT_STR(lizard_test_format(r), "(HIT-constructor base)");

  r = lizard_test_eval(&e, "(HIT-constructor 'inj (U 0))");
  TEST_ASSERT_STR(lizard_test_format(r), "(HIT-constructor inj (U 0))");

  r = lizard_test_eval(&e, "(HIT-path 'loop 'base 'base)");
  TEST_ASSERT_STR(lizard_test_format(r), "(HIT-path loop base base)");

  /* Predicates */
  r = lizard_test_eval(&e, "(HIT-constructor? (HIT-constructor 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e, "(HIT-path? (HIT-path 'p 'a 'b))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  /* Declaration with side-effect registration. */
  lizard_test_eval(&e,
                   "(define s1 (HIT 'S1 "
                   "  (HIT-constructor 'base) "
                   "  (HIT-path 'loop 'base 'base)))");

  r = lizard_test_eval(&e, "(HIT? s1)");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  /* Registry lookup finds it. */
  r = lizard_test_eval(&e, "(HIT-lookup 'S1)");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(HIT S1 (HIT-constructor base) (HIT-path loop base base))");

  /* Unregistered name returns nil. */
  r = lizard_test_eval(&e, "(HIT-lookup 'NotDefined)");
  TEST_ASSERT_STR(lizard_test_format(r), "()");

  /* HIT-ref of a registered HIT types at (U 0). */
  r = lizard_test_eval(&e, "(infer (context) (HIT-ref 'S1))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");

  /* HIT-app of base types at the host HIT. */
  r = lizard_test_eval(&e, "(infer (context) (HIT-app 'base))");
  TEST_ASSERT_STR(lizard_test_format(r), "(HIT-ref S1)");

  /* Declaring a second HIT — constructor lookup across multiple. */
  lizard_test_eval(&e,
                   "(define tree (HIT 'Tree "
                   "  (HIT-constructor 'leaf) "
                   "  (HIT-constructor 'node)))");

  r = lizard_test_eval(&e, "(infer (context) (HIT-app 'leaf))");
  TEST_ASSERT_STR(lizard_test_format(r), "(HIT-ref Tree)");
  r = lizard_test_eval(&e, "(infer (context) (HIT-app 'base))");
  TEST_ASSERT_STR(lizard_test_format(r), "(HIT-ref S1)");

  /* Both HITs in registry. */
  r = lizard_test_eval(&e, "(HIT-lookup 'Tree)");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(HIT Tree (HIT-constructor leaf) (HIT-constructor node))");

  /* HIT instance under reduction: HIT-app stays as-is (no comp rules). */
  r = lizard_test_eval(&e, "(reduce (HIT-app 'base))");
  TEST_ASSERT_STR(lizard_test_format(r), "(HIT-app base)");

  /* HIT-ref under reduction: also unchanged. */
  r = lizard_test_eval(&e, "(reduce (HIT-ref 'S1))");
  TEST_ASSERT_STR(lizard_test_format(r), "(HIT-ref S1)");

  /* Cleanup so other tests don't see our entries. */
  lizard_tt_hit_registry_reset();

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
