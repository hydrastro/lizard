#ifndef LIZARD_PRIMS_COMMON_H
#define LIZARD_PRIMS_COMMON_H

#include "lizard_internal.h"

int lizard_prim_no_args(lz_list_t *args);
int lizard_prim_single_arg(lz_list_t *args);
int lizard_prim_two_args(lz_list_t *args);
int lizard_prim_three_args(lz_list_t *args);
int lizard_prim_four_args(lz_list_t *args);
lizard_ast_node_t *lizard_prim_nth_arg(lz_list_t *args, int n);
lizard_ast_node_t *lizard_prim_make_stat(lizard_heap_t *heap,
                                         const char *key,
                                         unsigned long val);
lizard_ast_node_t *lizard_prim_cons(lizard_heap_t *heap,
                                    lizard_ast_node_t *car,
                                    lizard_ast_node_t *cdr);

#endif /* LIZARD_PRIMS_COMMON_H */
