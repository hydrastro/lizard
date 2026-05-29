/* src/expansion_trace_report.c -- owned snapshots of expansion traces.
 *
 * Reports are intentionally independent from lizard_context_t and from the
 * Lizard heap.  Tooling may snapshot a trace, continue evaluating, or destroy
 * the context while still inspecting the report.  No strict warnings are
 * suppressed: allocation, copy, indexing, and formatting are explicit.
 */

#include "expansion_trace_report.h"
#include "report_writer.h"
#include "report_schema.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct lizard_owned_trace_event {
  char *stage;
  char *detail;
  char *origin_filename;
  int origin_line;
  int origin_column;
  int origin_phase;
  unsigned long origin_scope_summary;
} lizard_owned_trace_event_t;

struct lizard_expansion_trace_report {
  unsigned long count;
  lizard_owned_trace_event_t *events;
};

static void report_clear_public_event(lizard_expansion_trace_event_t *event) {
  if (event != NULL) {
    event->stage = NULL;
    event->detail = NULL;
    event->origin_filename = NULL;
    event->origin_line = 0;
    event->origin_column = 0;
    event->origin_phase = 0;
    event->origin_scope_summary = 0UL;
  }
}

static void report_clear_owned_event(lizard_owned_trace_event_t *event) {
  if (event != NULL) {
    event->stage = NULL;
    event->detail = NULL;
    event->origin_filename = NULL;
    event->origin_line = 0;
    event->origin_column = 0;
    event->origin_phase = 0;
    event->origin_scope_summary = 0UL;
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

static void report_free_owned_event(lizard_owned_trace_event_t *event) {
  if (event != NULL) {
    free(event->stage);
    free(event->detail);
    free(event->origin_filename);
    report_clear_owned_event(event);
  }
}

static int report_copy_to_owned(lizard_owned_trace_event_t *dst,
                                const lizard_expansion_trace_event_t *src) {
  report_clear_owned_event(dst);
  if (src == NULL || dst == NULL) {
    return 0;
  }
  dst->stage = report_copy_string(src->stage);
  if (src->stage != NULL && dst->stage == NULL) {
    return 0;
  }
  dst->detail = report_copy_string(src->detail);
  if (src->detail != NULL && dst->detail == NULL) {
    report_free_owned_event(dst);
    return 0;
  }
  dst->origin_filename = report_copy_string(src->origin_filename);
  if (src->origin_filename != NULL && dst->origin_filename == NULL) {
    report_free_owned_event(dst);
    return 0;
  }
  dst->origin_line = src->origin_line;
  dst->origin_column = src->origin_column;
  dst->origin_phase = src->origin_phase;
  dst->origin_scope_summary = src->origin_scope_summary;
  return 1;
}

static void report_copy_owned_view(lizard_expansion_trace_event_t *dst,
                                   const lizard_owned_trace_event_t *src) {
  report_clear_public_event(dst);
  if (dst != NULL && src != NULL) {
    dst->stage = src->stage;
    dst->detail = src->detail;
    dst->origin_filename = src->origin_filename;
    dst->origin_line = src->origin_line;
    dst->origin_column = src->origin_column;
    dst->origin_phase = src->origin_phase;
    dst->origin_scope_summary = src->origin_scope_summary;
  }
}

static int report_format_owned_event(const lizard_owned_trace_event_t *event,
                                     char *buffer, size_t buffer_size) {
  char tmp[512];
  const char *stage;
  const char *detail;
  const char *filename;
  size_t i;

  if (buffer == NULL || buffer_size == 0U) {
    return 0;
  }
  if (event == NULL) {
    sprintf(tmp, "#<expansion-trace-event null>");
  } else {
    stage = event->stage != NULL ? event->stage : "<none>";
    detail = event->detail != NULL ? event->detail : "";
    filename = event->origin_filename != NULL ? event->origin_filename
                                              : "<unknown>";
    sprintf(tmp,
            "#<expansion-trace-event stage=%s detail=\"%s\" origin=%s:%d:%d phase=%d scope-summary=%lu>",
            stage, detail, filename, event->origin_line,
            event->origin_column, event->origin_phase,
            event->origin_scope_summary);
  }

  i = 0U;
  while (i + 1U < buffer_size && tmp[i] != '\0') {
    buffer[i] = tmp[i];
    i++;
  }
  buffer[i] = '\0';
  return event != NULL;
}

lizard_expansion_trace_report_t *lizard_expansion_trace_report_from_surface(
    const lizard_surface_term_t *surface) {
  lizard_expansion_trace_report_t *report;
  lizard_expansion_trace_event_t view;
  unsigned long count;
  unsigned long i;

  count = lizard_surface_trace_count(surface);
  report = (lizard_expansion_trace_report_t *)malloc(sizeof(*report));
  if (report == NULL) {
    return NULL;
  }
  report->count = count;
  report->events = NULL;

  if (count == 0UL) {
    return report;
  }

  report->events = (lizard_owned_trace_event_t *)calloc(
      (size_t)count, sizeof(report->events[0]));
  if (report->events == NULL) {
    free(report);
    return NULL;
  }

  for (i = 0UL; i < count; i++) {
    report_clear_public_event(&view);
    if (!lizard_surface_trace_event_at(surface, i, &view)) {
      lizard_expansion_trace_report_destroy(report);
      return NULL;
    }
    if (!report_copy_to_owned(&report->events[i], &view)) {
      lizard_expansion_trace_report_destroy(report);
      return NULL;
    }
  }
  return report;
}

void lizard_expansion_trace_report_destroy(
    lizard_expansion_trace_report_t *report) {
  unsigned long i;

  if (report == NULL) {
    return;
  }
  if (report->events != NULL) {
    for (i = 0UL; i < report->count; i++) {
      report_free_owned_event(&report->events[i]);
    }
    free(report->events);
  }
  free(report);
}

unsigned long lizard_expansion_trace_report_count(
    const lizard_expansion_trace_report_t *report) {
  return report != NULL ? report->count : 0UL;
}

int lizard_expansion_trace_report_event(
    const lizard_expansion_trace_report_t *report, unsigned long index,
    lizard_expansion_trace_event_t *out_event) {
  report_clear_public_event(out_event);
  if (report == NULL || out_event == NULL || index >= report->count) {
    return 0;
  }
  report_copy_owned_view(out_event, &report->events[index]);
  return 1;
}

int lizard_expansion_trace_report_event_string(
    const lizard_expansion_trace_report_t *report, unsigned long index,
    char *buffer, size_t buffer_size) {
  if (buffer != NULL && buffer_size > 0U) {
    buffer[0] = '\0';
  }
  if (report == NULL || index >= report->count) {
    return report_format_owned_event(NULL, buffer, buffer_size);
  }
  return report_format_owned_event(&report->events[index], buffer,
                                   buffer_size);
}

int lizard_expansion_trace_report_fprint(
    FILE *fp, const lizard_expansion_trace_report_t *report) {
  unsigned long count;
  unsigned long i;
  lizard_expansion_trace_event_t event;

  if (fp == NULL) {
    return 0;
  }
  count = lizard_expansion_trace_report_count(report);
  if (fprintf(fp, "%s\tv=%d\tcount=%lu\n",
              lizard_report_schema_type(LIZARD_REPORT_SCHEMA_EXPANSION_TRACE),
              lizard_report_schema_version(LIZARD_REPORT_SCHEMA_EXPANSION_TRACE),
              count) < 0) {
    return 0;
  }
  for (i = 0UL; i < count; i++) {
    if (!lizard_expansion_trace_report_event(report, i, &event)) {
      return 0;
    }
    if (fprintf(fp, "event\tindex=%lu\tstage=", i) < 0) return 0;
    if (!lizard_report_fprint_text_field(fp, event.stage)) return 0;
    if (fputs("\tdetail=", fp) < 0) return 0;
    if (!lizard_report_fprint_text_field(fp, event.detail)) return 0;
    if (fputs("\torigin=", fp) < 0) return 0;
    if (!lizard_report_fprint_text_field(fp, event.origin_filename)) return 0;
    if (fprintf(fp, "\tline=%d\tcolumn=%d\tphase=%d\tscope=%lu\n",
                event.origin_line, event.origin_column, event.origin_phase,
                event.origin_scope_summary) < 0) {
      return 0;
    }
  }
  return 1;
}


int lizard_expansion_trace_report_fprint_json(
    FILE *fp, const lizard_expansion_trace_report_t *report) {
  unsigned long count;
  unsigned long i;
  lizard_expansion_trace_event_t event;

  if (fp == NULL) {
    return 0;
  }
  count = lizard_expansion_trace_report_count(report);
  if (fputs("{\"type\":", fp) < 0) {
    return 0;
  }
  if (!lizard_report_schema_fprint_type_json(
          fp, LIZARD_REPORT_SCHEMA_EXPANSION_TRACE)) {
    return 0;
  }
  if (fprintf(fp, ",\"version\":%d,\"count\":%lu,\"events\":[",
              lizard_report_schema_version(LIZARD_REPORT_SCHEMA_EXPANSION_TRACE),
              count) < 0) {
    return 0;
  }
  for (i = 0UL; i < count; i++) {
    if (!lizard_expansion_trace_report_event(report, i, &event)) {
      return 0;
    }
    if (i > 0UL && fputc(',', fp) == EOF) {
      return 0;
    }
    if (fprintf(fp, "{\"index\":%lu,\"stage\":", i) < 0) return 0;
    if (!lizard_report_fprint_json_string(fp, event.stage)) return 0;
    if (fputs(",\"detail\":", fp) < 0) return 0;
    if (!lizard_report_fprint_json_string(fp, event.detail)) return 0;
    if (fputs(",\"origin\":", fp) < 0) return 0;
    if (!lizard_report_fprint_json_string(fp, event.origin_filename)) return 0;
    if (fprintf(fp,
                ",\"line\":%d,\"column\":%d,\"phase\":%d,\"scope\":%lu}",
                event.origin_line, event.origin_column, event.origin_phase,
                event.origin_scope_summary) < 0) {
      return 0;
    }
  }
  return fputs("]}\n", fp) >= 0;
}
