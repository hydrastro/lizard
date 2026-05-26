/* tests/tt_context_test.c
 *
 * Couniverse-stratified context layer:
 *   Uco -2 : variables
 *   Uco -1 : contexts
 *   Uco  0 : substitutions, judgments
 *
 * Nothing about these is type-checked. The tests verify that the
 * stratification levels come out correctly, that context-extend and
 * context-lookup behave like an environment (with shadowing), and
 * that pattern-matching on these structures works.
 */
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* Variables sit at Uco -2. */
  lizard_test_eval(&e, "(define vx (variable 'x (U 0)))");
  r = lizard_test_eval(&e, "(variable? vx)");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(uco-level vx)");
  TEST_ASSERT(lizard_test_is_int(r, -2));
  r = lizard_test_eval(&e, "(variable-name vx)");
  TEST_ASSERT(lizard_test_is_symbol(r, "x"));
  r = lizard_test_eval(&e, "(type-of vx)");
  TEST_ASSERT(lizard_test_is_symbol(r, "variable"));

  /* Contexts sit at Uco -1. */
  lizard_test_eval(&e,
                   "(define Gamma (context (variable 'x (U 0))"
                   "                       (variable 'y (U 0))))");
  r = lizard_test_eval(&e, "(context? Gamma)");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(uco-level Gamma)");
  TEST_ASSERT(lizard_test_is_int(r, -1));
  r = lizard_test_eval(&e, "(context-length Gamma)");
  TEST_ASSERT(lizard_test_is_int(r, 2));

  /* (empty-context) is a context of length 0. */
  r = lizard_test_eval(&e, "(context-length (empty-context))");
  TEST_ASSERT(lizard_test_is_int(r, 0));
  r = lizard_test_eval(&e, "(context? (empty-context))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* context-extend is non-mutating: original Gamma still has 2. */
  lizard_test_eval(&e,
                   "(define Gamma2 (context-extend Gamma"
                   "                  (variable 'z (U 1))))");
  r = lizard_test_eval(&e, "(context-length Gamma2)");
  TEST_ASSERT(lizard_test_is_int(r, 3));
  r = lizard_test_eval(&e, "(context-length Gamma)");
  TEST_ASSERT(lizard_test_is_int(r, 2));

  /* Lookup returns the matching variable, or #f. */
  r = lizard_test_eval(&e, "(variable-name (context-lookup Gamma2 'x))");
  TEST_ASSERT(lizard_test_is_symbol(r, "x"));
  r = lizard_test_eval(&e, "(variable-name (context-lookup Gamma2 'z))");
  TEST_ASSERT(lizard_test_is_symbol(r, "z"));
  r = lizard_test_eval(&e, "(context-lookup Gamma2 'no-such)");
  TEST_ASSERT(lizard_test_is_false(r));

  /* Shadowing: extending with a same-named variable returns the
   * innermost one. */
  lizard_test_eval(&e,
                   "(define G3 (context-extend"
                   "             (context-extend (empty-context)"
                   "                             (variable 'x (U 0)))"
                   "             (variable 'x (U 5))))");
  r = lizard_test_eval(&e,
                       "(U-level (variable-type (context-lookup G3 'x)))");
  TEST_ASSERT(lizard_test_is_int(r, 5));   /* innermost wins */

  /* Substitutions sit at Uco 0. */
  lizard_test_eval(&e,
                   "(define s (substitution Gamma (empty-context)"
                   "                        (list (cons 'x 0) (cons 'y 1))))");
  r = lizard_test_eval(&e, "(substitution? s)");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(uco-level s)");
  TEST_ASSERT(lizard_test_is_int(r, 0));
  r = lizard_test_eval(&e, "(context? (substitution-source s))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* Judgments sit at Uco 0 too. They package (ctx, term, type). */
  lizard_test_eval(&e, "(define j (judgment Gamma 'x (U 0)))");
  r = lizard_test_eval(&e, "(judgment? j)");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(uco-level j)");
  TEST_ASSERT(lizard_test_is_int(r, 0));
  r = lizard_test_eval(&e, "(judgment-term j)");
  TEST_ASSERT(lizard_test_is_symbol(r, "x"));
  r = lizard_test_eval(&e, "(context? (judgment-context j))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* uco-level on a non-context-layer value returns #f. */
  r = lizard_test_eval(&e, "(uco-level 42)");
  TEST_ASSERT(lizard_test_is_false(r));
  r = lizard_test_eval(&e, "(uco-level (U 0))");
  TEST_ASSERT(lizard_test_is_false(r));   /* (U 0) is a universe value,
                                             not a couniverse-stratified
                                             thing */

  /* Metaprogramming: walk a context, collect names. */
  lizard_test_eval(&e,
                   "(define G4"
                   "  (context-extend"
                   "    (context-extend"
                   "      (context-extend (empty-context)"
                   "                      (variable 'a (U 0)))"
                   "      (variable 'b (U 0)))"
                   "    (variable 'c (U 0))))");
  lizard_test_eval(&e,
                   "(define (map f xs) (if (null? xs) '()"
                   "                       (cons (f (car xs)) (map f (cdr xs)))))");
  r = lizard_test_eval(&e,
                       "(map variable-name (context-bindings G4))");
  TEST_ASSERT_STR(lizard_test_format(r), "(a b c)");

  /* Pattern-dispatching by uco-level — the kind of thing a future
   * type checker would do at every step. */
  lizard_test_eval(&e,
                   "(define (couniverse-layer x)"
                   "  (cond ((variable? x)     'binding-site)"
                   "        ((context? x)      'context)"
                   "        ((substitution? x) 'morphism)"
                   "        ((judgment? x)     'judgment)"
                   "        (else              'not-applicable)))");
  r = lizard_test_eval(&e, "(couniverse-layer (variable 'x 'A))");
  TEST_ASSERT(lizard_test_is_symbol(r, "binding-site"));
  r = lizard_test_eval(&e, "(couniverse-layer (empty-context))");
  TEST_ASSERT(lizard_test_is_symbol(r, "context"));
  r = lizard_test_eval(&e,
                       "(couniverse-layer (substitution (empty-context)"
                       "                                (empty-context) '()))");
  TEST_ASSERT(lizard_test_is_symbol(r, "morphism"));
  r = lizard_test_eval(&e,
                       "(couniverse-layer (judgment (empty-context) 'x 'A))");
  TEST_ASSERT(lizard_test_is_symbol(r, "judgment"));
  r = lizard_test_eval(&e, "(couniverse-layer 42)");
  TEST_ASSERT(lizard_test_is_symbol(r, "not-applicable"));

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
