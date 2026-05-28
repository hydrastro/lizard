#include "primitives.h"
#include "prims_common.h"
#include "mem.h"
#include "errors.h"

#include "gc.h"

/* Split from primitives.c as part of Recoverable Core file hygiene. */

lizard_ast_node_t *lizard_primitive_gc_stats(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_gc_stats_t stats;
  lizard_ast_node_t *result;
  (void)args;

  lizard_gc_collect_stats(heap, env, 1, &stats);

  /* Build alist: ((segments . N) (bytes . N) (total . N) (live . N) (garbage . N)) */
  result = lizard_make_nil(heap);
  result = lizard_prim_cons(heap, lizard_prim_make_stat(heap, "garbage",  (unsigned long)stats.nodes_garbage), result);
  result = lizard_prim_cons(heap, lizard_prim_make_stat(heap, "live",     (unsigned long)stats.nodes_marked),  result);
  result = lizard_prim_cons(heap, lizard_prim_make_stat(heap, "total",    (unsigned long)stats.nodes_total),   result);
  result = lizard_prim_cons(heap, lizard_prim_make_stat(heap, "bytes",    (unsigned long)stats.total_bytes_allocated), result);
  result = lizard_prim_cons(heap, lizard_prim_make_stat(heap, "segments", (unsigned long)stats.total_segments), result);
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
  result = lizard_prim_cons(heap, lizard_prim_make_stat(heap, "freed-bytes",      (unsigned long)freed),                result);
  result = lizard_prim_cons(heap, lizard_prim_make_stat(heap, "segments-after",   (unsigned long)after.total_segments), result);
  result = lizard_prim_cons(heap, lizard_prim_make_stat(heap, "segments-before",  (unsigned long)before.total_segments),result);
  result = lizard_prim_cons(heap, lizard_prim_make_stat(heap, "garbage",          (unsigned long)before.nodes_garbage), result);
  result = lizard_prim_cons(heap, lizard_prim_make_stat(heap, "live",             (unsigned long)before.nodes_marked),  result);
  return result;
}

lizard_ast_node_t *lizard_primitive_error_location(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  lizard_ast_node_t *err;
  lizard_ast_node_t *result;
  (void)env;
  if (!lizard_prim_single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  err = ((lizard_ast_list_node_t *)args->head)->ast;
  if (err == NULL || err->type != AST_ERROR) {
    return lizard_make_nil(heap);
  }
  if (err->span.start_line == 0) {
    return lizard_make_nil(heap);
  }
  /* Build alist: ((line . N) (column . N)) */
  result = lizard_make_nil(heap);
  result = lizard_prim_cons(heap, lizard_prim_make_stat(heap, "column", (unsigned long)err->span.start_column), result);
  result = lizard_prim_cons(heap, lizard_prim_make_stat(heap, "line",   (unsigned long)err->span.start_line),   result);
  return result;
}
