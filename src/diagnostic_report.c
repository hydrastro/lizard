/* src/diagnostic_report.c -- owned diagnostic report snapshots.
 *
 * Phase 2P: expose parser/tokenizer/expansion diagnostics through the same
 * tooling-friendly text/JSON report style as expansion traces.  Reports own
 * copied strings and do not borrow from lizard_context_t.
 */

#include "diagnostic_report.h"
#include "report_writer.h"
#include "report_schema.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct lizard_owned_diagnostic_event {
  lizard_status_t status;
  char *message;
  char *filename;
  int start_line;
  int start_column;
  int end_line;
  int end_column;
  int start_offset;
  int end_offset;
} lizard_owned_diagnostic_event_t;

struct lizard_diagnostic_report {
  unsigned long count;
  lizard_owned_diagnostic_event_t *events;
};

static char *diagnostic_report_copy_string(const char *text) {
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

static void diagnostic_report_clear_public(
    lizard_diagnostic_event_t *event) {
  if (event != NULL) {
    event->status = LIZARD_STATUS_OK;
    event->message = NULL;
    event->filename = NULL;
    event->start_line = 0;
    event->start_column = 0;
    event->end_line = 0;
    event->end_column = 0;
    event->start_offset = 0;
    event->end_offset = 0;
  }
}

static void diagnostic_report_clear_owned(
    lizard_owned_diagnostic_event_t *event) {
  if (event != NULL) {
    event->status = LIZARD_STATUS_OK;
    event->message = NULL;
    event->filename = NULL;
    event->start_line = 0;
    event->start_column = 0;
    event->end_line = 0;
    event->end_column = 0;
    event->start_offset = 0;
    event->end_offset = 0;
  }
}

static void diagnostic_report_free_owned(
    lizard_owned_diagnostic_event_t *event) {
  if (event != NULL) {
    free(event->message);
    free(event->filename);
    diagnostic_report_clear_owned(event);
  }
}

static int diagnostic_report_diagnostic_has_event(
    const lizard_diagnostic_t *diagnostic) {
  if (diagnostic == NULL) {
    return 0;
  }
  if (diagnostic->status != LIZARD_STATUS_OK) {
    return 1;
  }
  if (diagnostic->message[0] != '\0') {
    return 1;
  }
  if (diagnostic->span.filename != NULL) {
    return 1;
  }
  if (diagnostic->span.start_line != 0 || diagnostic->span.start_column != 0) {
    return 1;
  }
  return 0;
}

static int diagnostic_report_copy_from_diagnostic(
    lizard_owned_diagnostic_event_t *dst,
    const lizard_diagnostic_t *diagnostic) {
  diagnostic_report_clear_owned(dst);
  if (dst == NULL || diagnostic == NULL) {
    return 0;
  }
  dst->status = diagnostic->status;
  dst->message = diagnostic_report_copy_string(diagnostic->message);
  if (diagnostic->message[0] != '\0' && dst->message == NULL) {
    return 0;
  }
  dst->filename = diagnostic_report_copy_string(diagnostic->span.filename);
  if (diagnostic->span.filename != NULL && dst->filename == NULL) {
    diagnostic_report_free_owned(dst);
    return 0;
  }
  dst->start_line = diagnostic->span.start_line;
  dst->start_column = diagnostic->span.start_column;
  dst->end_line = diagnostic->span.end_line;
  dst->end_column = diagnostic->span.end_column;
  dst->start_offset = diagnostic->span.start_offset;
  dst->end_offset = diagnostic->span.end_offset;
  return 1;
}

lizard_diagnostic_report_t *lizard_diagnostic_report_from_diagnostic(
    const lizard_diagnostic_t *diagnostic) {
  lizard_diagnostic_report_t *report;

  report = (lizard_diagnostic_report_t *)malloc(sizeof(*report));
  if (report == NULL) {
    return NULL;
  }
  report->count = 0UL;
  report->events = NULL;

  if (!diagnostic_report_diagnostic_has_event(diagnostic)) {
    return report;
  }

  report->events = (lizard_owned_diagnostic_event_t *)calloc(
      1U, sizeof(report->events[0]));
  if (report->events == NULL) {
    free(report);
    return NULL;
  }
  if (!diagnostic_report_copy_from_diagnostic(&report->events[0], diagnostic)) {
    lizard_diagnostic_report_destroy(report);
    return NULL;
  }
  report->count = 1UL;
  return report;
}

void lizard_diagnostic_report_destroy(lizard_diagnostic_report_t *report) {
  unsigned long i;

  if (report == NULL) {
    return;
  }
  if (report->events != NULL) {
    for (i = 0UL; i < report->count; i++) {
      diagnostic_report_free_owned(&report->events[i]);
    }
    free(report->events);
  }
  free(report);
}

unsigned long lizard_diagnostic_report_count(
    const lizard_diagnostic_report_t *report) {
  return report != NULL ? report->count : 0UL;
}

int lizard_diagnostic_report_event(
    const lizard_diagnostic_report_t *report, unsigned long index,
    lizard_diagnostic_event_t *out_event) {
  const lizard_owned_diagnostic_event_t *src;

  diagnostic_report_clear_public(out_event);
  if (report == NULL || out_event == NULL || index >= report->count) {
    return 0;
  }
  src = &report->events[index];
  out_event->status = src->status;
  out_event->message = src->message;
  out_event->filename = src->filename;
  out_event->start_line = src->start_line;
  out_event->start_column = src->start_column;
  out_event->end_line = src->end_line;
  out_event->end_column = src->end_column;
  out_event->start_offset = src->start_offset;
  out_event->end_offset = src->end_offset;
  return 1;
}

static int diagnostic_report_format_event(
    const lizard_owned_diagnostic_event_t *event, char *buffer,
    size_t buffer_size) {
  char tmp[512];
  const char *message;
  const char *filename;
  size_t i;

  if (buffer == NULL || buffer_size == 0U) {
    return 0;
  }
  if (event == NULL) {
    sprintf(tmp, "#<diagnostic-event null>");
  } else {
    message = event->message != NULL ? event->message : "";
    filename = event->filename != NULL ? event->filename : "<unknown>";
    sprintf(tmp,
            "#<diagnostic-event status=%d origin=%s:%d:%d message=\"%s\">",
            (int)event->status, filename, event->start_line,
            event->start_column, message);
  }
  i = 0U;
  while (i + 1U < buffer_size && tmp[i] != '\0') {
    buffer[i] = tmp[i];
    i++;
  }
  buffer[i] = '\0';
  return event != NULL;
}

int lizard_diagnostic_report_event_string(
    const lizard_diagnostic_report_t *report, unsigned long index,
    char *buffer, size_t buffer_size) {
  if (buffer != NULL && buffer_size > 0U) {
    buffer[0] = '\0';
  }
  if (report == NULL || index >= report->count) {
    return diagnostic_report_format_event(NULL, buffer, buffer_size);
  }
  return diagnostic_report_format_event(&report->events[index], buffer,
                                        buffer_size);
}

int lizard_diagnostic_report_fprint(
    FILE *fp, const lizard_diagnostic_report_t *report) {
  unsigned long i;
  const lizard_owned_diagnostic_event_t *event;

  if (fp == NULL || report == NULL) {
    return 0;
  }
  if (fprintf(fp, "%s\tv=%d\tcount=%lu\n",
              lizard_report_schema_type(LIZARD_REPORT_SCHEMA_DIAGNOSTIC),
              lizard_report_schema_version(LIZARD_REPORT_SCHEMA_DIAGNOSTIC),
              report->count) < 0) {
    return 0;
  }
  for (i = 0UL; i < report->count; i++) {
    event = &report->events[i];
    if (fprintf(fp, "diagnostic\tindex=%lu\tstatus=%d\tmessage=", i,
                (int)event->status) < 0) {
      return 0;
    }
    if (!lizard_report_fprint_text_field(fp, event->message)) return 0;
    if (fputs("\torigin=", fp) < 0) return 0;
    if (!lizard_report_fprint_text_field(fp, event->filename)) return 0;
    if (fprintf(fp,
                "\tline=%d\tcolumn=%d\tend-line=%d\tend-column=%d\tstart-offset=%d\tend-offset=%d\n",
                event->start_line, event->start_column, event->end_line,
                event->end_column, event->start_offset,
                event->end_offset) < 0) {
      return 0;
    }
  }
  return 1;
}

int lizard_diagnostic_report_fprint_json(
    FILE *fp, const lizard_diagnostic_report_t *report) {
  unsigned long i;
  const lizard_owned_diagnostic_event_t *event;

  if (fp == NULL || report == NULL) {
    return 0;
  }
  if (fputs("{\"type\":", fp) < 0) {
    return 0;
  }
  if (!lizard_report_schema_fprint_type_json(
          fp, LIZARD_REPORT_SCHEMA_DIAGNOSTIC)) {
    return 0;
  }
  if (fprintf(fp, ",\"version\":%d,\"count\":%lu,\"diagnostics\":[",
              lizard_report_schema_version(LIZARD_REPORT_SCHEMA_DIAGNOSTIC),
              report->count) < 0) {
    return 0;
  }
  for (i = 0UL; i < report->count; i++) {
    event = &report->events[i];
    if (i > 0UL && fputc(',', fp) == EOF) return 0;
    if (fprintf(fp, "{\"index\":%lu,\"status\":%d,\"message\":", i,
                (int)event->status) < 0) {
      return 0;
    }
    if (!lizard_report_fprint_json_string(fp, event->message)) return 0;
    if (fputs(",\"origin\":{\"filename\":", fp) < 0) return 0;
    if (!lizard_report_fprint_json_string(fp, event->filename)) return 0;
    if (fprintf(fp,
                ",\"line\":%d,\"column\":%d,\"endLine\":%d,\"endColumn\":%d,\"startOffset\":%d,\"endOffset\":%d}}",
                event->start_line, event->start_column, event->end_line,
                event->end_column, event->start_offset,
                event->end_offset) < 0) {
      return 0;
    }
  }
  return fputs("]}\n", fp) >= 0;
}

lizard_diagnostic_report_t *lizard_context_diagnostic_report(
    lizard_context_t *context) {
  return lizard_diagnostic_report_from_diagnostic(
      lizard_context_last_diagnostic(context));
}

lizard_diagnostic_report_t *lizard_syntax_expansion_report_diagnostic_report(
    const lizard_syntax_expansion_report_t *report) {
  return lizard_diagnostic_report_from_diagnostic(
      lizard_syntax_expansion_report_diagnostic(report));
}
