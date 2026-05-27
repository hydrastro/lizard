#ifndef LIZARD_RUNTIME_H
#define LIZARD_RUNTIME_H

#include "lizard_internal.h"
#include "lizard_api.h"

struct lizard_runtime {
  lizard_heap_t *heap;
  size_t initial_heap_size;
  size_t max_segment_size;
  char last_error[256];
  lizard_diagnostic_t diagnostic;
};

struct lizard_context {
  lizard_runtime_t *runtime;
  lizard_env_t *env;
  lizard_ast_node_t *last_value;
  char last_error[256];
  lizard_diagnostic_t diagnostic;
};

void lizard_runtime_set_error(lizard_runtime_t *runtime, const char *message);
void lizard_context_set_error(lizard_context_t *context, const char *message);

#endif /* LIZARD_RUNTIME_H */
