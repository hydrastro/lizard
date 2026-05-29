/* Track T: Type-theory / cubical primitives.
 * Split out of primitives.c (#7 monolith split). Registration stays in
 * primitives.c; these definitions are linked from here. */
#include "primitives.h"
#include "env.h"
#include "errors.h"
#include "lizard_internal.h"
#include "mem.h"
#include "parser.h"
#include "printer.h"
#include "runtime.h"
#include "tokenizer.h"
#include "kernel.h"
#include "kernel_sexp.h"
#include "prims_shared.h"
#include <setjmp.h>
#include <stdint.h>
#include <string.h>


lizard_ast_node_t *lizard_primitive_tt_pi(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_PI;
  n->data.tt_pi.binder = nth_arg(args, 0);
  n->data.tt_pi.domain = nth_arg(args, 1);
  n->data.tt_pi.codomain = nth_arg(args, 2);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_sigma(lz_list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_SIGMA;
  n->data.tt_sigma.binder = nth_arg(args, 0);
  n->data.tt_sigma.domain = nth_arg(args, 1);
  n->data.tt_sigma.codomain = nth_arg(args, 2);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_pi_fresh(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_PI_FRESH;
  n->data.tt_pi_fresh.binder = nth_arg(args, 0);
  n->data.tt_pi_fresh.domain = nth_arg(args, 1);
  n->data.tt_pi_fresh.codomain = nth_arg(args, 2);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_sigma_fresh(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_SIGMA_FRESH;
  n->data.tt_sigma_fresh.binder = nth_arg(args, 0);
  n->data.tt_sigma_fresh.domain = nth_arg(args, 1);
  n->data.tt_sigma_fresh.codomain = nth_arg(args, 2);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_co_pi_fresh(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_CO_PI_FRESH;
  n->data.tt_co_pi_fresh.binder = nth_arg(args, 0);
  n->data.tt_co_pi_fresh.domain = nth_arg(args, 1);
  n->data.tt_co_pi_fresh.codomain = nth_arg(args, 2);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_co_sigma_fresh(lz_list_t *args,
                                                      lizard_env_t *env,
                                                      lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_CO_SIGMA_FRESH;
  n->data.tt_co_sigma_fresh.binder = nth_arg(args, 0);
  n->data.tt_co_sigma_fresh.domain = nth_arg(args, 1);
  n->data.tt_co_sigma_fresh.codomain = nth_arg(args, 2);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_box(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_BOX;
  n->data.tt_box.argument = nth_arg(args, 0);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_diamond(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_DIAMOND;
  n->data.tt_diamond.argument = nth_arg(args, 0);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_box_intro(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_BOX_INTRO;
  n->data.tt_box_intro.body = nth_arg(args, 0);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_box_elim(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_BOX_ELIM;
  n->data.tt_box_elim.binder = nth_arg(args, 0);
  n->data.tt_box_elim.scrutinee = nth_arg(args, 1);
  n->data.tt_box_elim.body = nth_arg(args, 2);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_diamond_intro(lz_list_t *args,
                                                     lizard_env_t *env,
                                                     lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_DIAMOND_INTRO;
  n->data.tt_diamond_intro.body = nth_arg(args, 0);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_diamond_elim(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_DIAMOND_ELIM;
  n->data.tt_diamond_elim.binder = nth_arg(args, 0);
  n->data.tt_diamond_elim.scrutinee = nth_arg(args, 1);
  n->data.tt_diamond_elim.body = nth_arg(args, 2);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_box_app(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_BOX_APP;
  n->data.tt_box_app.fun = nth_arg(args, 0);
  n->data.tt_box_app.arg = nth_arg(args, 1);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_diamond_bind(lz_list_t *args,
                                                     lizard_env_t *env,
                                                     lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_DIAMOND_BIND;
  n->data.tt_diamond_bind.fun = nth_arg(args, 0);
  n->data.tt_diamond_bind.arg = nth_arg(args, 1);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_diamond_intro_sym(lz_list_t *args,
                                                          lizard_env_t *env,
                                                          lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_DIAMOND_INTRO_SYM;
  n->data.tt_diamond_intro_sym.body = nth_arg(args, 0);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_poss_coerce(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_POSS_COERCE;
  n->data.tt_poss_coerce.body = nth_arg(args, 0);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_at(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_APP;
  n->data.tt_app.fun = nth_arg(args, 0);
  n->data.tt_app.arg = nth_arg(args, 1);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_sum(lz_list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_SUM;
  n->data.tt_sum.left = nth_arg(args, 0);
  n->data.tt_sum.right = nth_arg(args, 1);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_universe(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n, *lvl;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  lvl = nth_arg(args, 0);
  if (lvl->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_UNIVERSE;
  n->data.tt_universe.level = mpz_get_si(lvl->data.number);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_couniverse(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *n, *lvl;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  lvl = nth_arg(args, 0);
  if (lvl->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_COUNIVERSE;
  n->data.tt_couniverse.level = mpz_get_si(lvl->data.number);
  return n;
}
static long *parse_dim_args(lz_list_t *args, lizard_heap_t *heap,
                            long *count_out) {
  long *raw, *deduped;
  long count, i, j, unique;
  lz_list_node_t *cur;
  lizard_ast_node_t *arg;
  count = 0;
  cur = args->head;
  while (cur != args->nil) { count++; cur = cur->next; }
  if (count == 0) { *count_out = 0; return NULL; }
  raw = lizard_heap_alloc(sizeof(long) * (size_t)count);
  for (i = 0; i < count; i++) {
    arg = nth_arg(args, (int)i);
    if (arg == NULL || arg->type != AST_NUMBER) {
      *count_out = -1; return NULL;  /* sentinel for error */
    }
    raw[i] = mpz_get_si(arg->data.number);
    if (raw[i] < 0) {
      *count_out = -1; return NULL;
    }
  }
  /* Insertion sort. */
  for (i = 1; i < count; i++) {
    long key = raw[i];
    j = i - 1;
    while (j >= 0 && raw[j] > key) {
      raw[j+1] = raw[j];
      j--;
    }
    raw[j+1] = key;
  }
  /* Dedup in place. */
  unique = 1;
  for (i = 1; i < count; i++) {
    if (raw[i] != raw[unique-1]) {
      raw[unique++] = raw[i];
    }
  }
  if (unique == 0) { *count_out = 0; return NULL; }
  deduped = lizard_heap_alloc(sizeof(long) * (size_t)unique);
  for (i = 0; i < unique; i++) deduped[i] = raw[i];
  *count_out = unique;
  return deduped;
}
lizard_ast_node_t *lizard_primitive_tt_universe_set(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  long *dims, count;
  (void)env;
  dims = parse_dim_args(args, heap, &count);
  if (count < 0) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_UNIVERSE_SET;
  n->data.tt_universe_set.dims = dims;
  n->data.tt_universe_set.count = count;
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_couniverse_set(lz_list_t *args,
                                                      lizard_env_t *env,
                                                      lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  long *dims, count;
  (void)env;
  dims = parse_dim_args(args, heap, &count);
  if (count < 0) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_COUNIVERSE_SET;
  n->data.tt_couniverse_set.dims = dims;
  n->data.tt_couniverse_set.count = count;
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_co_max(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_CO_MAX;
  n->data.tt_co_max.left = nth_arg(args, 0);
  n->data.tt_co_max.right = nth_arg(args, 1);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_co_min(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_CO_MIN;
  n->data.tt_co_min.left = nth_arg(args, 0);
  n->data.tt_co_min.right = nth_arg(args, 1);
  return n;
}
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
lizard_ast_node_t *lizard_primitive_tt_id(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_ID;
  n->data.tt_id.domain = nth_arg(args, 0);
  n->data.tt_id.a = nth_arg(args, 1);
  n->data.tt_id.b = nth_arg(args, 2);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_refl(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_REFL;
  n->data.tt_refl.value = nth_arg(args, 0);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_inductive(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  lz_list_t *ctors;
  lz_list_node_t *it;
  (void)env;
  if (args->head == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  ctors = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  for (it = args->head->next; it != args->nil; it = it->next) {
    lizard_ast_list_node_t *w =
        lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    w->ast = ((lizard_ast_list_node_t *)it)->ast;
    list_append(ctors, &w->node);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_INDUCTIVE;
  n->data.tt_inductive.name = nth_arg(args, 0);
  n->data.tt_inductive.constructors = ctors;
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_coinductive(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  lz_list_t *dtors;
  lz_list_node_t *it;
  (void)env;
  if (args->head == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  dtors = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  for (it = args->head->next; it != args->nil; it = it->next) {
    lizard_ast_list_node_t *w =
        lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    w->ast = ((lizard_ast_list_node_t *)it)->ast;
    list_append(dtors, &w->node);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_COINDUCTIVE;
  n->data.tt_coinductive.name = nth_arg(args, 0);
  n->data.tt_coinductive.destructors = dtors;
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_annot(lz_list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_ANNOT;
  n->data.tt_annot.term = nth_arg(args, 0);
  n->data.tt_annot.type = nth_arg(args, 1);
  return n;
}
#define TT_PREDICATE(name, tag)                                                \
  lizard_ast_node_t *lizard_primitive_##name(lz_list_t *args,                  \
                                             lizard_env_t *env,                \
                                             lizard_heap_t *heap) {            \
    lizard_ast_node_t *x;                                                      \
    (void)env;                                                                 \
    if (!single_arg(args)) {                                                   \
      return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);             \
    }                                                                          \
    x = nth_arg(args, 0);                                                      \
    return lizard_make_bool(heap, x && x->type == tag);                        \
  }
TT_PREDICATE(tt_pip,          AST_TT_PI)
TT_PREDICATE(tt_sigmap,       AST_TT_SIGMA)
TT_PREDICATE(tt_pi_freshp,    AST_TT_PI_FRESH)
TT_PREDICATE(tt_sigma_freshp, AST_TT_SIGMA_FRESH)
TT_PREDICATE(tt_co_pi_freshp,    AST_TT_CO_PI_FRESH)
TT_PREDICATE(tt_co_sigma_freshp, AST_TT_CO_SIGMA_FRESH)
TT_PREDICATE(tt_boxp,     AST_TT_BOX)
TT_PREDICATE(tt_diamondp, AST_TT_DIAMOND)
TT_PREDICATE(tt_box_introp, AST_TT_BOX_INTRO)
TT_PREDICATE(tt_box_elimp,  AST_TT_BOX_ELIM)
TT_PREDICATE(tt_diamond_introp, AST_TT_DIAMOND_INTRO)
TT_PREDICATE(tt_diamond_elimp,  AST_TT_DIAMOND_ELIM)
TT_PREDICATE(tt_box_appp,       AST_TT_BOX_APP)
TT_PREDICATE(tt_diamond_bindp,  AST_TT_DIAMOND_BIND)
TT_PREDICATE(tt_diamond_intro_symp, AST_TT_DIAMOND_INTRO_SYM)
TT_PREDICATE(tt_poss_coercep,       AST_TT_POSS_COERCE)
TT_PREDICATE(tt_appp,         AST_TT_APP)
TT_PREDICATE(tt_sump,         AST_TT_SUM)
TT_PREDICATE(tt_universep,    AST_TT_UNIVERSE)
TT_PREDICATE(tt_couniversep,  AST_TT_COUNIVERSE)
TT_PREDICATE(tt_universe_setp, AST_TT_UNIVERSE_SET)
TT_PREDICATE(tt_couniverse_setp, AST_TT_COUNIVERSE_SET)
TT_PREDICATE(tt_co_maxp,       AST_TT_CO_MAX)
TT_PREDICATE(tt_co_minp,       AST_TT_CO_MIN)
TT_PREDICATE(tt_hit_declp,     AST_TT_HIT_DECL)
TT_PREDICATE(tt_hit_constructorp, AST_TT_HIT_CONSTRUCTOR)
TT_PREDICATE(tt_hit_pathp,     AST_TT_HIT_PATH)
TT_PREDICATE(tt_hit_refp,      AST_TT_HIT_REF)
TT_PREDICATE(tt_hit_appp,      AST_TT_HIT_APP)
TT_PREDICATE(tt_idp,          AST_TT_ID)
TT_PREDICATE(tt_reflp,        AST_TT_REFL)
TT_PREDICATE(tt_inductivep,   AST_TT_INDUCTIVE)
TT_PREDICATE(tt_coinductivep, AST_TT_COINDUCTIVE)
TT_PREDICATE(tt_annotp,       AST_TT_ANNOT)

#define TT_ACCESSOR(name, tag, expr)                                           \
  lizard_ast_node_t *lizard_primitive_##name(lz_list_t *args,                  \
                                             lizard_env_t *env,                \
                                             lizard_heap_t *heap) {            \
    lizard_ast_node_t *x;                                                      \
    lizard_ast_node_t *r;                                                      \
    (void)env;                                                                 \
    if (!single_arg(args)) {                                                   \
      return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);             \
    }                                                                          \
    x = nth_arg(args, 0);                                                      \
    if (!x || x->type != tag) {                                                \
      return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);                  \
    }                                                                          \
    r = (expr);                                                                \
    return r ? r : lizard_make_nil(heap);                                      \
  }
TT_ACCESSOR(tt_pi_binder,    AST_TT_PI,    x->data.tt_pi.binder)
TT_ACCESSOR(tt_pi_domain,    AST_TT_PI,    x->data.tt_pi.domain)
TT_ACCESSOR(tt_pi_codomain,  AST_TT_PI,    x->data.tt_pi.codomain)
TT_ACCESSOR(tt_sigma_binder, AST_TT_SIGMA, x->data.tt_sigma.binder)
TT_ACCESSOR(tt_sigma_domain, AST_TT_SIGMA, x->data.tt_sigma.domain)
TT_ACCESSOR(tt_sigma_codomain, AST_TT_SIGMA, x->data.tt_sigma.codomain)
TT_ACCESSOR(tt_app_fun,      AST_TT_APP,   x->data.tt_app.fun)
TT_ACCESSOR(tt_app_arg,      AST_TT_APP,   x->data.tt_app.arg)
TT_ACCESSOR(tt_sum_left,     AST_TT_SUM,   x->data.tt_sum.left)
TT_ACCESSOR(tt_sum_right,    AST_TT_SUM,   x->data.tt_sum.right)
TT_ACCESSOR(tt_box_arg,      AST_TT_BOX,     x->data.tt_box.argument)
TT_ACCESSOR(tt_diamond_arg,  AST_TT_DIAMOND, x->data.tt_diamond.argument)
TT_ACCESSOR(tt_box_intro_body, AST_TT_BOX_INTRO, x->data.tt_box_intro.body)
TT_ACCESSOR(tt_box_elim_binder,    AST_TT_BOX_ELIM, x->data.tt_box_elim.binder)
TT_ACCESSOR(tt_box_elim_scrutinee, AST_TT_BOX_ELIM, x->data.tt_box_elim.scrutinee)
TT_ACCESSOR(tt_box_elim_body,      AST_TT_BOX_ELIM, x->data.tt_box_elim.body)
TT_ACCESSOR(tt_diamond_intro_body,    AST_TT_DIAMOND_INTRO, x->data.tt_diamond_intro.body)
TT_ACCESSOR(tt_diamond_elim_binder,    AST_TT_DIAMOND_ELIM, x->data.tt_diamond_elim.binder)
TT_ACCESSOR(tt_diamond_elim_scrutinee, AST_TT_DIAMOND_ELIM, x->data.tt_diamond_elim.scrutinee)
TT_ACCESSOR(tt_diamond_elim_body,      AST_TT_DIAMOND_ELIM, x->data.tt_diamond_elim.body)
TT_ACCESSOR(tt_box_app_fun, AST_TT_BOX_APP, x->data.tt_box_app.fun)
TT_ACCESSOR(tt_box_app_arg, AST_TT_BOX_APP, x->data.tt_box_app.arg)
TT_ACCESSOR(tt_diamond_bind_fun, AST_TT_DIAMOND_BIND, x->data.tt_diamond_bind.fun)
TT_ACCESSOR(tt_diamond_bind_arg, AST_TT_DIAMOND_BIND, x->data.tt_diamond_bind.arg)
TT_ACCESSOR(tt_diamond_intro_sym_body, AST_TT_DIAMOND_INTRO_SYM, x->data.tt_diamond_intro_sym.body)
TT_ACCESSOR(tt_poss_coerce_body,       AST_TT_POSS_COERCE,       x->data.tt_poss_coerce.body)
TT_ACCESSOR(tt_id_domain,    AST_TT_ID,    x->data.tt_id.domain)
TT_ACCESSOR(tt_id_a,         AST_TT_ID,    x->data.tt_id.a)
TT_ACCESSOR(tt_id_b,         AST_TT_ID,    x->data.tt_id.b)
TT_ACCESSOR(tt_refl_value,   AST_TT_REFL,  x->data.tt_refl.value)
TT_ACCESSOR(tt_inductive_name, AST_TT_INDUCTIVE, x->data.tt_inductive.name)
TT_ACCESSOR(tt_coinductive_name, AST_TT_COINDUCTIVE,
            x->data.tt_coinductive.name)
TT_ACCESSOR(tt_annot_term,   AST_TT_ANNOT, x->data.tt_annot.term)
TT_ACCESSOR(tt_annot_type,   AST_TT_ANNOT, x->data.tt_annot.type)

lizard_ast_node_t *lizard_primitive_tt_universe_level(lz_list_t *args,
                                                      lizard_env_t *env,
                                                      lizard_heap_t *heap) {
  lizard_ast_node_t *x, *r;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  x = nth_arg(args, 0);
  if (!x || x->type != AST_TT_UNIVERSE) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_NUMBER;
  mpz_init_set_si(r->data.number, x->data.tt_universe.level);
  return r;
}
lizard_ast_node_t *lizard_primitive_tt_couniverse_level(lz_list_t *args,
                                                        lizard_env_t *env,
                                                        lizard_heap_t *heap) {
  lizard_ast_node_t *x, *r;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  x = nth_arg(args, 0);
  if (!x || x->type != AST_TT_COUNIVERSE) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_NUMBER;
  mpz_init_set_si(r->data.number, x->data.tt_couniverse.level);
  return r;
}
lizard_ast_node_t *lizard_primitive_tt_inductive_ctors(lz_list_t *args,
                                                       lizard_env_t *env,
                                                       lizard_heap_t *heap) {
  lizard_ast_node_t *x, *result;
  lz_list_node_t *it;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  x = nth_arg(args, 0);
  if (!x || x->type != AST_TT_INDUCTIVE) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  result = lizard_make_nil(heap);
  /* Build the list in reverse so the result preserves order. */
  for (it = x->data.tt_inductive.constructors->head;
       it != x->data.tt_inductive.constructors->nil; it = it->next) {
    /* skip — we'll build properly below */;
  }
  {
    /* easier: collect into an array first */
    size_t n = 0, i;
    lizard_ast_node_t **buf;
    for (it = x->data.tt_inductive.constructors->head;
         it != x->data.tt_inductive.constructors->nil; it = it->next) n++;
    buf = lizard_heap_alloc((n + 1) * sizeof(lizard_ast_node_t *));
    i = 0;
    for (it = x->data.tt_inductive.constructors->head;
         it != x->data.tt_inductive.constructors->nil; it = it->next) {
      buf[i++] = ((lizard_ast_list_node_t *)it)->ast;
    }
    for (i = n; i > 0; i--) {
      lizard_ast_node_t *p = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      p->type = AST_PAIR;
      p->data.pair.car = buf[i - 1];
      p->data.pair.cdr = result;
      result = p;
    }
  }
  return result;
}
lizard_ast_node_t *lizard_primitive_tt_coinductive_dtors(lz_list_t *args,
                                                         lizard_env_t *env,
                                                         lizard_heap_t *heap) {
  lizard_ast_node_t *x, *result;
  lz_list_node_t *it;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  x = nth_arg(args, 0);
  if (!x || x->type != AST_TT_COINDUCTIVE) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  result = lizard_make_nil(heap);
  {
    size_t n = 0, i;
    lizard_ast_node_t **buf;
    for (it = x->data.tt_coinductive.destructors->head;
         it != x->data.tt_coinductive.destructors->nil; it = it->next) n++;
    buf = lizard_heap_alloc((n + 1) * sizeof(lizard_ast_node_t *));
    i = 0;
    for (it = x->data.tt_coinductive.destructors->head;
         it != x->data.tt_coinductive.destructors->nil; it = it->next) {
      buf[i++] = ((lizard_ast_list_node_t *)it)->ast;
    }
    for (i = n; i > 0; i--) {
      lizard_ast_node_t *p = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      p->type = AST_PAIR;
      p->data.pair.car = buf[i - 1];
      p->data.pair.cdr = result;
      result = p;
    }
  }
  return result;
}
lizard_ast_node_t *lizard_primitive_tt_variable(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_VARIABLE;
  n->data.tt_variable.name = nth_arg(args, 0);
  n->data.tt_variable.type = nth_arg(args, 1);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_context(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  lz_list_t *bindings;
  lz_list_node_t *it;
  (void)env;
  bindings = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  for (it = args->head; it != args->nil; it = it->next) {
    lizard_ast_list_node_t *w =
        lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    w->ast = ((lizard_ast_list_node_t *)it)->ast;
    list_append(bindings, &w->node);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_CONTEXT;
  n->data.tt_context.bindings = bindings;
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_ctx_extend(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *ctx, *var, *n;
  lz_list_t *bindings;
  lz_list_node_t *it;
  lizard_ast_list_node_t *w;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  ctx = nth_arg(args, 0);
  var = nth_arg(args, 1);
  if (!ctx || ctx->type != AST_TT_CONTEXT) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  bindings = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  for (it = ctx->data.tt_context.bindings->head;
       it != ctx->data.tt_context.bindings->nil; it = it->next) {
    w = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    w->ast = ((lizard_ast_list_node_t *)it)->ast;
    list_append(bindings, &w->node);
  }
  w = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
  w->ast = var;
  list_append(bindings, &w->node);
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_CONTEXT;
  n->data.tt_context.bindings = bindings;
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_ctx_lookup(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *ctx, *name;
  lz_list_node_t *it;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  ctx = nth_arg(args, 0);
  name = nth_arg(args, 1);
  if (!ctx || ctx->type != AST_TT_CONTEXT) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  if (!name || name->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  /* Walk right-to-left (innermost first) so shadowing works the
   * usual way. */
  {
    /* Build an array first so we can iterate backwards. */
    size_t n = 0, i;
    lizard_ast_node_t **buf;
    for (it = ctx->data.tt_context.bindings->head;
         it != ctx->data.tt_context.bindings->nil; it = it->next) n++;
    buf = lizard_heap_alloc((n + 1) * sizeof(lizard_ast_node_t *));
    i = 0;
    for (it = ctx->data.tt_context.bindings->head;
         it != ctx->data.tt_context.bindings->nil; it = it->next) {
      buf[i++] = ((lizard_ast_list_node_t *)it)->ast;
    }
    for (i = n; i > 0; i--) {
      lizard_ast_node_t *v = buf[i - 1];
      if (v && v->type == AST_TT_VARIABLE && v->data.tt_variable.name &&
          v->data.tt_variable.name->type == AST_SYMBOL &&
          strcmp(v->data.tt_variable.name->data.variable,
                 name->data.variable) == 0) {
        return v;
      }
    }
  }
  return lizard_make_bool(heap, false);
}
lizard_ast_node_t *lizard_primitive_tt_ctx_bindings(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  lizard_ast_node_t *ctx, *result;
  lz_list_node_t *it;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  ctx = nth_arg(args, 0);
  if (!ctx || ctx->type != AST_TT_CONTEXT) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  result = lizard_make_nil(heap);
  {
    size_t n = 0, i;
    lizard_ast_node_t **buf;
    for (it = ctx->data.tt_context.bindings->head;
         it != ctx->data.tt_context.bindings->nil; it = it->next) n++;
    buf = lizard_heap_alloc((n + 1) * sizeof(lizard_ast_node_t *));
    i = 0;
    for (it = ctx->data.tt_context.bindings->head;
         it != ctx->data.tt_context.bindings->nil; it = it->next) {
      buf[i++] = ((lizard_ast_list_node_t *)it)->ast;
    }
    for (i = n; i > 0; i--) {
      lizard_ast_node_t *p = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      p->type = AST_PAIR;
      p->data.pair.car = buf[i - 1];
      p->data.pair.cdr = result;
      result = p;
    }
  }
  return result;
}
lizard_ast_node_t *lizard_primitive_tt_ctx_length(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *ctx, *r;
  lz_list_node_t *it;
  long count = 0;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  ctx = nth_arg(args, 0);
  if (!ctx || ctx->type != AST_TT_CONTEXT) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  for (it = ctx->data.tt_context.bindings->head;
       it != ctx->data.tt_context.bindings->nil; it = it->next) count++;
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_NUMBER;
  mpz_init_set_si(r->data.number, count);
  return r;
}
lizard_ast_node_t *lizard_primitive_tt_substitution(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  lizard_ast_node_t *src, *tgt, *maps_arg, *n;
  lz_list_t *mappings;
  lizard_ast_node_t *cur;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  src = nth_arg(args, 0);
  tgt = nth_arg(args, 1);
  maps_arg = nth_arg(args, 2);
  mappings = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  for (cur = maps_arg; cur && cur->type == AST_PAIR; cur = cur->data.pair.cdr) {
    lizard_ast_list_node_t *w =
        lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    w->ast = cur->data.pair.car;
    list_append(mappings, &w->node);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_SUBSTITUTION;
  n->data.tt_substitution.source = src;
  n->data.tt_substitution.target = tgt;
  n->data.tt_substitution.mappings = mappings;
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_judgment(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_JUDGMENT;
  n->data.tt_judgment.context = nth_arg(args, 0);
  n->data.tt_judgment.term = nth_arg(args, 1);
  n->data.tt_judgment.type = nth_arg(args, 2);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_empty_ctx(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  (void)args;
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_CONTEXT;
  n->data.tt_context.bindings =
      list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_uco_level(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *x, *r;
  long lvl;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  x = nth_arg(args, 0);
  if (!x) return lizard_make_bool(heap, false);
  switch (x->type) {
  case AST_TT_VARIABLE:     lvl = -2; break;
  case AST_TT_CONTEXT:      lvl = -1; break;
  case AST_TT_SUBSTITUTION: lvl =  0; break;
  case AST_TT_JUDGMENT:     lvl =  0; break;
  default:
    return lizard_make_bool(heap, false);
  }
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_NUMBER;
  mpz_init_set_si(r->data.number, lvl);
  return r;
}
TT_PREDICATE(tt_variablep,     AST_TT_VARIABLE)
TT_PREDICATE(tt_contextp,      AST_TT_CONTEXT)
TT_PREDICATE(tt_substitutionp, AST_TT_SUBSTITUTION)
TT_PREDICATE(tt_judgmentp,     AST_TT_JUDGMENT)

TT_ACCESSOR(tt_var_name,      AST_TT_VARIABLE,     x->data.tt_variable.name)
TT_ACCESSOR(tt_var_type,      AST_TT_VARIABLE,     x->data.tt_variable.type)
TT_ACCESSOR(tt_subst_source,  AST_TT_SUBSTITUTION, x->data.tt_substitution.source)
TT_ACCESSOR(tt_subst_target,  AST_TT_SUBSTITUTION, x->data.tt_substitution.target)
TT_ACCESSOR(tt_judg_context,  AST_TT_JUDGMENT,     x->data.tt_judgment.context)
TT_ACCESSOR(tt_judg_term,     AST_TT_JUDGMENT,     x->data.tt_judgment.term)
TT_ACCESSOR(tt_judg_type,     AST_TT_JUDGMENT,     x->data.tt_judgment.type)

lizard_ast_node_t *lizard_primitive_tt_equiv(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (args->head == args->nil || args->head->next == args->nil ||
      args->head->next->next == args->nil ||
      args->head->next->next->next == args->nil ||
      args->head->next->next->next->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_EQUIV;
  n->data.tt_equiv.left  = nth_arg(args, 0);
  n->data.tt_equiv.right = nth_arg(args, 1);
  n->data.tt_equiv.fwd   = nth_arg(args, 2);
  n->data.tt_equiv.bwd   = nth_arg(args, 3);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_transport(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_TRANSPORT;
  n->data.tt_transport.path = nth_arg(args, 0);
  n->data.tt_transport.value = nth_arg(args, 1);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_id_sym(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_ID_SYM;
  n->data.tt_id_sym.path = nth_arg(args, 0);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_id_trans(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_ID_TRANS;
  n->data.tt_id_trans.p = nth_arg(args, 0);
  n->data.tt_id_trans.q = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_equivp,     AST_TT_EQUIV)
TT_PREDICATE(tt_transportp, AST_TT_TRANSPORT)
TT_PREDICATE(tt_id_symp,    AST_TT_ID_SYM)
TT_PREDICATE(tt_id_transp,  AST_TT_ID_TRANS)

TT_ACCESSOR(tt_equiv_left,    AST_TT_EQUIV,     x->data.tt_equiv.left)
TT_ACCESSOR(tt_equiv_right,   AST_TT_EQUIV,     x->data.tt_equiv.right)
TT_ACCESSOR(tt_equiv_fwd,     AST_TT_EQUIV,     x->data.tt_equiv.fwd)
TT_ACCESSOR(tt_equiv_bwd,     AST_TT_EQUIV,     x->data.tt_equiv.bwd)
TT_ACCESSOR(tt_transport_path,  AST_TT_TRANSPORT, x->data.tt_transport.path)
TT_ACCESSOR(tt_transport_value, AST_TT_TRANSPORT, x->data.tt_transport.value)
TT_ACCESSOR(tt_id_sym_path,   AST_TT_ID_SYM,    x->data.tt_id_sym.path)
TT_ACCESSOR(tt_id_trans_p,    AST_TT_ID_TRANS,  x->data.tt_id_trans.p)
TT_ACCESSOR(tt_id_trans_q,    AST_TT_ID_TRANS,  x->data.tt_id_trans.q)

lizard_ast_node_t *lizard_primitive_tt_lambda(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_LAMBDA;
  n->data.tt_lambda.binder = nth_arg(args, 0);
  n->data.tt_lambda.body   = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_lambdap, AST_TT_LAMBDA)
TT_ACCESSOR(tt_lambda_binder, AST_TT_LAMBDA, x->data.tt_lambda.binder)
TT_ACCESSOR(tt_lambda_body,   AST_TT_LAMBDA, x->data.tt_lambda.body)

lizard_ast_node_t *lizard_primitive_tt_ap(lz_list_t *args,
                                          lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_AP;
  n->data.tt_ap.fn   = nth_arg(args, 0);
  n->data.tt_ap.path = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_app_p_hott, AST_TT_AP)
TT_ACCESSOR(tt_ap_fn,   AST_TT_AP, x->data.tt_ap.fn)
TT_ACCESSOR(tt_ap_path, AST_TT_AP, x->data.tt_ap.path)

lizard_ast_node_t *lizard_primitive_tt_pair(lz_list_t *args,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_PAIR;
  n->data.tt_pair.fst = nth_arg(args, 0);
  n->data.tt_pair.snd = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_pairp, AST_TT_PAIR)
TT_ACCESSOR(tt_pair_first,  AST_TT_PAIR, x->data.tt_pair.fst)
TT_ACCESSOR(tt_pair_second, AST_TT_PAIR, x->data.tt_pair.snd)

lizard_ast_node_t *lizard_primitive_tt_fst(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_FST;
  n->data.tt_proj.target = nth_arg(args, 0);
  return n;
}
TT_PREDICATE(tt_fstp, AST_TT_FST)
TT_ACCESSOR(tt_fst_target, AST_TT_FST, x->data.tt_proj.target)

lizard_ast_node_t *lizard_primitive_tt_snd(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_SND;
  n->data.tt_proj.target = nth_arg(args, 0);
  return n;
}
TT_PREDICATE(tt_sndp, AST_TT_SND)
TT_ACCESSOR(tt_snd_target, AST_TT_SND, x->data.tt_proj.target)

lizard_ast_node_t *lizard_primitive_tt_inl(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_INL;
  n->data.tt_inj.value = nth_arg(args, 0);
  return n;
}
TT_PREDICATE(tt_inlp, AST_TT_INL)
TT_ACCESSOR(tt_inl_value, AST_TT_INL, x->data.tt_inj.value)

lizard_ast_node_t *lizard_primitive_tt_inr(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_INR;
  n->data.tt_inj.value = nth_arg(args, 0);
  return n;
}
TT_PREDICATE(tt_inrp, AST_TT_INR)
TT_ACCESSOR(tt_inr_value, AST_TT_INR, x->data.tt_inj.value)

lizard_ast_node_t *lizard_primitive_tt_case(lz_list_t *args,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_CASE;
  n->data.tt_case.scrutinee    = nth_arg(args, 0);
  n->data.tt_case.left_branch  = nth_arg(args, 1);
  n->data.tt_case.right_branch = nth_arg(args, 2);
  return n;
}
TT_PREDICATE(tt_casep, AST_TT_CASE)
TT_ACCESSOR(tt_case_scrutinee, AST_TT_CASE, x->data.tt_case.scrutinee)
TT_ACCESSOR(tt_case_left,  AST_TT_CASE, x->data.tt_case.left_branch)
TT_ACCESSOR(tt_case_right, AST_TT_CASE, x->data.tt_case.right_branch)

lizard_ast_node_t *lizard_primitive_tt_unit(lz_list_t *args,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env; (void)args;
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_UNIT;
  return n;
}
TT_PREDICATE(tt_unitp, AST_TT_UNIT)

lizard_ast_node_t *lizard_primitive_tt_tt(lz_list_t *args,
                                          lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env; (void)args;
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_TT;
  return n;
}
TT_PREDICATE(tt_ttp, AST_TT_TT)

lizard_ast_node_t *lizard_primitive_tt_bot(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env; (void)args;
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_BOT;
  return n;
}
TT_PREDICATE(tt_botp, AST_TT_BOT)

lizard_ast_node_t *lizard_primitive_tt_j(lz_list_t *args,
                                         lizard_env_t *env,
                                         lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_J;
  n->data.tt_j.motive    = nth_arg(args, 0);
  n->data.tt_j.refl_case = nth_arg(args, 1);
  n->data.tt_j.path      = nth_arg(args, 2);
  return n;
}
TT_PREDICATE(tt_jp, AST_TT_J)
TT_ACCESSOR(tt_j_motive,    AST_TT_J, x->data.tt_j.motive)
TT_ACCESSOR(tt_j_refl_case, AST_TT_J, x->data.tt_j.refl_case)
TT_ACCESSOR(tt_j_path,      AST_TT_J, x->data.tt_j.path)

lizard_ast_node_t *lizard_primitive_tt_xport(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_XPORT;
  n->data.tt_xport.motive = nth_arg(args, 0);
  n->data.tt_xport.path   = nth_arg(args, 1);
  n->data.tt_xport.value  = nth_arg(args, 2);
  return n;
}
TT_PREDICATE(tt_xportp, AST_TT_XPORT)
TT_ACCESSOR(tt_xport_motive, AST_TT_XPORT, x->data.tt_xport.motive)
TT_ACCESSOR(tt_xport_path,   AST_TT_XPORT, x->data.tt_xport.path)
TT_ACCESSOR(tt_xport_value,  AST_TT_XPORT, x->data.tt_xport.value)

lizard_ast_node_t *lizard_primitive_tt_u_var(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *n, *name_arg;
  char *buf;
  size_t len;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (!name_arg || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_U_VAR;
  len = strlen(name_arg->data.variable);
  buf = lizard_heap_alloc(len + 1);
  strcpy(buf, name_arg->data.variable);
  n->data.tt_u_var.name = buf;
  return n;
}
TT_PREDICATE(tt_u_varp, AST_TT_U_VAR)

lizard_ast_node_t *lizard_primitive_tt_u_suc(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_U_SUC;
  n->data.tt_u_suc.operand = nth_arg(args, 0);
  return n;
}
TT_PREDICATE(tt_u_sucp, AST_TT_U_SUC)
TT_ACCESSOR(tt_u_suc_operand, AST_TT_U_SUC, x->data.tt_u_suc.operand)

lizard_ast_node_t *lizard_primitive_tt_u_max(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_U_MAX;
  n->data.tt_u_max.left = nth_arg(args, 0);
  n->data.tt_u_max.right = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_u_maxp, AST_TT_U_MAX)
TT_ACCESSOR(tt_u_max_left,  AST_TT_U_MAX, x->data.tt_u_max.left)
TT_ACCESSOR(tt_u_max_right, AST_TT_U_MAX, x->data.tt_u_max.right)

lizard_ast_node_t *lizard_primitive_tt_u_min(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_U_MIN;
  n->data.tt_u_min.left = nth_arg(args, 0);
  n->data.tt_u_min.right = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_u_minp, AST_TT_U_MIN)
TT_ACCESSOR(tt_u_min_left,  AST_TT_U_MIN, x->data.tt_u_min.left)
TT_ACCESSOR(tt_u_min_right, AST_TT_U_MIN, x->data.tt_u_min.right)

lizard_ast_node_t *lizard_primitive_tt_interval(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env; (void)args;
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_INTERVAL;
  return n;
}
TT_PREDICATE(tt_intervalp, AST_TT_INTERVAL)

lizard_ast_node_t *lizard_primitive_tt_i0(lz_list_t *args,
                                          lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env; (void)args;
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_I0;
  return n;
}
TT_PREDICATE(tt_i0p, AST_TT_I0)

lizard_ast_node_t *lizard_primitive_tt_i1(lz_list_t *args,
                                          lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env; (void)args;
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_I1;
  return n;
}
TT_PREDICATE(tt_i1p, AST_TT_I1)

lizard_ast_node_t *lizard_primitive_tt_i_var(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *n, *name_arg;
  char *buf;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (!name_arg || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_I_VAR;
  buf = lizard_heap_alloc(strlen(name_arg->data.variable) + 1);
  strcpy(buf, name_arg->data.variable);
  n->data.tt_i_var.name = buf;
  return n;
}
TT_PREDICATE(tt_i_varp, AST_TT_I_VAR)

lizard_ast_node_t *lizard_primitive_tt_i_and(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_I_AND;
  n->data.tt_i_binop.left = nth_arg(args, 0);
  n->data.tt_i_binop.right = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_i_andp, AST_TT_I_AND)

lizard_ast_node_t *lizard_primitive_tt_i_or(lz_list_t *args,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_I_OR;
  n->data.tt_i_binop.left = nth_arg(args, 0);
  n->data.tt_i_binop.right = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_i_orp, AST_TT_I_OR)

lizard_ast_node_t *lizard_primitive_tt_i_neg(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_I_NEG;
  n->data.tt_i_neg.operand = nth_arg(args, 0);
  return n;
}
TT_PREDICATE(tt_i_negp, AST_TT_I_NEG)

lizard_ast_node_t *lizard_primitive_tt_path(lz_list_t *args,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_PATH;
  n->data.tt_path.domain = nth_arg(args, 0);
  n->data.tt_path.a      = nth_arg(args, 1);
  n->data.tt_path.b      = nth_arg(args, 2);
  return n;
}
TT_PREDICATE(tt_pathp, AST_TT_PATH)
TT_ACCESSOR(tt_path_domain, AST_TT_PATH, x->data.tt_path.domain)
TT_ACCESSOR(tt_path_a,      AST_TT_PATH, x->data.tt_path.a)
TT_ACCESSOR(tt_path_b,      AST_TT_PATH, x->data.tt_path.b)

lizard_ast_node_t *lizard_primitive_tt_path_abs(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n, *binder_arg;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  binder_arg = nth_arg(args, 0);
  if (!binder_arg || binder_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_PATH_ABS;
  n->data.tt_path_abs.binder = binder_arg;
  n->data.tt_path_abs.body = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_path_absp, AST_TT_PATH_ABS)
TT_ACCESSOR(tt_path_abs_binder, AST_TT_PATH_ABS, x->data.tt_path_abs.binder)
TT_ACCESSOR(tt_path_abs_body,   AST_TT_PATH_ABS, x->data.tt_path_abs.body)

lizard_ast_node_t *lizard_primitive_tt_path_app(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_PATH_APP;
  n->data.tt_path_app.path  = nth_arg(args, 0);
  n->data.tt_path_app.point = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_path_appp, AST_TT_PATH_APP)
TT_ACCESSOR(tt_path_app_path,  AST_TT_PATH_APP, x->data.tt_path_app.path)
TT_ACCESSOR(tt_path_app_point, AST_TT_PATH_APP, x->data.tt_path_app.point)

lizard_ast_node_t *lizard_primitive_tt_f0(lz_list_t *args,
                                          lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env; (void)args;
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_F0;
  return n;
}
TT_PREDICATE(tt_f0p, AST_TT_F0)

lizard_ast_node_t *lizard_primitive_tt_f1(lz_list_t *args,
                                          lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env; (void)args;
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_F1;
  return n;
}
TT_PREDICATE(tt_f1p, AST_TT_F1)

lizard_ast_node_t *lizard_primitive_tt_f_eq(lz_list_t *args,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_F_EQ;
  n->data.tt_f_eq.left  = nth_arg(args, 0);
  n->data.tt_f_eq.right = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_f_eqp, AST_TT_F_EQ)
TT_ACCESSOR(tt_f_eq_left,  AST_TT_F_EQ, x->data.tt_f_eq.left)
TT_ACCESSOR(tt_f_eq_right, AST_TT_F_EQ, x->data.tt_f_eq.right)

lizard_ast_node_t *lizard_primitive_tt_f_and(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_F_AND;
  n->data.tt_f_binop.left  = nth_arg(args, 0);
  n->data.tt_f_binop.right = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_f_andp, AST_TT_F_AND)

lizard_ast_node_t *lizard_primitive_tt_f_or(lz_list_t *args,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_F_OR;
  n->data.tt_f_binop.left  = nth_arg(args, 0);
  n->data.tt_f_binop.right = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_f_orp, AST_TT_F_OR)

lizard_ast_node_t *lizard_primitive_tt_partial(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_PARTIAL;
  n->data.tt_partial.face = nth_arg(args, 0);
  n->data.tt_partial.type = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_partialp, AST_TT_PARTIAL)
TT_ACCESSOR(tt_partial_face, AST_TT_PARTIAL, x->data.tt_partial.face)
TT_ACCESSOR(tt_partial_type, AST_TT_PARTIAL, x->data.tt_partial.type)

lizard_ast_node_t *lizard_primitive_tt_sub(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_SUB;
  n->data.tt_sub.type    = nth_arg(args, 0);
  n->data.tt_sub.face    = nth_arg(args, 1);
  n->data.tt_sub.partial = nth_arg(args, 2);
  return n;
}
TT_PREDICATE(tt_subp, AST_TT_SUB)
TT_ACCESSOR(tt_sub_type,    AST_TT_SUB, x->data.tt_sub.type)
TT_ACCESSOR(tt_sub_face,    AST_TT_SUB, x->data.tt_sub.face)
TT_ACCESSOR(tt_sub_partial, AST_TT_SUB, x->data.tt_sub.partial)

#define MAKE_COMP_FAMILY(NAME, AST_KIND)                                       \
  lizard_ast_node_t *lizard_primitive_tt_##NAME(lz_list_t *args,               \
                                                lizard_env_t *env,             \
                                                lizard_heap_t *heap) {         \
    lizard_ast_node_t *n;                                                      \
    (void)env;                                                                 \
    if (!four_args(args)) {                                                    \
      return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);             \
    }                                                                          \
    n = lizard_heap_alloc(sizeof(lizard_ast_node_t));                          \
    n->type = AST_KIND;                                                        \
    n->data.tt_comp.type_family = nth_arg(args, 0);                            \
    n->data.tt_comp.face        = nth_arg(args, 1);                            \
    n->data.tt_comp.partial     = nth_arg(args, 2);                            \
    n->data.tt_comp.base        = nth_arg(args, 3);                            \
    return n;                                                                  \
  }                                                                            \
  TT_PREDICATE(tt_##NAME##p, AST_KIND)

MAKE_COMP_FAMILY(comp,  AST_TT_COMP)
MAKE_COMP_FAMILY(hcomp, AST_TT_HCOMP)
MAKE_COMP_FAMILY(fill,  AST_TT_FILL)

TT_ACCESSOR(tt_comp_type_family, AST_TT_COMP, x->data.tt_comp.type_family)
TT_ACCESSOR(tt_comp_face,        AST_TT_COMP, x->data.tt_comp.face)
TT_ACCESSOR(tt_comp_partial,     AST_TT_COMP, x->data.tt_comp.partial)
TT_ACCESSOR(tt_comp_base,        AST_TT_COMP, x->data.tt_comp.base)

lizard_ast_node_t *lizard_primitive_tt_equiv_type(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_EQUIV_TYPE;
  n->data.tt_equiv_type.domain   = nth_arg(args, 0);
  n->data.tt_equiv_type.codomain = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_equiv_typep, AST_TT_EQUIV_TYPE)
TT_ACCESSOR(tt_equiv_domain,   AST_TT_EQUIV_TYPE, x->data.tt_equiv_type.domain)
TT_ACCESSOR(tt_equiv_codomain, AST_TT_EQUIV_TYPE, x->data.tt_equiv_type.codomain)

lizard_ast_node_t *lizard_primitive_tt_id_equiv(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_ID_EQUIV;
  n->data.tt_equiv_op.operand = nth_arg(args, 0);
  return n;
}
TT_PREDICATE(tt_id_equivp, AST_TT_ID_EQUIV)

lizard_ast_node_t *lizard_primitive_tt_equiv_fun(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_EQUIV_FUN;
  n->data.tt_equiv_op.operand = nth_arg(args, 0);
  return n;
}
TT_PREDICATE(tt_equiv_funp, AST_TT_EQUIV_FUN)

lizard_ast_node_t *lizard_primitive_tt_equiv_inv(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_EQUIV_INV;
  n->data.tt_equiv_op.operand = nth_arg(args, 0);
  return n;
}
TT_PREDICATE(tt_equiv_invp, AST_TT_EQUIV_INV)

lizard_ast_node_t *lizard_primitive_tt_glue(lz_list_t *args,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!four_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_GLUE;
  n->data.tt_glue.base  = nth_arg(args, 0);
  n->data.tt_glue.face  = nth_arg(args, 1);
  n->data.tt_glue.t     = nth_arg(args, 2);
  n->data.tt_glue.equiv = nth_arg(args, 3);
  return n;
}
TT_PREDICATE(tt_gluep, AST_TT_GLUE)
TT_ACCESSOR(tt_glue_base,  AST_TT_GLUE, x->data.tt_glue.base)
TT_ACCESSOR(tt_glue_face,  AST_TT_GLUE, x->data.tt_glue.face)
TT_ACCESSOR(tt_glue_t,     AST_TT_GLUE, x->data.tt_glue.t)
TT_ACCESSOR(tt_glue_equiv, AST_TT_GLUE, x->data.tt_glue.equiv)

lizard_ast_node_t *lizard_primitive_tt_glue_intro(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_GLUE_INTRO;
  n->data.tt_glue_intro.face = nth_arg(args, 0);
  n->data.tt_glue_intro.t    = nth_arg(args, 1);
  n->data.tt_glue_intro.a    = nth_arg(args, 2);
  return n;
}
TT_PREDICATE(tt_glue_introp, AST_TT_GLUE_INTRO)

lizard_ast_node_t *lizard_primitive_tt_unglue(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_UNGLUE;
  n->data.tt_unglue.equiv  = nth_arg(args, 0);
  n->data.tt_unglue.target = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_unglue_p, AST_TT_UNGLUE)

lizard_ast_node_t *lizard_primitive_tt_ua(lz_list_t *args,
                                          lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_UA;
  n->data.tt_ua.equiv = nth_arg(args, 0);
  return n;
}
TT_PREDICATE(tt_uap, AST_TT_UA)

lizard_ast_node_t *lizard_primitive_tt_system_nil(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env; (void)args;
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_SYSTEM_NIL;
  return n;
}
TT_PREDICATE(tt_system_nilp, AST_TT_SYSTEM_NIL)

lizard_ast_node_t *lizard_primitive_tt_system_cons(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_SYSTEM_CONS;
  n->data.tt_system_cons.face  = nth_arg(args, 0);
  n->data.tt_system_cons.value = nth_arg(args, 1);
  n->data.tt_system_cons.next  = nth_arg(args, 2);
  return n;
}
TT_PREDICATE(tt_system_consp, AST_TT_SYSTEM_CONS)
TT_ACCESSOR(tt_system_face,  AST_TT_SYSTEM_CONS, x->data.tt_system_cons.face)
TT_ACCESSOR(tt_system_value, AST_TT_SYSTEM_CONS, x->data.tt_system_cons.value)
TT_ACCESSOR(tt_system_next,  AST_TT_SYSTEM_CONS, x->data.tt_system_cons.next)

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
