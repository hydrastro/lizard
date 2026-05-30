#include "syntax_expansion_report.h"

#include "diagnostic_report.h"
#include "diagnostics.h"
#include "env.h"
#include "mem.h"
#include "parser.h"
#include "printer.h"
#include "report_writer.h"
#include "runtime.h"
#include "tokenizer.h"
#include "surface_term.h"
#include "expansion_context.h"
#include "syntax_expander.h"
#include "expansion_trace_report.h"

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

static char *copy_string(const char *text) {
  char *copy;
  size_t len;

  if (text == NULL) {
    text = "";
  }
  len = strlen(text) + 1U;
  copy = (char *)malloc(len);
  if (copy == NULL) {
    return NULL;
  }
  memcpy(copy, text, len);
  return copy;
}

static lizard_syntax_expansion_report_t *report_new(void) {
  lizard_syntax_expansion_report_t *report;

  report = (lizard_syntax_expansion_report_t *)malloc(sizeof(*report));
  if (report == NULL) {
    return NULL;
  }
  report->status = LIZARD_STATUS_OK;
  lizard_diagnostic_clear(&report->diagnostic);
  report->form_count = 0UL;
  lizard_source_span_clear(&report->span);
  report->phase = 0;
  report->scope_summary = 0UL;
  report->expanded_ast_summary = NULL;
  report->trace_report = NULL;
  return report;
}

static int report_append(lizard_syntax_expansion_report_t *report,
                         const char *text) {
  char *combined;
  size_t old_len;
  size_t text_len;

  if (report == NULL || text == NULL) {
    return 0;
  }
  old_len = report->expanded_ast_summary != NULL
                ? strlen(report->expanded_ast_summary)
                : 0U;
  text_len = strlen(text);
  combined = (char *)malloc(old_len + text_len + 2U);
  if (combined == NULL) {
    return 0;
  }
  if (report->expanded_ast_summary != NULL) {
    memcpy(combined, report->expanded_ast_summary, old_len);
    combined[old_len] = '\n';
    memcpy(combined + old_len + 1U, text, text_len + 1U);
  } else {
    memcpy(combined, text, text_len + 1U);
  }
  free(report->expanded_ast_summary);
  report->expanded_ast_summary = combined;
  return 1;
}

static char *ast_to_string(lizard_ast_node_t *ast) {
  FILE *fp;
  long size;
  char *buffer;
  size_t got;

  fp = tmpfile();
  if (fp == NULL) {
    return copy_string("<unprintable-ast>");
  }
  lizard_fprint_ast(fp, ast, 0);
  if (fflush(fp) != 0) {
    fclose(fp);
    return copy_string("<unprintable-ast>");
  }
  size = ftell(fp);
  if (size < 0L || fseek(fp, 0L, SEEK_SET) != 0) {
    fclose(fp);
    return copy_string("<unprintable-ast>");
  }
  buffer = (char *)malloc((size_t)size + 1U);
  if (buffer == NULL) {
    fclose(fp);
    return NULL;
  }
  got = fread(buffer, 1U, (size_t)size, fp);
  fclose(fp);
  buffer[got] = '\0';
  return buffer;
}

static void set_report_diagnostic(lizard_syntax_expansion_report_t *report,
                                  const lizard_diagnostic_t *diagnostic,
                                  lizard_status_t fallback,
                                  lizard_diagnostic_category_t category,
                                  const char *message) {
  lizard_source_span_t span;

  if (report == NULL) {
    return;
  }
  if (diagnostic != NULL) {
    lizard_diagnostic_copy(&report->diagnostic, diagnostic);
    report->status = diagnostic->status;
    if (report->status == LIZARD_STATUS_OK) {
      report->status = fallback;
      lizard_diagnostic_set(&report->diagnostic, fallback, category,
                            &diagnostic->span, message);
    }
  } else {
    lizard_source_span_clear(&span);
    lizard_diagnostic_set(&report->diagnostic, fallback, category, &span,
                          message);
    report->status = fallback;
  }
}

lizard_syntax_expansion_report_t *lizard_context_syntax_expansion_report(
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
    set_report_diagnostic(report, NULL, LIZARD_STATUS_INVALID_ARGUMENT,
                          LIZARD_DIAGNOSTIC_CATEGORY_UNKNOWN,
                          "invalid syntax expansion request");
    return report;
  }
  lizard_runtime_make_current(context->runtime);
  lizard_diagnostic_clear(&diagnostic);
  surface_list = lizard_surface_parse_source(context->runtime->heap, source,
                                             filename, &diagnostic);
  if (surface_list == NULL) {
    if (diagnostic.status == LIZARD_STATUS_OK) {
      diagnostic.status = LIZARD_STATUS_PARSE_ERROR;
    }
    set_report_diagnostic(report, &diagnostic, LIZARD_STATUS_PARSE_ERROR,
                          LIZARD_DIAGNOSTIC_CATEGORY_PARSER, "parse failed");
    if (filename != NULL) {
      report->diagnostic.span.filename = filename;
    }
    return report;
  }
  expansion = lizard_expansion_context_new(context->runtime->heap, 0,
                                           "expand-only");
  if (expansion == NULL) {
    set_report_diagnostic(report, NULL, LIZARD_STATUS_OUT_OF_MEMORY,
                          LIZARD_DIAGNOSTIC_CATEGORY_UNKNOWN,
                          "unable to allocate expansion context");
    return report;
  }
  last_expanded_surface = NULL;
  for (iter = surface_list->head; iter != surface_list->nil;
       iter = iter->next) {
    lizard_surface_list_node_t *node;
    lizard_ast_node_t *expanded_ast;
    lizard_surface_term_t *expanded_surface;
    char *text;

    node = (lizard_surface_list_node_t *)iter;
    expanded_ast = NULL;
    expanded_surface = NULL;
    if (!lizard_syntax_expand_surface(expansion, context->env, node->term,
                                      &expanded_surface, &expanded_ast)) {
      set_report_diagnostic(report, NULL, LIZARD_STATUS_EVAL_ERROR,
                            LIZARD_DIAGNOSTIC_CATEGORY_MACRO,
                            "macro expansion failed");
      return report;
    }
    last_expanded_surface = expanded_surface;
    text = ast_to_string(expanded_ast);
    if (text == NULL || !report_append(report, text)) {
      free(text);
      set_report_diagnostic(report, NULL, LIZARD_STATUS_OUT_OF_MEMORY,
                            LIZARD_DIAGNOSTIC_CATEGORY_UNKNOWN,
                            "unable to format expanded AST");
      return report;
    }
    free(text);
    report->form_count++;
  }
  if (report->expanded_ast_summary == NULL) {
    report->expanded_ast_summary = copy_string("");
    if (report->expanded_ast_summary == NULL) {
      set_report_diagnostic(report, NULL, LIZARD_STATUS_OUT_OF_MEMORY,
                            LIZARD_DIAGNOSTIC_CATEGORY_UNKNOWN,
                            "unable to allocate expansion summary");
      return report;
    }
  }
  if (last_expanded_surface != NULL) {
    (void)lizard_surface_span(last_expanded_surface, &report->span);
    report->phase = lizard_surface_phase(last_expanded_surface);
    report->scope_summary = lizard_surface_scope(last_expanded_surface);
    report->trace_report =
        lizard_expansion_trace_report_from_surface(last_expanded_surface);
    if (report->trace_report == NULL) {
      set_report_diagnostic(report, NULL, LIZARD_STATUS_OUT_OF_MEMORY,
                            LIZARD_DIAGNOSTIC_CATEGORY_UNKNOWN,
                            "unable to snapshot expansion trace");
      return report;
    }
  }
  report->status = LIZARD_STATUS_OK;
  lizard_diagnostic_clear(&report->diagnostic);
  if (report->span.filename == NULL && filename != NULL) {
    report->span.filename = filename;
  }
  return report;
}

void lizard_syntax_expansion_report_destroy(
    lizard_syntax_expansion_report_t *report) {
  if (report == NULL) {
    return;
  }
  lizard_expansion_trace_report_destroy(report->trace_report);
  free(report->expanded_ast_summary);
  free(report);
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

lizard_diagnostic_report_t *lizard_syntax_expansion_report_diagnostic_report(
    const lizard_syntax_expansion_report_t *report) {
  return lizard_diagnostic_report_from_diagnostic(
      lizard_syntax_expansion_report_diagnostic(report));
}

int lizard_syntax_expansion_report_fprint(
    FILE *fp, const lizard_syntax_expansion_report_t *report) {
  if (fp == NULL || report == NULL) {
    return 0;
  }
  if (fprintf(fp, "lizard-syntax-expansion\tv=1\tstatus=%d\tforms=%lu\n",
              (int)report->status, report->form_count) < 0) {
    return 0;
  }
  if (fputs("expanded\t", fp) < 0) return 0;
  if (!lizard_report_fprint_text_field(
          fp, lizard_syntax_expansion_report_expanded_ast_summary(report))) {
    return 0;
  }
  if (fputc('\n', fp) == EOF) return 0;
  if (report->status != LIZARD_STATUS_OK || report->diagnostic.message[0] != '\0') {
    lizard_diagnostic_report_t *diagnostics;
    int ok;

    diagnostics = lizard_syntax_expansion_report_diagnostic_report(report);
    if (diagnostics == NULL) return 0;
    ok = lizard_diagnostic_report_fprint(fp, diagnostics);
    lizard_diagnostic_report_destroy(diagnostics);
    return ok;
  }
  return 1;
}

int lizard_syntax_expansion_report_fprint_json(
    FILE *fp, const lizard_syntax_expansion_report_t *report) {
  lizard_diagnostic_report_t *diagnostics;
  int ok;

  if (fp == NULL || report == NULL) {
    return 0;
  }
  if (fprintf(fp,
              "{\"type\":\"lizard-syntax-expansion\",\"version\":1,\"status\":%d,\"forms\":%lu,\"expanded\":",
              (int)report->status, report->form_count) < 0) {
    return 0;
  }
  if (!lizard_report_fprint_json_string(
          fp, lizard_syntax_expansion_report_expanded_ast_summary(report))) {
    return 0;
  }
  if (fputs(",\"trace\":", fp) < 0) return 0;
  if (report->trace_report != NULL) {
    if (!lizard_expansion_trace_report_fprint_json(fp, report->trace_report)) {
      return 0;
    }
  } else {
    if (fputs("null", fp) < 0) return 0;
  }
  if (fputs(",\"diagnostics\":", fp) < 0) return 0;
  diagnostics = lizard_syntax_expansion_report_diagnostic_report(report);
  if (diagnostics == NULL) return 0;
  ok = lizard_diagnostic_report_fprint_json(fp, diagnostics);
  lizard_diagnostic_report_destroy(diagnostics);
  if (!ok) return 0;
  return fputs("}\n", fp) >= 0;
}
