/* tests/reflection_test.c
 *
 * Runtime introspection: type-of, defined?, env-keys, procedure-arity.
 */
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* type-of for every flavour we expose. */
  r = lizard_test_eval(&e, "(type-of 42)");
  TEST_ASSERT(lizard_test_is_symbol(r, "number"));
  r = lizard_test_eval(&e, "(type-of \"hi\")");
  TEST_ASSERT(lizard_test_is_symbol(r, "string"));
  r = lizard_test_eval(&e, "(type-of 'foo)");
  TEST_ASSERT(lizard_test_is_symbol(r, "symbol"));
  r = lizard_test_eval(&e, "(type-of #t)");
  TEST_ASSERT(lizard_test_is_symbol(r, "boolean"));
  r = lizard_test_eval(&e, "(type-of '())");
  TEST_ASSERT(lizard_test_is_symbol(r, "nil"));
  r = lizard_test_eval(&e, "(type-of '(1 2 3))");
  TEST_ASSERT(lizard_test_is_symbol(r, "pair"));
  r = lizard_test_eval(&e, "(type-of (cons 1 2))");
  TEST_ASSERT(lizard_test_is_symbol(r, "pair"));
  r = lizard_test_eval(&e, "(type-of (lambda (x) x))");
  TEST_ASSERT(lizard_test_is_symbol(r, "procedure"));
  r = lizard_test_eval(&e, "(type-of +)");
  TEST_ASSERT(lizard_test_is_symbol(r, "primitive"));

  /* defined? */
  r = lizard_test_eval(&e, "(defined? '+)");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(defined? 'no-such-binding)");
  TEST_ASSERT(lizard_test_is_false(r));
  r = lizard_test_eval(&e, "(define my-special-thing 42)"
                           "(defined? 'my-special-thing)");
  TEST_ASSERT(lizard_test_is_true(r));

  /* procedure-arity counts formal parameters. */
  r = lizard_test_eval(&e, "(procedure-arity (lambda () 1))");
  TEST_ASSERT(lizard_test_is_int(r, 0));
  r = lizard_test_eval(&e, "(procedure-arity (lambda (x) x))");
  TEST_ASSERT(lizard_test_is_int(r, 1));
  r = lizard_test_eval(&e, "(procedure-arity (lambda (a b c d e) a))");
  TEST_ASSERT(lizard_test_is_int(r, 5));
  /* Primitives are variadic from Lisp's POV. */
  r = lizard_test_eval(&e, "(procedure-arity +)");
  TEST_ASSERT(lizard_test_is_symbol(r, "variadic"));

  /* env-keys returns a list including the symbols we defined. */
  r = lizard_test_eval(&e, "(define needle-1 1)"
                           "(define needle-2 2)"
                           "(define (member? x xs)"
                           "  (cond ((null? xs) #f)"
                           "        ((= x (car xs)) #t)"
                           "        (else (member? x (cdr xs)))))"
                           "(and (member? 'needle-1 (env-keys))"
                           "     (member? 'needle-2 (env-keys))"
                           "     (member? '+ (env-keys)))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* Errors. */
  r = lizard_test_eval(&e, "(type-of)");
  TEST_ASSERT(lizard_test_is_error(r));
  r = lizard_test_eval(&e, "(defined? 42)"); /* non-symbol */
  TEST_ASSERT(lizard_test_is_error(r));
  r = lizard_test_eval(&e, "(procedure-arity 42)");
  TEST_ASSERT(lizard_test_is_error(r));

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
