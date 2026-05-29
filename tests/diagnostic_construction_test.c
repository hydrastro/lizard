/* tests/diagnostic_construction_test.c
 *
 * Phase 2Z: canonical diagnostic construction helpers.
 */

#include "lizard_api.h"
#include "test_harness.h"

#include <string.h>

int main(void) {
  lizard_source_span_t span;
  lizard_diagnostic_t diagnostic;
  lizard_diagnostic_t copy;

  lizard_source_span_clear(&span);
  TEST_ASSERT(span.filename == NULL);
  TEST_ASSERT_EQ(span.start_line, 0);
  TEST_ASSERT_EQ(span.start_column, 0);

  lizard_source_span_set(&span, "diag-test.lisp", 2, 3, 2, 9, 10, 16);
  TEST_ASSERT_STR(span.filename, "diag-test.lisp");
  TEST_ASSERT_EQ(span.start_line, 2);
  TEST_ASSERT_EQ(span.start_column, 3);
  TEST_ASSERT_EQ(span.end_column, 9);
  TEST_ASSERT_EQ(span.end_offset, 16);

  lizard_diagnostic_clear(&diagnostic);
  TEST_ASSERT_EQ(diagnostic.status, LIZARD_STATUS_OK);
  TEST_ASSERT_EQ(diagnostic.severity, LIZARD_DIAGNOSTIC_SEVERITY_INFO);
  TEST_ASSERT_EQ(diagnostic.category, LIZARD_DIAGNOSTIC_CATEGORY_UNKNOWN);
  TEST_ASSERT(diagnostic.message[0] == '\0');

  lizard_diagnostic_set(&diagnostic, LIZARD_STATUS_PARSE_ERROR,
                        LIZARD_DIAGNOSTIC_CATEGORY_TOKENIZER, &span,
                        "bad token");
  TEST_ASSERT_EQ(diagnostic.status, LIZARD_STATUS_PARSE_ERROR);
  TEST_ASSERT_EQ(diagnostic.severity, LIZARD_DIAGNOSTIC_SEVERITY_ERROR);
  TEST_ASSERT_EQ(diagnostic.category, LIZARD_DIAGNOSTIC_CATEGORY_TOKENIZER);
  TEST_ASSERT_STR(diagnostic.span.filename, "diag-test.lisp");
  TEST_ASSERT_STR(diagnostic.message, "bad token");

  lizard_diagnostic_copy(&copy, &diagnostic);
  TEST_ASSERT_EQ(copy.status, LIZARD_STATUS_PARSE_ERROR);
  TEST_ASSERT_EQ(copy.category, LIZARD_DIAGNOSTIC_CATEGORY_TOKENIZER);
  TEST_ASSERT_STR(copy.message, "bad token");

  lizard_diagnostic_set_simple(&diagnostic, LIZARD_STATUS_IO_ERROR,
                               LIZARD_DIAGNOSTIC_CATEGORY_UNKNOWN,
                               "io failed");
  TEST_ASSERT_EQ(diagnostic.status, LIZARD_STATUS_IO_ERROR);
  TEST_ASSERT_EQ(diagnostic.category, LIZARD_DIAGNOSTIC_CATEGORY_IO);
  TEST_ASSERT_STR(diagnostic.message, "io failed");

  TEST_RETURN();
}
