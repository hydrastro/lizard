#ifndef LIZARD_RUNTIME_H
#define LIZARD_RUNTIME_H

#include "lizard_internal.h"
#include "lizard_api.h"
#include <setjmp.h>

/* Forward declarations for opaque internal types whose linked-list
 * heads live in the runtime (Phase 0 Turn B.2). Definitions remain
 * in tt_equality.c. */
struct logic_rule_entry;
struct hit_registry_entry;
struct lizard_tt_flag;

struct lizard_runtime {
  lizard_heap_t *heap;
  size_t initial_heap_size;
  size_t max_segment_size;
  char last_error[256];
  lizard_diagnostic_t diagnostic;
  /* Phase 0 B.1: counters and callcc state. */
  unsigned long gensym_counter;
  unsigned long sr_counter;
  jmp_buf callcc_buf;
  int callcc_active;
  lizard_ast_node_t *callcc_value;
  /* Phase 0 B.2: logic config, HIT registry, normalization flags. */
  struct logic_rule_entry *logic_config_head;
  const char *logic_last_set_bundle;
  struct hit_registry_entry *hit_registry_head;
  struct lizard_tt_flag *flag_list;
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
