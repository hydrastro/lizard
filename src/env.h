#ifndef LIZARD_ENV_H
#define LIZARD_ENV_H

#include "lizard.h"
#include <stdio.h>
#include <stdlib.h>

lizard_env_t *lizard_env_create(lizard_heap_t *heap, lizard_env_t *parent);
void lizard_env_define(lizard_heap_t *heap, lizard_env_t *env,
                       const char *symbol, lizard_ast_node_t *value);
lizard_ast_node_t *lizard_env_lookup(lizard_env_t *env, const char *symbol);
int lizard_env_set(lizard_env_t *env, const char *symbol,
                   lizard_ast_node_t *value);

#endif /* LIZARD_ENV_H */