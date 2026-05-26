# Stable C API Sketch

This is the intended shape of Lizard's stable embeddable API. It is not yet an ABI promise. The current `include/lizard.h` still exposes implementation details; the migration target is an opaque API plus private internal headers.

## Public types

```c
typedef struct lizard_runtime lizard_runtime_t;
typedef struct lizard_context lizard_context_t;
typedef struct lizard_value lizard_value_t;
typedef struct lizard_error lizard_error_t;

typedef enum lizard_status {
  LIZARD_OK = 0,
  LIZARD_ERR_PARSE,
  LIZARD_ERR_EVAL,
  LIZARD_ERR_TYPE,
  LIZARD_ERR_OOM,
  LIZARD_ERR_INTERNAL
} lizard_status_t;

typedef struct lizard_config {
  size_t initial_heap_bytes;
  size_t max_heap_bytes;
  void *userdata;
  void *(*alloc)(void *userdata, size_t size);
  void (*free)(void *userdata, void *ptr);
} lizard_config_t;
```

## Lifecycle

```c
lizard_runtime_t *lizard_runtime_new(const lizard_config_t *cfg);
void lizard_runtime_free(lizard_runtime_t *runtime);

lizard_context_t *lizard_context_new(lizard_runtime_t *runtime);
void lizard_context_free(lizard_context_t *context);
```

## Evaluation

```c
lizard_status_t lizard_eval_string(lizard_context_t *ctx,
                                   const char *source,
                                   lizard_value_t **out);

lizard_status_t lizard_eval_file(lizard_context_t *ctx,
                                 const char *path,
                                 lizard_value_t **out);
```

## Values

```c
int lizard_value_is_nil(const lizard_value_t *v);
int lizard_value_is_number(const lizard_value_t *v);
int lizard_value_is_string(const lizard_value_t *v);
const char *lizard_value_type_name(const lizard_value_t *v);

lizard_status_t lizard_value_to_cstr(lizard_context_t *ctx,
                                     const lizard_value_t *v,
                                     const char **out);
```

## Native primitives

```c
typedef lizard_status_t (*lizard_native_fn)(lizard_context_t *ctx,
                                            int argc,
                                            lizard_value_t **argv,
                                            lizard_value_t **out,
                                            void *userdata);

lizard_status_t lizard_define_native(lizard_context_t *ctx,
                                     const char *name,
                                     lizard_native_fn fn,
                                     void *userdata);
```

## Diagnostics

```c
const lizard_error_t *lizard_last_error(lizard_context_t *ctx);
const char *lizard_error_message(const lizard_error_t *err);
const char *lizard_error_file(const lizard_error_t *err);
unsigned lizard_error_line(const lizard_error_t *err);
unsigned lizard_error_column(const lizard_error_t *err);
```

## Migration rule

Anything that exposes AST layout, environment layout, list internals, GMP internals, or allocator internals belongs in private headers, not the stable C API.
