/* src/report_schema.c -- centralized stable report schema names/versions. */

#include "report_schema.h"
#include "report_writer.h"

const char *lizard_report_schema_type(lizard_report_schema_kind_t kind) {
  switch (kind) {
  case LIZARD_REPORT_SCHEMA_EXPANSION_TRACE:
    return "lizard-expansion-trace";
  case LIZARD_REPORT_SCHEMA_DIAGNOSTIC:
    return "lizard-diagnostic-report";
  case LIZARD_REPORT_SCHEMA_SYNTAX_EXPANSION:
    return "lizard-syntax-expansion";
  }
  return "lizard-unknown-report";
}

int lizard_report_schema_version(lizard_report_schema_kind_t kind) {
  switch (kind) {
  case LIZARD_REPORT_SCHEMA_EXPANSION_TRACE:
  case LIZARD_REPORT_SCHEMA_DIAGNOSTIC:
  case LIZARD_REPORT_SCHEMA_SYNTAX_EXPANSION:
    return 1;
  }
  return 0;
}

int lizard_report_schema_valid(lizard_report_schema_kind_t kind) {
  switch (kind) {
  case LIZARD_REPORT_SCHEMA_EXPANSION_TRACE:
  case LIZARD_REPORT_SCHEMA_DIAGNOSTIC:
  case LIZARD_REPORT_SCHEMA_SYNTAX_EXPANSION:
    return 1;
  }
  return 0;
}

int lizard_report_schema_fprint_type_json(FILE *fp,
                                          lizard_report_schema_kind_t kind) {
  return lizard_report_fprint_json_string(
      fp, lizard_report_schema_type(kind));
}
