#ifndef LIZARD_GC_METADATA_H
#define LIZARD_GC_METADATA_H

#include "lizard_internal.h"
#include "object_model.h"

/* Phase 3B: side-table metadata for the future object-level non-moving GC.
 *
 * This table is C-owned and external to heap objects.  It does not change
 * object layout, object addresses, mark/sweep behavior, or allocation
 * semantics.  Registration failures are counted for diagnostics/audits but do
 * not make runtime allocation fail.
 */
typedef enum lizard_gc_object_kind {
  LIZARD_GC_OBJECT_UNKNOWN = 0,
  LIZARD_GC_OBJECT_AST_NODE = 1,
  LIZARD_GC_OBJECT_AST_LIST_NODE = 2,
  LIZARD_GC_OBJECT_ENV = 3,
  LIZARD_GC_OBJECT_ENV_ENTRY = 4,
  LIZARD_GC_OBJECT_KERNEL_TERM = 5,
  LIZARD_GC_OBJECT_RAW = 6
} lizard_gc_object_kind_t;

typedef struct lizard_gc_metadata_stats {
  size_t entries;
  size_t capacity;
  size_t bytes_registered;
  size_t ast_nodes;
  size_t ast_list_nodes;
  size_t env_objects;
  size_t env_entries;
  size_t kernel_terms;
  size_t raw_objects;
  size_t trace_required;
  size_t registration_failures;
} lizard_gc_metadata_stats_t;

const char *lizard_gc_object_kind_name(lizard_gc_object_kind_t kind);
lizard_gc_object_kind_t lizard_gc_infer_object_kind(size_t size);

lizard_gc_metadata_table_t *lizard_gc_metadata_table_create(void);
void lizard_gc_metadata_table_destroy(lizard_gc_metadata_table_t *table);
int lizard_gc_metadata_register(lizard_gc_metadata_table_t *table,
                                void *ptr,
                                size_t size,
                                lizard_gc_object_kind_t kind,
                                lizard_object_owner_t owner,
                                lizard_object_trace_policy_t trace_policy);
int lizard_gc_metadata_lookup(lizard_gc_metadata_table_t *table,
                              const void *ptr,
                              lizard_gc_object_kind_t *out_kind,
                              size_t *out_size,
                              lizard_object_trace_policy_t *out_trace_policy);
void lizard_gc_metadata_collect_stats(lizard_gc_metadata_table_t *table,
                                      lizard_gc_metadata_stats_t *out_stats);

#endif /* LIZARD_GC_METADATA_H */
