/* src/gc.h — Garbage collector infrastructure (Phase D).
 *
 * Mark-and-sweep GC for lizard's AST heap. D.1 provides the mark
 * traversal and stats; D.2 will add the sweep/collection phase.
 */
#ifndef LIZARD_GC_H
#define LIZARD_GC_H

#include "lizard_internal.h"
#include "gc_metadata.h"

typedef struct lizard_gc_stats {
  size_t total_segments;
  size_t total_bytes_allocated;
  size_t nodes_total;         /* all AST-node-sized chunks in the heap */
  size_t nodes_marked;        /* reachable from roots after a mark pass */
  size_t nodes_garbage;       /* total - marked */
} lizard_gc_stats_t;

/* Mark a single node and all its descendants. Idempotent — already-
 * marked nodes are skipped (prevents infinite loops on cycles). */
void lizard_gc_mark_node(lizard_ast_node_t *node);

/* Mark all bindings reachable from an environment chain. */
void lizard_gc_mark_env(lizard_env_t *env);

/* Clear all mark bits across the entire heap. Must be called before
 * a new mark pass. */
void lizard_gc_clear_marks(lizard_heap_t *heap);

/* Count marked nodes across the heap. Call after a mark pass. */
size_t lizard_gc_count_marked(lizard_heap_t *heap);

/* Gather heap statistics. If do_mark is nonzero, performs a clear +
 * mark-from-roots + count cycle using the given env as the root. */
void lizard_gc_collect_stats(lizard_heap_t *heap, lizard_env_t *env,
                             int do_mark, lizard_gc_stats_t *out);

/* Run a full GC cycle: mark from roots, then free any heap segment
 * that contains zero live objects. Returns the number of bytes freed.
 * Does NOT move objects — no pointer updating needed. */
size_t lizard_gc_collect(lizard_heap_t *heap, lizard_env_t *env);

/* Phase 5: per-object, non-moving, conservative mark & sweep. Reclaims
 * individual dead objects into per-size free lists for reuse (addresses of
 * live objects are never changed). Returns the number of bytes reclaimed. */
size_t lizard_gc_collect_objects(lizard_heap_t *heap, lizard_env_t *env);


/* Phase 3B: non-invasive metadata side-table inspection. */
void lizard_gc_metadata_stats(lizard_heap_t *heap,
                              lizard_gc_metadata_stats_t *out_stats);
int lizard_gc_metadata_lookup_object(
    lizard_heap_t *heap, const void *ptr, lizard_gc_object_kind_t *out_kind,
    size_t *out_size, lizard_object_trace_policy_t *out_trace_policy);

#endif /* LIZARD_GC_H */
