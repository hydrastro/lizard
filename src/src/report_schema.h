#ifndef LIZARD_REPORT_SCHEMA_H
#define LIZARD_REPORT_SCHEMA_H

#include "lizard_api.h"

#include <stdio.h>

/* Stable report schema registry.
 *
 * Report writers must obtain type names and version numbers from this
 * registry instead of hardcoding strings.  This keeps text and JSON tooling
 * formats versioned in one place.
 */
typedef enum lizard_report_schema_kind {
  LIZARD_REPORT_SCHEMA_EXPANSION_TRACE = 0,
  LIZARD_REPORT_SCHEMA_DIAGNOSTIC = 1,
  LIZARD_REPORT_SCHEMA_SYNTAX_EXPANSION = 2
} lizard_report_schema_kind_t;

const char *lizard_report_schema_type(lizard_report_schema_kind_t kind);
int lizard_report_schema_version(lizard_report_schema_kind_t kind);
int lizard_report_schema_valid(lizard_report_schema_kind_t kind);
int lizard_report_schema_fprint_type_json(FILE *fp,
                                          lizard_report_schema_kind_t kind);

int lizard_report_schema_supports_text(lizard_report_schema_kind_t kind);
int lizard_report_schema_supports_json(lizard_report_schema_kind_t kind);
int lizard_report_schema_stable_v1(lizard_report_schema_kind_t kind);
int lizard_report_schema_info_from_kind(lizard_report_schema_kind_t kind,
                                        lizard_report_schema_info_t *out_info);
int lizard_report_schema_kind_at(unsigned long index,
                                 lizard_report_schema_kind_t *out_kind);
int lizard_report_schema_format_supported(const lizard_report_schema_info_t *info,
                                          const char *format);

#endif /* LIZARD_REPORT_SCHEMA_H */
