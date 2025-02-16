#include "env.h"
#include "lang.h"
#include "lizard.h"

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

lizard_heap_t *lizard_heap_create(size_t initial_size, size_t reserved_size) {
  lizard_heap_t *heap = malloc(sizeof(lizard_heap_t));
  if (!heap) {
    fprintf(stderr, "Error: Unable to allocate lizard_heap structure.\n");
    exit(1);
  }
  heap->reserved = reserved_size;
  heap->start = mmap(NULL, reserved_size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (heap->start == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }
  heap->top = heap->start;
  heap->end = heap->start + initial_size;
  return heap;
}

void lizard_heap_destroy(lizard_heap_t *heap) {
  if (heap) {
    if (heap->start) {
      munmap(heap->start, heap->reserved);
    }
    free(heap);
  }
}

void *lizard_heap_alloc(lizard_heap_t *heap, size_t size) {
  void *ptr;
  size_t alignment = sizeof(void *);
  size = (size + alignment - 1) & ~(alignment - 1);
  if (heap->top + size > heap->end) {
    size_t used = (size_t)(heap->top - heap->start);
    size_t current_committed = (size_t)(heap->end - heap->start);
    size_t new_committed = current_committed * 2;
    if (new_committed > heap->reserved)
      new_committed = heap->reserved;
    if (used + size > new_committed) {
      fprintf(stderr, "Error: Out of reserved memory in lizard_heap.\n");
      exit(1);
    }
    /* TODO: use mprotect */
    heap->end = heap->start + new_committed;
  }
  ptr = heap->top;
  heap->top += size;
  return ptr;
}

lizard_ast_node_t *lizard_make_bool(lizard_heap_t *heap, bool value) {
  lizard_ast_node_t *node;
  node = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
  node->type = AST_BOOL;
  node->data.boolean = value;
  return node;
}

lizard_ast_node_t *lizard_make_nil(lizard_heap_t *heap) {
  lizard_ast_node_t *node;
  node = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
  node->type = AST_NIL;
  return node;
}

lizard_ast_node_t *lizard_make_macro_def(lizard_heap_t *heap,
                                         lizard_ast_node_t *name,
                                         lizard_ast_node_t *transformer) {
  lizard_ast_node_t *node;
  node = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
  node->type = AST_MACRO;
  node->data.macro_def.variable = name;
  node->data.macro_def.transformer = transformer;
  return node;
}

lizard_ast_node_t *lizard_make_error(lizard_heap_t *heap, int error_code) {
  lizard_ast_list_node_t *msg_node;
  lizard_ast_node_t *node = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
  node->type = AST_ERROR;
  node->data.error.code = error_code;
  node->data.error.data = list_create();

  msg_node = lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
  msg_node->ast.type = AST_STRING;
  msg_node->ast.data.string = lizard_error_messages[LIZARD_LANG_EN][error_code];
  list_append(node->data.error.data, &msg_node->node);

  return node;
}
