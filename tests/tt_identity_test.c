/* tests/tt_identity_test.c
 *
 * Identity manipulation + equivalence notation. NOTHING IS CHECKED.
 * Tests verify construction, predicate, accessor, and printing only.
 */
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* Symmetry. */
  r = lizard_test_eval(&e, "(Id-sym (refl 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Id-sym (refl x))");
  r = lizard_test_eval(&e, "(Id-sym? (Id-sym (refl 'x)))");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(type-of (Id-sym (refl 'x)))");
  TEST_ASSERT(lizard_test_is_symbol(r, "Id-sym"));
  r = lizard_test_eval(&e, "(Id-sym-path (Id-sym (refl 'q)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(refl q)");

  /* Transitivity. */
  r = lizard_test_eval(&e, "(Id-trans (refl 'a) (refl 'b))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Id-trans (refl a) (refl b))");
  r = lizard_test_eval(&e, "(Id-trans? (Id-trans 'p 'q))");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(Id-trans-p (Id-trans 'pp 'qq))");
  TEST_ASSERT(lizard_test_is_symbol(r, "pp"));
  r = lizard_test_eval(&e, "(Id-trans-q (Id-trans 'pp 'qq))");
  TEST_ASSERT(lizard_test_is_symbol(r, "qq"));

  /* Transport. */
  r = lizard_test_eval(&e, "(transport (refl 'x) 42)");
  TEST_ASSERT_STR(lizard_test_format(r), "(transport (refl x) 42)");
  r = lizard_test_eval(&e, "(transport? (transport 'p 'v))");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(transport-value (transport 'p 99))");
  TEST_ASSERT(lizard_test_is_int(r, 99));

  /* Equivalence — four-arg constructor. */
  r = lizard_test_eval(&e, "(equivalence 'A 'B 'f 'g)");
  TEST_ASSERT_STR(lizard_test_format(r), "(equivalence A B f g)");
  r = lizard_test_eval(&e, "(equivalence? (equivalence 'A 'B 'f 'g))");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(
      &e, "(equivalence-fwd (equivalence 'A 'B 'forward 'backward))");
  TEST_ASSERT(lizard_test_is_symbol(r, "forward"));
  r = lizard_test_eval(
      &e, "(equivalence-bwd (equivalence 'A 'B 'forward 'backward))");
  TEST_ASSERT(lizard_test_is_symbol(r, "backward"));

  /* Composition is just structural — (Id-sym (Id-sym p)) is NOT p,
     it's a two-level deep symmetric structure. A real checker would
     reduce (Id-sym (Id-sym p)) → p, but lizard doesn't. */
  r = lizard_test_eval(&e, "(Id-sym (Id-sym (refl 'a)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Id-sym (Id-sym (refl a)))");
  /* Symmetry reaches in twice — that's the depth, opaque. */
  r = lizard_test_eval(&e,
                       "(Id-sym? (Id-sym-path (Id-sym (Id-sym (refl 'a)))))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* Sketching the J-rule eliminator pattern as a dispatcher. The
     real J rule says: to define something for all (p : Id A a b),
     it suffices to define it for refl. We're not implementing it,
     just showing the dispatch shape. */
  lizard_test_eval(&e, "(define (J-shape p on-refl on-sym on-trans on-other)"
                       "  (cond ((refl? p)     (on-refl p))"
                       "        ((Id-sym? p)   (on-sym p))"
                       "        ((Id-trans? p) (on-trans p))"
                       "        (else          (on-other p))))");
  r = lizard_test_eval(&e, "(J-shape (refl 'x)"
                           "  (lambda (p) 'is-refl)"
                           "  (lambda (p) 'is-sym)"
                           "  (lambda (p) 'is-trans)"
                           "  (lambda (p) 'other))");
  TEST_ASSERT(lizard_test_is_symbol(r, "is-refl"));

  r = lizard_test_eval(&e, "(J-shape (Id-sym (refl 'x))"
                           "  (lambda (p) 'is-refl)"
                           "  (lambda (p) 'is-sym)"
                           "  (lambda (p) 'is-trans)"
                           "  (lambda (p) 'other))");
  TEST_ASSERT(lizard_test_is_symbol(r, "is-sym"));

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
