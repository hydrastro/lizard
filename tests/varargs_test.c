/* tests/varargs_test.c
 *
 * Variadic lambdas: (lambda args body) where `args` is bound to the
 * entire argument list.
 */
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* Zero args produces an empty list. */
  r = lizard_test_eval(&e, "((lambda xs xs))");
  TEST_ASSERT(r && r->type == AST_NIL);

  /* One arg → singleton list. */
  r = lizard_test_eval(&e, "((lambda xs xs) 1)");
  TEST_ASSERT_STR(lizard_test_format(r), "(1)");

  /* Many args. */
  r = lizard_test_eval(&e, "((lambda xs xs) 1 2 3 4 5)");
  TEST_ASSERT_STR(lizard_test_format(r), "(1 2 3 4 5)");

  /* Args of mixed types. */
  r = lizard_test_eval(&e, "((lambda xs xs) 1 \"two\" 'three)");
  TEST_ASSERT_STR(lizard_test_format(r), "(1 \"two\" three)");

  /* Define a variadic sum. */
  r = lizard_test_eval(
      &e, "(define sum"
          "  (lambda xs"
          "    (define (loop ys acc)"
          "      (if (null? ys) acc (loop (cdr ys) (+ acc (car ys)))))"
          "    (loop xs 0)))"
          "(sum 1 2 3 4 5 6 7 8 9 10)");
  TEST_ASSERT(lizard_test_is_int(r, 55));

  /* Empty variadic call. */
  r = lizard_test_eval(&e, "(sum)");
  TEST_ASSERT(lizard_test_is_int(r, 0));

  /* Variadic max. */
  r = lizard_test_eval(
      &e, "(define (max2 a b) (if (> a b) a b))"
          "(define vmax"
          "  (lambda xs"
          "    (define (loop ys best)"
          "      (if (null? ys) best (loop (cdr ys) (max2 best (car ys)))))"
          "    (if (null? xs) 'no-max (loop (cdr xs) (car xs)))))"
          "(vmax 3 1 4 1 5 9 2 6 5 3 5)");
  TEST_ASSERT(lizard_test_is_int(r, 9));

  /* Variadic function that just returns its arg count via length. */
  r = lizard_test_eval(
      &e, "(define (length xs) (if (null? xs) 0 (+ 1 (length (cdr xs)))))"
          "(define count (lambda xs (length xs)))"
          "(count 'a 'b 'c 'd 'e)");
  TEST_ASSERT(lizard_test_is_int(r, 5));

  /* Variadic forwarding through a wrapper. */
  r = lizard_test_eval(
      &e, "(define (apply-list f xs)"
          "  (if (null? xs) (f)"
          "      (if (null? (cdr xs)) (f (car xs))"
          "          (if (null? (cdr (cdr xs))) (f (car xs) (car (cdr xs)))"
          "              42))))"
          "(apply-list sum '(10 20 30))");
  /* Falls through to the 42 case since we only handle 0/1/2 here. */
  TEST_ASSERT(lizard_test_is_int(r, 42));

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
