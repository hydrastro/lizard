/* prims_gc.c — extracted from primitives.c (#7 monolith split).
 * Registration stays in primitives.c; definitions linked from here. */
#include "primitives.h"
#include "env.h"
#include "errors.h"
#include "lizard_internal.h"
#include "mem.h"
#include "parser.h"
#include "printer.h"
#include "runtime.h"
#include "tokenizer.h"
#include "prims_shared.h"
#include "gc.h"
#include <setjmp.h>
#include <stdint.h>
#include <string.h>

lizard_ast_node_t *lizard_primitive_gc_stats(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_gc_stats_t stats;
  lizard_ast_node_t *result;
  (void)args;

  lizard_gc_collect_stats(heap, env, 1, &stats);

  /* Build alist: ((segments . N) (bytes . N) (total . N) (live . N) (garbage . N)) */
  result = lizard_make_nil(heap);
  result = gc_cons(heap, gc_make_stat(heap, "garbage",  (unsigned long)stats.nodes_garbage), result);
  result = gc_cons(heap, gc_make_stat(heap, "live",     (unsigned long)stats.nodes_marked),  result);
  result = gc_cons(heap, gc_make_stat(heap, "total",    (unsigned long)stats.nodes_total),   result);
  result = gc_cons(heap, gc_make_stat(heap, "bytes",    (unsigned long)stats.total_bytes_allocated), result);
  result = gc_cons(heap, gc_make_stat(heap, "segments", (unsigned long)stats.total_segments), result);
  return result;
}
lizard_ast_node_t *lizard_primitive_gc(lz_list_t *args,
                                        lizard_env_t *env,
                                        lizard_heap_t *heap) {
  lizard_gc_stats_t before, after;
  size_t freed;
  lizard_ast_node_t *result;
  (void)args;

  /* Stats before collection. */
  lizard_gc_collect_stats(heap, env, 1, &before);

  /* Collect — frees dead segments. */
  freed = lizard_gc_collect(heap, env);

  /* Stats after. */
  lizard_gc_collect_stats(heap, env, 1, &after);

  /* Build result alist. */
  result = lizard_make_nil(heap);
  result = gc_cons(heap, gc_make_stat(heap, "freed-bytes",      (unsigned long)freed),                result);
  result = gc_cons(heap, gc_make_stat(heap, "segments-after",   (unsigned long)after.total_segments), result);
  result = gc_cons(heap, gc_make_stat(heap, "segments-before",  (unsigned long)before.total_segments),result);
  result = gc_cons(heap, gc_make_stat(heap, "garbage",          (unsigned long)before.nodes_garbage), result);
  result = gc_cons(heap, gc_make_stat(heap, "live",             (unsigned long)before.nodes_marked),  result);
  return result;
}
