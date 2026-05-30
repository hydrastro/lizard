#include "gc.h"
#include "gc_metadata.h"
#include "mem.h"
#include "test_harness.h"

#include <string.h>

int main(void) {
  lizard_gc_metadata_table_t *table;
  lizard_gc_metadata_stats_t stats;
  lizard_gc_object_kind_t kind;
  lizard_gc_object_kind_t inferred;
  lizard_object_trace_policy_t trace_policy;
  lizard_ast_node_t *node;
  lizard_ast_list_node_t *list_node;
  void *raw_ptr;
  lizard_heap_t *local_heap;
  size_t size;
  int marker;
  int ok;

  TEST_ASSERT_STR(lizard_gc_object_kind_name(LIZARD_GC_OBJECT_AST_NODE),
                  "ast-node");
  TEST_ASSERT_STR(lizard_gc_object_kind_name(LIZARD_GC_OBJECT_RAW), "raw");

  inferred = lizard_gc_infer_object_kind(align_size(sizeof(lizard_ast_node_t)));
  TEST_ASSERT_EQ(inferred, LIZARD_GC_OBJECT_AST_NODE);
  inferred = lizard_gc_infer_object_kind(17U);
  TEST_ASSERT_EQ(inferred, LIZARD_GC_OBJECT_RAW);

  table = lizard_gc_metadata_table_create();
  TEST_ASSERT(table != NULL);
  lizard_gc_metadata_collect_stats(table, &stats);
  TEST_ASSERT_EQ(stats.entries, 0U);
  TEST_ASSERT(stats.capacity > 0U);

  marker = 123;
  ok = lizard_gc_metadata_register(table, &marker, sizeof(marker),
                                   LIZARD_GC_OBJECT_RAW,
                                   LIZARD_OBJECT_OWNER_HEAP,
                                   LIZARD_OBJECT_TRACE_NONE);
  TEST_ASSERT(ok);
  kind = LIZARD_GC_OBJECT_UNKNOWN;
  size = 0U;
  trace_policy = LIZARD_OBJECT_TRACE_CUSTOM;
  ok = lizard_gc_metadata_lookup(table, &marker, &kind, &size, &trace_policy);
  TEST_ASSERT(ok);
  TEST_ASSERT_EQ(kind, LIZARD_GC_OBJECT_RAW);
  TEST_ASSERT_EQ(size, sizeof(marker));
  TEST_ASSERT_EQ(trace_policy, LIZARD_OBJECT_TRACE_NONE);
  lizard_gc_metadata_collect_stats(table, &stats);
  TEST_ASSERT_EQ(stats.entries, 1U);
  TEST_ASSERT_EQ(stats.bytes_registered, sizeof(marker));
  TEST_ASSERT_EQ(stats.trace_required, 0U);
  lizard_gc_metadata_table_destroy(table);

  local_heap = lizard_heap_create(4096U, 4096U);
  TEST_ASSERT(local_heap != NULL);
  heap = local_heap;
  node = (lizard_ast_node_t *)lizard_heap_alloc(sizeof(lizard_ast_node_t));
  TEST_ASSERT(node != NULL);
  lizard_gc_metadata_stats(local_heap, &stats);
  TEST_ASSERT(stats.entries > 0U);
  TEST_ASSERT(stats.ast_nodes > 0U);
  kind = LIZARD_GC_OBJECT_UNKNOWN;
  size = 0U;
  trace_policy = LIZARD_OBJECT_TRACE_NONE;
  ok = lizard_gc_metadata_lookup_object(local_heap, node, &kind, &size,
                                        &trace_policy);
  TEST_ASSERT(ok);
  TEST_ASSERT_EQ(kind, LIZARD_GC_OBJECT_AST_NODE);
  TEST_ASSERT_EQ(size, align_size(sizeof(lizard_ast_node_t)));
  TEST_ASSERT_EQ(trace_policy, LIZARD_OBJECT_TRACE_AST);

  list_node = (lizard_ast_list_node_t *)lizard_heap_alloc_tagged(
      sizeof(lizard_ast_list_node_t), LIZARD_GC_OBJECT_AST_LIST_NODE,
      LIZARD_OBJECT_TRACE_LIST);
  TEST_ASSERT(list_node != NULL);
  raw_ptr = lizard_heap_alloc_tagged(17U, LIZARD_GC_OBJECT_RAW,
                                     LIZARD_OBJECT_TRACE_NONE);
  TEST_ASSERT(raw_ptr != NULL);
  lizard_gc_metadata_stats(local_heap, &stats);
  TEST_ASSERT(stats.ast_list_nodes > 0U);
  TEST_ASSERT(stats.raw_objects > 0U);
  kind = LIZARD_GC_OBJECT_UNKNOWN;
  trace_policy = LIZARD_OBJECT_TRACE_NONE;
  ok = lizard_gc_metadata_lookup_object(local_heap, list_node, &kind, &size,
                                        &trace_policy);
  TEST_ASSERT(ok);
  TEST_ASSERT_EQ(kind, LIZARD_GC_OBJECT_AST_LIST_NODE);
  TEST_ASSERT_EQ(trace_policy, LIZARD_OBJECT_TRACE_LIST);
  lizard_heap_destroy(local_heap);
  heap = NULL;

  TEST_RETURN();
}
