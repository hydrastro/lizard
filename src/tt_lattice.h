#ifndef LIZARD_TT_LATTICE_H
#define LIZARD_TT_LATTICE_H

#include "lizard_internal.h"

lizard_ast_node_t *lizard_tt_make_u_suc(lizard_heap_t *heap,
                                        lizard_ast_node_t *u);
lizard_ast_node_t *lizard_tt_make_u_max(lizard_heap_t *heap,
                                        lizard_ast_node_t *u,
                                        lizard_ast_node_t *v);
lizard_ast_node_t *lizard_tt_make_u_min(lizard_heap_t *heap,
                                        lizard_ast_node_t *u,
                                        lizard_ast_node_t *v);
lizard_ast_node_t *lizard_tt_make_universe_set(lizard_heap_t *heap,
                                               long *dims, long count);
lizard_ast_node_t *lizard_tt_universe_set_union(lizard_heap_t *heap,
                                                lizard_ast_node_t *a,
                                                lizard_ast_node_t *b);
lizard_ast_node_t *lizard_tt_universe_set_intersect(lizard_heap_t *heap,
                                                    lizard_ast_node_t *a,
                                                    lizard_ast_node_t *b);
int lizard_tt_universe_set_subset(lizard_ast_node_t *a,
                                  lizard_ast_node_t *b);

lizard_ast_node_t *lizard_tt_make_couniverse_set(lizard_heap_t *heap,
                                                 long *dims, long count);
lizard_ast_node_t *lizard_tt_make_co_max(lizard_heap_t *heap,
                                         lizard_ast_node_t *u,
                                         lizard_ast_node_t *v);
lizard_ast_node_t *lizard_tt_make_co_min(lizard_heap_t *heap,
                                         lizard_ast_node_t *u,
                                         lizard_ast_node_t *v);
lizard_ast_node_t *lizard_tt_couniverse_set_union(lizard_heap_t *heap,
                                                  lizard_ast_node_t *a,
                                                  lizard_ast_node_t *b);
lizard_ast_node_t *lizard_tt_couniverse_set_intersect(lizard_heap_t *heap,
                                                      lizard_ast_node_t *a,
                                                      lizard_ast_node_t *b);
int lizard_tt_couniverse_set_subset(lizard_ast_node_t *a,
                                    lizard_ast_node_t *b);

#endif /* LIZARD_TT_LATTICE_H */
