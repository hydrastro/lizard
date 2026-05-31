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

/* ======================================================================
 * Phase 3: real module namespaces — (module …) / (export …) / (import …)
 *
 * A module is evaluated in a hermetic environment (a child of a pristine
 * primitives-only env), so its private definitions never collide with the
 * importer or with other modules.  `export` names the public bindings (with no
 * export form, every top-level definition is public).  `import` brings a
 * namespace's exports into the caller — qualified `alias:name` by default, or
 * unqualified with `:only (…)` / `:all`.  The legacy `(import "file")` form
 * (load into the caller, used pervasively) is left untouched; only a symbol
 * source or extra options route here.
 * ====================================================================== */

static char *mod_dup(const char *s) {
  char *r = (char *)malloc(strlen(s) + 1);
  if (r != NULL) strcpy(r, s);
  return r;
}

static lizard_env_t *module_base_env(lizard_heap_t *heap) {
  if (heap->runtime == NULL) return NULL;
  if (heap->runtime->module_base_env == NULL) {
    lizard_env_t *base = lizard_env_create(heap, NULL);
    if (base != NULL) lizard_install_primitives(heap, base);
    heap->runtime->module_base_env = base;
  }
  return heap->runtime->module_base_env;
}

static lizard_namespace_t *ns_find(lizard_heap_t *heap, const char *name) {
  lizard_namespace_t *n;
  if (heap->runtime == NULL) return NULL;
  for (n = heap->runtime->namespaces_head; n != NULL; n = n->next)
    if (strcmp(n->name, name) == 0) return n;
  return NULL;
}

static int ns_is_exported(lizard_namespace_t *ns, const char *name) {
  lizard_export_t *e;
  if (ns->exports == NULL) return 1;          /* no export form => export all */
  for (e = ns->exports; e != NULL; e = e->next)
    if (strcmp(e->name, name) == 0) return 1;
  return 0;
}

/* Evaluate a chain of raw body forms in `menv`; returns 0 on a Lizard error. */
static int run_forms_pairchain(lizard_heap_t *heap, lizard_env_t *menv,
                               lizard_ast_node_t *body) {
  lizard_ast_node_t *cur;
  for (cur = body; cur != NULL && cur->type == AST_PAIR; cur = cur->data.pair.cdr) {
    lizard_ast_node_t *expanded =
        lizard_expand_macros(cur->data.pair.car, menv, heap);
    lizard_ast_node_t *r = lizard_eval(expanded, menv, heap, lizard_identity_cont);
    if (r != NULL && r->type == AST_ERROR) return 0;
  }
  return 1;
}

/* Evaluate parsed file forms (an lz_list) in `menv`; returns 0 on error. */
static int run_forms_list(lizard_heap_t *heap, lizard_env_t *menv,
                          lz_list_t *forms) {
  lz_list_node_t *iter;
  if (forms == NULL) return 1;
  for (iter = forms->head; iter != forms->nil; iter = iter->next) {
    lizard_ast_node_t *expanded =
        lizard_expand_macros(((lizard_ast_list_node_t *)iter)->ast, menv, heap);
    lizard_ast_node_t *r = lizard_eval(expanded, menv, heap, lizard_identity_cont);
    if (r != NULL && r->type == AST_ERROR) return 0;
  }
  return 1;
}

/* Create a namespace, make it the current module, returning the previous
 * current module via *prev (restore with the value after evaluating). */
static lizard_namespace_t *ns_begin(lizard_heap_t *heap, const char *name,
                                    lizard_namespace_t **prev) {
  lizard_namespace_t *ns = (lizard_namespace_t *)malloc(sizeof(*ns));
  if (ns == NULL) return NULL;
  ns->name = mod_dup(name);
  ns->env = lizard_env_create(heap, module_base_env(heap));
  ns->exports = NULL;
  ns->next = NULL;
  *prev = heap->runtime->current_module;
  heap->runtime->current_module = ns;
  return ns;
}

static void ns_commit(lizard_heap_t *heap, lizard_namespace_t *ns,
                      lizard_namespace_t *prev) {
  heap->runtime->current_module = prev;
  ns->next = heap->runtime->namespaces_head;
  heap->runtime->namespaces_head = ns;
}

lizard_ast_node_t *lizard_primitive_pct_module(lz_list_t *args, lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *name_node, *body;
  lizard_namespace_t *ns, *prev;
  (void)env;
  if (args->head == args->nil || args->head->next == args->nil)
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_ARGC);
  name_node = ((lizard_ast_list_node_t *)args->head)->ast;
  body = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (name_node->type != AST_SYMBOL)
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_ARGT);
  ns = ns_begin(heap, name_node->data.variable, &prev);
  if (ns == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  if (!run_forms_pairchain(heap, ns->env, body)) {
    heap->runtime->current_module = prev;
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  ns_commit(heap, ns, prev);
  return lizard_make_nil(heap);
}

lizard_ast_node_t *lizard_primitive_pct_export(lz_list_t *args, lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *names, *cur;
  lizard_namespace_t *ns;
  (void)env;
  if (heap->runtime == NULL || heap->runtime->current_module == NULL)
    return lizard_make_error(heap, LIZARD_ERROR_USER);  /* export outside a module */
  ns = heap->runtime->current_module;
  if (args->head == args->nil) return lizard_make_nil(heap);
  names = ((lizard_ast_list_node_t *)args->head)->ast;
  for (cur = names; cur != NULL && cur->type == AST_PAIR; cur = cur->data.pair.cdr) {
    lizard_ast_node_t *s = cur->data.pair.car;
    if (s->type == AST_SYMBOL) {
      lizard_export_t *e = (lizard_export_t *)malloc(sizeof(*e));
      if (e != NULL) {
        e->name = mod_dup(s->data.variable);
        e->next = ns->exports;
        ns->exports = e;
      }
    }
  }
  return lizard_make_nil(heap);
}

static char *basename_no_ext(lizard_heap_t *heap, const char *path) {
  const char *slash = strrchr(path, '/');
  const char *base = slash ? slash + 1 : path;
  const char *dot = strrchr(base, '.');
  size_t len = dot ? (size_t)(dot - base) : strlen(base);
  char *r = (char *)lizard_heap_alloc(len + 1);
  memcpy(r, base, len);
  r[len] = '\0';
  return r;
}

static lz_list_t *parse_module_file(lizard_heap_t *heap, const char *path) {
  char *resolved = NULL;
  FILE *fp = resolve_module_file(path, heap, &resolved);
  long file_size;
  char *source;
  size_t got;
  if (fp == NULL) return NULL;
  if (fseek(fp, 0L, SEEK_END) != 0) { fclose(fp); return NULL; }
  file_size = ftell(fp);
  if (file_size < 0) { fclose(fp); return NULL; }
  rewind(fp);
  source = (char *)lizard_heap_alloc((size_t)file_size + 1);
  got = fread(source, 1, (size_t)file_size, fp);
  fclose(fp);
  if (got != (size_t)file_size) return NULL;
  source[file_size] = '\0';
  return lizard_parse(lizard_tokenize(source), heap);
}

static void bind_qualified(lizard_heap_t *heap, lizard_env_t *env,
                           const char *prefix, const char *name,
                           lizard_ast_node_t *val) {
  char *q = (char *)lizard_heap_alloc(strlen(prefix) + 1 + strlen(name) + 1);
  strcpy(q, prefix);
  strcat(q, ":");
  strcat(q, name);
  lizard_env_define(heap, env, q, val);
}

/* Bind a namespace's exports into `env`.  Default: qualified `prefix:name`.
 * all_flag: unqualified.  Returns 0 on error (e.g. an unknown :only name). */
static int import_bindings(lizard_heap_t *heap, lizard_env_t *env,
                           lizard_namespace_t *ns, const char *prefix,
                           lizard_ast_node_t *only, int all_flag) {
  if (only != NULL) {
    lizard_ast_node_t *cur;
    for (cur = only; cur != NULL && cur->type == AST_PAIR; cur = cur->data.pair.cdr) {
      lizard_ast_node_t *s = cur->data.pair.car;
      lizard_ast_node_t *v;
      if (s->type != AST_SYMBOL) return 0;
      if (!ns_is_exported(ns, s->data.variable)) return 0;   /* not public */
      v = lizard_env_lookup(ns->env, s->data.variable);
      if (v == NULL) return 0;
      lizard_env_define(heap, env, s->data.variable, v);     /* unqualified */
    }
    return 1;
  }
  if (ns->exports != NULL) {
    lizard_export_t *e;
    for (e = ns->exports; e != NULL; e = e->next) {
      lizard_ast_node_t *v = lizard_env_lookup(ns->env, e->name);
      if (v == NULL) continue;
      if (all_flag) lizard_env_define(heap, env, e->name, v);
      else bind_qualified(heap, env, prefix, e->name, v);
    }
  } else {
    /* export-all: every binding defined in the module's own frame, skipping
     * names that were themselves imported (they contain ':'). */
    lizard_env_entry_t *en;
    for (en = ns->env->entries; en != NULL; en = en->next) {
      if (strchr(en->symbol, ':') != NULL) continue;
      if (all_flag) lizard_env_define(heap, env, en->symbol, en->value);
      else bind_qualified(heap, env, prefix, en->symbol, en->value);
    }
  }
  return 1;
}

lizard_ast_node_t *lizard_primitive_pct_import(lz_list_t *args, lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *all, *src, *cur;
  lizard_ast_node_t *alias = NULL, *only = NULL;
  int all_flag = 0;
  lizard_namespace_t *ns = NULL;
  const char *prefix;
  if (args->head == args->nil)
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_ARGC);
  all = ((lizard_ast_list_node_t *)args->head)->ast;   /* (SRC opts...) */
  if (all == NULL || all->type != AST_PAIR)
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_ARGT);
  src = all->data.pair.car;
  /* Legacy form: a lone string keeps the historic "load file into the caller"
   * behaviour (search-path + load-once), delegating to the import primitive. */
  if (src->type == AST_STRING &&
      (all->data.pair.cdr == NULL || all->data.pair.cdr->type != AST_PAIR)) {
    lz_list_t *one = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
    lizard_ast_list_node_t *w =
        (lizard_ast_list_node_t *)lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    w->ast = src;
    list_append(one, &w->node);
    return lizard_primitive_import(one, env, heap);
  }
  /* parse options */
  for (cur = all->data.pair.cdr; cur != NULL && cur->type == AST_PAIR;
       cur = cur->data.pair.cdr) {
    lizard_ast_node_t *k = cur->data.pair.car;
    if (k->type == AST_SYMBOL && strcmp(k->data.variable, ":as") == 0 &&
        cur->data.pair.cdr != NULL && cur->data.pair.cdr->type == AST_PAIR) {
      alias = cur->data.pair.cdr->data.pair.car;
      cur = cur->data.pair.cdr;
    } else if (k->type == AST_SYMBOL && strcmp(k->data.variable, ":only") == 0 &&
               cur->data.pair.cdr != NULL && cur->data.pair.cdr->type == AST_PAIR) {
      only = cur->data.pair.cdr->data.pair.car;
      cur = cur->data.pair.cdr;
    } else if (k->type == AST_SYMBOL && strcmp(k->data.variable, ":all") == 0) {
      all_flag = 1;
    }
  }
  /* resolve the namespace */
  if (src->type == AST_STRING) {
    const char *path = src->data.string;
    const char *nm = alias && alias->type == AST_SYMBOL
                       ? alias->data.variable : basename_no_ext(heap, path);
    lz_list_t *forms = parse_module_file(heap, path);
    lizard_namespace_t *prev;
    if (forms == NULL) return lizard_make_error(heap, LIZARD_ERROR_LOAD_OPEN);
    ns = ns_begin(heap, nm, &prev);
    if (ns == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
    if (!run_forms_list(heap, ns->env, forms)) {
      heap->runtime->current_module = prev;
      return lizard_make_error(heap, LIZARD_ERROR_USER);
    }
    ns_commit(heap, ns, prev);
  } else if (src->type == AST_SYMBOL) {
    ns = ns_find(heap, src->data.variable);
  }
  if (ns == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  prefix = alias && alias->type == AST_SYMBOL ? alias->data.variable : ns->name;
  if (!import_bindings(heap, env, ns, prefix, only, all_flag))
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  return lizard_make_nil(heap);
}

