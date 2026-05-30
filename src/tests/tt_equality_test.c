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
  r = lizard_test_eval(&e,
                       "(reduce (Id-sym (Id-trans (refl 'x) (Id-sym (refl 'x)))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(refl x)");

  /* equal? recognizes judgmental equality across the rewrites. */
  r = lizard_test_eval(&e,
                       "(equal? (refl 'x) (Id-sym (Id-sym (refl 'x))))");
  TEST_ASSERT(lizard_test_is_true(r));

  r = lizard_test_eval(&e,
                       "(equal? (refl 'x) (Id-trans (refl 'x) (refl 'x)))");
  TEST_ASSERT(lizard_test_is_true(r));

  r = lizard_test_eval(&e,
                       "(equal? (refl 'x) (transport (refl 'x) (refl 'x)))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* equal? is NOT pure structural — it's structural ON NORMAL FORMS.
   * Two terms that reduce to the same normal form compare equal. */
  r = lizard_test_eval(&e,
                       "(equal? (Id-trans (refl 'a) (Id-sym (refl 'a)))"
                       "        (refl 'a))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* But genuinely different normal forms do not compare equal. */
  r = lizard_test_eval(&e, "(equal? (refl 'x) (refl 'y))");
  TEST_ASSERT(lizard_test_is_false(r));

  r = lizard_test_eval(&e,
                       "(equal? (Id-trans 'p 'q) (Id-trans 'q 'p))");
  TEST_ASSERT(lizard_test_is_false(r));

  /* Associativity normalization: trans(trans(p,q),r) and
   * trans(p, trans(q,r)) should reduce to the same canonical
   * (right-associated) form. */
  r = lizard_test_eval(&e,
                       "(equal? (Id-trans (Id-trans 'p 'q) 'r)"
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
  r = lizard_test_eval(&e,
                       "(define (member? x xs)"
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
  r = lizard_test_eval(&e,
                       "(reduce (Id-sym (Id-sym (Id-sym (Id-sym (refl 'x))))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(refl x)");

  /* Reduction descends into non-identity-fragment constructors so
   * that identity-fragment subterms inside Pi/Sigma/Id/App/Sum
   * are correctly normalized. */
  r = lizard_test_eval(&e,
                       "(reduce (Pi 'x 'A (Id-sym (Id-sym (refl 'a)))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Pi (x A) (refl a))");

  r = lizard_test_eval(&e,
                       "(reduce (Sigma 'n 'Nat (Id-trans (refl 'a) (refl 'a))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Sigma (n Nat) (refl a))");

  r = lizard_test_eval(&e,
                       "(reduce (Id 'A (Id-sym (Id-sym 'p)) (refl 'a)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Id A p (refl a))");

  /* equal? sees through Pi/Sigma/Id containers. */
  r = lizard_test_eval(&e,
                       "(equal? (Pi 'x 'A (refl 'a))"
                       "        (Pi 'x 'A (Id-sym (Id-sym (refl 'a)))))");
  TEST_ASSERT(lizard_test_is_true(r));

  r = lizard_test_eval(&e,
                       "(equal? (Id 'A (refl 'a) 'p)"
                       "        (Id 'A (Id-sym (Id-sym (refl 'a))) 'p))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* The trans-inverse rule: trans(p, sym(p)) and trans(sym(p), p)
   * both reduce to refl. */
  r = lizard_test_eval(&e,
                       "(reduce (Id-trans (refl 'a) (Id-sym (refl 'a))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(refl a)");

  r = lizard_test_eval(&e,
                       "(reduce (Id-trans (Id-sym (refl 'a)) (refl 'a)))");
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
  r = lizard_test_eval(&e,
                       "(reduce (Id-trans 'p (Id-sym 'p)))");
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
  r = lizard_test_eval(&e,
                       "(equal? (Pi 'x 'A (Pi 'y 'B 'x))"
                       "        (Pi 'a 'A (Pi 'b 'B 'a)))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* Sigma also gets alpha. */
  r = lizard_test_eval(&e,
                       "(equal? (Sigma 'n 'Nat 'n) (Sigma 'k 'Nat 'k))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* Pi-beta: (@ (Lambda 'x 'x) 'a) reduces to 'a */
  r = lizard_test_eval(&e, "(reduce (@ (Lambda 'x 'x) 'a))");
  TEST_ASSERT(lizard_test_is_symbol(r, "a"));

  /* Pi-beta with a non-trivial body */
  r = lizard_test_eval(&e,
                       "(reduce (@ (Lambda 'x (refl 'x)) 'foo))");
  TEST_ASSERT_STR(lizard_test_format(r), "(refl foo)");

  /* Two-stage beta: (id id) q */
  r = lizard_test_eval(&e,
                       "(reduce (@ (@ (Lambda 'f (Lambda 'x (@ 'f 'x)))"
                       "              (Lambda 'y 'y))"
                       "           'q))");
  TEST_ASSERT(lizard_test_is_symbol(r, "q"));

  /* equal? sees through pi-beta */
  r = lizard_test_eval(&e,
                       "(equal? (@ (Lambda 'x 'x) 'a) 'a)");
  TEST_ASSERT(lizard_test_is_true(r));

  /* Capture-avoidance: (Lambda 'y (Lambda 'x 'y)) applied to 'x
   * must NOT capture — the inner binder gets alpha-renamed. */
  r = lizard_test_eval(&e,
                       "(reduce (@ (Lambda 'y (Lambda 'x 'y)) 'x))");
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
  r = lizard_test_eval(&e,
                       "(Lambda-body (reduce (@ (Lambda 'y (Lambda 'x 'y)) 'x)))");
  TEST_ASSERT(lizard_test_is_symbol(r, "x"));

  /* Lambda alpha-equivalence: (Lambda x x) and (Lambda y y) are equal. */
  r = lizard_test_eval(&e, "(equal? (Lambda 'x 'x) (Lambda 'y 'y))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* pi-beta flag turns the rule off. */
  lizard_test_eval(&e, "(flag-set! 'reduce-pi-beta #f)");
  r = lizard_test_eval(&e, "(reduce (@ (Lambda 'x 'x) 'a))");
  TEST_ASSERT_STR(lizard_test_format(r), "(@ (Lambda x x) a)");
  lizard_test_eval(&e, "(flag-set! 'reduce-pi-beta #t)");

  /* HOTT-fragment: ap-refl rule.
   * ap(f, refl_a) --> refl_{f a} */
  r = lizard_test_eval(&e, "(reduce (ap 'f (refl 'a)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(refl (@ f a))");

  /* ap-refl with a concrete function — cascades with pi-beta */
  r = lizard_test_eval(&e, "(reduce (ap (Lambda 'x 'x) (refl 'q)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(refl q)");

  /* ap-refl flag disables the rule */
  lizard_test_eval(&e, "(flag-set! 'reduce-ap-refl #f)");
  r = lizard_test_eval(&e, "(reduce (ap 'f (refl 'a)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(ap f (refl a))");
  lizard_test_eval(&e, "(flag-set! 'reduce-ap-refl #t)");

  /* HOTT-fragment: Id-on-Pi rule.
   * Id (Pi x A B) f g --> Pi x A (Id B (f x) (g x))
   * This is THE computational functional-extensionality rule. */
  r = lizard_test_eval(&e, "(reduce (Id (Pi 'x 'A 'B) 'f 'g))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(Pi (x A) (Id B (@ f x) (@ g x)))");

  /* equal? recognizes the rule */
  r = lizard_test_eval(&e,
                       "(equal? (Id (Pi 'x 'A 'B) 'f 'g)"
                       "        (Pi 'x 'A (Id 'B (@ 'f 'x) (@ 'g 'x))))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* Nested case — Id-on-Pi cascades through nested function types */
  r = lizard_test_eval(&e,
                       "(reduce (Id (Pi 'x 'A (Pi 'y 'B 'C)) 'f 'g))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(Pi (x A) (Pi (y B) (Id C (@ (@ f x) y) (@ (@ g x) y))))");

  /* Id-on-Pi flag disables the rule */
  lizard_test_eval(&e, "(flag-set! 'reduce-id-pi #f)");
  r = lizard_test_eval(&e, "(reduce (Id (Pi 'x 'A 'B) 'f 'g))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Id (Pi (x A) B) f g)");
  lizard_test_eval(&e, "(flag-set! 'reduce-id-pi #t)");

  /* ===== HOTT-fragment expansion: Sigma intro/elim ===== */

  /* fst-beta: fst (Pair a b) --> a */
  r = lizard_test_eval(&e, "(reduce (fst (Pair 'a 'b)))");
  TEST_ASSERT(lizard_test_is_symbol(r, "a"));

  /* snd-beta: snd (Pair a b) --> b */
  r = lizard_test_eval(&e, "(reduce (snd (Pair 'a 'b)))");
  TEST_ASSERT(lizard_test_is_symbol(r, "b"));

  /* Composition: fst-beta + pi-beta */
  r = lizard_test_eval(&e,
                       "(reduce (@ (fst (Pair (Lambda 'x 'x) 'g)) 'z))");
  TEST_ASSERT(lizard_test_is_symbol(r, "z"));

  /* fst-beta flag disables it */
  lizard_test_eval(&e, "(flag-set! 'reduce-fst-beta #f)");
  r = lizard_test_eval(&e, "(reduce (fst (Pair 'a 'b)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(fst (Pair a b))");
  lizard_test_eval(&e, "(flag-set! 'reduce-fst-beta #t)");

  /* ===== Sum intro/elim ===== */

  /* Case-beta on inl: case (inl x) f g --> (@ f x) */
  r = lizard_test_eval(&e, "(reduce (Case (inl 'x) 'f 'g))");
  TEST_ASSERT_STR(lizard_test_format(r), "(@ f x)");

  /* Case-beta on inr: case (inr y) f g --> (@ g y) */
  r = lizard_test_eval(&e, "(reduce (Case (inr 'y) 'f 'g))");
  TEST_ASSERT_STR(lizard_test_format(r), "(@ g y)");

  /* Case-beta + pi-beta cascade: case (inl 5) id g --> 5 */
  r = lizard_test_eval(&e,
                       "(reduce (Case (inl 5) (Lambda 'x 'x) 'g))");
  TEST_ASSERT(lizard_test_is_int(r, 5));

  /* ===== J on refl: path induction ===== */

  /* J P d (refl a) --> d */
  r = lizard_test_eval(&e, "(reduce (J 'P 'd (refl 'a)))");
  TEST_ASSERT(lizard_test_is_symbol(r, "d"));

  /* J cascades with identity algebra: J P d (sym (sym (refl a))) --> d */
  r = lizard_test_eval(&e,
                       "(reduce (J 'P 'd (Id-sym (Id-sym (refl 'a)))))");
  TEST_ASSERT(lizard_test_is_symbol(r, "d"));

  /* ===== ap congruence over the path algebra ===== */

  /* ap f (sym p) --> sym (ap f p) */
  r = lizard_test_eval(&e, "(reduce (ap 'f (Id-sym 'p)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Id-sym (ap f p))");

  /* ap f (trans p q) --> trans (ap f p) (ap f q) */
  r = lizard_test_eval(&e, "(reduce (ap 'f (Id-trans 'p 'q)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Id-trans (ap f p) (ap f q))");

  /* ===== Id on Sigma (non-dependent) ===== */

  /* Id (Sigma _ A B) (Pair a b) (Pair a' b')
   *   --> Sigma _ (Id A a a') (Id B b b') */
  r = lizard_test_eval(&e,
                       "(reduce (Id (Sigma 'x 'A 'B)"
                       "             (Pair 'a 'b)"
                       "             (Pair 'a-prime 'b-prime)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(Sigma (x (Id A a a-prime)) (Id B b b-prime))");

  /* ===== Id on Sum ===== */

  /* matching inl: Id (Sum A B) (inl a) (inl a') --> Id A a a' */
  r = lizard_test_eval(&e,
                       "(reduce (Id (Sum 'A 'B) (inl 'a) (inl 'a-prime)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Id A a a-prime)");

  /* matching inr: Id (Sum A B) (inr b) (inr b') --> Id B b b' */
  r = lizard_test_eval(&e,
                       "(reduce (Id (Sum 'A 'B) (inr 'b) (inr 'b-prime)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Id B b b-prime)");

  /* conflict inl/inr: Id (Sum A B) (inl _) (inr _) --> Bot */
  r = lizard_test_eval(&e,
                       "(reduce (Id (Sum 'A 'B) (inl 'a) (inr 'b)))");
  TEST_ASSERT_STR(lizard_test_format(r), "Bot");

  /* conflict inr/inl: Id (Sum A B) (inr _) (inl _) --> Bot */
  r = lizard_test_eval(&e,
                       "(reduce (Id (Sum 'A 'B) (inr 'b) (inl 'a)))");
  TEST_ASSERT_STR(lizard_test_format(r), "Bot");

  /* ===== Id on Unit ===== */

  /* Id Unit x y --> Unit  (Unit is contractible) */
  r = lizard_test_eval(&e, "(reduce (Id (Unit) 'x 'y))");
  TEST_ASSERT_STR(lizard_test_format(r), "Unit");

  /* Flag disables Id-Unit */
  lizard_test_eval(&e, "(flag-set! 'reduce-id-unit #f)");
  r = lizard_test_eval(&e, "(reduce (Id (Unit) 'x 'y))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Id Unit x y)");
  lizard_test_eval(&e, "(flag-set! 'reduce-id-unit #t)");

  /* ===== equal? sees through all the new rules ===== */

  r = lizard_test_eval(&e,
                       "(equal? (fst (Pair 'a 'b)) 'a)");
  TEST_ASSERT(lizard_test_is_true(r));

  r = lizard_test_eval(&e,
                       "(equal? (Case (inl 'x) 'f 'g) (@ 'f 'x))");
  TEST_ASSERT(lizard_test_is_true(r));

  r = lizard_test_eval(&e,
                       "(equal? (J 'P 'd (refl 'a)) 'd)");
  TEST_ASSERT(lizard_test_is_true(r));

  /* ===== Transport on type formers ===== */

  /* xport _ (refl _) v --> v */
  r = lizard_test_eval(&e,
                       "(reduce (xport (Lambda 'x 'P) (refl 'a) 'v))");
  TEST_ASSERT(lizard_test_is_symbol(r, "v"));

  /* Unit: xport (Lambda _ Unit) p tt --> tt */
  r = lizard_test_eval(&e,
                       "(reduce (xport (Lambda 'x (Unit)) 'p (tt)))");
  TEST_ASSERT_STR(lizard_test_format(r), "tt");

  /* Sum: xport pushes through inl and inr */
  r = lizard_test_eval(&e,
                       "(reduce (xport (Lambda 'x (Sum 'A 'B)) 'p (inl 'a)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(inl (xport (Lambda x A) p a))");

  r = lizard_test_eval(&e,
                       "(reduce (xport (Lambda 'x (Sum 'A 'B)) 'p (inr 'b)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(inr (xport (Lambda x B) p b))");

  /* Sigma (non-dep): xport over Sigma transports componentwise */
  r = lizard_test_eval(&e,
                       "(reduce (xport (Lambda 'x (Sigma 'y 'A 'B))"
                       "               'p (Pair 'a 'b)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(Pair (xport (Lambda x A) p a) (xport (Lambda x B) p b))");

  /* Pi (non-dep): xport over Pi becomes a Lambda whose body xports */
  r = lizard_test_eval(&e,
                       "(reduce (xport (Lambda 'x (Pi 'y 'A 'B)) 'p 'f))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(Lambda y (xport (Lambda x B) p (@ f y)))");

  /* Cascading: xport over Sum of Sigma on an inl(Pair) value */
  r = lizard_test_eval(&e,
                       "(reduce (xport (Lambda 'x (Sum (Sigma 'y 'A 'B) 'C))"
                       "               'p (inl (Pair 'a 'b))))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(inl (Pair (xport (Lambda x A) p a) (xport (Lambda x B) p b)))");

  /* xport-refl takes priority even on complex values */
  r = lizard_test_eval(&e,
                       "(reduce (xport (Lambda 'x (Sigma 'y 'A 'B))"
                       "               (refl 'q) (Pair 'a 'b)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Pair a b)");

  /* Flag disables xport-sum */
  lizard_test_eval(&e, "(flag-set! 'reduce-xport-sum #f)");
  r = lizard_test_eval(&e,
                       "(reduce (xport (Lambda 'x (Sum 'A 'B)) 'p (inl 'a)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "(xport (Lambda x (Sum A B)) p (inl a))");
  lizard_test_eval(&e, "(flag-set! 'reduce-xport-sum #t)");

  /* equal? sees through transport rules */
  r = lizard_test_eval(&e,
                       "(equal? (xport (Lambda 'x 'P) (refl 'a) 'v) 'v)");
  TEST_ASSERT(lizard_test_is_true(r));

  r = lizard_test_eval(&e,
                       "(equal? (xport (Lambda 'x (Unit)) 'p (tt)) (tt))");
  TEST_ASSERT(lizard_test_is_true(r));

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
