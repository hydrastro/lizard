#ifndef LIZARD_SYNTAX_EXPANSION_REPORT_H
#define LIZARD_SYNTAX_EXPANSION_REPORT_H

#include "lizard_api.h"

lizard_syntax_expansion_report_t *lizard_syntax_expansion_report_create(
    lizard_context_t *context, const char *source, const char *filename);

#endif /* LIZARD_SYNTAX_EXPANSION_REPORT_H */
