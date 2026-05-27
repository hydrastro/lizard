/* src/gc.h — Garbage collector infrastructure (Phase D).
 *
 * Mark-and-sweep GC for lizard's AST heap. D.1 provides the mark
 * traversal and stats; D.2 will add the sweep/collection phase.
 */
#ifndef LIZARD_GC_H
#define LIZARD_GC_H

#include "lizard_internal.h"

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

#endif /* LIZARD_GC_H */
