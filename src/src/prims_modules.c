/* prims_modules.c — extracted from primitives.c (#7 monolith split).
 * Registration stays in primitives.c; definitions linked from here. */
#include "primitives.h"
#include "env.h"
#include "errors.h"
#include "lizard_internal.h"
#include "mem.h"
#include "parser.h"
#include "printer.h"
#include "runtime.h"
#include "tokenizer.h"
#include "prims_shared.h"
#include <setjmp.h>
#include <stdint.h>
#include <string.h>

lizard_ast_node_t *lizard_primitive_load(lz_list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap) {
  lizard_ast_list_node_t *arg;
  const char *path;
  FILE *fp;
  long file_size;
  char *source;
  size_t got;
  lz_list_t *tokens;
  lz_list_t *ast_list;
  lz_list_node_t *iter;
  lizard_ast_node_t *result;

  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_ARGC);
  }
  arg = (lizard_ast_list_node_t *)args->head;
  if (arg->ast->type != AST_STRING) {
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_ARGT);
  }
  path = arg->ast->data.string;
  fp = fopen(path, "rb");
  if (!fp) {
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_OPEN);
  }
  if (fseek(fp, 0L, SEEK_END) != 0) {
    fclose(fp);
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_READ);
  }
  file_size = ftell(fp);
  if (file_size < 0) {
    fclose(fp);
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_READ);
  }
  rewind(fp);
  source = (char *)lizard_heap_alloc((size_t)file_size + 1);
  got = fread(source, 1, (size_t)file_size, fp);
  fclose(fp);
  if (got != (size_t)file_size) {
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_READ);
  }
  source[file_size] = '\0';

  tokens = lizard_tokenize(source);
  ast_list = lizard_parse(tokens, heap);
  result = lizard_make_nil(heap);
  for (iter = ast_list->head; iter != ast_list->nil; iter = iter->next) {
    lizard_ast_node_t *expanded = lizard_expand_macros(
        ((lizard_ast_list_node_t *)iter)->ast, env, heap);
    result = lizard_eval(expanded, env, heap, lizard_identity_cont);
    if (result && result->type == AST_ERROR) {
      return result;
    }
  }
  return result;
}
static FILE *resolve_module_file(const char *path, lizard_heap_t *heap,
                                 char **out_resolved) {
  FILE *fp;
  lizard_search_path_t *sp;
  /* Try the raw path first. */
  fp = fopen(path, "rb");
  if (fp != NULL) {
    size_t len = strlen(path) + 1;
    *out_resolved = (char *)lizard_heap_alloc(len);
    strcpy(*out_resolved, path);
    return fp;
  }
  /* Try each search path directory. */
  if (heap->runtime != NULL) {
    for (sp = heap->runtime->search_path_head; sp != NULL; sp = sp->next) {
      size_t dir_len = strlen(sp->directory);
      size_t path_len = strlen(path);
      char *full = (char *)lizard_heap_alloc(dir_len + path_len + 1);
      strcpy(full, sp->directory);
      strcat(full, path);
      fp = fopen(full, "rb");
      if (fp != NULL) {
        *out_resolved = full;
        return fp;
      }
    }
  }
  *out_resolved = NULL;
  return NULL;
}
static lizard_module_entry_t *find_cached_module(lizard_heap_t *heap,
                                                  const char *path) {
  lizard_module_entry_t *e;
  if (heap->runtime == NULL) return NULL;
  for (e = heap->runtime->modules_head; e != NULL; e = e->next) {
    if (strcmp(e->canonical_path, path) == 0) return e;
  }
  return NULL;
}
static void cache_module(lizard_heap_t *heap, const char *path,
                         lizard_ast_node_t *result) {
  lizard_module_entry_t *e;
  if (heap->runtime == NULL) return;
  e = (lizard_module_entry_t *)malloc(sizeof(lizard_module_entry_t));
  if (e == NULL) return;
  e->canonical_path = (char *)malloc(strlen(path) + 1);
  if (e->canonical_path != NULL) {
    strcpy(e->canonical_path, path);
  }
  e->result = result;
  e->next = heap->runtime->modules_head;
  heap->runtime->modules_head = e;
}
lizard_ast_node_t *lizard_primitive_import(lz_list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_list_node_t *arg;
  const char *path;
  char *resolved;
  FILE *fp;
  long file_size;
  char *source;
  size_t got;
  lz_list_t *tokens;
  lz_list_t *ast_list;
  lz_list_node_t *iter;
  lizard_ast_node_t *result;
  lizard_module_entry_t *cached;

  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_ARGC);
  }
  arg = (lizard_ast_list_node_t *)args->head;
  if (arg->ast->type != AST_STRING) {
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_ARGT);
  }
  path = arg->ast->data.string;

  /* Check cache first. */
  cached = find_cached_module(heap, path);
  if (cached != NULL) {
    return cached->result;
  }

  /* Resolve via search path. */
  fp = resolve_module_file(path, heap, &resolved);
  if (fp == NULL) {
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_OPEN);
  }
  /* Also check cache under the resolved path. */
  if (resolved != NULL && strcmp(resolved, path) != 0) {
    cached = find_cached_module(heap, resolved);
    if (cached != NULL) {
      fclose(fp);
      return cached->result;
    }
  }

  /* Read file. */
  if (fseek(fp, 0L, SEEK_END) != 0) {
    fclose(fp);
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_READ);
  }
  file_size = ftell(fp);
  if (file_size < 0) {
    fclose(fp);
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_READ);
  }
  rewind(fp);
  source = (char *)lizard_heap_alloc((size_t)file_size + 1);
  got = fread(source, 1, (size_t)file_size, fp);
  fclose(fp);
  if (got != (size_t)file_size) {
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_READ);
  }
  source[file_size] = '\0';

  /* Evaluate in the current environment (like load). */
  tokens = lizard_tokenize(source);
  ast_list = lizard_parse(tokens, heap);
  result = lizard_make_nil(heap);
  for (iter = ast_list->head; iter != ast_list->nil; iter = iter->next) {
    lizard_ast_node_t *expanded = lizard_expand_macros(
        ((lizard_ast_list_node_t *)iter)->ast, env, heap);
    result = lizard_eval(expanded, env, heap, lizard_identity_cont);
    if (result && result->type == AST_ERROR) {
      return result;
    }
  }

  /* Cache under both the original and resolved paths. */
  cache_module(heap, path, result);
  if (resolved != NULL && strcmp(resolved, path) != 0) {
    cache_module(heap, resolved, result);
  }
  return result;
}
lizard_ast_node_t *lizard_primitive_module_loadedp(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  lizard_ast_list_node_t *arg;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  arg = (lizard_ast_list_node_t *)args->head;
  if (arg->ast->type != AST_STRING) {
    return lizard_make_bool(heap, 0);
  }
  return lizard_make_bool(heap, find_cached_module(heap, arg->ast->data.string) != NULL);
}
lizard_ast_node_t *lizard_primitive_module_search_path(lz_list_t *args,
                                                        lizard_env_t *env,
                                                        lizard_heap_t *heap) {
  lizard_ast_node_t *result;
  lizard_search_path_t *sp;
  (void)env;
  (void)args;
  result = lizard_make_nil(heap);
  if (heap->runtime == NULL) return result;
  /* Build list in reverse so the result is in path order. */
  {
    lizard_search_path_t *entries[64];
    int count = 0;
    int i;
    for (sp = heap->runtime->search_path_head; sp != NULL && count < 64; sp = sp->next) {
      entries[count++] = sp;
    }
    for (i = count - 1; i >= 0; i--) {
      lizard_ast_node_t *pair = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      lizard_ast_node_t *str = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      str->type = AST_STRING;
      str->data.string = entries[i]->directory;
      pair->type = AST_PAIR;
      pair->data.pair.car = str;
      pair->data.pair.cdr = result;
      result = pair;
    }
  }
  return result;
}
