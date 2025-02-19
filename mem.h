#ifndef LIZARD_MEM_H
#define LIZARD_MEM_H

#include "lizard.h"

lizard_heap_segment_t *lizard_create_heap_segment(size_t size);
void *lizard_heap_realloc(void *ptr, size_t old_size, size_t new_size);
lizard_heap_t *lizard_heap_create(size_t initial_size, size_t reserved_size);
void lizard_heap_destroy(lizard_heap_t *heap);
void *lizard_heap_alloc(size_t size);
void lizard_heap_free(void *ptr);
void lizard_heap_free_wrapper(void *ptr, size_t size);
lizard_ast_node_t *lizard_make_primitive(lizard_heap_t *heap,
                                         lizard_primitive_func_t func);
lizard_ast_node_t *lizard_make_bool(lizard_heap_t *heap, bool value);
lizard_ast_node_t *lizard_make_nil(lizard_heap_t *heap);
lizard_ast_node_t *lizard_make_macro_def(lizard_heap_t *heap,
                                         lizard_ast_node_t *name,
                                         lizard_ast_node_t *transformer);
lizard_ast_node_t *lizard_make_error(lizard_heap_t *heap, int error_code);
lizard_ast_node_t *lizard_make_continuation(
    lizard_ast_node_t *(*current_cont)(lizard_ast_node_t *, lizard_env_t *,
                                       lizard_heap_t *),
    lizard_heap_t *heap);

#endif /* LIZARD_MEM_H */
