#ifndef LIZARD_REPORT_WRITER_H
#define LIZARD_REPORT_WRITER_H

#include <stdio.h>

int lizard_report_fprint_text_field(FILE *fp, const char *text);
int lizard_report_fprint_json_string(FILE *fp, const char *text);

#endif /* LIZARD_REPORT_WRITER_H */
