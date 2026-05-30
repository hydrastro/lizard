/* tests/hashes_test.c
 *
 * Hash table creation, set/ref/remove, growth under load, key types.
 */
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* Basic set + ref. */
  r = lizard_test_eval(&e,
                       "(define h (make-hash-table))"
                       "(hash-set! h 'name \"lizard\")"
                       "(hash-set! h 'age 5)"
                       "(hash-ref h 'name)");
  TEST_ASSERT_STR(lizard_test_format(r), "\"lizard\"");

  r = lizard_test_eval(&e, "(hash-ref h 'age)");
  TEST_ASSERT(lizard_test_is_int(r, 5));

  /* hash-size grows with new keys. */
  r = lizard_test_eval(&e, "(hash-size h)");
  TEST_ASSERT(lizard_test_is_int(r, 2));

  /* hash-set! on existing key updates without growing size. */
  r = lizard_test_eval(&e,
                       "(hash-set! h 'age 6)"
                       "(hash-size h)");
  TEST_ASSERT(lizard_test_is_int(r, 2));
  r = lizard_test_eval(&e, "(hash-ref h 'age)");
  TEST_ASSERT(lizard_test_is_int(r, 6));

  /* hash-has-key? */
  r = lizard_test_eval(&e, "(hash-has-key? h 'name)");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(hash-has-key? h 'not-here)");
  TEST_ASSERT(lizard_test_is_false(r));

  /* hash-ref with default. */
  r = lizard_test_eval(&e, "(hash-ref h 'nope 'fallback)");
  TEST_ASSERT(lizard_test_is_symbol(r, "fallback"));
  /* Default-less miss returns #f. */
  r = lizard_test_eval(&e, "(hash-ref h 'nope)");
  TEST_ASSERT(lizard_test_is_false(r));

  /* String keys. */
  r = lizard_test_eval(&e,
                       "(define h2 (make-hash-table))"
                       "(hash-set! h2 \"alpha\" 1)"
                       "(hash-set! h2 \"beta\" 2)"
                       "(hash-set! h2 \"gamma\" 3)"
                       "(hash-ref h2 \"beta\")");
  TEST_ASSERT(lizard_test_is_int(r, 2));

  /* Numeric keys including bignums. */
  r = lizard_test_eval(&e,
                       "(define h3 (make-hash-table))"
                       "(hash-set! h3 1 'one)"
                       "(hash-set! h3 2 'two)"
                       "(hash-set! h3 (expt 2 100) 'huge)"
                       "(hash-ref h3 (expt 2 100))");
  TEST_ASSERT(lizard_test_is_symbol(r, "huge"));

  /* Growth: insert 100 entries to force several rehashes. */
  r = lizard_test_eval(&e,
                       "(define big (make-hash-table))"
                       "(define (loop i n)"
                       "  (if (= i n) 'done"
                       "      (begin (hash-set! big i (* i i))"
                       "             (loop (+ i 1) n))))"
                       "(loop 0 200)"
                       "(hash-size big)");
  TEST_ASSERT(lizard_test_is_int(r, 200));

  /* And every entry is still findable. */
  r = lizard_test_eval(&e, "(hash-ref big 137)");
  TEST_ASSERT(lizard_test_is_int(r, 137 * 137));
  r = lizard_test_eval(&e, "(hash-ref big 199)");
  TEST_ASSERT(lizard_test_is_int(r, 199 * 199));

  /* hash-keys returns all keys (as a list). */
  r = lizard_test_eval(&e,
                       "(define (length xs)"
                       "  (if (null? xs) 0 (+ 1 (length (cdr xs)))))"
                       "(length (hash-keys big))");
  TEST_ASSERT(lizard_test_is_int(r, 200));

  /* hash-remove! actually removes. */
  r = lizard_test_eval(&e,
                       "(hash-remove! big 50)"
                       "(hash-has-key? big 50)");
  TEST_ASSERT(lizard_test_is_false(r));
  r = lizard_test_eval(&e, "(hash-size big)");
  TEST_ASSERT(lizard_test_is_int(r, 199));
  /* Others still present after a removal. */
  r = lizard_test_eval(&e, "(hash-ref big 51)");
  TEST_ASSERT(lizard_test_is_int(r, 51 * 51));
  r = lizard_test_eval(&e, "(hash-ref big 49)");
  TEST_ASSERT(lizard_test_is_int(r, 49 * 49));

  /* Errors. */
  r = lizard_test_eval(&e, "(hash-ref 42 'k)");
  TEST_ASSERT(lizard_test_is_error(r));
  r = lizard_test_eval(&e, "(hash-set! \"not-a-hash\" 'k 'v)");
  TEST_ASSERT(lizard_test_is_error(r));

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
