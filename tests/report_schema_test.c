#include "report_schema.h"
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
  if (pos < 0L || (unsigned long)pos >= (unsigned long)buffer_size) {
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
  char buffer[128];
  const char *name;
  int version;
  int valid;
  int ok;

  name = lizard_report_schema_type(LIZARD_REPORT_SCHEMA_EXPANSION_TRACE);
  TEST_ASSERT(strcmp(name, "lizard-expansion-trace") == 0);
  version = lizard_report_schema_version(LIZARD_REPORT_SCHEMA_EXPANSION_TRACE);
  TEST_ASSERT_EQ(version, 1);
  valid = lizard_report_schema_valid(LIZARD_REPORT_SCHEMA_EXPANSION_TRACE);
  TEST_ASSERT_EQ(valid, 1);

  name = lizard_report_schema_type(LIZARD_REPORT_SCHEMA_DIAGNOSTIC);
  TEST_ASSERT(strcmp(name, "lizard-diagnostic-report") == 0);
  version = lizard_report_schema_version(LIZARD_REPORT_SCHEMA_DIAGNOSTIC);
  TEST_ASSERT_EQ(version, 1);
  valid = lizard_report_schema_valid(LIZARD_REPORT_SCHEMA_DIAGNOSTIC);
  TEST_ASSERT_EQ(valid, 1);

  name = lizard_report_schema_type(LIZARD_REPORT_SCHEMA_SYNTAX_EXPANSION);
  TEST_ASSERT(strcmp(name, "lizard-syntax-expansion") == 0);
  version = lizard_report_schema_version(LIZARD_REPORT_SCHEMA_SYNTAX_EXPANSION);
  TEST_ASSERT_EQ(version, 1);
  valid = lizard_report_schema_valid(LIZARD_REPORT_SCHEMA_SYNTAX_EXPANSION);
  TEST_ASSERT_EQ(valid, 1);

  valid = lizard_report_schema_valid((lizard_report_schema_kind_t)99);
  TEST_ASSERT_EQ(valid, 0);
  version = lizard_report_schema_version((lizard_report_schema_kind_t)99);
  TEST_ASSERT_EQ(version, 0);
  name = lizard_report_schema_type((lizard_report_schema_kind_t)99);
  TEST_ASSERT(strcmp(name, "lizard-unknown-report") == 0);

  fp = tmpfile();
  TEST_ASSERT(fp != NULL);
  ok = lizard_report_schema_fprint_type_json(
      fp, LIZARD_REPORT_SCHEMA_SYNTAX_EXPANSION);
  TEST_ASSERT(ok);
  ok = read_tmpfile(fp, buffer, sizeof(buffer));
  TEST_ASSERT(ok);
  TEST_ASSERT(strcmp(buffer, "\"lizard-syntax-expansion\"") == 0);
  fclose(fp);

  TEST_RETURN();
}
