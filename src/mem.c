#include "mem.h"
#include "env.h"
#include "gc_metadata.h"
#include "lang.h"
#include "lizard_internal.h"

static const char *lizard_error_messages_lang_en[LIZARD_ERROR_COUNT] = {
#define X(fst, snd) snd,
    LIZARD_ERROR_MESSAGES_EN
#undef X
};

static const char **lizard_error_messages[LIZARD_LANG_COUNT] = {
#define X(fst, snd) snd,
    LIZARD_ERROR_MESSAGES
#undef X
};

size_t align_size(size_t size) {
  size_t alignment = sizeof(void *);
  return (size + alignment - 1) & ~(alignment - 1);
}

lizard_heap_segment_t *lizard_create_heap_segment(size_t size) {
  lizard_heap_segment_t *seg;
  seg = (lizard_heap_segment_t *)malloc(sizeof(lizard_heap_segment_t));
  if (seg == NULL) {
    fprintf(stderr, "Error: Unable to allocate lizard_heap_segment_t\n");
    exit(1);
  }
  seg->start = mmap(NULL, size, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (seg->start == MAP_FAILED) {
    fprintf(stderr, "mmap failed for segment size %lu\n", size);
    perror("mmap");
    exit(1);
  }
  seg->top = seg->start;
  seg->end = seg->start + size;
  seg->next = NULL;
  return seg;
}

lizard_heap_t *lizard_heap_create(size_t initial_size,
                                  size_t max_segment_size) {
  lizard_heap_t *heap;
  heap = (lizard_heap_t *)malloc(sizeof(lizard_heap_t));
  if (heap == NULL) {
    fprintf(stderr, "Error: Unable to allocate lizard_heap_t\n");
    exit(1);
  }
  heap->initial_size = initial_size;
  heap->max_segment_size = max_segment_size;
  heap->head = lizard_create_heap_segment(initial_size);
  heap->current = heap->head;
  heap->runtime = NULL;  /* Phase 0: set by lizard_runtime_create */
  heap->gc_metadata = lizard_gc_metadata_table_create();
  heap->free_buckets = NULL;
  heap->free_bucket_count = 0U;
  heap->free_bucket_cap = 0U;
  return heap;
}

/* Phase 5: per-size free list of reclaimed chunks.  A freed chunk stores the
 * next-pointer in its first word; buckets are keyed by exact aligned size. */
static struct lizard_free_bucket *freelist_bucket(lizard_heap_t *h, size_t size) {
  size_t i;
  for (i = 0; i < h->free_bucket_count; i++) {
    if (h->free_buckets[i].size == size) return &h->free_buckets[i];
  }
  if (h->free_bucket_count == h->free_bucket_cap) {
    size_t ncap = h->free_bucket_cap ? h->free_bucket_cap * 2U : 16U;
    struct lizard_free_bucket *p = (struct lizard_free_bucket *)realloc(
        h->free_buckets, ncap * sizeof(struct lizard_free_bucket));
    if (p == NULL) return NULL;
    h->free_buckets = p;
    h->free_bucket_cap = ncap;
  }
  h->free_buckets[h->free_bucket_count].size = size;
  h->free_buckets[h->free_bucket_count].head = NULL;
  return &h->free_buckets[h->free_bucket_count++];
}

void lizard_heap_reclaim(void *ptr, size_t size) {
  struct lizard_free_bucket *b;
  if (heap == NULL || ptr == NULL) return;
  b = freelist_bucket(heap, size);
  if (b == NULL) return;           /* OOM growing buckets: just drop it */
  *(void **)ptr = b->head;         /* link this chunk into the bucket */
  b->head = ptr;
}

static void *freelist_take(lizard_heap_t *h, size_t size) {
  size_t i;
  for (i = 0; i < h->free_bucket_count; i++) {
    if (h->free_buckets[i].size == size && h->free_buckets[i].head != NULL) {
      void *p = h->free_buckets[i].head;
      h->free_buckets[i].head = *(void **)p;
      return p;
    }
  }
  return NULL;
}
void *lizard_heap_realloc(void *ptr, size_t old_size, size_t new_size) {
  lizard_heap_segment_t *seg;
  void *new_ptr;
  size_t additional;
  size_t copy_size;

  new_size = align_size(new_size);

  if (ptr == NULL) {
    return lizard_heap_alloc(new_size);
  }
  if (new_size == 0) {
    return NULL;
  }

  seg = heap->current;
  if ((char *)ptr + old_size == seg->top) {
    additional = new_size - old_size;
    if (seg->top + additional <= seg->end) {
      seg->top += additional;
      return ptr;
    }
  }

  new_ptr = lizard_heap_alloc(new_size);
  if (new_ptr == NULL) {
    return NULL;
  }
  copy_size = old_size < new_size ? old_size : new_size;
  memcpy(new_ptr, ptr, copy_size);
  return new_ptr;
}

static lizard_object_trace_policy_t lizard_heap_default_trace_policy(
    lizard_gc_object_kind_t kind) {
  switch (kind) {
  case LIZARD_GC_OBJECT_AST_NODE:
    return LIZARD_OBJECT_TRACE_AST;
  case LIZARD_GC_OBJECT_AST_LIST_NODE:
    return LIZARD_OBJECT_TRACE_LIST;
  case LIZARD_GC_OBJECT_ENV:
  case LIZARD_GC_OBJECT_ENV_ENTRY:
    return LIZARD_OBJECT_TRACE_ENV;
  case LIZARD_GC_OBJECT_KERNEL_TERM:
    return LIZARD_OBJECT_TRACE_CUSTOM;
  case LIZARD_GC_OBJECT_UNKNOWN:
  case LIZARD_GC_OBJECT_RAW:
    return LIZARD_OBJECT_TRACE_NONE;
  }
  return LIZARD_OBJECT_TRACE_NONE;
}

void *lizard_heap_alloc_tagged(size_t size,
                               lizard_gc_object_kind_t kind,
                               lizard_object_trace_policy_t trace_policy) {
  lizard_heap_segment_t *seg;
  void *ptr;
  size_t current_seg_size;
  size_t new_segment_size;

  size = align_size(size);
  if (kind == LIZARD_GC_OBJECT_UNKNOWN) {
    kind = lizard_gc_infer_object_kind(size);
  }
  if (trace_policy == LIZARD_OBJECT_TRACE_CUSTOM &&
      kind != LIZARD_GC_OBJECT_KERNEL_TERM) {
    trace_policy = lizard_heap_default_trace_policy(kind);
  }

  /* Phase 5: reuse a previously reclaimed chunk of the same size if one is
   * available, before extending the bump pointer. */
  ptr = freelist_take(heap, size);
  if (ptr != NULL) {
    memset(ptr, 0, size);
    if (heap->gc_metadata != NULL) {
      (void)lizard_gc_metadata_register(heap->gc_metadata, ptr, size, kind,
                                        LIZARD_OBJECT_OWNER_HEAP, trace_policy);
    }
    return ptr;
  }

  seg = heap->current;
  if (seg->top + size > seg->end) {
    current_seg_size = (size_t)(seg->end - seg->start);
    new_segment_size = current_seg_size * 2U;
    if (new_segment_size < size) {
      new_segment_size = size;
    }
    if (new_segment_size > heap->max_segment_size) {
      new_segment_size = heap->max_segment_size;
    }
    seg->next = lizard_create_heap_segment(new_segment_size);
    heap->current = seg->next;
    seg = heap->current;
  }
  ptr = seg->top;
  seg->top += size;
  memset(ptr, 0, size);
  if (heap->gc_metadata != NULL) {
    (void)lizard_gc_metadata_register(heap->gc_metadata, ptr, size, kind,
                                      LIZARD_OBJECT_OWNER_HEAP, trace_policy);
  }
  return ptr;
}

void *lizard_heap_alloc(size_t size) {
  size_t aligned_size;
  lizard_gc_object_kind_t kind;

  aligned_size = align_size(size);
  kind = lizard_gc_infer_object_kind(aligned_size);
  return lizard_heap_alloc_tagged(size, kind,
                                  lizard_heap_default_trace_policy(kind));
}

void lizard_heap_destroy(lizard_heap_t *heap) {
  lizard_heap_segment_t *seg;
  lizard_heap_segment_t *next;
  size_t seg_size;

  seg = heap->head;
  while (seg != NULL) {
    next = seg->next;
    seg_size = (size_t)(seg->end - seg->start);
    munmap(seg->start, seg_size);
    free(seg);
    seg = next;
  }
  lizard_gc_metadata_table_destroy(heap->gc_metadata);
  free(heap);
}

void lizard_heap_free(void *ptr) { (void)ptr; }

void lizard_heap_free_wrapper(void *ptr, size_t size) { (void)ptr; }

lizard_ast_node_t *lizard_make_bool(lizard_heap_t *heap, bool value) {
  lizard_ast_node_t *node;
  node = lizard_heap_alloc_tagged(sizeof(lizard_ast_node_t),
                                LIZARD_GC_OBJECT_AST_NODE,
                                LIZARD_OBJECT_TRACE_AST);
  node->type = AST_BOOL;
  node->data.boolean = value;
  return node;
}

lizard_ast_node_t *lizard_make_nil(lizard_heap_t *heap) {
  lizard_ast_node_t *node;
  node = lizard_heap_alloc_tagged(sizeof(lizard_ast_node_t),
                                LIZARD_GC_OBJECT_AST_NODE,
                                LIZARD_OBJECT_TRACE_AST);
  node->type = AST_NIL;
  return node;
}

lizard_ast_node_t *lizard_make_real(lizard_heap_t *heap, double value) {
  lizard_ast_node_t *node;
  node = lizard_heap_alloc_tagged(sizeof(lizard_ast_node_t),
                                LIZARD_GC_OBJECT_AST_NODE,
                                LIZARD_OBJECT_TRACE_AST);
  node->type = AST_REAL;
  node->data.real = value;
  return node;
}

lizard_ast_node_t *lizard_make_macro_def(lizard_heap_t *heap,
                                         lizard_ast_node_t *name,
                                         lizard_ast_node_t *transformer) {
  lizard_ast_node_t *node;
  node = lizard_heap_alloc_tagged(sizeof(lizard_ast_node_t),
                                LIZARD_GC_OBJECT_AST_NODE,
                                LIZARD_OBJECT_TRACE_AST);
  node->type = AST_MACRO;
  node->data.macro_def.variable = name;
  node->data.macro_def.transformer = transformer;
  return node;
}

lizard_ast_node_t *lizard_make_error(lizard_heap_t *heap, int error_code) {
  lizard_ast_list_node_t *msg_node;
  lizard_ast_node_t *node = lizard_heap_alloc_tagged(sizeof(lizard_ast_node_t),
                                LIZARD_GC_OBJECT_AST_NODE,
                                LIZARD_OBJECT_TRACE_AST);
  node->type = AST_ERROR;
  node->data.error.code = error_code;
  node->data.error.data =
      list_create_alloc(lizard_heap_alloc, lizard_heap_free);

  msg_node = lizard_heap_alloc_tagged(sizeof(lizard_ast_list_node_t),
                                     LIZARD_GC_OBJECT_AST_LIST_NODE,
                                     LIZARD_OBJECT_TRACE_LIST);
  msg_node->ast = lizard_heap_alloc_tagged(sizeof(lizard_ast_node_t),
                                          LIZARD_GC_OBJECT_AST_NODE,
                                          LIZARD_OBJECT_TRACE_AST);
  msg_node->ast->type = AST_STRING;
  msg_node->ast->data.string =
      lizard_error_messages[LIZARD_LANG_EN][error_code];
  list_append(node->data.error.data, &msg_node->node);

  return node;
}

/* Phase F: structured diagnostics — error with source location. */
lizard_ast_node_t *lizard_make_error_at(lizard_heap_t *heap, int error_code,
                                         lizard_source_span_t span) {
  lizard_ast_node_t *node = lizard_make_error(heap, error_code);
  node->span = span;
  return node;
}

lizard_ast_node_t *lizard_make_continuation(
    lizard_ast_node_t *(*current_cont)(lizard_ast_node_t *, lizard_env_t *,
                                       lizard_heap_t *),
    lizard_heap_t *heap) {
  lizard_ast_node_t *cont_obj = lizard_heap_alloc_tagged(
      sizeof(lizard_ast_node_t), LIZARD_GC_OBJECT_AST_NODE,
      LIZARD_OBJECT_TRACE_AST);
  cont_obj->type = AST_CONTINUATION;
  cont_obj->data.continuation.captured_cont = current_cont;
  return cont_obj;
}
