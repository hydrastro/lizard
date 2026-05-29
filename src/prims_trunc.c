/* src/prims_trunc.c -- propositional truncation primitives.
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

static int two_args(lz_list_t *args) {
  return args->head != args->nil && args->head->next != args->nil &&
         args->head->next->next == args->nil;
}

static int three_args(lz_list_t *args) {
  return args->head != args->nil && args->head->next != args->nil &&
         args->head->next->next != args->nil &&
         args->head->next->next->next == args->nil;
}

static int four_args(lz_list_t *args) {
  return args->head != args->nil && args->head->next != args->nil &&
         args->head->next->next != args->nil &&
         args->head->next->next->next != args->nil &&
         args->head->next->next->next->next == args->nil;
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

static int lizard_rule_on(const char *name) {
  return lizard_logic_rule_enabled(name) == 1;
}

lizard_ast_node_t *lizard_primitive_tt_trunc(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args) || !lizard_rule_on("truncations-enabled")) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_TRUNC;
  n->data.tt_trunc.level = nth_arg(args, 0);
  n->data.tt_trunc.type = nth_arg(args, 1);
  return n;
}

lizard_ast_node_t *lizard_primitive_tt_truncp(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  return lizard_make_bool(heap, x != NULL && x->type == AST_TT_TRUNC);
}

lizard_ast_node_t *lizard_primitive_tt_trunc_level(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  if (x == NULL || x->type != AST_TT_TRUNC) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  return x->data.tt_trunc.level;
}

lizard_ast_node_t *lizard_primitive_tt_trunc_type(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  if (x == NULL || x->type != AST_TT_TRUNC) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  return x->data.tt_trunc.type;
}

lizard_ast_node_t *lizard_primitive_tt_trunc_intro(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args) || !lizard_rule_on("truncations-enabled")) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_TRUNC_INTRO;
  n->data.tt_trunc_intro.value = nth_arg(args, 0);
  return n;
}

lizard_ast_node_t *lizard_primitive_tt_trunc_introp(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  return lizard_make_bool(heap, x != NULL && x->type == AST_TT_TRUNC_INTRO);
}

lizard_ast_node_t *lizard_primitive_tt_trunc_value(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  if (x == NULL || x->type != AST_TT_TRUNC_INTRO) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  return x->data.tt_trunc_intro.value;
}

lizard_ast_node_t *lizard_primitive_tt_trunc_elim(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  int has_prop;
  (void)env;
  /* Accept 3 args (scaffold, no prop witness) or 4 args (checked,
   * with prop witness for propositionality). */
  has_prop = four_args(args);
  if (!has_prop && !three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  if (!lizard_rule_on("truncations-enabled")) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_TRUNC_ELIM;
  n->data.tt_trunc_elim.motive = nth_arg(args, 0);
  n->data.tt_trunc_elim.handler = nth_arg(args, 1);
  if (has_prop) {
    n->data.tt_trunc_elim.prop  = nth_arg(args, 2);
    n->data.tt_trunc_elim.value = nth_arg(args, 3);
  } else {
    n->data.tt_trunc_elim.prop  = NULL;
    n->data.tt_trunc_elim.value = nth_arg(args, 2);
  }
  return n;
}

lizard_ast_node_t *lizard_primitive_tt_trunc_elimp(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  return lizard_make_bool(heap, x != NULL && x->type == AST_TT_TRUNC_ELIM);
}

lizard_ast_node_t *lizard_primitive_tt_trunc_elim_motive(lz_list_t *args,
                                                         lizard_env_t *env,
                                                         lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  if (x == NULL || x->type != AST_TT_TRUNC_ELIM) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  return x->data.tt_trunc_elim.motive;
}

lizard_ast_node_t *lizard_primitive_tt_trunc_elim_handler(lz_list_t *args,
                                                          lizard_env_t *env,
                                                          lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  if (x == NULL || x->type != AST_TT_TRUNC_ELIM) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  return x->data.tt_trunc_elim.handler;
}

lizard_ast_node_t *lizard_primitive_tt_trunc_elim_value(lz_list_t *args,
                                                        lizard_env_t *env,
                                                        lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  if (x == NULL || x->type != AST_TT_TRUNC_ELIM) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  return x->data.tt_trunc_elim.value;
}

lizard_ast_node_t *lizard_primitive_tt_trunc_elim_prop(lz_list_t *args,
                                                       lizard_env_t *env,
                                                       lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  if (x == NULL || x->type != AST_TT_TRUNC_ELIM) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  if (x->data.tt_trunc_elim.prop == NULL) return lizard_make_nil(heap);
  return x->data.tt_trunc_elim.prop;
}

