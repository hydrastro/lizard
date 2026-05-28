#ifndef LIZARD_TT_GLUE_H
#define LIZARD_TT_GLUE_H

#include "lizard_internal.h"

lizard_ast_node_t *lizard_tt_make_equiv_type(lizard_heap_t *heap,
                                             lizard_ast_node_t *A,
                                             lizard_ast_node_t *B);
lizard_ast_node_t *lizard_tt_make_id_equiv(lizard_heap_t *heap,
                                           lizard_ast_node_t *A);
lizard_ast_node_t *lizard_tt_make_equiv_fun(lizard_heap_t *heap,
                                            lizard_ast_node_t *e);
lizard_ast_node_t *lizard_tt_make_equiv_inv(lizard_heap_t *heap,
                                            lizard_ast_node_t *e);
lizard_ast_node_t *lizard_tt_make_glue(lizard_heap_t *heap,
                                       lizard_ast_node_t *A,
                                       lizard_ast_node_t *face,
                                       lizard_ast_node_t *T,
                                       lizard_ast_node_t *e);
lizard_ast_node_t *lizard_tt_make_glue_intro(lizard_heap_t *heap,
                                             lizard_ast_node_t *face,
                                             lizard_ast_node_t *t,
                                             lizard_ast_node_t *a);
lizard_ast_node_t *lizard_tt_make_unglue(lizard_heap_t *heap,
                                         lizard_ast_node_t *e,
                                         lizard_ast_node_t *g);
lizard_ast_node_t *lizard_tt_make_ua(lizard_heap_t *heap,
                                     lizard_ast_node_t *e);
lizard_ast_node_t *lizard_tt_make_system_nil(lizard_heap_t *heap);
lizard_ast_node_t *lizard_tt_make_system_cons(lizard_heap_t *heap,
                                              lizard_ast_node_t *face,
                                              lizard_ast_node_t *value,
                                              lizard_ast_node_t *next);

#endif /* LIZARD_TT_GLUE_H */
