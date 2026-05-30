/* tests/strings_test.c
 *
 * String operations and the symbol/string/number conversion roundtrips.
 */
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* string-length */
  r = lizard_test_eval(&e, "(string-length \"\")");
  TEST_ASSERT(lizard_test_is_int(r, 0));
  r = lizard_test_eval(&e, "(string-length \"hello\")");
  TEST_ASSERT(lizard_test_is_int(r, 5));
  r = lizard_test_eval(&e, "(string-length \"hello world\")");
  TEST_ASSERT(lizard_test_is_int(r, 11));

  /* string-append */
  r = lizard_test_eval(&e, "(string-append \"foo\" \"bar\")");
  TEST_ASSERT_STR(lizard_test_format(r), "\"foobar\"");
  r = lizard_test_eval(&e, "(string-append \"a\" \"b\" \"c\" \"d\")");
  TEST_ASSERT_STR(lizard_test_format(r), "\"abcd\"");
  /* Empty append is empty string. */
  r = lizard_test_eval(&e, "(string-append \"\" \"\")");
  TEST_ASSERT_STR(lizard_test_format(r), "\"\"");
  /* Length of append = sum of lengths. */
  r = lizard_test_eval(&e,
                       "(- (string-length (string-append \"foo\" \"bar\" \"baz\"))"
                       "   (+ (string-length \"foo\")"
                       "      (string-length \"bar\")"
                       "      (string-length \"baz\")))");
  TEST_ASSERT(lizard_test_is_int(r, 0));

  /* substring */
  r = lizard_test_eval(&e, "(substring \"hello world\" 0 5)");
  TEST_ASSERT_STR(lizard_test_format(r), "\"hello\"");
  r = lizard_test_eval(&e, "(substring \"hello world\" 6 11)");
  TEST_ASSERT_STR(lizard_test_format(r), "\"world\"");
  /* Default end = string length. */
  r = lizard_test_eval(&e, "(substring \"hello world\" 6)");
  TEST_ASSERT_STR(lizard_test_format(r), "\"world\"");
  /* Empty substring. */
  r = lizard_test_eval(&e, "(substring \"hello\" 3 3)");
  TEST_ASSERT_STR(lizard_test_format(r), "\"\"");

  /* string=? */
  r = lizard_test_eval(&e, "(string=? \"abc\" \"abc\")");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(string=? \"abc\" \"abd\")");
  TEST_ASSERT(lizard_test_is_false(r));
  r = lizard_test_eval(&e, "(string=? \"\" \"\")");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(string=? \"a\" \"aa\")");
  TEST_ASSERT(lizard_test_is_false(r));

  /* number->string */
  r = lizard_test_eval(&e, "(number->string 0)");
  TEST_ASSERT_STR(lizard_test_format(r), "\"0\"");
  r = lizard_test_eval(&e, "(number->string 12345)");
  TEST_ASSERT_STR(lizard_test_format(r), "\"12345\"");
  r = lizard_test_eval(&e, "(number->string -42)");
  TEST_ASSERT_STR(lizard_test_format(r), "\"-42\"");
  /* Bignum -> string. */
  r = lizard_test_eval(&e, "(string-length (number->string (expt 2 100)))");
  TEST_ASSERT(lizard_test_is_int(r, 31));

  /* string->number */
  r = lizard_test_eval(&e, "(string->number \"42\")");
  TEST_ASSERT(lizard_test_is_int(r, 42));
  r = lizard_test_eval(&e, "(string->number \"-100\")");
  TEST_ASSERT(lizard_test_is_int(r, -100));
  /* Bad input returns #f. */
  r = lizard_test_eval(&e, "(string->number \"not a number\")");
  TEST_ASSERT(lizard_test_is_false(r));

  /* Round-trip: number -> string -> number. */
  r = lizard_test_eval(&e,
                       "(- (string->number (number->string 987654321))"
                       "   987654321)");
  TEST_ASSERT(lizard_test_is_int(r, 0));

  /* symbol/string interconversion. */
  r = lizard_test_eval(&e, "(symbol->string 'hello)");
  TEST_ASSERT_STR(lizard_test_format(r), "\"hello\"");
  r = lizard_test_eval(&e, "(string->symbol \"foo-bar\")");
  TEST_ASSERT(lizard_test_is_symbol(r, "foo-bar"));
  /* Round-trip. */
  r = lizard_test_eval(&e,
                       "(= (string->symbol (symbol->string 'xyz)) 'xyz)");
  TEST_ASSERT(lizard_test_is_true(r));

  /* Errors. */
  r = lizard_test_eval(&e, "(string-length 42)");
  TEST_ASSERT(lizard_test_is_error(r));
  r = lizard_test_eval(&e, "(string-append \"x\" 42)");
  TEST_ASSERT(lizard_test_is_error(r));
  r = lizard_test_eval(&e, "(substring \"x\" 10 20)");  /* out of range */
  TEST_ASSERT(lizard_test_is_error(r));

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
