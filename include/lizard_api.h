#ifndef LIZARD_API_H
#define LIZARD_API_H

/* Stable embeddable C API for Lizard.
 *
 * This header deliberately keeps the interpreter internals opaque.  Code
 * that embeds Lizard should include this file, not the implementation-facing
 * include/lizard.h header.  The current implementation still has a process-
 * global active heap underneath; the runtime/context API centralizes that
 * dependency so the internals can later move to fully reentrant state without
 * changing embedders.
 */

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lizard_runtime lizard_runtime_t;
typedef struct lizard_context lizard_context_t;
typedef struct lizard_expansion_trace_report lizard_expansion_trace_report_t;
typedef struct lizard_diagnostic_report lizard_diagnostic_report_t;
typedef struct lizard_syntax_expansion_report lizard_syntax_expansion_report_t;
typedef struct lizard_ast_node lizard_value_t;

typedef enum {
  LIZARD_STATUS_OK = 0,
  LIZARD_STATUS_ERROR = 1,
  LIZARD_STATUS_INVALID_ARGUMENT = 2,
  LIZARD_STATUS_IO_ERROR = 3,
  LIZARD_STATUS_OUT_OF_MEMORY = 4,
  LIZARD_STATUS_PARSE_ERROR = 5,
  LIZARD_STATUS_EVAL_ERROR = 6,
  LIZARD_STATUS_TYPE_ERROR = 7
} lizard_status_t;

typedef struct lizard_source_span {
  const char *filename;
  int start_line;
  int start_column;
  int end_line;
  int end_column;
  int start_offset;
  int end_offset;
} lizard_source_span_t;

typedef struct lizard_diagnostic {
  lizard_status_t status;
  lizard_source_span_t span;
  char message[256];
} lizard_diagnostic_t;


typedef struct lizard_expansion_trace_event {
  const char *stage;
  const char *detail;
  const char *origin_filename;
  int origin_line;
  int origin_column;
  int origin_phase;
  unsigned long origin_scope_summary;
} lizard_expansion_trace_event_t;


typedef struct lizard_report_schema_info {
  const char *type;
  int version;
  int supports_text;
  int supports_json;
  int stable_v1;
} lizard_report_schema_info_t;

typedef struct lizard_diagnostic_event {
  lizard_status_t status;
  const char *message;
  const char *filename;
  int start_line;
  int start_column;
  int end_line;
  int end_column;
  int start_offset;
  int end_offset;
} lizard_diagnostic_event_t;


/* Phase 2S: report schema/capability discovery for tooling. */
unsigned long lizard_report_schema_count(void);
int lizard_report_schema_get(unsigned long index,
                             lizard_report_schema_info_t *out_info);
const char *lizard_report_schema_list_type(void);
int lizard_report_schema_list_version(void);
int lizard_report_schema_list_fprint(FILE *fp);
int lizard_report_schema_list_fprint_json(FILE *fp);

typedef enum {
  LIZARD_VALUE_STRING,
  LIZARD_VALUE_NUMBER,
  LIZARD_VALUE_SYMBOL,
  LIZARD_VALUE_BOOL,
  LIZARD_VALUE_PAIR,
  LIZARD_VALUE_NIL,
  LIZARD_VALUE_VECTOR,
  LIZARD_VALUE_HASH,
  LIZARD_VALUE_PROCEDURE,
  LIZARD_VALUE_MACRO,
  LIZARD_VALUE_PROMISE,
  LIZARD_VALUE_CONTINUATION,
  LIZARD_VALUE_ERROR,
  LIZARD_VALUE_SYNTAX,
  LIZARD_VALUE_TYPE_THEORY,
  LIZARD_VALUE_INTERNAL
} lizard_value_type_t;

typedef struct lizard_runtime_options {
  size_t initial_heap_size;
  size_t max_segment_size;
} lizard_runtime_options_t;

void lizard_runtime_options_default(lizard_runtime_options_t *options);

lizard_runtime_t *lizard_runtime_create(const lizard_runtime_options_t *options);
void lizard_runtime_destroy(lizard_runtime_t *runtime);
void lizard_runtime_make_current(lizard_runtime_t *runtime);

lizard_context_t *lizard_context_create(lizard_runtime_t *runtime);
void lizard_context_destroy(lizard_context_t *context);
lizard_status_t lizard_context_eval_string(lizard_context_t *context,
                                           const char *source,
                                           lizard_value_t **out_value);
lizard_status_t lizard_context_eval_file(lizard_context_t *context,
                                         const char *path,
                                         lizard_value_t **out_value);
lizard_value_t *lizard_context_last_value(lizard_context_t *context);
const char *lizard_context_last_error(lizard_context_t *context);
const lizard_diagnostic_t *lizard_context_last_diagnostic(
    lizard_context_t *context);

/* Phase 2I: optional traced macro-expansion path.  Disabled by default; when
 * enabled, context evaluation still returns the same runtime values but records
 * untrusted SurfaceTerm expansion metadata for tooling. */
void lizard_context_set_trace_expansion(lizard_context_t *context,
                                        int enabled);
int lizard_context_trace_expansion_enabled(lizard_context_t *context);
unsigned long lizard_context_last_expansion_trace_count(
    lizard_context_t *context);
const char *lizard_context_last_expansion_stage(lizard_context_t *context);
const char *lizard_context_last_expansion_detail(lizard_context_t *context);
int lizard_context_last_expansion_span(lizard_context_t *context,
                                       lizard_source_span_t *out_span);

/* Phase 2J: full expansion trace reporting.  Index 0 is the latest event,
 * matching lizard_context_last_expansion_* accessors. */
unsigned long lizard_context_expansion_trace_count(lizard_context_t *context);
int lizard_context_expansion_trace_event(lizard_context_t *context,
                                         unsigned long index,
                                         lizard_expansion_trace_event_t *out_event);
int lizard_context_expansion_trace_event_string(lizard_context_t *context,
                                                unsigned long index,
                                                char *buffer,
                                                size_t buffer_size);

/* Phase 2K: owned expansion trace reports.  A report snapshots trace events
 * from the latest traced expansion and remains valid until explicitly
 * destroyed, even if the context later evaluates other code or is destroyed. */
lizard_expansion_trace_report_t *lizard_context_expansion_trace_report(
    lizard_context_t *context);
void lizard_expansion_trace_report_destroy(
    lizard_expansion_trace_report_t *report);
unsigned long lizard_expansion_trace_report_count(
    const lizard_expansion_trace_report_t *report);
int lizard_expansion_trace_report_event(
    const lizard_expansion_trace_report_t *report,
    unsigned long index,
    lizard_expansion_trace_event_t *out_event);
int lizard_expansion_trace_report_event_string(
    const lizard_expansion_trace_report_t *report,
    unsigned long index,
    char *buffer,
    size_t buffer_size);
int lizard_expansion_trace_report_fprint(
    FILE *fp,
    const lizard_expansion_trace_report_t *report);
int lizard_expansion_trace_report_fprint_json(
    FILE *fp,
    const lizard_expansion_trace_report_t *report);

/* Phase 2P: owned diagnostic reports.  These snapshot diagnostics so tooling
 * can inspect parser/tokenizer/expansion failures independently of context
 * lifetime or later evaluations. */
lizard_diagnostic_report_t *lizard_context_diagnostic_report(
    lizard_context_t *context);
lizard_diagnostic_report_t *lizard_syntax_expansion_report_diagnostic_report(
    const lizard_syntax_expansion_report_t *report);
void lizard_diagnostic_report_destroy(lizard_diagnostic_report_t *report);
unsigned long lizard_diagnostic_report_count(
    const lizard_diagnostic_report_t *report);
int lizard_diagnostic_report_event(
    const lizard_diagnostic_report_t *report,
    unsigned long index,
    lizard_diagnostic_event_t *out_event);
int lizard_diagnostic_report_event_string(
    const lizard_diagnostic_report_t *report,
    unsigned long index,
    char *buffer,
    size_t buffer_size);
int lizard_diagnostic_report_fprint(
    FILE *fp,
    const lizard_diagnostic_report_t *report);
int lizard_diagnostic_report_fprint_json(
    FILE *fp,
    const lizard_diagnostic_report_t *report);

/* Phase 2N: owned syntax expansion reports. These parse and macro-expand
 * source without evaluating it, so tooling can inspect expansion output and
 * trace data independently of the evaluator. */
lizard_syntax_expansion_report_t *lizard_context_syntax_expansion_report(
    lizard_context_t *context, const char *source, const char *filename);
void lizard_syntax_expansion_report_destroy(
    lizard_syntax_expansion_report_t *report);
lizard_status_t lizard_syntax_expansion_report_status(
    const lizard_syntax_expansion_report_t *report);
const lizard_diagnostic_t *lizard_syntax_expansion_report_diagnostic(
    const lizard_syntax_expansion_report_t *report);
unsigned long lizard_syntax_expansion_report_form_count(
    const lizard_syntax_expansion_report_t *report);
const char *lizard_syntax_expansion_report_expanded_ast_summary(
    const lizard_syntax_expansion_report_t *report);
int lizard_syntax_expansion_report_span(
    const lizard_syntax_expansion_report_t *report,
    lizard_source_span_t *out_span);
int lizard_syntax_expansion_report_phase(
    const lizard_syntax_expansion_report_t *report);
unsigned long lizard_syntax_expansion_report_scope_summary(
    const lizard_syntax_expansion_report_t *report);
unsigned long lizard_syntax_expansion_report_trace_count(
    const lizard_syntax_expansion_report_t *report);
int lizard_syntax_expansion_report_trace_event(
    const lizard_syntax_expansion_report_t *report, unsigned long index,
    lizard_expansion_trace_event_t *out_event);
int lizard_syntax_expansion_report_fprint(
    FILE *fp,
    const lizard_syntax_expansion_report_t *report);
int lizard_syntax_expansion_report_fprint_json(
    FILE *fp,
    const lizard_syntax_expansion_report_t *report);

lizard_value_type_t lizard_value_type(const lizard_value_t *value);
const char *lizard_value_type_name(lizard_value_type_t type);
int lizard_value_is_error(const lizard_value_t *value);
int lizard_value_error_code(const lizard_value_t *value);
int lizard_value_source_span(const lizard_value_t *value,
                             lizard_source_span_t *out_span);
void lizard_value_fprint(FILE *fp, lizard_value_t *value);
void lizard_value_print(lizard_value_t *value);

#ifdef __cplusplus
}
#endif

#endif /* LIZARD_API_H */
