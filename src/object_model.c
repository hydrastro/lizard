/* src/object_model.c -- Phase 3A ownership vocabulary.
 *
 * This is intentionally metadata-only.  It documents and tests the ownership
 * model before the object-level non-moving mark/sweep transition mutates heap
 * layout.  No allocation behavior changes in Phase 3A.
 */

#include "object_model.h"

static void object_model_clear(lizard_object_model_info_t *out_info) {
  if (out_info != NULL) {
    out_info->name = NULL;
    out_info->owner = LIZARD_OBJECT_OWNER_UNKNOWN;
    out_info->trace_policy = LIZARD_OBJECT_TRACE_NONE;
    out_info->stable_address = 0;
    out_info->movable = 0;
    out_info->explicitly_freed = 0;
  }
}

static void object_model_set(lizard_object_model_info_t *out_info,
                             const char *name,
                             lizard_object_owner_t owner,
                             lizard_object_trace_policy_t trace_policy,
                             int stable_address, int movable,
                             int explicitly_freed) {
  object_model_clear(out_info);
  if (out_info != NULL) {
    out_info->name = name;
    out_info->owner = owner;
    out_info->trace_policy = trace_policy;
    out_info->stable_address = stable_address;
    out_info->movable = movable;
    out_info->explicitly_freed = explicitly_freed;
  }
}

const char *lizard_object_owner_name(lizard_object_owner_t owner) {
  switch (owner) {
  case LIZARD_OBJECT_OWNER_UNKNOWN:
    return "unknown";
  case LIZARD_OBJECT_OWNER_HEAP:
    return "heap";
  case LIZARD_OBJECT_OWNER_C_MALLOC:
    return "c-malloc";
  case LIZARD_OBJECT_OWNER_BORROWED:
    return "borrowed";
  case LIZARD_OBJECT_OWNER_STATIC:
    return "static";
  case LIZARD_OBJECT_OWNER_CONTEXT:
    return "context";
  }
  return "unknown";
}

const char *lizard_object_trace_policy_name(
    lizard_object_trace_policy_t policy) {
  switch (policy) {
  case LIZARD_OBJECT_TRACE_NONE:
    return "none";
  case LIZARD_OBJECT_TRACE_AST:
    return "ast";
  case LIZARD_OBJECT_TRACE_ENV:
    return "env";
  case LIZARD_OBJECT_TRACE_LIST:
    return "list";
  case LIZARD_OBJECT_TRACE_CUSTOM:
    return "custom";
  }
  return "none";
}

int lizard_object_owner_is_runtime_managed(lizard_object_owner_t owner) {
  return owner == LIZARD_OBJECT_OWNER_HEAP ||
         owner == LIZARD_OBJECT_OWNER_CONTEXT;
}

int lizard_object_trace_policy_requires_mark(
    lizard_object_trace_policy_t policy) {
  return policy == LIZARD_OBJECT_TRACE_AST ||
         policy == LIZARD_OBJECT_TRACE_ENV ||
         policy == LIZARD_OBJECT_TRACE_LIST ||
         policy == LIZARD_OBJECT_TRACE_CUSTOM;
}

void lizard_object_model_ast_node(lizard_object_model_info_t *out_info) {
  object_model_set(out_info, "ast-node", LIZARD_OBJECT_OWNER_HEAP,
                   LIZARD_OBJECT_TRACE_AST, 1, 0, 0);
}

void lizard_object_model_heap_list_node(lizard_object_model_info_t *out_info) {
  object_model_set(out_info, "heap-list-node", LIZARD_OBJECT_OWNER_HEAP,
                   LIZARD_OBJECT_TRACE_LIST, 1, 0, 0);
}

void lizard_object_model_report_object(lizard_object_model_info_t *out_info) {
  object_model_set(out_info, "report-object", LIZARD_OBJECT_OWNER_C_MALLOC,
                   LIZARD_OBJECT_TRACE_NONE, 1, 0, 1);
}

void lizard_object_model_context_object(lizard_object_model_info_t *out_info) {
  object_model_set(out_info, "context-object", LIZARD_OBJECT_OWNER_CONTEXT,
                   LIZARD_OBJECT_TRACE_CUSTOM, 1, 0, 1);
}
