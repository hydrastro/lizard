/* src/syntax_expansion_report.c -- owned syntax expansion reports.
 *
 * Phase 2N scaffold: produce an inspectable expansion report without
 * evaluating the expanded code.  Reports own all strings they expose and are
 * safe to inspect after later evaluations or context destruction.
 */

#include "syntax_expansion_report.h"
#include "report_writer.h"
#include "report_schema.h"

#include "diagnostic_report.h"
#include "env.h"
#include "expansion_context.h"
#include "expansion_trace_report.h"
#include "mem.h"
#include "printer.h"
#include "runtime.h"
#include "surface_term.h"
#include "syntax_expander.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct lizard_syntax_expansion_report {
  lizard_status_t status;
  lizard_diagnostic_t diagnostic;
  unsigned long form_count;
  lizard_source_span_t span;
  int phase;
  unsigned long scope_summary;
  char *expanded_ast_summary;
  lizard_expansion_trace_report_t *trace_report;
};

static void report_clear_span(lizard_source_span_t *span) {
  if (span != NULL) {
    span->filename = NULL;
    span->start_line = 0;
    span->start_column = 0;
    span->end_line = 0;
    span->end_column = 0;
    span->start_offset = 0;
    span->end_offset = 0;
  }
}

static void report_clear_diagnostic(lizard_diagnostic_t *diagnostic) {
  if (diagnostic != NULL) {
    diagnostic->status = LIZARD_STATUS_OK;
    report_clear_span(&diagnostic->span);
    diagnostic->message[0] = '\0';
  }
}

static char *report_copy_string(const char *text) {
  char *copy;
  size_t len;

  if (text == NULL) {
    return NULL;
  }
  len = strlen(text) + 1U;
  copy = (char *)malloc(len);
  if (copy == NULL) {
    return NULL;
  }
  memcpy(copy, text, len);
  return copy;
}

static int report_assign_summary(lizard_syntax_expansion_report_t *report,
                                 const char *text) {
  char *copy;

  if (report == NULL) {
    return 0;
  }
  copy = report_copy_string(text != NULL ? text : "");
  if (copy == NULL) {
    return 0;
  }
  free(report->expanded_ast_summary);
  report->expanded_ast_summary = copy;
  return 1;
}

static char *report_ast_to_string(lizard_ast_node_t *ast) {
  FILE *fp;
  long size;
  size_t read_count;
  char *buffer;

  fp = tmpfile();
  if (fp == NULL) {
    return report_copy_string("<unable-to-open-tempfile>");
  }
  lizard_fprint_ast(fp, ast, 0);
  if (fflush(fp) != 0) {
    fclose(fp);
    return report_copy_string("<unable-to-flush-tempfile>");
  }
  size = ftell(fp);
  if (size < 0L) {
    fclose(fp);
    return report_copy_string("<unable-to-measure-ast>");
  }
  if (fseek(fp, 0L, SEEK_SET) != 0) {
    fclose(fp);
    return report_copy_string("<unable-to-rewind-ast>");
  }
  buffer = (char *)malloc((size_t)size + 1U);
  if (buffer == NULL) {
    fclose(fp);
    return NULL;
  }
  read_count = fread(buffer, 1U, (size_t)size, fp);
  fclose(fp);
  buffer[read_count] = '\0';
  return buffer;
}

static int report_append_owned(char **dst, const char *suffix) {
  size_t old_len;
  size_t suffix_len;
  char *combined;

  if (dst == NULL || suffix == NULL) {
    return 0;
  }
  old_len = *dst != NULL ? strlen(*dst) : 0U;
  suffix_len = strlen(suffix);
  combined = (char *)malloc(old_len + suffix_len + 1U);
  if (combined == NULL) {
    return 0;
  }
  if (*dst != NULL) {
    memcpy(combined, *dst, old_len);
  }
  memcpy(combined + old_len, suffix, suffix_len + 1U);
  free(*dst);
  *dst = combined;
  return 1;
}

static int report_append_ast(lizard_syntax_expansion_report_t *report,
                             lizard_ast_node_t *ast) {
  char *ast_text;
  int ok;

  if (report == NULL) {
    return 0;
  }
  ast_text = report_ast_to_string(ast);
  if (ast_text == NULL) {
    return 0;
  }
  ok = 1;
  if (report->expanded_ast_summary == NULL) {
    ok = report_assign_summary(report, ast_text);
  } else {
    ok = report_append_owned(&report->expanded_ast_summary, "\n") &&
         report_append_owned(&report->expanded_ast_summary, ast_text);
  }
  free(ast_text);
  return ok;
}

static void report_fill_from_surface(lizard_syntax_expansion_report_t *report,
                                     const lizard_surface_term_t *surface) {
  if (report == NULL || surface == NULL) {
    return;
  }
  (void)lizard_surface_span(surface, &report->span);
  report->phase = lizard_surface_phase(surface);
  report->scope_summary = lizard_surface_scope(surface);
}

static lizard_syntax_expansion_report_t *report_new(void) {
  lizard_syntax_expansion_report_t *report;

  report = (lizard_syntax_expansion_report_t *)malloc(sizeof(*report));
  if (report == NULL) {
    return NULL;
  }
  report->status = LIZARD_STATUS_OK;
  report_clear_diagnostic(&report->diagnostic);
  report->form_count = 0UL;
  report_clear_span(&report->span);
  report->phase = 0;
  report->scope_summary = 0UL;
  report->expanded_ast_summary = NULL;
  report->trace_report = NULL;
  return report;
}

lizard_syntax_expansion_report_t *lizard_syntax_expansion_report_create(
    lizard_context_t *context, const char *source, const char *filename) {
  lizard_syntax_expansion_report_t *report;
  lizard_expansion_context_t *expansion;
  lz_list_t *surface_list;
  lz_list_node_t *iter;
  lizard_diagnostic_t diagnostic;
  lizard_surface_term_t *last_expanded_surface;

  report = report_new();
  if (report == NULL) {
    return NULL;
  }
  if (context == NULL || context->runtime == NULL || source == NULL) {
    report->status = LIZARD_STATUS_INVALID_ARGUMENT;
    report->diagnostic.status = LIZARD_STATUS_INVALID_ARGUMENT;
    strncpy(report->diagnostic.message, "invalid expansion report request",
            sizeof(report->diagnostic.message) - 1U);
    report->diagnostic.message[sizeof(report->diagnostic.message) - 1U] = '\0';
    return report;
  }

  lizard_runtime_make_current(context->runtime);
  report_clear_diagnostic(&diagnostic);
  surface_list = lizard_surface_parse_source(context->runtime->heap, source,
                                             filename, &diagnostic);
  if (surface_list == NULL) {
    report->status = diagnostic.status != LIZARD_STATUS_OK
                         ? diagnostic.status
                         : LIZARD_STATUS_PARSE_ERROR;
    report->diagnostic = diagnostic;
    return report;
  }

  expansion = lizard_expansion_context_new(context->runtime->heap, 0,
                                           "expand-only");
  if (expansion == NULL) {
    report->status = LIZARD_STATUS_OUT_OF_MEMORY;
    report->diagnostic.status = LIZARD_STATUS_OUT_OF_MEMORY;
    strncpy(report->diagnostic.message, "unable to allocate expansion context",
            sizeof(report->diagnostic.message) - 1U);
    report->diagnostic.message[sizeof(report->diagnostic.message) - 1U] = '\0';
    return report;
  }

  last_expanded_surface = NULL;
  for (iter = surface_list->head; iter != surface_list->nil; iter = iter->next) {
    lizard_surface_list_node_t *node;
    lizard_ast_node_t *expanded_ast;
    lizard_surface_term_t *expanded_surface;

    node = (lizard_surface_list_node_t *)iter;
    expanded_ast = NULL;
    expanded_surface = NULL;
    if (!lizard_syntax_expand_surface(expansion, context->env, node->term,
                                      &expanded_surface, &expanded_ast)) {
      report->status = LIZARD_STATUS_EVAL_ERROR;
      report->diagnostic.status = LIZARD_STATUS_EVAL_ERROR;
      strncpy(report->diagnostic.message, "macro expansion failed",
              sizeof(report->diagnostic.message) - 1U);
      report->diagnostic.message[sizeof(report->diagnostic.message) - 1U] = '\0';
      return report;
    }
    report->form_count++;
    last_expanded_surface = expanded_surface;
    if (!report_append_ast(report, expanded_ast)) {
      report->status = LIZARD_STATUS_OUT_OF_MEMORY;
      report->diagnostic.status = LIZARD_STATUS_OUT_OF_MEMORY;
      strncpy(report->diagnostic.message, "unable to format expanded AST",
              sizeof(report->diagnostic.message) - 1U);
      report->diagnostic.message[sizeof(report->diagnostic.message) - 1U] = '\0';
      return report;
    }
  }

  if (report->expanded_ast_summary == NULL) {
    if (!report_assign_summary(report, "")) {
      report->status = LIZARD_STATUS_OUT_OF_MEMORY;
      report->diagnostic.status = LIZARD_STATUS_OUT_OF_MEMORY;
      return report;
    }
  }
  if (last_expanded_surface != NULL) {
    report_fill_from_surface(report, last_expanded_surface);
    report->trace_report = lizard_expansion_trace_report_from_surface(
        last_expanded_surface);
    if (report->trace_report == NULL) {
      report->status = LIZARD_STATUS_OUT_OF_MEMORY;
      report->diagnostic.status = LIZARD_STATUS_OUT_OF_MEMORY;
      strncpy(report->diagnostic.message, "unable to snapshot expansion trace",
              sizeof(report->diagnostic.message) - 1U);
      report->diagnostic.message[sizeof(report->diagnostic.message) - 1U] = '\0';
      return report;
    }
  }
  report->status = LIZARD_STATUS_OK;
  report_clear_diagnostic(&report->diagnostic);
  return report;
}

void lizard_syntax_expansion_report_destroy(
    lizard_syntax_expansion_report_t *report) {
  if (report == NULL) {
    return;
  }
  free(report->expanded_ast_summary);
  lizard_expansion_trace_report_destroy(report->trace_report);
  free(report);
}

lizard_status_t lizard_syntax_expansion_report_status(
    const lizard_syntax_expansion_report_t *report) {
  return report != NULL ? report->status : LIZARD_STATUS_INVALID_ARGUMENT;
}

const lizard_diagnostic_t *lizard_syntax_expansion_report_diagnostic(
    const lizard_syntax_expansion_report_t *report) {
  return report != NULL ? &report->diagnostic : NULL;
}

unsigned long lizard_syntax_expansion_report_form_count(
    const lizard_syntax_expansion_report_t *report) {
  return report != NULL ? report->form_count : 0UL;
}

const char *lizard_syntax_expansion_report_expanded_ast_summary(
    const lizard_syntax_expansion_report_t *report) {
  return report != NULL && report->expanded_ast_summary != NULL
             ? report->expanded_ast_summary
             : "";
}

int lizard_syntax_expansion_report_span(
    const lizard_syntax_expansion_report_t *report,
    lizard_source_span_t *out_span) {
  if (report == NULL || out_span == NULL) {
    return 0;
  }
  *out_span = report->span;
  return 1;
}

int lizard_syntax_expansion_report_phase(
    const lizard_syntax_expansion_report_t *report) {
  return report != NULL ? report->phase : 0;
}

unsigned long lizard_syntax_expansion_report_scope_summary(
    const lizard_syntax_expansion_report_t *report) {
  return report != NULL ? report->scope_summary : 0UL;
}

unsigned long lizard_syntax_expansion_report_trace_count(
    const lizard_syntax_expansion_report_t *report) {
  return report != NULL && report->trace_report != NULL
             ? lizard_expansion_trace_report_count(report->trace_report)
             : 0UL;
}

int lizard_syntax_expansion_report_trace_event(
    const lizard_syntax_expansion_report_t *report, unsigned long index,
    lizard_expansion_trace_event_t *out_event) {
  if (report == NULL || report->trace_report == NULL) {
    if (out_event != NULL) {
      out_event->stage = NULL;
      out_event->detail = NULL;
      out_event->origin_filename = NULL;
      out_event->origin_line = 0;
      out_event->origin_column = 0;
      out_event->origin_phase = 0;
      out_event->origin_scope_summary = 0UL;
    }
    return 0;
  }
  return lizard_expansion_trace_report_event(report->trace_report, index,
                                             out_event);
}

int lizard_syntax_expansion_report_fprint(
    FILE *fp, const lizard_syntax_expansion_report_t *report) {
  const char *filename;

  if (fp == NULL || report == NULL) {
    return 0;
  }
  filename = report->span.filename != NULL ? report->span.filename : "-";
  if (fprintf(fp, "%s\tv=%d\tstatus=%d\tforms=%lu\torigin=",
              lizard_report_schema_type(LIZARD_REPORT_SCHEMA_SYNTAX_EXPANSION),
              lizard_report_schema_version(LIZARD_REPORT_SCHEMA_SYNTAX_EXPANSION),
              (int)report->status, report->form_count) < 0) {
    return 0;
  }
  if (!lizard_report_fprint_text_field(fp, filename)) {
    return 0;
  }
  if (fprintf(fp, "\tline=%d\tcolumn=%d\tphase=%d\tscope=%lu\nexpanded\t",
              report->span.start_line, report->span.start_column,
              report->phase, report->scope_summary) < 0) {
    return 0;
  }
  if (!lizard_report_fprint_text_field(fp,
                                     lizard_syntax_expansion_report_expanded_ast_summary(report))) {
    return 0;
  }
  if (fputc('\n', fp) == EOF) {
    return 0;
  }
  if (report->diagnostic.status != LIZARD_STATUS_OK ||
      report->diagnostic.message[0] != '\0') {
    lizard_diagnostic_report_t *diagnostic_report;
    int ok;

    diagnostic_report = lizard_diagnostic_report_from_diagnostic(
        &report->diagnostic);
    if (diagnostic_report == NULL) {
      return 0;
    }
    ok = lizard_diagnostic_report_fprint(fp, diagnostic_report);
    lizard_diagnostic_report_destroy(diagnostic_report);
    if (!ok) {
      return 0;
    }
  }
  if (report->trace_report != NULL) {
    return lizard_expansion_trace_report_fprint(fp, report->trace_report);
  }
  return 1;
}


int lizard_syntax_expansion_report_fprint_json(
    FILE *fp, const lizard_syntax_expansion_report_t *report) {
  const char *filename;
  const char *summary;

  if (fp == NULL || report == NULL) {
    return 0;
  }
  filename = report->span.filename != NULL ? report->span.filename : "";
  summary = lizard_syntax_expansion_report_expanded_ast_summary(report);
  if (fputs("{\"type\":", fp) < 0) {
    return 0;
  }
  if (!lizard_report_schema_fprint_type_json(
          fp, LIZARD_REPORT_SCHEMA_SYNTAX_EXPANSION)) {
    return 0;
  }
  if (fprintf(fp, ",\"version\":%d,\"status\":%d,\"forms\":%lu,\"origin\":{\"filename\":",
              lizard_report_schema_version(LIZARD_REPORT_SCHEMA_SYNTAX_EXPANSION),
              (int)report->status, report->form_count) < 0) {
    return 0;
  }
  if (!lizard_report_fprint_json_string(fp, filename)) return 0;
  if (fprintf(fp,
              ",\"line\":%d,\"column\":%d,\"phase\":%d,\"scope\":%lu},\"expanded\":",
              report->span.start_line, report->span.start_column,
              report->phase, report->scope_summary) < 0) {
    return 0;
  }
  if (!lizard_report_fprint_json_string(fp, summary)) return 0;
  if (fputs(",\"diagnostics\":", fp) < 0) return 0;
  {
    lizard_diagnostic_report_t *diagnostic_report;
    int ok;

    diagnostic_report = lizard_diagnostic_report_from_diagnostic(
        &report->diagnostic);
    if (diagnostic_report == NULL) {
      return 0;
    }
    ok = lizard_diagnostic_report_fprint_json(fp, diagnostic_report);
    lizard_diagnostic_report_destroy(diagnostic_report);
    if (!ok) {
      return 0;
    }
  }
  if (fputs(",\"trace\":", fp) < 0) return 0;
  if (report->trace_report != NULL) {
    if (!lizard_expansion_trace_report_fprint_json(fp, report->trace_report)) {
      return 0;
    }
  } else {
    if (fputs("{\"type\":", fp) < 0) {
      return 0;
    }
    if (!lizard_report_schema_fprint_type_json(
            fp, LIZARD_REPORT_SCHEMA_EXPANSION_TRACE)) {
      return 0;
    }
    if (fprintf(fp, ",\"version\":%d,\"count\":0,\"events\":[]}\n",
                lizard_report_schema_version(LIZARD_REPORT_SCHEMA_EXPANSION_TRACE)) < 0) {
      return 0;
    }
  }
  return fputs("}\n", fp) >= 0;
}

lizard_syntax_expansion_report_t *lizard_context_syntax_expansion_report(
    lizard_context_t *context, const char *source, const char *filename) {
  return lizard_syntax_expansion_report_create(context, source, filename);
}
