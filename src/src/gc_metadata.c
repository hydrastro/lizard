/* src/gc_metadata.c -- Phase 3B GC metadata side-table scaffold. */

#include "gc_metadata.h"
#include "mem.h"

#include <stdlib.h>
#include <string.h>

#define LIZARD_GC_METADATA_INITIAL_CAPACITY 128U

typedef struct lizard_gc_metadata_entry {
  void *ptr;
  size_t size;
  lizard_gc_object_kind_t kind;
  lizard_object_owner_t owner;
  lizard_object_trace_policy_t trace_policy;
} lizard_gc_metadata_entry_t;

struct lizard_gc_metadata_table {
  lizard_gc_metadata_entry_t *entries;
  size_t count;
  size_t capacity;
  size_t registration_failures;
};

static void metadata_stats_clear(lizard_gc_metadata_stats_t *stats) {
  if (stats != NULL) {
    stats->entries = 0U;
    stats->capacity = 0U;
    stats->bytes_registered = 0U;
    stats->ast_nodes = 0U;
    stats->ast_list_nodes = 0U;
    stats->env_objects = 0U;
    stats->env_entries = 0U;
    stats->kernel_terms = 0U;
    stats->raw_objects = 0U;
    stats->trace_required = 0U;
    stats->registration_failures = 0U;
  }
}

static void metadata_entry_clear(lizard_gc_metadata_entry_t *entry) {
  if (entry != NULL) {
    entry->ptr = NULL;
    entry->size = 0U;
    entry->kind = LIZARD_GC_OBJECT_UNKNOWN;
    entry->owner = LIZARD_OBJECT_OWNER_UNKNOWN;
    entry->trace_policy = LIZARD_OBJECT_TRACE_NONE;
  }
}

static int metadata_ensure_capacity(lizard_gc_metadata_table_t *table,
                                    size_t needed) {
  lizard_gc_metadata_entry_t *next_entries;
  size_t next_capacity;
  size_t i;

  if (table == NULL) {
    return 0;
  }
  if (needed <= table->capacity) {
    return 1;
  }
  next_capacity = table->capacity == 0U ? LIZARD_GC_METADATA_INITIAL_CAPACITY
                                        : table->capacity;
  while (next_capacity < needed) {
    next_capacity *= 2U;
  }
  next_entries = (lizard_gc_metadata_entry_t *)realloc(
      table->entries, next_capacity * sizeof(table->entries[0]));
  if (next_entries == NULL) {
    table->registration_failures++;
    return 0;
  }
  table->entries = next_entries;
  for (i = table->capacity; i < next_capacity; i++) {
    metadata_entry_clear(&table->entries[i]);
  }
  table->capacity = next_capacity;
  return 1;
}

const char *lizard_gc_object_kind_name(lizard_gc_object_kind_t kind) {
  switch (kind) {
  case LIZARD_GC_OBJECT_UNKNOWN:
    return "unknown";
  case LIZARD_GC_OBJECT_AST_NODE:
    return "ast-node";
  case LIZARD_GC_OBJECT_AST_LIST_NODE:
    return "ast-list-node";
  case LIZARD_GC_OBJECT_ENV:
    return "env";
  case LIZARD_GC_OBJECT_ENV_ENTRY:
    return "env-entry";
  case LIZARD_GC_OBJECT_KERNEL_TERM:
    return "kernel-term";
  case LIZARD_GC_OBJECT_RAW:
    return "raw";
  }
  return "unknown";
}

lizard_gc_object_kind_t lizard_gc_infer_object_kind(size_t size) {
  if (size == align_size(sizeof(lizard_ast_node_t))) {
    return LIZARD_GC_OBJECT_AST_NODE;
  }
  if (size == align_size(sizeof(lizard_ast_list_node_t))) {
    return LIZARD_GC_OBJECT_AST_LIST_NODE;
  }
  if (size == align_size(sizeof(lizard_env_t))) {
    return LIZARD_GC_OBJECT_ENV;
  }
  if (size == align_size(sizeof(lizard_env_entry_t))) {
    return LIZARD_GC_OBJECT_ENV_ENTRY;
  }
  return LIZARD_GC_OBJECT_RAW;
}

lizard_gc_metadata_table_t *lizard_gc_metadata_table_create(void) {
  lizard_gc_metadata_table_t *table;

  table = (lizard_gc_metadata_table_t *)malloc(sizeof(*table));
  if (table == NULL) {
    return NULL;
  }
  table->entries = NULL;
  table->count = 0U;
  table->capacity = 0U;
  table->registration_failures = 0U;
  if (!metadata_ensure_capacity(table, LIZARD_GC_METADATA_INITIAL_CAPACITY)) {
    free(table);
    return NULL;
  }
  return table;
}

void lizard_gc_metadata_table_destroy(lizard_gc_metadata_table_t *table) {
  if (table == NULL) {
    return;
  }
  free(table->entries);
  free(table);
}

int lizard_gc_metadata_register(lizard_gc_metadata_table_t *table,
                                void *ptr,
                                size_t size,
                                lizard_gc_object_kind_t kind,
                                lizard_object_owner_t owner,
                                lizard_object_trace_policy_t trace_policy) {
  lizard_gc_metadata_entry_t *entry;

  if (table == NULL || ptr == NULL) {
    return 0;
  }
  if (!metadata_ensure_capacity(table, table->count + 1U)) {
    return 0;
  }
  entry = &table->entries[table->count];
  entry->ptr = ptr;
  entry->size = size;
  entry->kind = kind;
  entry->owner = owner;
  entry->trace_policy = trace_policy;
  table->count++;
  return 1;
}

int lizard_gc_metadata_lookup(lizard_gc_metadata_table_t *table,
                              const void *ptr,
                              lizard_gc_object_kind_t *out_kind,
                              size_t *out_size,
                              lizard_object_trace_policy_t *out_trace_policy) {
  size_t i;

  if (out_kind != NULL) {
    *out_kind = LIZARD_GC_OBJECT_UNKNOWN;
  }
  if (out_size != NULL) {
    *out_size = 0U;
  }
  if (out_trace_policy != NULL) {
    *out_trace_policy = LIZARD_OBJECT_TRACE_NONE;
  }
  if (table == NULL || ptr == NULL) {
    return 0;
  }
  for (i = 0U; i < table->count; i++) {
    if (table->entries[i].ptr == ptr) {
      if (out_kind != NULL) {
        *out_kind = table->entries[i].kind;
      }
      if (out_size != NULL) {
        *out_size = table->entries[i].size;
      }
      if (out_trace_policy != NULL) {
        *out_trace_policy = table->entries[i].trace_policy;
      }
      return 1;
    }
  }
  return 0;
}

void lizard_gc_metadata_collect_stats(lizard_gc_metadata_table_t *table,
                                      lizard_gc_metadata_stats_t *out_stats) {
  size_t i;

  metadata_stats_clear(out_stats);
  if (table == NULL || out_stats == NULL) {
    return;
  }
  out_stats->entries = table->count;
  out_stats->capacity = table->capacity;
  out_stats->registration_failures = table->registration_failures;
  for (i = 0U; i < table->count; i++) {
    out_stats->bytes_registered += table->entries[i].size;
    switch (table->entries[i].kind) {
    case LIZARD_GC_OBJECT_AST_NODE:
      out_stats->ast_nodes++;
      break;
    case LIZARD_GC_OBJECT_AST_LIST_NODE:
      out_stats->ast_list_nodes++;
      break;
    case LIZARD_GC_OBJECT_ENV:
      out_stats->env_objects++;
      break;
    case LIZARD_GC_OBJECT_ENV_ENTRY:
      out_stats->env_entries++;
      break;
    case LIZARD_GC_OBJECT_KERNEL_TERM:
      out_stats->kernel_terms++;
      break;
    case LIZARD_GC_OBJECT_RAW:
      out_stats->raw_objects++;
      break;
    case LIZARD_GC_OBJECT_UNKNOWN:
      break;
    }
    if (lizard_object_trace_policy_requires_mark(
            table->entries[i].trace_policy)) {
      out_stats->trace_required++;
    }
  }
}
