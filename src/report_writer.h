#ifndef LIZARD_REPORT_WRITER_H
#define LIZARD_REPORT_WRITER_H

#include <stdio.h>

/* Shared report-format escaping helpers.
 *
 * Text fields use the stable tab-separated report convention:
 *   NULL -> "-"
 *   backslash, tab, newline, carriage return are escaped.
 *
 * JSON fields use JSON string escaping:
 *   NULL -> ""
 *   quotes/backslashes/control bytes are escaped.
 */
int lizard_report_fprint_text_field(FILE *fp, const char *text);
int lizard_report_fprint_json_string(FILE *fp, const char *text);

#endif /* LIZARD_REPORT_WRITER_H */
