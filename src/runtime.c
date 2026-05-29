#include "runtime.h"
#include "expansion_trace_report.h"

#include "env.h"
#include "mem.h"
#include "parser.h"
#include "primitives.h"
#include "printer.h"
#include "tokenizer.h"
#include "surface_term.h"
#include "expansion_context.h"
#include "syntax_expander.h"

#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

lizard_heap_t *heap = NULL;

static void copy_error(char *dst, size_t dst_size, const char *message) {
  size_t n;
  if (dst == NULL || dst_size == 0U) {
    return;
  }
  if (message == NULL) {
    message = "";
  }
  n = strlen(message);
  if (n >= dst_size) {
    n = dst_size - 1U;
  }
  memcpy(dst, message, n);
  dst[n] = '\0';
}

static void clear_span(lizard_source_span_t *span) {
  if (span == NULL) {
    return;
  }
  span->filename = NULL;
  span->start_line = 0;
  span->start_column = 0;
  span->end_line = 0;
  span->end_column = 0;
  span->start_offset = 0;
  span->end_offset = 0;
}

static void set_diagnostic(lizard_diagnostic_t *diagnostic,
                           lizard_status_t status, const char *message) {
  if (diagnostic == NULL) {
    return;
  }
  diagnostic->status = status;
  clear_span(&diagnostic->span);
  copy_error(diagnostic->message, sizeof(diagnostic->message), message);
}

void lizard_runtime_set_error(lizard_runtime_t *runtime, const char *message) {
  if (runtime != NULL) {
    copy_error(runtime->last_error, sizeof(runtime->last_error), message);
    set_diagnostic(&runtime->diagnostic, LIZARD_STATUS_ERROR, message);
  }
}

void lizard_context_set_error(lizard_context_t *context, const char *message) {
  if (context != NULL) {
    copy_error(context->last_error, sizeof(context->last_error), message);
    set_diagnostic(&context->diagnostic, LIZARD_STATUS_ERROR, message);
    lizard_runtime_set_error(context->runtime, message);
  }
}

void lizard_runtime_options_default(lizard_runtime_options_t *options) {
  if (options == NULL) {
    return;
  }
  options->initial_heap_size = 1024U * 1024U;
  options->max_segment_size = 16U * 1024U * 1024U;
}

lizard_runtime_t *lizard_runtime_create(const lizard_runtime_options_t *options) {
  lizard_runtime_options_t defaults;
  lizard_runtime_t *runtime;

  if (options == NULL) {
    lizard_runtime_options_default(&defaults);
    options = &defaults;
  }

  runtime = (lizard_runtime_t *)malloc(sizeof(lizard_runtime_t));
  if (runtime == NULL) {
    return NULL;
  }
  runtime->initial_heap_size = options->initial_heap_size;
  runtime->max_segment_size = options->max_segment_size;
  runtime->last_error[0] = '\0';
  set_diagnostic(&runtime->diagnostic, LIZARD_STATUS_OK, "");

  mp_set_memory_functions(lizard_heap_alloc, lizard_heap_realloc,
                          lizard_heap_free_wrapper);

  runtime->heap = lizard_heap_create(runtime->initial_heap_size,
                                     runtime->max_segment_size);
  if (runtime->heap == NULL) {
    free(runtime);
    return NULL;
  }
  /* Phase 0: wire the back-pointer so any function holding the heap
   * can reach runtime state via heap->runtime. */
  runtime->heap->runtime = runtime;
  /* Phase 0: initialize formerly-global state. */
  runtime->gensym_counter = 0;
  runtime->sr_counter = 0;
  runtime->callcc_active = 0;
  runtime->callcc_value = NULL;
  /* Phase 0 B.2: logic config, HIT registry, flags start empty. */
  runtime->logic_config_head = NULL;
  runtime->logic_last_set_bundle = NULL;
  runtime->hit_registry_head = NULL;
  runtime->flag_list = NULL;
  /* Phase 1M: kernel/proof state starts empty per runtime. */
  runtime->kernel_current_proof = NULL;
  runtime->kernel_meta_ctx = NULL;
  runtime->kernel_kdef_ctx = NULL;
  /* Phase C: module loader starts with "lib/" on the search path. */
  runtime->modules_head = NULL;
  {
    lizard_search_path_t *lib_path;
    lib_path = (lizard_search_path_t *)malloc(sizeof(lizard_search_path_t));
    if (lib_path != NULL) {
      lib_path->directory = (char *)malloc(5);
      if (lib_path->directory != NULL) {
        strcpy(lib_path->directory, "lib/");
      }
      lib_path->next = NULL;
      runtime->search_path_head = lib_path;
    } else {
      runtime->search_path_head = NULL;
    }
  }
  heap = runtime->heap;
  return runtime;
}

void lizard_runtime_destroy(lizard_runtime_t *runtime) {
  if (runtime == NULL) {
    return;
  }
  if (heap == runtime->heap) {
    heap = NULL;
  }
  if (runtime->heap != NULL) {
    lizard_heap_destroy(runtime->heap);
  }
  free(runtime);
}

void lizard_runtime_make_current(lizard_runtime_t *runtime) {
  if (runtime != NULL) {
    heap = runtime->heap;
  }
}

lizard_context_t *lizard_context_create(lizard_runtime_t *runtime) {
  lizard_context_t *context;
  if (runtime == NULL) {
    return NULL;
  }
  lizard_runtime_make_current(runtime);
  context = (lizard_context_t *)malloc(sizeof(lizard_context_t));
  if (context == NULL) {
    lizard_runtime_set_error(runtime, "unable to allocate lizard context");
    return NULL;
  }
  context->runtime = runtime;
  context->last_value = NULL;
  context->last_error[0] = '\0';
  set_diagnostic(&context->diagnostic, LIZARD_STATUS_OK, "");
  context->trace_expansion = 0;
  context->last_surface = NULL;
  context->last_expanded_surface = NULL;
  context->env = lizard_env_create(runtime->heap, NULL);
  if (context->env == NULL) {
    free(context);
    lizard_runtime_set_error(runtime, "unable to allocate lizard environment");
    return NULL;
  }
  lizard_install_primitives(runtime->heap, context->env);
  return context;
}

void lizard_context_destroy(lizard_context_t *context) {
  if (context == NULL) {
    return;
  }
  free(context);
}

static lizard_status_t eval_parsed_forms(lizard_context_t *context,
                                         lz_list_t *ast_list,
                                         lizard_value_t **out_value) {
  lz_list_node_t *iter;
  lizard_ast_node_t *expanded;
  lizard_ast_node_t *result;

  result = lizard_make_nil(context->runtime->heap);
  for (iter = ast_list->head; iter != ast_list->nil; iter = iter->next) {
    expanded = lizard_expand_macros(((lizard_ast_list_node_t *)iter)->ast,
                                    context->env, context->runtime->heap);
    result = lizard_eval(expanded, context->env, context->runtime->heap,
                         lizard_identity_cont);
    context->last_value = result;
    if (result != NULL && result->type == AST_ERROR) {
      lizard_context_set_error(context, "evaluation returned a Lizard error");
      context->diagnostic.status = LIZARD_STATUS_EVAL_ERROR;
      if (context->runtime != NULL) {
        context->runtime->diagnostic.status = LIZARD_STATUS_EVAL_ERROR;
      }
      if (out_value != NULL) {
        *out_value = result;
      }
      return LIZARD_STATUS_EVAL_ERROR;
    }
  }
  context->last_value = result;
  if (out_value != NULL) {
    *out_value = result;
  }
  context->last_error[0] = '\0';
  set_diagnostic(&context->diagnostic, LIZARD_STATUS_OK, "");
  set_diagnostic(&context->runtime->diagnostic, LIZARD_STATUS_OK, "");
  return LIZARD_STATUS_OK;
}

static lizard_status_t eval_surface_forms(lizard_context_t *context,
                                          lz_list_t *surface_list,
                                          lizard_value_t **out_value) {
  lz_list_node_t *iter;
  lizard_expansion_context_t *expansion;
  lizard_ast_node_t *expanded_ast;
  lizard_surface_term_t *expanded_surface;
  lizard_ast_node_t *result;

  expansion = NULL;
  result = lizard_make_nil(context->runtime->heap);
  context->last_surface = NULL;
  context->last_expanded_surface = NULL;

  if (surface_list == NULL) {
    if (out_value != NULL) {
      *out_value = result;
    }
    return LIZARD_STATUS_OK;
  }

  expansion = lizard_expansion_context_new(context->runtime->heap, 0,
                                           "context-eval");
  if (expansion == NULL) {
    lizard_context_set_error(context, "unable to allocate expansion context");
    return LIZARD_STATUS_OUT_OF_MEMORY;
  }

  for (iter = surface_list->head; iter != surface_list->nil; iter = iter->next) {
    lizard_surface_list_node_t *surface_node;
    surface_node = (lizard_surface_list_node_t *)iter;
    expanded_ast = NULL;
    expanded_surface = NULL;
    context->last_surface = surface_node->term;
    if (!lizard_syntax_expand_surface(expansion, context->env,
                                      surface_node->term,
                                      &expanded_surface, &expanded_ast)) {
      lizard_context_set_error(context, "macro expansion failed");
      context->diagnostic.status = LIZARD_STATUS_EVAL_ERROR;
      if (context->runtime != NULL) {
        context->runtime->diagnostic.status = LIZARD_STATUS_EVAL_ERROR;
      }
      return LIZARD_STATUS_EVAL_ERROR;
    }
    context->last_expanded_surface = expanded_surface;
    result = lizard_eval(expanded_ast, context->env, context->runtime->heap,
                         lizard_identity_cont);
    context->last_value = result;
    if (result != NULL && result->type == AST_ERROR) {
      lizard_context_set_error(context, "evaluation returned a Lizard error");
      context->diagnostic.status = LIZARD_STATUS_EVAL_ERROR;
      if (context->runtime != NULL) {
        context->runtime->diagnostic.status = LIZARD_STATUS_EVAL_ERROR;
      }
      if (out_value != NULL) {
        *out_value = result;
      }
      return LIZARD_STATUS_EVAL_ERROR;
    }
  }
  context->last_value = result;
  if (out_value != NULL) {
    *out_value = result;
  }
  context->last_error[0] = '\0';
  set_diagnostic(&context->diagnostic, LIZARD_STATUS_OK, "");
  set_diagnostic(&context->runtime->diagnostic, LIZARD_STATUS_OK, "");
  return LIZARD_STATUS_OK;
}

static lizard_status_t eval_traced_source(lizard_context_t *context,
                                          const char *source,
                                          const char *filename,
                                          lizard_value_t **out_value) {
  lz_list_t *surface_list;
  lizard_diagnostic_t diagnostic;

  diagnostic.status = LIZARD_STATUS_OK;
  clear_span(&diagnostic.span);
  diagnostic.message[0] = '\0';
  context->last_surface = NULL;
  context->last_expanded_surface = NULL;

  surface_list = lizard_surface_parse_source(context->runtime->heap, source,
                                             filename, &diagnostic);
  if (surface_list == NULL) {
    context->diagnostic = diagnostic;
    copy_error(context->last_error, sizeof(context->last_error),
               diagnostic.message);
    if (context->runtime != NULL) {
      context->runtime->diagnostic = diagnostic;
      copy_error(context->runtime->last_error,
                 sizeof(context->runtime->last_error), diagnostic.message);
    }
    if (context->diagnostic.status == LIZARD_STATUS_OK) {
      context->diagnostic.status = LIZARD_STATUS_PARSE_ERROR;
    }
    return LIZARD_STATUS_PARSE_ERROR;
  }
  return eval_surface_forms(context, surface_list, out_value);
}

lizard_status_t lizard_context_eval_string(lizard_context_t *context,
                                           const char *source,
                                           lizard_value_t **out_value) {
  lz_list_t *tokens;
  lz_list_t *ast_list;

  if (out_value != NULL) {
    *out_value = NULL;
  }
  if (context == NULL || source == NULL) {
    return LIZARD_STATUS_INVALID_ARGUMENT;
  }
  lizard_runtime_make_current(context->runtime);
  context->last_surface = NULL;
  context->last_expanded_surface = NULL;
  if (context->trace_expansion) {
    return eval_traced_source(context, source, "<string>", out_value);
  }
  tokens = lizard_tokenize_source(source, "<string>", &context->diagnostic);
  if (tokens == NULL) {
    const lizard_diagnostic_t *td;
    td = lizard_tokenizer_last_diagnostic();
    if (td != NULL && td->message[0] != '\0') {
      copy_error(context->last_error, sizeof(context->last_error), td->message);
      context->diagnostic = *td;
      if (context->runtime != NULL) {
        copy_error(context->runtime->last_error,
                   sizeof(context->runtime->last_error), td->message);
        context->runtime->diagnostic = *td;
      }
    } else {
      lizard_context_set_error(context, "tokenization failed");
      context->diagnostic.status = LIZARD_STATUS_PARSE_ERROR;
      if (context->runtime != NULL) {
        context->runtime->diagnostic.status = LIZARD_STATUS_PARSE_ERROR;
      }
    }
    return LIZARD_STATUS_PARSE_ERROR;
  }
  ast_list = lizard_parse(tokens, context->runtime->heap);
  if (ast_list == NULL) {
    const lizard_diagnostic_t *pd;
    pd = lizard_parser_last_diagnostic();
    if (pd != NULL && pd->message[0] != '\0') {
      copy_error(context->last_error, sizeof(context->last_error), pd->message);
      context->diagnostic = *pd; /* keep PARSE_ERROR status + source span */
      if (context->runtime != NULL) {
        copy_error(context->runtime->last_error,
                   sizeof(context->runtime->last_error), pd->message);
        context->runtime->diagnostic = *pd;
      }
      return pd->status;
    }
    lizard_context_set_error(context, "parse failed");
    context->diagnostic.status = LIZARD_STATUS_PARSE_ERROR;
    if (context->runtime != NULL) {
      context->runtime->diagnostic.status = LIZARD_STATUS_PARSE_ERROR;
    }
    return LIZARD_STATUS_PARSE_ERROR;
  }
  return eval_parsed_forms(context, ast_list, out_value);
}

lizard_status_t lizard_context_eval_file(lizard_context_t *context,
                                         const char *path,
                                         lizard_value_t **out_value) {
  FILE *fp;
  long file_size;
  char *source;
  size_t got;
  lizard_status_t status;

  if (out_value != NULL) {
    *out_value = NULL;
  }
  if (context == NULL || path == NULL) {
    return LIZARD_STATUS_INVALID_ARGUMENT;
  }
  fp = fopen(path, "rb");
  if (fp == NULL) {
    lizard_context_set_error(context, "unable to open file");
    set_diagnostic(&context->diagnostic, LIZARD_STATUS_IO_ERROR,
                   "unable to open file");
    return LIZARD_STATUS_IO_ERROR;
  }
  if (fseek(fp, 0L, SEEK_END) != 0) {
    fclose(fp);
    lizard_context_set_error(context, "unable to seek file");
    return LIZARD_STATUS_IO_ERROR;
  }
  file_size = ftell(fp);
  if (file_size < 0L) {
    fclose(fp);
    lizard_context_set_error(context, "unable to determine file size");
    return LIZARD_STATUS_IO_ERROR;
  }
  rewind(fp);
  source = (char *)malloc((size_t)file_size + 1U);
  if (source == NULL) {
    fclose(fp);
    lizard_context_set_error(context, "unable to allocate source buffer");
    return LIZARD_STATUS_OUT_OF_MEMORY;
  }
  got = fread(source, 1U, (size_t)file_size, fp);
  fclose(fp);
  if (got != (size_t)file_size) {
    free(source);
    lizard_context_set_error(context, "unable to read file");
    return LIZARD_STATUS_IO_ERROR;
  }
  source[file_size] = '\0';
  lizard_runtime_make_current(context->runtime);
  context->last_surface = NULL;
  context->last_expanded_surface = NULL;
  if (context->trace_expansion) {
    status = eval_traced_source(context, source, path, out_value);
    free(source);
    return status;
  }
  {
    lz_list_t *tokens;
    lz_list_t *ast_list;
    tokens = lizard_tokenize_source(source, path, &context->diagnostic);
    if (tokens == NULL) {
      const lizard_diagnostic_t *td;
      td = lizard_tokenizer_last_diagnostic();
      if (td != NULL && td->message[0] != '\0') {
        copy_error(context->last_error, sizeof(context->last_error), td->message);
        context->diagnostic = *td;
        if (context->runtime != NULL) {
          copy_error(context->runtime->last_error,
                     sizeof(context->runtime->last_error), td->message);
          context->runtime->diagnostic = *td;
        }
      } else {
        lizard_context_set_error(context, "tokenization failed");
        context->diagnostic.status = LIZARD_STATUS_PARSE_ERROR;
      }
      free(source);
      return LIZARD_STATUS_PARSE_ERROR;
    }
    ast_list = lizard_parse(tokens, context->runtime->heap);
    if (ast_list == NULL) {
      const lizard_diagnostic_t *pd;
      pd = lizard_parser_last_diagnostic();
      if (pd != NULL && pd->message[0] != '\0') {
        copy_error(context->last_error, sizeof(context->last_error), pd->message);
        context->diagnostic = *pd;
        if (context->runtime != NULL) {
          copy_error(context->runtime->last_error,
                     sizeof(context->runtime->last_error), pd->message);
          context->runtime->diagnostic = *pd;
        }
        status = pd->status;
      } else {
        lizard_context_set_error(context, "parse failed");
        context->diagnostic.status = LIZARD_STATUS_PARSE_ERROR;
        status = LIZARD_STATUS_PARSE_ERROR;
      }
      free(source);
      return status;
    }
    status = eval_parsed_forms(context, ast_list, out_value);
  }
  free(source);
  return status;
}

void lizard_context_set_trace_expansion(lizard_context_t *context,
                                        int enabled) {
  if (context != NULL) {
    context->trace_expansion = enabled != 0;
    if (!context->trace_expansion) {
      context->last_surface = NULL;
      context->last_expanded_surface = NULL;
    }
  }
}

int lizard_context_trace_expansion_enabled(lizard_context_t *context) {
  return context != NULL ? context->trace_expansion : 0;
}

unsigned long lizard_context_last_expansion_trace_count(
    lizard_context_t *context) {
  if (context == NULL || context->last_expanded_surface == NULL) {
    return 0UL;
  }
  return lizard_surface_trace_count(context->last_expanded_surface);
}

const char *lizard_context_last_expansion_stage(lizard_context_t *context) {
  if (context == NULL || context->last_expanded_surface == NULL) {
    return NULL;
  }
  return lizard_surface_trace_latest_stage(context->last_expanded_surface);
}

const char *lizard_context_last_expansion_detail(lizard_context_t *context) {
  if (context == NULL || context->last_expanded_surface == NULL) {
    return NULL;
  }
  return lizard_surface_trace_latest_detail(context->last_expanded_surface);
}

int lizard_context_last_expansion_span(lizard_context_t *context,
                                       lizard_source_span_t *out_span) {
  if (context == NULL || context->last_expanded_surface == NULL ||
      out_span == NULL) {
    return 0;
  }
  return lizard_surface_span(context->last_expanded_surface, out_span);
}


unsigned long lizard_context_expansion_trace_count(lizard_context_t *context) {
  return lizard_context_last_expansion_trace_count(context);
}

int lizard_context_expansion_trace_event(lizard_context_t *context,
                                         unsigned long index,
                                         lizard_expansion_trace_event_t *out_event) {
  if (context == NULL || context->last_expanded_surface == NULL ||
      out_event == NULL) {
    if (out_event != NULL) {
      out_event->stage = NULL;
      out_event->detail = NULL;
      out_event->origin_filename = NULL;
      out_event->origin_line = 0;
      out_event->origin_column = 0;
      out_event->origin_phase = 0;
      out_event->origin_scope_summary = 0UL;
    }
    return 0;
  }
  return lizard_surface_trace_event_at(context->last_expanded_surface,
                                       index, out_event);
}

int lizard_context_expansion_trace_event_string(lizard_context_t *context,
                                                unsigned long index,
                                                char *buffer,
                                                size_t buffer_size) {
  if (context == NULL || context->last_expanded_surface == NULL ||
      buffer == NULL || buffer_size == 0U) {
    if (buffer != NULL && buffer_size > 0U) {
      buffer[0] = '\0';
    }
    return 0;
  }
  return lizard_surface_trace_event_string(context->runtime->heap,
                                           context->last_expanded_surface,
                                           index, buffer, buffer_size);
}

lizard_expansion_trace_report_t *lizard_context_expansion_trace_report(
    lizard_context_t *context) {
  if (context == NULL || context->last_expanded_surface == NULL) {
    return lizard_expansion_trace_report_from_surface(NULL);
  }
  return lizard_expansion_trace_report_from_surface(
      context->last_expanded_surface);
}

lizard_value_t *lizard_context_last_value(lizard_context_t *context) {
  if (context == NULL) {
    return NULL;
  }
  return context->last_value;
}

const char *lizard_context_last_error(lizard_context_t *context) {
  if (context == NULL) {
    return "invalid Lizard context";
  }
  return context->last_error;
}

const lizard_diagnostic_t *lizard_context_last_diagnostic(
    lizard_context_t *context) {
  if (context == NULL) {
    return NULL;
  }
  return &context->diagnostic;
}

lizard_value_type_t lizard_value_type(const lizard_value_t *value) {
  if (value == NULL) {
    return LIZARD_VALUE_INTERNAL;
  }
  if (value->type >= AST_TT_PI && value->type <= AST_TT_EXTENSION) {
    return LIZARD_VALUE_TYPE_THEORY;
  }
  switch (value->type) {
  case AST_STRING:
    return LIZARD_VALUE_STRING;
  case AST_NUMBER:
    return LIZARD_VALUE_NUMBER;
  case AST_SYMBOL:
    return LIZARD_VALUE_SYMBOL;
  case AST_BOOL:
    return LIZARD_VALUE_BOOL;
  case AST_PAIR:
    return LIZARD_VALUE_PAIR;
  case AST_NIL:
    return LIZARD_VALUE_NIL;
  case AST_VECTOR:
    return LIZARD_VALUE_VECTOR;
  case AST_HASH:
    return LIZARD_VALUE_HASH;
  case AST_PRIMITIVE:
  case AST_LAMBDA:
    return LIZARD_VALUE_PROCEDURE;
  case AST_MACRO:
    return LIZARD_VALUE_MACRO;
  case AST_PROMISE:
    return LIZARD_VALUE_PROMISE;
  case AST_CONTINUATION:
  case AST_CALLCC:
    return LIZARD_VALUE_CONTINUATION;
  case AST_ERROR:
    return LIZARD_VALUE_ERROR;
  case AST_SYNTAX_RULES:
    return LIZARD_VALUE_SYNTAX;
  default:
    return LIZARD_VALUE_INTERNAL;
  }
}

const char *lizard_value_type_name(lizard_value_type_t type) {
  switch (type) {
  case LIZARD_VALUE_STRING:
    return "string";
  case LIZARD_VALUE_NUMBER:
    return "number";
  case LIZARD_VALUE_SYMBOL:
    return "symbol";
  case LIZARD_VALUE_BOOL:
    return "bool";
  case LIZARD_VALUE_PAIR:
    return "pair";
  case LIZARD_VALUE_NIL:
    return "nil";
  case LIZARD_VALUE_VECTOR:
    return "vector";
  case LIZARD_VALUE_HASH:
    return "hash";
  case LIZARD_VALUE_PROCEDURE:
    return "procedure";
  case LIZARD_VALUE_MACRO:
    return "macro";
  case LIZARD_VALUE_PROMISE:
    return "promise";
  case LIZARD_VALUE_CONTINUATION:
    return "continuation";
  case LIZARD_VALUE_ERROR:
    return "error";
  case LIZARD_VALUE_SYNTAX:
    return "syntax";
  case LIZARD_VALUE_TYPE_THEORY:
    return "type-theory";
  default:
    return "internal";
  }
}

int lizard_value_is_error(const lizard_value_t *value) {
  return value != NULL && value->type == AST_ERROR;
}

int lizard_value_error_code(const lizard_value_t *value) {
  if (value == NULL || value->type != AST_ERROR) {
    return 0;
  }
  return value->data.error.code;
}

int lizard_value_source_span(const lizard_value_t *value,
                             lizard_source_span_t *out_span) {
  if (value == NULL || out_span == NULL) {
    return 0;
  }
  *out_span = value->span;
  return value->span.start_line != 0 || value->span.start_column != 0 ||
         value->span.start_offset != 0;
}

void lizard_value_fprint(FILE *fp, lizard_value_t *value) {
  if (fp == NULL) {
    return;
  }
  if (value == NULL) {
    fputs("<null>", fp);
    return;
  }
  lizard_fprint_value(fp, value);
}

void lizard_value_print(lizard_value_t *value) {
  lizard_value_fprint(stdout, value);
}
