/* src/prims_theory_ext.c -- optional theory-extension scaffolds.
 *
 * Split out of primitives.c as part of Recoverable Core Phase 1K.
 */
#include "errors.h"
#include "lizard_internal.h"
#include "mem.h"
#include "primitives.h"

#include <string.h>


static int no_args(lz_list_t *args) {
  return args->head == args->nil;
}

static int single_arg(lz_list_t *args) {
  return args->head != args->nil && args->head->next == args->nil;
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

/* ===== Optional proof-theory scaffolds =====
 *
 * These constructors are intentionally opt-in. They are useful for designing
 * cubical S¹, truncations, and user-provided theory extensions, but they are
 * not trusted kernel terms yet.
 */

static int lizard_rule_on(const char *name) {
  return lizard_logic_rule_enabled(name) == 1;
}

static lizard_ast_node_t *lizard_make_nullary_tt(lizard_heap_t *heap,
                                                  lizard_ast_node_type_t type) {
  lizard_ast_node_t *n;
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = type;
  return n;
}

lizard_ast_node_t *lizard_primitive_tt_s1(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  (void)env;
  if (!no_args(args) || !lizard_rule_on("cubical-s1-enabled")) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  return lizard_make_nullary_tt(heap, AST_TT_S1);
}

lizard_ast_node_t *lizard_primitive_tt_s1p(lz_list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  return lizard_make_bool(heap, x != NULL && x->type == AST_TT_S1);
}

lizard_ast_node_t *lizard_primitive_tt_s1_base(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  (void)env;
  if (!no_args(args) || !lizard_rule_on("cubical-s1-enabled")) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  return lizard_make_nullary_tt(heap, AST_TT_S1_BASE);
}

lizard_ast_node_t *lizard_primitive_tt_s1_basep(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  return lizard_make_bool(heap, x != NULL && x->type == AST_TT_S1_BASE);
}

lizard_ast_node_t *lizard_primitive_tt_s1_loop(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  (void)env;
  if (!no_args(args) || !lizard_rule_on("cubical-s1-enabled")) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  return lizard_make_nullary_tt(heap, AST_TT_S1_LOOP);
}

lizard_ast_node_t *lizard_primitive_tt_s1_loopp(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  return lizard_make_bool(heap, x != NULL && x->type == AST_TT_S1_LOOP);
}

/* Truncation primitives live in prims_trunc.c. */

static lizard_ast_node_t *tt_list_to_lisp_list(lz_list_t *list,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *head;
  lizard_ast_node_t *tail;
  lz_list_node_t *iter;

  head = lizard_make_nil(heap);
  tail = NULL;
  if (list == NULL) return head;
  for (iter = list->head; iter != list->nil; iter = iter->next) {
    lizard_ast_node_t *cell;
    cell = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    cell->type = AST_PAIR;
    cell->data.pair.car = ((lizard_ast_list_node_t *)iter)->ast;
    cell->data.pair.cdr = lizard_make_nil(heap);
    if (tail == NULL) {
      head = cell;
    } else {
      tail->data.pair.cdr = cell;
    }
    tail = cell;
  }
  return head;
}

lizard_ast_node_t *lizard_primitive_tt_extension(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  lizard_ast_node_t *name;
  lz_list_t *rest;
  lz_list_node_t *iter;
  (void)env;
  if (args->head == args->nil || !lizard_rule_on("theory-extensions-enabled")) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  name = nth_arg(args, 0);
  if (name == NULL || name->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  rest = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  for (iter = args->head->next; iter != args->nil; iter = iter->next) {
    lizard_ast_list_node_t *copy;
    copy = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    copy->ast = ((lizard_ast_list_node_t *)iter)->ast;
    list_append(rest, &copy->node);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_EXTENSION;
  n->data.tt_extension.name = name;
  n->data.tt_extension.args = rest;
  return n;
}

lizard_ast_node_t *lizard_primitive_tt_extensionp(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  return lizard_make_bool(heap, x != NULL && x->type == AST_TT_EXTENSION);
}

lizard_ast_node_t *lizard_primitive_tt_extension_name(lz_list_t *args,
                                                      lizard_env_t *env,
                                                      lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  if (x == NULL || x->type != AST_TT_EXTENSION) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  return x->data.tt_extension.name;
}

lizard_ast_node_t *lizard_primitive_tt_extension_args(lz_list_t *args,
                                                      lizard_env_t *env,
                                                      lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  if (x == NULL || x->type != AST_TT_EXTENSION) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  return tt_list_to_lisp_list(x->data.tt_extension.args, heap);
}

