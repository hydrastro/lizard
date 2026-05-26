/* tests/syntax_rules_test.c
 *
 * (syntax-rules ...) pattern-matching macros with basic hygiene.
 */
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* Trivial identity. */
  lizard_test_eval(&e,
                   "(define-syntax id (syntax-rules ()"
                   "  ((_ x) x)))");
  r = lizard_test_eval(&e, "(id 42)");
  TEST_ASSERT(lizard_test_is_int(r, 42));
  r = lizard_test_eval(&e, "(id (+ 10 5))");
  TEST_ASSERT(lizard_test_is_int(r, 15));

  /* Computed argument is not re-evaluated; pattern substitutes a
     reference to the same expression. */
  lizard_test_eval(&e,
                   "(define-syntax square (syntax-rules ()"
                   "  ((_ x) (* x x))))");
  r = lizard_test_eval(&e, "(square 7)");
  TEST_ASSERT(lizard_test_is_int(r, 49));

  /* Multi-arg pattern. */
  lizard_test_eval(&e,
                   "(define-syntax add3 (syntax-rules ()"
                   "  ((_ a b c) (+ a b c))))");
  r = lizard_test_eval(&e, "(add3 1 2 3)");
  TEST_ASSERT(lizard_test_is_int(r, 6));

  /* Multiple clauses — chooses by arity. */
  lizard_test_eval(&e,
                   "(define-syntax sum (syntax-rules ()"
                   "  ((_) 0)"
                   "  ((_ a) a)"
                   "  ((_ a b) (+ a b))"
                   "  ((_ a b c) (+ a b c))))");
  r = lizard_test_eval(&e, "(sum)");
  TEST_ASSERT(lizard_test_is_int(r, 0));
  r = lizard_test_eval(&e, "(sum 5)");
  TEST_ASSERT(lizard_test_is_int(r, 5));
  r = lizard_test_eval(&e, "(sum 10 20)");
  TEST_ASSERT(lizard_test_is_int(r, 30));
  r = lizard_test_eval(&e, "(sum 1 2 3)");
  TEST_ASSERT(lizard_test_is_int(r, 6));

  /* Literal identifier — only matches when the call uses the exact
     same identifier. */
  lizard_test_eval(&e,
                   "(define-syntax give (syntax-rules (to)"
                   "  ((_ x to y) (cons y x))))");
  r = lizard_test_eval(&e, "(give 1 to 2)");
  /* Should produce (2 . 1) */
  TEST_ASSERT_STR(lizard_test_format(r), "(2 . 1)");

  /* Macro using let — verifies hygiene. The introduced binding
     `tmp` must NOT capture a `tmp` the caller passes in. */
  lizard_test_eval(&e,
                   "(define-syntax swap! (syntax-rules ()"
                   "  ((_ a b) (let ((tmp a)) (set! a b) (set! b tmp)))))");
  lizard_test_eval(&e, "(define x 1)");
  lizard_test_eval(&e, "(define y 2)");
  lizard_test_eval(&e, "(swap! x y)");
  r = lizard_test_eval(&e, "x");
  TEST_ASSERT(lizard_test_is_int(r, 2));
  r = lizard_test_eval(&e, "y");
  TEST_ASSERT(lizard_test_is_int(r, 1));

  /* The killer hygiene test: the caller's variable is named `tmp`. */
  lizard_test_eval(&e, "(define tmp 99)");
  lizard_test_eval(&e, "(define z 100)");
  lizard_test_eval(&e, "(swap! tmp z)");
  r = lizard_test_eval(&e, "tmp");
  TEST_ASSERT(lizard_test_is_int(r, 100));
  r = lizard_test_eval(&e, "z");
  TEST_ASSERT(lizard_test_is_int(r, 99));

  /* Macro expanding to a lambda. */
  lizard_test_eval(&e,
                   "(define-syntax thunk (syntax-rules ()"
                   "  ((_ body) (lambda () body))))");
  r = lizard_test_eval(&e, "((thunk (+ 100 1)))");
  TEST_ASSERT(lizard_test_is_int(r, 101));

  /* Wildcard pattern. */
  lizard_test_eval(&e,
                   "(define-syntax second (syntax-rules ()"
                   "  ((_ _ x) x)))");
  r = lizard_test_eval(&e, "(second 'ignored 'kept)");
  TEST_ASSERT(lizard_test_is_symbol(r, "kept"));

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
