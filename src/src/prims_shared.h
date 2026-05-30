/* prims_shared.h — internal helpers shared between primitives.c and the
 * extracted prims_*.c modules (#7 monolith split). Not part of the public API. */
#ifndef LIZARD_PRIMS_SHARED_H
#define LIZARD_PRIMS_SHARED_H

#include "lizard_internal.h"

int no_args(lz_list_t *args);
int single_arg(lz_list_t *args);
int two_args(lz_list_t *args);
int three_args(lz_list_t *args);
int four_args(lz_list_t *args);
lizard_ast_node_t *nth_arg(lz_list_t *args, int n);
int lizard_rule_on(const char *name);

lizard_ast_node_t *make_vector(lizard_heap_t *heap, size_t n, lizard_ast_node_t *fill);
lizard_ast_node_t *make_string(lizard_heap_t *heap, const char *src, size_t len);
lizard_ast_node_t *make_symbol(lizard_heap_t *heap, const char *name);
lizard_ast_node_t *gc_make_stat(lizard_heap_t *heap, const char *key, unsigned long val);
lizard_ast_node_t *gc_cons(lizard_heap_t *heap, lizard_ast_node_t *car, lizard_ast_node_t *cdr);
#endif
