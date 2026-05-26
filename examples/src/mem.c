#include "mem.h"
#include "env.h"
#include "lang.h"
#include "lizard.h"

extern lizard_heap_t *heap;

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
  return heap;
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

void *lizard_heap_alloc(size_t size) {
  lizard_heap_segment_t *seg;
  void *ptr;
  size_t current_seg_size, new_segment_size;

  size = align_size(size);

  seg = heap->current;
  if (seg->top + size > seg->end) {
    current_seg_size = (size_t)(seg->end - seg->start);
    new_segment_size = current_seg_size * 2;
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
  return ptr;
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
  free(heap);
}

void lizard_heap_free(void *ptr) { (void)ptr; }

void lizard_heap_free_wrapper(void *ptr, size_t size) { (void)ptr; }

lizard_ast_node_t *lizard_make_bool(lizard_heap_t *heap, bool value) {
  lizard_ast_node_t *node;
  node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  node->type = AST_BOOL;
  node->data.boolean = value;
  return node;
}

lizard_ast_node_t *lizard_make_nil(lizard_heap_t *heap) {
  lizard_ast_node_t *node;
  node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  node->type = AST_NIL;
  return node;
}

lizard_ast_node_t *lizard_make_macro_def(lizard_heap_t *heap,
                                         lizard_ast_node_t *name,
                                         lizard_ast_node_t *transformer) {
  lizard_ast_node_t *node;
  node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  node->type = AST_MACRO;
  node->data.macro_def.variable = name;
  node->data.macro_def.transformer = transformer;
  return node;
}

lizard_ast_node_t *lizard_make_error(lizard_heap_t *heap, int error_code) {
  lizard_ast_list_node_t *msg_node;
  lizard_ast_node_t *node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  node->type = AST_ERROR;
  node->data.error.code = error_code;
  node->data.error.data =
      list_create_alloc(lizard_heap_alloc, lizard_heap_free);

  msg_node = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
  msg_node->ast = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  msg_node->ast->type = AST_STRING;
  msg_node->ast->data.string =
      lizard_error_messages[LIZARD_LANG_EN][error_code];
  list_append(node->data.error.data, &msg_node->node);

  return node;
}

lizard_ast_node_t *lizard_make_continuation(
    lizard_ast_node_t *(*current_cont)(lizard_ast_node_t *, lizard_env_t *,
                                       lizard_heap_t *),
    lizard_heap_t *heap) {
  lizard_ast_node_t *cont_obj = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  cont_obj->type = AST_CONTINUATION;
  cont_obj->data.continuation.captured_cont = current_cont;
  return cont_obj;
}
