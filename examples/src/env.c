#include "env.h"
#include "lizard.h"
#include "mem.h"
#include <stdio.h>
#include <stdlib.h>

lizard_env_t *lizard_env_create(lizard_heap_t *heap, lizard_env_t *parent) {
  lizard_env_t *env = lizard_heap_alloc(sizeof(lizard_env_t));
  env->entries = NULL;
  env->parent = parent;
  return env;
}

void lizard_env_define(lizard_heap_t *heap, lizard_env_t *env,
                       const char *symbol, lizard_ast_node_t *value) {
  lizard_env_entry_t *entry = lizard_heap_alloc(sizeof(lizard_env_entry_t));
  entry->symbol = symbol;
  entry->value = value;
  entry->next = env->entries;
  env->entries = entry;
}

lizard_ast_node_t *lizard_env_lookup(lizard_env_t *env, const char *symbol) {
  lizard_env_t *e;
  lizard_env_entry_t *entry;
  for (e = env; e != NULL; e = e->parent) {
    for (entry = e->entries; entry != NULL; entry = entry->next) {
      if (strcmp(entry->symbol, symbol) == 0) {
        return entry->value;
      }
    }
  }
  return NULL;
}

int lizard_env_set(lizard_env_t *env, const char *symbol,
                   lizard_ast_node_t *value) {
  lizard_env_t *e;
  lizard_env_entry_t *entry;
  for (e = env; e != NULL; e = e->parent) {
    for (entry = e->entries; entry != NULL; entry = entry->next) {
      if (strcmp(entry->symbol, symbol) == 0) {
        entry->value = value;
        return 1;
      }
    }
  }
  return 0;
}
