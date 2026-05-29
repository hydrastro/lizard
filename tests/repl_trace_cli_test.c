/* tests/repl_trace_cli_test.c
 *
 * CLI regression coverage for Phase 2L.  The trace flags must remain
 * opt-in, printable, and compatible with normal --eval execution.
 */

#include "test_harness.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int file_contains(const char *path, const char *needle) {
  FILE *fp;
  char buffer[4096];
  size_t n;

  fp = fopen(path, "rb");
  if (fp == NULL) {
    return 0;
  }
  n = fread(buffer, 1U, sizeof(buffer) - 1U, fp);
  fclose(fp);
  buffer[n] = '\0';
  return strstr(buffer, needle) != NULL;
}

int main(void) {
  int status;
  const char *plain_out;
  const char *trace_out;
  const char *trace_file_out;
  const char *trace_file;
  const char *expand_out;
  const char *expand_json_out;
  const char *expand_bad_format_out;

  plain_out = "build/tests/repl_trace_cli_plain.out";
  trace_out = "build/tests/repl_trace_cli_trace.out";
  trace_file_out = "build/tests/repl_trace_cli_trace_file.out";
  trace_file = "build/tests/repl_trace_cli_trace.lines";
  expand_out = "build/tests/repl_trace_cli_expand.out";
  expand_json_out = "build/tests/repl_trace_cli_expand_json.out";
  expand_bad_format_out = "build/tests/repl_trace_cli_expand_bad_format.out";

  status = system("build/lizard --eval '(+ 1 2)' > build/tests/repl_trace_cli_plain.out 2>&1");
  TEST_ASSERT_EQ(status, 0);
  TEST_ASSERT(file_contains(plain_out, "=> 3"));
  TEST_ASSERT(!file_contains(plain_out, "Expansion trace:"));

  status = system("build/lizard --trace-expansion --print-expansion-trace --eval '(+ 1 2)' > build/tests/repl_trace_cli_trace.out 2>&1");
  TEST_ASSERT_EQ(status, 0);
  TEST_ASSERT(file_contains(trace_out, "=> 3"));
  TEST_ASSERT(file_contains(trace_out, "Expansion trace:"));
  TEST_ASSERT(file_contains(trace_out, "macro-expand"));

  status = system("build/lizard --trace-expansion-file build/tests/repl_trace_cli_trace.lines --eval '(+ 1 2)' > build/tests/repl_trace_cli_trace_file.out 2>&1");
  TEST_ASSERT_EQ(status, 0);
  TEST_ASSERT(file_contains(trace_file_out, "=> 3"));
  TEST_ASSERT(!file_contains(trace_file_out, "Expansion trace:"));
  TEST_ASSERT(file_contains(trace_file, "lizard-expansion-trace"));
  TEST_ASSERT(file_contains(trace_file, "v=1"));
  TEST_ASSERT(file_contains(trace_file, "event\tindex=0"));
  TEST_ASSERT(file_contains(trace_file, "stage=macro-expand"));

  status = system("build/lizard --expand-only --eval '(+ 1 2)' > build/tests/repl_trace_cli_expand.out 2>&1");
  TEST_ASSERT_EQ(status, 0);
  TEST_ASSERT(file_contains(expand_out, "lizard-syntax-expansion"));
  TEST_ASSERT(file_contains(expand_out, "expanded"));
  TEST_ASSERT(file_contains(expand_out, "macro-expand"));
  TEST_ASSERT(!file_contains(expand_out, "=> 3"));

  status = system("build/lizard --expand-only --expand-only-format json --eval '(+ 1 2)' > build/tests/repl_trace_cli_expand_json.out 2>&1");
  TEST_ASSERT_EQ(status, 0);
  TEST_ASSERT(file_contains(expand_json_out, "\"type\":\"lizard-syntax-expansion\""));
  TEST_ASSERT(file_contains(expand_json_out, "\"trace\":"));
  TEST_ASSERT(file_contains(expand_json_out, "macro-expand"));
  TEST_ASSERT(!file_contains(expand_json_out, "lizard-syntax-expansion\tv=1"));
  TEST_ASSERT(!file_contains(expand_json_out, "=> 3"));

  status = system("build/lizard --expand-only --expand-only-format bad --eval '(+ 1 2)' > build/tests/repl_trace_cli_expand_bad_format.out 2>&1");
  TEST_ASSERT(status != 0);
  TEST_ASSERT(file_contains(expand_bad_format_out, "invalid --expand-only-format"));

  (void)remove(plain_out);
  (void)remove(trace_out);
  (void)remove(trace_file_out);
  (void)remove(trace_file);
  (void)remove(expand_out);
  (void)remove(expand_json_out);
  (void)remove(expand_bad_format_out);
  TEST_RETURN();
}
