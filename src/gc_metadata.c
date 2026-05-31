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
  unsigned char mark;   /* GC: set during the mark phase, cleared by sweep */
} lizard_gc_metadata_entry_t;

struct lizard_gc_metadata_table {
  lizard_gc_metadata_entry_t *entries;
  size_t count;
  size_t capacity;
  size_t registration_failures;
  size_t *sorted;       /* indices into entries[], sorted by ptr (GC mark) */
  size_t sorted_count;
  size_t sorted_cap;
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
  table->sorted = NULL;
  table->sorted_count = 0U;
  table->sorted_cap = 0U;
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
  free(table->sorted);
  free(table);
}

/* ===================================================================
 * GC support: mark phase (sorted address lookup) + per-object sweep.
 * The collector (gc.c) is non-moving and fully conservative, so these
 * routines only need to (a) find which tracked object contains an
 * arbitrary address, (b) record marks, and (c) free + remove the
 * unmarked objects after marking.
 * ================================================================== */

static int metadata_sorted_cmp(const void *a, const void *b,
                               lizard_gc_metadata_entry_t *entries) {
  size_t ia = *(const size_t *)a;
  size_t ib = *(const size_t *)b;
  if (entries[ia].ptr < entries[ib].ptr) return -1;
  if (entries[ia].ptr > entries[ib].ptr) return 1;
  return 0;
}

/* qsort lacks a context argument in C89, so sort with a simple insertion /
 * shell sort over the index array keyed by entries[].ptr. */
static void metadata_sort_index(lizard_gc_metadata_table_t *t) {
  size_t gap, i, j, tmp;
  for (gap = t->sorted_count / 2U; gap > 0U; gap /= 2U) {
    for (i = gap; i < t->sorted_count; i++) {
      tmp = t->sorted[i];
      j = i;
      while (j >= gap &&
             metadata_sorted_cmp(&t->sorted[j - gap], &tmp, t->entries) > 0) {
        t->sorted[j] = t->sorted[j - gap];
        j -= gap;
      }
      t->sorted[j] = tmp;
    }
  }
}

void lizard_gc_metadata_prepare_marking(lizard_gc_metadata_table_t *table) {
  size_t i;
  if (table == NULL) return;
  if (table->sorted_cap < table->count) {
    size_t newcap = table->count > 0U ? table->count : 1U;
    size_t *p = (size_t *)realloc(table->sorted, newcap * sizeof(size_t));
    if (p == NULL) { table->sorted_count = 0U; return; }
    table->sorted = p;
    table->sorted_cap = newcap;
  }
  table->sorted_count = table->count;
  for (i = 0; i < table->count; i++) {
    table->entries[i].mark = 0;
    table->sorted[i] = i;
  }
  metadata_sort_index(table);
}

/* If `addr` falls inside a tracked object that is currently UNMARKED, mark it,
 * fill the out-parameters with its extent, and return 1.  Otherwise return 0
 * (address not tracked, or object already marked). */
int lizard_gc_metadata_mark_addr(lizard_gc_metadata_table_t *table,
                                 const void *addr, void **out_base,
                                 size_t *out_size) {
  size_t lo, hi;
  const char *a = (const char *)addr;
  if (table == NULL || table->sorted_count == 0U) return 0;
  lo = 0U;
  hi = table->sorted_count; /* find last entry with ptr <= addr */
  while (lo < hi) {
    size_t mid = lo + (hi - lo) / 2U;
    if ((const char *)table->entries[table->sorted[mid]].ptr <= a) {
      lo = mid + 1U;
    } else {
      hi = mid;
    }
  }
  if (lo == 0U) return 0;
  {
    lizard_gc_metadata_entry_t *e = &table->entries[table->sorted[lo - 1U]];
    const char *base = (const char *)e->ptr;
    if (a >= base && a < base + e->size) {
      if (e->mark) return 0;
      e->mark = 1;
      if (out_base) *out_base = e->ptr;
      if (out_size) *out_size = e->size;
      return 1;
    }
  }
  return 0;
}

size_t lizard_gc_metadata_sweep(lizard_gc_metadata_table_t *table,
                                lizard_gc_free_fn free_cb, void *ctx) {
  size_t i, live = 0U, freed = 0U;
  if (table == NULL) return 0U;
  for (i = 0; i < table->count; i++) {
    lizard_gc_metadata_entry_t *e = &table->entries[i];
    if (e->mark) {
      e->mark = 0;
      if (live != i) table->entries[live] = *e;
      live++;
    } else {
      if (free_cb) free_cb(e->ptr, e->size, e->kind, ctx);
      freed++;
    }
  }
  table->count = live;
  table->sorted_count = 0U; /* index is now stale */
  return freed;
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
  entry->mark = 0;
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
