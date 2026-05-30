/* src/report_schema.c -- centralized stable report schema names/versions. */

#include "report_schema.h"
#include "report_writer.h"

#include <string.h>

#define LIZARD_REPORT_SCHEMA_PUBLIC_COUNT 3UL
#define LIZARD_REPORT_SCHEMA_LIST_TYPE "lizard-report-schema-list"
#define LIZARD_REPORT_SCHEMA_LIST_VERSION 1

static int schema_capability_for_valid_kind(lizard_report_schema_kind_t kind) {
  return lizard_report_schema_valid(kind);
}

const char *lizard_report_schema_type(lizard_report_schema_kind_t kind) {
  switch (kind) {
  case LIZARD_REPORT_SCHEMA_EXPANSION_TRACE:
    return "lizard-expansion-trace";
  case LIZARD_REPORT_SCHEMA_DIAGNOSTIC:
    return "lizard-diagnostic-report";
  case LIZARD_REPORT_SCHEMA_SYNTAX_EXPANSION:
    return "lizard-syntax-expansion";
  }
  return "lizard-unknown-report";
}

int lizard_report_schema_version(lizard_report_schema_kind_t kind) {
  switch (kind) {
  case LIZARD_REPORT_SCHEMA_EXPANSION_TRACE:
  case LIZARD_REPORT_SCHEMA_DIAGNOSTIC:
  case LIZARD_REPORT_SCHEMA_SYNTAX_EXPANSION:
    return 1;
  }
  return 0;
}

int lizard_report_schema_valid(lizard_report_schema_kind_t kind) {
  switch (kind) {
  case LIZARD_REPORT_SCHEMA_EXPANSION_TRACE:
  case LIZARD_REPORT_SCHEMA_DIAGNOSTIC:
  case LIZARD_REPORT_SCHEMA_SYNTAX_EXPANSION:
    return 1;
  }
  return 0;
}

int lizard_report_schema_supports_text(lizard_report_schema_kind_t kind) {
  return schema_capability_for_valid_kind(kind);
}

int lizard_report_schema_supports_json(lizard_report_schema_kind_t kind) {
  return schema_capability_for_valid_kind(kind);
}

int lizard_report_schema_stable_v1(lizard_report_schema_kind_t kind) {
  return lizard_report_schema_valid(kind) &&
         lizard_report_schema_version(kind) == 1;
}

int lizard_report_schema_fprint_type_json(FILE *fp,
                                          lizard_report_schema_kind_t kind) {
  return lizard_report_fprint_json_string(
      fp, lizard_report_schema_type(kind));
}

int lizard_report_schema_kind_at(unsigned long index,
                                 lizard_report_schema_kind_t *out_kind) {
  if (out_kind == NULL || index >= LIZARD_REPORT_SCHEMA_PUBLIC_COUNT) {
    return 0;
  }
  *out_kind = (lizard_report_schema_kind_t)index;
  return lizard_report_schema_valid(*out_kind);
}

int lizard_report_schema_info_from_kind(lizard_report_schema_kind_t kind,
                                        lizard_report_schema_info_t *out_info) {
  if (out_info == NULL) {
    return 0;
  }
  out_info->type = NULL;
  out_info->version = 0;
  out_info->supports_text = 0;
  out_info->supports_json = 0;
  out_info->stable_v1 = 0;
  if (!lizard_report_schema_valid(kind)) {
    return 0;
  }
  out_info->type = lizard_report_schema_type(kind);
  out_info->version = lizard_report_schema_version(kind);
  out_info->supports_text = lizard_report_schema_supports_text(kind);
  out_info->supports_json = lizard_report_schema_supports_json(kind);
  out_info->stable_v1 = lizard_report_schema_stable_v1(kind);
  return 1;
}

unsigned long lizard_report_schema_count(void) {
  return LIZARD_REPORT_SCHEMA_PUBLIC_COUNT;
}

int lizard_report_schema_get(unsigned long index,
                             lizard_report_schema_info_t *out_info) {
  lizard_report_schema_kind_t kind;

  if (!lizard_report_schema_kind_at(index, &kind)) {
    if (out_info != NULL) {
      out_info->type = NULL;
      out_info->version = 0;
      out_info->supports_text = 0;
      out_info->supports_json = 0;
      out_info->stable_v1 = 0;
    }
    return 0;
  }
  return lizard_report_schema_info_from_kind(kind, out_info);
}


int lizard_report_schema_format_supported(const lizard_report_schema_info_t *info,
                                          const char *format) {
  if (info == NULL || info->type == NULL || info->version <= 0) {
    return 0;
  }
  if (format == NULL || strcmp(format, "any") == 0) {
    return info->supports_text || info->supports_json;
  }
  if (strcmp(format, "text") == 0) {
    return info->supports_text;
  }
  if (strcmp(format, "json") == 0) {
    return info->supports_json;
  }
  return 0;
}

int lizard_report_schema_require(const char *type, int min_version,
                                 const char *format,
                                 lizard_report_schema_info_t *out_info) {
  unsigned long i;
  unsigned long count;
  lizard_report_schema_info_t info;

  if (out_info != NULL) {
    out_info->type = NULL;
    out_info->version = 0;
    out_info->supports_text = 0;
    out_info->supports_json = 0;
    out_info->stable_v1 = 0;
  }
  if (type == NULL || type[0] == '\0' || min_version < 0) {
    return 0;
  }
  count = lizard_report_schema_count();
  for (i = 0UL; i < count; i++) {
    if (!lizard_report_schema_get(i, &info)) {
      continue;
    }
    if (info.type != NULL && strcmp(info.type, type) == 0 &&
        info.version >= min_version &&
        lizard_report_schema_format_supported(&info, format)) {
      if (out_info != NULL) {
        *out_info = info;
      }
      return 1;
    }
  }
  return 0;
}

const char *lizard_report_schema_list_type(void) {
  return LIZARD_REPORT_SCHEMA_LIST_TYPE;
}

int lizard_report_schema_list_version(void) {
  return LIZARD_REPORT_SCHEMA_LIST_VERSION;
}

int lizard_report_schema_list_fprint(FILE *fp) {
  unsigned long i;
  lizard_report_schema_info_t info;

  if (fp == NULL) {
    return 0;
  }
  if (fputs(LIZARD_REPORT_SCHEMA_LIST_TYPE, fp) < 0) {
    return 0;
  }
  if (fprintf(fp, "\tv=%d\tcount=%lu\n", LIZARD_REPORT_SCHEMA_LIST_VERSION,
              LIZARD_REPORT_SCHEMA_PUBLIC_COUNT) < 0) {
    return 0;
  }
  for (i = 0UL; i < LIZARD_REPORT_SCHEMA_PUBLIC_COUNT; i++) {
    if (!lizard_report_schema_get(i, &info)) {
      return 0;
    }
    if (fprintf(fp, "schema\tindex=%lu\ttype=", i) < 0) {
      return 0;
    }
    if (!lizard_report_fprint_text_field(fp, info.type)) {
      return 0;
    }
    if (fprintf(fp,
                "\tversion=%d\tsupports-text=%d\tsupports-json=%d\tstable-v1=%d\n",
                info.version, info.supports_text, info.supports_json,
                info.stable_v1) < 0) {
      return 0;
    }
  }
  return 1;
}

int lizard_report_schema_list_fprint_json(FILE *fp) {
  unsigned long i;
  lizard_report_schema_info_t info;

  if (fp == NULL) {
    return 0;
  }
  if (fputs("{\"type\":", fp) < 0) {
    return 0;
  }
  if (!lizard_report_fprint_json_string(fp, LIZARD_REPORT_SCHEMA_LIST_TYPE)) {
    return 0;
  }
  if (fprintf(fp, ",\"version\":%d,\"schemas\":[",
              LIZARD_REPORT_SCHEMA_LIST_VERSION) < 0) {
    return 0;
  }
  for (i = 0UL; i < LIZARD_REPORT_SCHEMA_PUBLIC_COUNT; i++) {
    if (!lizard_report_schema_get(i, &info)) {
      return 0;
    }
    if (i != 0UL && fputc(',', fp) == EOF) {
      return 0;
    }
    if (fprintf(fp, "{\"index\":%lu,\"type\":", i) < 0) {
      return 0;
    }
    if (!lizard_report_fprint_json_string(fp, info.type)) {
      return 0;
    }
    if (fprintf(fp,
                ",\"version\":%d,\"supports_text\":%s,\"supports_json\":%s,\"stable_v1\":%s}",
                info.version,
                info.supports_text ? "true" : "false",
                info.supports_json ? "true" : "false",
                info.stable_v1 ? "true" : "false") < 0) {
      return 0;
    }
  }
  return fputs("]}\n", fp) >= 0;
}
