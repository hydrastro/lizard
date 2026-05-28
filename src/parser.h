#ifndef LIZARD_PARSER_H
#define LIZARD_PARSER_H

#include "lizard_internal.h"

lizard_ast_node_t *lizard_get_canonical_nil(lizard_heap_t *heap);
lizard_ast_node_t *lizard_parse_expression(lz_list_t *token_list,
                                           lz_list_node_t **current_node_pointer,
                                           int *depth, lizard_heap_t *);
lz_list_t *lizard_parse(lz_list_t *token_list, lizard_heap_t *);

lizard_ast_node_t *lizard_parse_datum(lz_list_t *token_list,
                                      lz_list_node_t **cur,
                                      lizard_heap_t *heap);

/* Returns the last parser diagnostic if the most recent lizard_parse()
 * failed (returned NULL), or NULL if it succeeded. */
const lizard_diagnostic_t *lizard_parser_last_diagnostic(void);

#endif /* LIZARD_PARSER_H */
