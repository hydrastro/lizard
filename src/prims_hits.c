/* src/prims_hits.c -- HIT construction/lookup primitives.
 *
 * Split out of primitives.c as part of Recoverable Core Phase 1J.
 */
#include "errors.h"
#include "lizard_internal.h"
#include "mem.h"
#include "primitives.h"

static int single_arg(lz_list_t *args) {
  return args->head != args->nil && args->head->next == args->nil;
}

static int three_args(lz_list_t *args) {
  return args->head != args->nil && args->head->next != args->nil &&
         args->head->next->next != args->nil &&
         args->head->next->next->next == args->nil;
}

static lizard_ast_node_t *nth_arg(lz_list_t *args, int n) {
  lz_list_node_t *cur;
  int i;
  cur = args->head;
  for (i = 0; cur != args->nil && i < n; i++) {
    cur = cur->next;
  }
  return cur == args->nil ? NULL : ((lizard_ast_list_node_t *)cur)->ast;
}

/* ===== Phase H.1 — HIT constructors =====
 *
 * (HIT 'name [HIT-constructor or HIT-path records ...])
 *   Build a HIT declaration. The variadic tail can be any mix of
 *   HIT_CONSTRUCTOR and HIT_PATH records; we sort them into two
 *   lists by node type. As a side effect, registers the declaration
 *   in the per-process HIT registry so it can be looked up by name.
 *
 * (HIT-constructor 'name arg-type-1 arg-type-2 ...)
 *   Build a point-constructor record. The first arg is the constructor
 *   name; the remaining args are the types of its arguments.
 *
 * (HIT-path 'name source target)
 *   Build a path-constructor record. Three args.
 *
 * (HIT-ref 'name) — use a registered HIT by name as a type.
 * (HIT-app 'cname arg1 arg2 ...) — apply a constructor.
 * (HIT-lookup 'name) — query the registry; returns the declaration or nil.
 */
lizard_ast_node_t *lizard_primitive_tt_hit_constructor(lz_list_t *args,
                                                       lizard_env_t *env,
                                                       lizard_heap_t *heap) {
  lizard_ast_node_t *n, *name_arg;
  lz_list_t *arg_types;
  lz_list_node_t *cur;
  int i, n_args;
  (void)env;
  /* Need at least the name; arg-types may be empty. */
  if (args->head == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (name_arg == NULL || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  /* Count args. */
  n_args = 0;
  cur = args->head;
  while (cur != args->nil) { n_args++; cur = cur->next; }
  /* Collect arg-types into a fresh list (everything after the name). */
  arg_types = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  for (i = 1; i < n_args; i++) {
    lizard_ast_list_node_t *cell = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    cell->ast = nth_arg(args, i);
    list_append(arg_types, &cell->node);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_HIT_CONSTRUCTOR;
  n->data.tt_hit_constructor.name = name_arg;
  n->data.tt_hit_constructor.arg_types = arg_types;
  return n;
}

lizard_ast_node_t *lizard_primitive_tt_hit_path(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_HIT_PATH;
  n->data.tt_hit_path.name = nth_arg(args, 0);
  n->data.tt_hit_path.source = nth_arg(args, 1);
  n->data.tt_hit_path.target = nth_arg(args, 2);
  return n;
}

lizard_ast_node_t *lizard_primitive_tt_hit_decl(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n, *name_arg, *child;
  lz_list_t *ctors, *paths;
  lz_list_node_t *cur;
  int i, n_args;
  (void)env;
  /* Need at least the name. */
  if (args->head == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (name_arg == NULL || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  ctors = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  paths = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  /* Count, then sort each tail arg by its AST type. */
  n_args = 0;
  cur = args->head;
  while (cur != args->nil) { n_args++; cur = cur->next; }
  for (i = 1; i < n_args; i++) {
    lizard_ast_list_node_t *cell;
    child = nth_arg(args, i);
    if (child == NULL) continue;
    cell = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    cell->ast = child;
    if (child->type == AST_TT_HIT_CONSTRUCTOR) {
      list_append(ctors, &cell->node);
    } else if (child->type == AST_TT_HIT_PATH) {
      list_append(paths, &cell->node);
    } else {
      /* Anything else is an arg-type error. */
      return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
    }
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_HIT_DECL;
  n->data.tt_hit_decl.name = name_arg;
  n->data.tt_hit_decl.constructors = ctors;
  n->data.tt_hit_decl.paths = paths;
  /* Side effect: register so subsequent (HIT-ref 'name) and (HIT-lookup
   * 'name) can find it. Note that this is module-level state, which is
   * a deliberate simplification for H.1. */
  lizard_tt_hit_register(n);
  return n;
}

lizard_ast_node_t *lizard_primitive_tt_hit_ref(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *n, *name_arg;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (name_arg == NULL || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_HIT_REF;
  n->data.tt_hit_ref.name = name_arg;
  return n;
}

lizard_ast_node_t *lizard_primitive_tt_hit_app(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *n, *name_arg;
  lz_list_t *app_args;
  lz_list_node_t *cur;
  int i, n_args;
  (void)env;
  if (args->head == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (name_arg == NULL || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  n_args = 0;
  cur = args->head;
  while (cur != args->nil) { n_args++; cur = cur->next; }
  app_args = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  for (i = 1; i < n_args; i++) {
    lizard_ast_list_node_t *cell = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    cell->ast = nth_arg(args, i);
    list_append(app_args, &cell->node);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_HIT_APP;
  n->data.tt_hit_app.name = name_arg;
  n->data.tt_hit_app.args = app_args;
  return n;
}

lizard_ast_node_t *lizard_primitive_tt_hit_lookup(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *name_arg, *result;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (name_arg == NULL || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  result = lizard_tt_hit_lookup(name_arg->data.variable);
  if (result == NULL) {
    /* Return nil for "not found". */
    lizard_ast_node_t *nil = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    nil->type = AST_NIL;
    return nil;
  }
  return result;
}
