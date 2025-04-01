#ifndef LIZARD_PARSER_H
#define LIZARD_PARSER_H

#include "lizard.h"

lizard_ast_node_t *lizard_get_canonical_nil(lizard_heap_t *heap);
lizard_ast_node_t *lizard_parse_expression(list_t *token_list,
                                           list_node_t **current_node_pointer,
                                           int *depth, lizard_heap_t *);
list_t *lizard_parse(list_t *token_list, lizard_heap_t *);

#endif /* LIZARD_PARSER_H */
