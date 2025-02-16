#ifndef LIZARD_HEAP_H
#define LIZARD_HEAP_H

#include "lizard.h"

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

#endif /* LIZARD_HEAP_H */