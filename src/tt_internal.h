/* tt_internal.h — internal helpers shared between the tt conversion engine
 * (tt_equality.c) and the extracted tt registry/config module (tt_logic.c).
 * Not part of the public API. */
#ifndef LIZARD_TT_INTERNAL_H
#define LIZARD_TT_INTERNAL_H

#include "lizard_internal.h"

int contains_free_var(lizard_ast_node_t *t, const char *name);
int universe_set_subset(lizard_ast_node_t *a, lizard_ast_node_t *b);
int couniverse_set_subset(lizard_ast_node_t *a, lizard_ast_node_t *b);

typedef struct lizard_tt_flag {
  const char *name;
  int value;
  struct lizard_tt_flag *next;
} lizard_tt_flag_t;

lizard_tt_flag_t **flag_list_ptr(void);

#endif
