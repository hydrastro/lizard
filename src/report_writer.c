#include "report_writer.h"

int lizard_report_fprint_text_field(FILE *fp, const char *text) {
  const unsigned char *p;

  if (fp == NULL) {
    return 0;
  }
  if (text == NULL) {
    return fputs("-", fp) >= 0;
  }
  p = (const unsigned char *)text;
  while (*p != '\0') {
    switch (*p) {
    case '\\':
      if (fputs("\\\\", fp) < 0) return 0;
      break;
    case '\t':
      if (fputs("\\t", fp) < 0) return 0;
      break;
    case '\n':
      if (fputs("\\n", fp) < 0) return 0;
      break;
    case '\r':
      if (fputs("\\r", fp) < 0) return 0;
      break;
    default:
      if (fputc((int)*p, fp) == EOF) return 0;
      break;
    }
    p++;
  }
  return 1;
}

int lizard_report_fprint_json_string(FILE *fp, const char *text) {
  const unsigned char *p;
  static const char hex[] = "0123456789abcdef";

  if (fp == NULL) {
    return 0;
  }
  if (fputc('"', fp) == EOF) {
    return 0;
  }
  if (text != NULL) {
    p = (const unsigned char *)text;
    while (*p != '\0') {
      if (*p == '"' || *p == '\\') {
        if (fputc('\\', fp) == EOF || fputc((int)*p, fp) == EOF) return 0;
      } else if (*p == '\b') {
        if (fputs("\\b", fp) < 0) return 0;
      } else if (*p == '\f') {
        if (fputs("\\f", fp) < 0) return 0;
      } else if (*p == '\n') {
        if (fputs("\\n", fp) < 0) return 0;
      } else if (*p == '\r') {
        if (fputs("\\r", fp) < 0) return 0;
      } else if (*p == '\t') {
        if (fputs("\\t", fp) < 0) return 0;
      } else if (*p < 0x20U) {
        if (fputs("\\u00", fp) < 0) return 0;
        if (fputc(hex[(*p >> 4) & 0x0FU], fp) == EOF) return 0;
        if (fputc(hex[*p & 0x0FU], fp) == EOF) return 0;
      } else {
        if (fputc((int)*p, fp) == EOF) return 0;
      }
      p++;
    }
  }
  return fputc('"', fp) != EOF;
}
