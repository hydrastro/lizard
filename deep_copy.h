#ifndef LIZARD_DEEP_COPY_H
#define LIZARD_DEEP_COPY_H

#include "lizard.h"

lizard_ast_node_t *lizard_ast_deep_copy(lizard_ast_node_t *node,
                                        lizard_heap_t *heap);

#endif /* LIZARD_DEEP_COPY_H */
