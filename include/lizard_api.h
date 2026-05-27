#ifndef LIZARD_API_H
#define LIZARD_API_H

/* Stable embeddable C API for Lizard.
 *
 * This header deliberately keeps the interpreter internals opaque.  Code
 * that embeds Lizard should include this file, not the implementation-facing
 * include/lizard.h header.  The current implementation still has a process-
 * global active heap underneath; the runtime/context API centralizes that
 * dependency so the internals can later move to fully reentrant state without
 * changing embedders.
 */

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lizard_runtime lizard_runtime_t;
typedef struct lizard_context lizard_context_t;
typedef struct lizard_ast_node lizard_value_t;

typedef enum {
  LIZARD_STATUS_OK = 0,
  LIZARD_STATUS_ERROR = 1,
  LIZARD_STATUS_INVALID_ARGUMENT = 2,
  LIZARD_STATUS_IO_ERROR = 3,
  LIZARD_STATUS_OUT_OF_MEMORY = 4
} lizard_status_t;

typedef enum {
  LIZARD_VALUE_STRING,
  LIZARD_VALUE_NUMBER,
  LIZARD_VALUE_SYMBOL,
  LIZARD_VALUE_BOOL,
  LIZARD_VALUE_PAIR,
  LIZARD_VALUE_NIL,
  LIZARD_VALUE_VECTOR,
  LIZARD_VALUE_HASH,
  LIZARD_VALUE_PROCEDURE,
  LIZARD_VALUE_MACRO,
  LIZARD_VALUE_PROMISE,
  LIZARD_VALUE_CONTINUATION,
  LIZARD_VALUE_ERROR,
  LIZARD_VALUE_SYNTAX,
  LIZARD_VALUE_TYPE_THEORY,
  LIZARD_VALUE_INTERNAL
} lizard_value_type_t;

typedef struct lizard_runtime_options {
  size_t initial_heap_size;
  size_t max_segment_size;
} lizard_runtime_options_t;

void lizard_runtime_options_default(lizard_runtime_options_t *options);

lizard_runtime_t *lizard_runtime_create(const lizard_runtime_options_t *options);
void lizard_runtime_destroy(lizard_runtime_t *runtime);
void lizard_runtime_make_current(lizard_runtime_t *runtime);

lizard_context_t *lizard_context_create(lizard_runtime_t *runtime);
void lizard_context_destroy(lizard_context_t *context);
lizard_status_t lizard_context_eval_string(lizard_context_t *context,
                                           const char *source,
                                           lizard_value_t **out_value);
lizard_status_t lizard_context_eval_file(lizard_context_t *context,
                                         const char *path,
                                         lizard_value_t **out_value);
lizard_value_t *lizard_context_last_value(lizard_context_t *context);
const char *lizard_context_last_error(lizard_context_t *context);

lizard_value_type_t lizard_value_type(const lizard_value_t *value);
const char *lizard_value_type_name(lizard_value_type_t type);
int lizard_value_is_error(const lizard_value_t *value);
int lizard_value_error_code(const lizard_value_t *value);
void lizard_value_fprint(FILE *fp, lizard_value_t *value);
void lizard_value_print(lizard_value_t *value);

#ifdef __cplusplus
}
#endif

#endif /* LIZARD_API_H */
