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

#endif
