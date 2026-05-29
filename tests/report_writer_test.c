#include "report_writer.h"
#include "test_harness.h"

#include <stdio.h>
#include <string.h>

static int read_tmpfile(FILE *fp, char *buffer, size_t buffer_size) {
  long pos;
  size_t wanted;
  size_t got;

  if (fp == NULL || buffer == NULL || buffer_size == 0U) {
    return 0;
  }
  if (fflush(fp) != 0) {
    return 0;
  }
  pos = ftell(fp);
  if (pos < 0L) {
    return 0;
  }
  if ((unsigned long)pos >= (unsigned long)buffer_size) {
    return 0;
  }
  if (fseek(fp, 0L, SEEK_SET) != 0) {
    return 0;
  }
  wanted = (size_t)pos;
  got = fread(buffer, 1U, wanted, fp);
  if (got != wanted) {
    return 0;
  }
  buffer[wanted] = '\0';
  return 1;
}

int main(void) {
  FILE *fp;
  char buffer[256];
  int ok;

  fp = tmpfile();
  TEST_ASSERT(fp != NULL);
  ok = lizard_report_fprint_text_field(fp, "a\tb\nc\r\\d");
  TEST_ASSERT(ok);
  ok = read_tmpfile(fp, buffer, sizeof(buffer));
  TEST_ASSERT(ok);
  TEST_ASSERT(strcmp(buffer, "a\\tb\\nc\\r\\\\d") == 0);
  fclose(fp);

  fp = tmpfile();
  TEST_ASSERT(fp != NULL);
  ok = lizard_report_fprint_text_field(fp, NULL);
  TEST_ASSERT(ok);
  ok = read_tmpfile(fp, buffer, sizeof(buffer));
  TEST_ASSERT(ok);
  TEST_ASSERT(strcmp(buffer, "-") == 0);
  fclose(fp);

  fp = tmpfile();
  TEST_ASSERT(fp != NULL);
  ok = lizard_report_fprint_json_string(fp, "q\"\\\t\n\r\b\f\001z");
  TEST_ASSERT(ok);
  ok = read_tmpfile(fp, buffer, sizeof(buffer));
  TEST_ASSERT(ok);
  TEST_ASSERT(strcmp(buffer, "\"q\\\"\\\\\\t\\n\\r\\b\\f\\u0001z\"") == 0);
  fclose(fp);

  fp = tmpfile();
  TEST_ASSERT(fp != NULL);
  ok = lizard_report_fprint_json_string(fp, NULL);
  TEST_ASSERT(ok);
  ok = read_tmpfile(fp, buffer, sizeof(buffer));
  TEST_ASSERT(ok);
  TEST_ASSERT(strcmp(buffer, "\"\"") == 0);
  fclose(fp);

  TEST_RETURN();
}
