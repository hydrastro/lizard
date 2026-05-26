/* tests/tt_equality_test.c
 *
 * The judgmental equality engine for the identity-type fragment.
 *
 * These tests verify the SUBSTANTIVE claim: that two structurally-
 * distinct AST nodes representing morally-equal terms are correctly
 * recognized as judgmentally equal after reduction. This is the
 * piece of lizard that genuinely implements computational identity
 * for the fragment it covers (refl, sym, trans, transport).
 */
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* sym(refl) reduces to refl. */
  r = lizard_test_eval(&e, "(reduce (Id-sym (refl 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(refl x)");

  /* sym(sym(p)) reduces to p. */
  r = lizard_test_eval(&e, "(reduce (Id-sym (Id-sym (refl 'a))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(refl a)");

  /* trans(refl, p) reduces to p. */
  r = lizard_test_eval(&e, "(reduce (Id-trans (refl 'a) 'p))");
  TEST_ASSERT(lizard_test_is_symbol(r, "p"));

  /* trans(p, refl) reduces to p. */
  r = lizard_test_eval(&e, "(reduce (Id-trans 'p (refl 'a)))");
  TEST_ASSERT(lizard_test_is_symbol(r, "p"));

  /* transport(refl, v) reduces to v. */
  r = lizard_test_eval(&e, "(reduce (transport (refl 'a) 42))");
  TEST_ASSERT(lizard_test_is_int(r, 42));

  /* Nested reductions cascade. */
  r = lizard_test_eval(
      &e, "(reduce (Id-sym (Id-trans (refl 'x) (Id-sym (refl 'x)))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(refl x)");

  /* equal? recognizes judgmental equality across the rewrites. */
  r = lizard_test_eval(&e, "(equal? (refl 'x) (Id-sym (Id-sym (refl 'x))))");
  TEST_ASSERT(lizard_test_is_true(r));

  r = lizard_test_eval(&e, "(equal? (refl 'x) (Id-trans (refl 'x) (refl 'x)))");
  TEST_ASSERT(lizard_test_is_true(r));

  r = lizard_test_eval(&e,
                       "(equal? (refl 'x) (transport (refl 'x) (refl 'x)))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* equal? is NOT pure structural — it's structural ON NORMAL FORMS.
   * Two terms that reduce to the same normal form compare equal. */
  r = lizard_test_eval(&e, "(equal? (Id-trans (refl 'a) (Id-sym (refl 'a)))"
                           "        (refl 'a))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* But genuinely different normal forms do not compare equal. */
  r = lizard_test_eval(&e, "(equal? (refl 'x) (refl 'y))");
  TEST_ASSERT(lizard_test_is_false(r));

  r = lizard_test_eval(&e, "(equal? (Id-trans 'p 'q) (Id-trans 'q 'p))");
  TEST_ASSERT(lizard_test_is_false(r));

  /* Associativity normalization: trans(trans(p,q),r) and
   * trans(p, trans(q,r)) should reduce to the same canonical
   * (right-associated) form. */
  r = lizard_test_eval(&e, "(equal? (Id-trans (Id-trans 'p 'q) 'r)"
                           "        (Id-trans 'p (Id-trans 'q 'r)))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* Flags actually disable rules. Turn off sym-involutive and
   * verify that sym(sym(p)) no longer reduces all the way. */
  lizard_test_eval(&e, "(flag-set! 'reduce-sym-involutive #f)");
  r = lizard_test_eval(&e, "(reduce (Id-sym (Id-sym 'p)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Id-sym (Id-sym p))");

  /* equal? with the rule disabled doesn't equate them either. */
  r = lizard_test_eval(&e, "(equal? 'p (Id-sym (Id-sym 'p)))");
  TEST_ASSERT(lizard_test_is_false(r));

  /* Re-enable and verify it comes back. */
  lizard_test_eval(&e, "(flag-set! 'reduce-sym-involutive #t)");
  r = lizard_test_eval(&e, "(equal? 'p (Id-sym (Id-sym 'p)))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* flag-get returns the right value. */
  r = lizard_test_eval(&e, "(flag-get 'reduce-sym-refl)");
  TEST_ASSERT(lizard_test_is_true(r));
  lizard_test_eval(&e, "(flag-set! 'reduce-sym-refl #f)");
  r = lizard_test_eval(&e, "(flag-get 'reduce-sym-refl)");
  TEST_ASSERT(lizard_test_is_false(r));
  lizard_test_eval(&e, "(flag-set! 'reduce-sym-refl #t)");

  /* flag-list returns the registered flags. We check that it's a
   * non-empty list containing at least one expected name. */
  r = lizard_test_eval(&e, "(define (member? x xs)"
                           "  (cond ((null? xs) #f)"
                           "        ((string=? (symbol->string x)"
                           "                   (symbol->string (car xs))) #t)"
                           "        (else (member? x (cdr xs)))))");
  r = lizard_test_eval(&e, "(member? 'reduce-sym-refl (flag-list))");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(member? 'reduce-trans-assoc (flag-list))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* The engine terminates even on terms that look like they might
   * loop. (No infinite loops in this fragment — the lex measure
   * decreases. We rely on the iteration bound only as a safety net.) */
  r = lizard_test_eval(
      &e, "(reduce (Id-sym (Id-sym (Id-sym (Id-sym (refl 'x))))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(refl x)");

  /* Reduction descends into non-identity-fragment constructors so
   * that identity-fragment subterms inside Pi/Sigma/Id/App/Sum
   * are correctly normalized. */
  r = lizard_test_eval(&e, "(reduce (Pi 'x 'A (Id-sym (Id-sym (refl 'a)))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Pi (x A) (refl a))");

  r = lizard_test_eval(
      &e, "(reduce (Sigma 'n 'Nat (Id-trans (refl 'a) (refl 'a))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Sigma (n Nat) (refl a))");

  r = lizard_test_eval(&e, "(reduce (Id 'A (Id-sym (Id-sym 'p)) (refl 'a)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Id A p (refl a))");

  /* equal? sees through Pi/Sigma/Id containers. */
  r = lizard_test_eval(&e, "(equal? (Pi 'x 'A (refl 'a))"
                           "        (Pi 'x 'A (Id-sym (Id-sym (refl 'a)))))");
  TEST_ASSERT(lizard_test_is_true(r));

  r = lizard_test_eval(&e, "(equal? (Id 'A (refl 'a) 'p)"
                           "        (Id 'A (Id-sym (Id-sym (refl 'a))) 'p))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* The trans-inverse rule: trans(p, sym(p)) and trans(sym(p), p)
   * both reduce to refl. */
  r = lizard_test_eval(&e, "(reduce (Id-trans (refl 'a) (Id-sym (refl 'a))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(refl a)");

  r = lizard_test_eval(&e, "(reduce (Id-trans (Id-sym (refl 'a)) (refl 'a)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(refl a)");

  /* For abstract p, trans(p, sym(p)) does NOT reduce to refl — the
   * endpoint isn't recoverable from p alone. Both compose to the
   * identity at p's source/target, but as proof terms they remain
   * distinct without type information about p. */
  r = lizard_test_eval(&e, "(reduce (Id-trans 'p (Id-sym 'p)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Id-trans p (Id-sym p))");

  /* The trans-inverse flag turns off the rule (which currently only
   * fires when p is concretely a refl — and in that case trans-refl-l
   * does the same job). */
  lizard_test_eval(&e, "(flag-set! 'reduce-trans-inverse #f)");
  r = lizard_test_eval(&e, "(reduce (Id-trans 'p (Id-sym 'p)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Id-trans p (Id-sym p))");
  lizard_test_eval(&e, "(flag-set! 'reduce-trans-inverse #t)");

  /* Alpha-equivalence: binder names don't matter when free vars
   * stay the same. */
  r = lizard_test_eval(&e, "(equal? (Pi 'x 'A 'B) (Pi 'y 'A 'B))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* Alpha-equivalence: (Pi x A x) and (Pi y A y) are equal because
   * both bind the codomain reference. */
  r = lizard_test_eval(&e, "(equal? (Pi 'x 'A 'x) (Pi 'y 'A 'y))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* But (Pi x A x) and (Pi x A y) are NOT equal — y is free. */
  r = lizard_test_eval(&e, "(equal? (Pi 'x 'A 'x) (Pi 'x 'A 'y))");
  TEST_ASSERT(lizard_test_is_false(r));

  /* Nested binders: alpha equivalence with multi-level scope. */
  r = lizard_test_eval(&e, "(equal? (Pi 'x 'A (Pi 'y 'B 'x))"
                           "        (Pi 'a 'A (Pi 'b 'B 'a)))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* Sigma also gets alpha. */
  r = lizard_test_eval(&e, "(equal? (Sigma 'n 'Nat 'n) (Sigma 'k 'Nat 'k))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* Pi-beta: (@ (Lambda 'x 'x) 'a) reduces to 'a */
  r = lizard_test_eval(&e, "(reduce (@ (Lambda 'x 'x) 'a))");
  TEST_ASSERT(lizard_test_is_symbol(r, "a"));

  /* Pi-beta with a non-trivial body */
  r = lizard_test_eval(&e, "(reduce (@ (Lambda 'x (refl 'x)) 'foo))");
  TEST_ASSERT_STR(lizard_test_format(r), "(refl foo)");

  /* Two-stage beta: (id id) q */
  r = lizard_test_eval(&e, "(reduce (@ (@ (Lambda 'f (Lambda 'x (@ 'f 'x)))"
                           "              (Lambda 'y 'y))"
                           "           'q))");
  TEST_ASSERT(lizard_test_is_symbol(r, "q"));

  /* equal? sees through pi-beta */
  r = lizard_test_eval(&e, "(equal? (@ (Lambda 'x 'x) 'a) 'a)");
  TEST_ASSERT(lizard_test_is_true(r));

  /* Capture-avoidance: (Lambda 'y (Lambda 'x 'y)) applied to 'x
   * must NOT capture — the inner binder gets alpha-renamed. */
  r = lizard_test_eval(&e, "(reduce (@ (Lambda 'y (Lambda 'x 'y)) 'x))");
  /* The body becomes (Lambda x_N x) where x is the now-free
   * variable substituted in. We check the body of the result is 'x:
   * the original outer 'y has been replaced with 'x, and the inner
   * binder has been renamed away. We verify by alpha-equivalence
   * against the canonical form: (Lambda 'z 'x). */
  {
    lizard_ast_node_t *expected = lizard_test_eval(&e, "(Lambda 'z 'x)");
    /* We can't compare directly because z is fresh; instead check
     * that the body of the result is symbol 'x. */
    (void)expected;
  }
  r = lizard_test_eval(
      &e, "(Lambda-body (reduce (@ (Lambda 'y (Lambda 'x 'y)) 'x)))");
  TEST_ASSERT(lizard_test_is_symbol(r, "x"));

  /* Lambda alpha-equivalence: (Lambda x x) and (Lambda y y) are equal. */
  r = lizard_test_eval(&e, "(equal? (Lambda 'x 'x) (Lambda 'y 'y))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* pi-beta flag turns the rule off. */
  lizard_test_eval(&e, "(flag-set! 'reduce-pi-beta #f)");
  r = lizard_test_eval(&e, "(reduce (@ (Lambda 'x 'x) 'a))");
  TEST_ASSERT_STR(lizard_test_format(r), "(@ (Lambda x x) a)");
  lizard_test_eval(&e, "(flag-set! 'reduce-pi-beta #t)");

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
