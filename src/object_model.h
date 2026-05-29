#ifndef LIZARD_OBJECT_MODEL_H
#define LIZARD_OBJECT_MODEL_H

#include "lizard_api.h"
#include <stddef.h>

/* Phase 3A: runtime/object ownership model scaffold.
 *
 * This does not change allocation or collection behavior yet.  It gives the
 * runtime a small vocabulary for describing where a value/object lives and
 * whether future object-level non-moving GC must trace it.
 */
typedef enum lizard_object_owner {
  LIZARD_OBJECT_OWNER_UNKNOWN = 0,
  LIZARD_OBJECT_OWNER_HEAP = 1,
  LIZARD_OBJECT_OWNER_C_MALLOC = 2,
  LIZARD_OBJECT_OWNER_BORROWED = 3,
  LIZARD_OBJECT_OWNER_STATIC = 4,
  LIZARD_OBJECT_OWNER_CONTEXT = 5
} lizard_object_owner_t;

typedef enum lizard_object_trace_policy {
  LIZARD_OBJECT_TRACE_NONE = 0,
  LIZARD_OBJECT_TRACE_AST = 1,
  LIZARD_OBJECT_TRACE_ENV = 2,
  LIZARD_OBJECT_TRACE_LIST = 3,
  LIZARD_OBJECT_TRACE_CUSTOM = 4
} lizard_object_trace_policy_t;

typedef struct lizard_object_model_info {
  const char *name;
  lizard_object_owner_t owner;
  lizard_object_trace_policy_t trace_policy;
  int stable_address;
  int movable;
  int explicitly_freed;
} lizard_object_model_info_t;

const char *lizard_object_owner_name(lizard_object_owner_t owner);
const char *lizard_object_trace_policy_name(
    lizard_object_trace_policy_t policy);
int lizard_object_owner_is_runtime_managed(lizard_object_owner_t owner);
int lizard_object_trace_policy_requires_mark(
    lizard_object_trace_policy_t policy);
void lizard_object_model_ast_node(lizard_object_model_info_t *out_info);
void lizard_object_model_heap_list_node(lizard_object_model_info_t *out_info);
void lizard_object_model_report_object(lizard_object_model_info_t *out_info);
void lizard_object_model_context_object(lizard_object_model_info_t *out_info);

#endif /* LIZARD_OBJECT_MODEL_H */
