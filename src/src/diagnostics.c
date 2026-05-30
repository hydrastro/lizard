/* src/diagnostics.c -- canonical diagnostic construction helpers. */

#include "diagnostics.h"

#include <string.h>

lizard_diagnostic_severity_t lizard_status_default_severity(
    lizard_status_t status) {
  switch (status) {
  case LIZARD_STATUS_OK:
    return LIZARD_DIAGNOSTIC_SEVERITY_INFO;
  case LIZARD_STATUS_ERROR:
  case LIZARD_STATUS_INVALID_ARGUMENT:
  case LIZARD_STATUS_IO_ERROR:
  case LIZARD_STATUS_OUT_OF_MEMORY:
  case LIZARD_STATUS_PARSE_ERROR:
  case LIZARD_STATUS_EVAL_ERROR:
  case LIZARD_STATUS_TYPE_ERROR:
    return LIZARD_DIAGNOSTIC_SEVERITY_ERROR;
  }
  return LIZARD_DIAGNOSTIC_SEVERITY_ERROR;
}

lizard_diagnostic_category_t lizard_status_default_category(
    lizard_status_t status) {
  switch (status) {
  case LIZARD_STATUS_OK:
    return LIZARD_DIAGNOSTIC_CATEGORY_UNKNOWN;
  case LIZARD_STATUS_PARSE_ERROR:
    return LIZARD_DIAGNOSTIC_CATEGORY_PARSER;
  case LIZARD_STATUS_EVAL_ERROR:
    return LIZARD_DIAGNOSTIC_CATEGORY_EVAL;
  case LIZARD_STATUS_TYPE_ERROR:
    return LIZARD_DIAGNOSTIC_CATEGORY_TYPE;
  case LIZARD_STATUS_IO_ERROR:
    return LIZARD_DIAGNOSTIC_CATEGORY_IO;
  case LIZARD_STATUS_OUT_OF_MEMORY:
  case LIZARD_STATUS_INVALID_ARGUMENT:
  case LIZARD_STATUS_ERROR:
    return LIZARD_DIAGNOSTIC_CATEGORY_UNKNOWN;
  }
  return LIZARD_DIAGNOSTIC_CATEGORY_UNKNOWN;
}

const char *lizard_diagnostic_severity_name(
    lizard_diagnostic_severity_t severity) {
  switch (severity) {
  case LIZARD_DIAGNOSTIC_SEVERITY_INFO:
    return "info";
  case LIZARD_DIAGNOSTIC_SEVERITY_WARNING:
    return "warning";
  case LIZARD_DIAGNOSTIC_SEVERITY_ERROR:
    return "error";
  }
  return "unknown";
}

const char *lizard_diagnostic_category_name(
    lizard_diagnostic_category_t category) {
  switch (category) {
  case LIZARD_DIAGNOSTIC_CATEGORY_UNKNOWN:
    return "unknown";
  case LIZARD_DIAGNOSTIC_CATEGORY_TOKENIZER:
    return "tokenizer";
  case LIZARD_DIAGNOSTIC_CATEGORY_PARSER:
    return "parser";
  case LIZARD_DIAGNOSTIC_CATEGORY_MACRO:
    return "macro";
  case LIZARD_DIAGNOSTIC_CATEGORY_EVAL:
    return "eval";
  case LIZARD_DIAGNOSTIC_CATEGORY_TYPE:
    return "type";
  case LIZARD_DIAGNOSTIC_CATEGORY_KERNEL:
    return "kernel";
  case LIZARD_DIAGNOSTIC_CATEGORY_IO:
    return "io";
  }
  return "unknown";
}


static void diagnostic_copy_message(char *dst, size_t dst_size,
                                    const char *message) {
  size_t n;

  if (dst == NULL || dst_size == 0U) {
    return;
  }
  if (message == NULL) {
    message = "";
  }
  n = strlen(message);
  if (n >= dst_size) {
    n = dst_size - 1U;
  }
  memcpy(dst, message, n);
  dst[n] = '\0';
}

void lizard_source_span_clear(lizard_source_span_t *span) {
  if (span == NULL) {
    return;
  }
  span->filename = NULL;
  span->start_line = 0;
  span->start_column = 0;
  span->end_line = 0;
  span->end_column = 0;
  span->start_offset = 0;
  span->end_offset = 0;
}

void lizard_source_span_set(lizard_source_span_t *span,
                            const char *filename,
                            int start_line,
                            int start_column,
                            int end_line,
                            int end_column,
                            int start_offset,
                            int end_offset) {
  if (span == NULL) {
    return;
  }
  span->filename = filename;
  span->start_line = start_line;
  span->start_column = start_column;
  span->end_line = end_line;
  span->end_column = end_column;
  span->start_offset = start_offset;
  span->end_offset = end_offset;
}

void lizard_diagnostic_clear(lizard_diagnostic_t *diagnostic) {
  if (diagnostic == NULL) {
    return;
  }
  diagnostic->status = LIZARD_STATUS_OK;
  diagnostic->severity = LIZARD_DIAGNOSTIC_SEVERITY_INFO;
  diagnostic->category = LIZARD_DIAGNOSTIC_CATEGORY_UNKNOWN;
  lizard_source_span_clear(&diagnostic->span);
  diagnostic->message[0] = '\0';
}

void lizard_diagnostic_set(lizard_diagnostic_t *diagnostic,
                           lizard_status_t status,
                           lizard_diagnostic_category_t category,
                           const lizard_source_span_t *span,
                           const char *message) {
  if (diagnostic == NULL) {
    return;
  }
  diagnostic->status = status;
  diagnostic->severity = lizard_status_default_severity(status);
  diagnostic->category = category;
  if (diagnostic->category == LIZARD_DIAGNOSTIC_CATEGORY_UNKNOWN) {
    diagnostic->category = lizard_status_default_category(status);
  }
  if (span != NULL) {
    diagnostic->span = *span;
  } else {
    lizard_source_span_clear(&diagnostic->span);
  }
  diagnostic_copy_message(diagnostic->message, sizeof(diagnostic->message),
                          message);
}

void lizard_diagnostic_set_simple(lizard_diagnostic_t *diagnostic,
                                  lizard_status_t status,
                                  lizard_diagnostic_category_t category,
                                  const char *message) {
  lizard_diagnostic_set(diagnostic, status, category, NULL, message);
}

void lizard_diagnostic_copy(lizard_diagnostic_t *dst,
                            const lizard_diagnostic_t *src) {
  if (dst == NULL) {
    return;
  }
  if (src == NULL) {
    lizard_diagnostic_clear(dst);
    return;
  }
  *dst = *src;
}
