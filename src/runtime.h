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

/* Phase C: module loader. */
typedef struct lizard_module_entry {
  char *canonical_path;         /* absolute or normalized path */
  lizard_ast_node_t *result;    /* last value from evaluating the module */
  struct lizard_module_entry *next;
} lizard_module_entry_t;

typedef struct lizard_search_path {
  char *directory;
  struct lizard_search_path *next;
} lizard_search_path_t;

/* Phase 3: module namespaces — real modules with isolated frames and export
 * control.  A namespace is a module's hermetic environment plus the set of
 * names it makes public (NULL exports => export everything defined at top
 * level). */
typedef struct lizard_export {
  char *name;
  struct lizard_export *next;
} lizard_export_t;

typedef struct lizard_namespace {
  char *name;
  lizard_env_t *env;            /* the module's own hermetic frame */
  lizard_export_t *exports;     /* explicit exports; NULL => export all */
  struct lizard_namespace *next;
} lizard_namespace_t;

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
  /* Phase C: module loader. */
  lizard_module_entry_t *modules_head;
  lizard_search_path_t *search_path_head;
  /* Phase 3: per-runtime kernel/proof state (isolation).  Opaque to avoid
   * pulling kernel.h/tactics.h into this header; primitives.c casts them to
   * proof_state_t* / meta_ctx_t* / kdef_ctx_t*. */
  void *kernel_proof_state;
  void *kernel_meta_ctx;
  void *kernel_def_ctx;
  /* Phase 3: module namespaces. */
  lizard_namespace_t *namespaces_head;
  lizard_namespace_t *current_module;   /* module being evaluated (for export) */
  lizard_env_t *module_base_env;        /* pristine primitive env for hermetic modules */
};

struct lizard_context {
  lizard_runtime_t *runtime;
  lizard_env_t *env;
  lizard_ast_node_t *last_value;
  char last_error[256];
  lizard_diagnostic_t diagnostic;
  /* Phase 2I: optional traced macro-expansion path. */
  int trace_expansion;
  struct lizard_surface_term *last_surface;
  struct lizard_surface_term *last_expanded_surface;
};

void lizard_runtime_set_error(lizard_runtime_t *runtime, const char *message);
void lizard_context_set_error(lizard_context_t *context, const char *message);
lizard_runtime_t *lizard_runtime_current(void);

#endif /* LIZARD_RUNTIME_H */
