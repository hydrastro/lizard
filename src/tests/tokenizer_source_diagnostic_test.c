#include "test_harness.h"
#include "test_helpers.h"

#include <string.h>

int main(void) {
  lizard_test_env_t e;
  lizard_diagnostic_t diagnostic;
  lz_list_t *tokens;

  lizard_test_env_init(&e);

  lizard_diagnostic_clear(&diagnostic);
  tokens = lizard_tokenize_source("(define x 1)", "source-test.lisp",
                                  &diagnostic);
  TEST_ASSERT(tokens != NULL);
  TEST_ASSERT_EQ(diagnostic.status, LIZARD_STATUS_OK);
  lizard_free_tokens(tokens);

  lizard_diagnostic_clear(&diagnostic);
  tokens = lizard_tokenize_source("\"unterminated", "bad-source.lisp",
                                  &diagnostic);
  TEST_ASSERT(tokens == NULL);
  TEST_ASSERT_EQ(diagnostic.status, LIZARD_STATUS_PARSE_ERROR);
  TEST_ASSERT_EQ(diagnostic.category, LIZARD_DIAGNOSTIC_CATEGORY_TOKENIZER);
  TEST_ASSERT(diagnostic.span.filename != NULL);
  TEST_ASSERT(strcmp(diagnostic.span.filename, "bad-source.lisp") == 0);
  TEST_ASSERT(strstr(diagnostic.message, "unterminated") != NULL);

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
