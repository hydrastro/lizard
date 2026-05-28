/* src/prims_tt.c -- type-theory notation and logic primitives.
 *
 * Split out from primitives.c as part of Recoverable Core Phase 1E.
 */
#include "errors.h"
#include "lizard_internal.h"
#include "mem.h"
#include "primitives.h"

#include <string.h>

/* ---------------------------------------------------------------------
 * Type-theory notation primitives.
 *
 * These primitives let users build, recognize, and inspect type-theory
 * forms (Pi, Sigma, @, Sum, U, Uco, Id, refl, Inductive, Coinductive)
 * as first-class values. NOTHING IS CHECKED — these are opaque
 * carriers for designing the surface of a proposed type theory, not
 * an implementation of one. A `(Pi (x Nat) Bool)` and a
 * `(Pi (x Nat) garbage)` both build and print fine.
 *
 * The naming convention: type-theoretic constructors are capitalized
 * (Pi, Sigma, U, Uco, Id, Sum, Inductive, Coinductive) to mirror the
 * mathematical notation; predicates/accessors use lowercase.
 * ------------------------------------------------------------------- */

static int no_args(lz_list_t *args) {
  return args->head == args->nil;
}
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
  lz_list_node_t *it = args->head;
  while (n-- > 0 && it != args->nil) it = it->next;
  if (it == args->nil) return NULL;
  return ((lizard_ast_list_node_t *)it)->ast;
}

/* (Pi binder domain codomain) -- if binder is nil, this is non-dep -> */
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

/* (pi-fresh 'x A B) — Phase L.3 dimension-creating Pi.
 * Same term shape as Pi; differs only in typing. */
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

/* (sigma-fresh 'x A B) — Phase L.3 dimension-creating Sigma. */
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

/* (co-pi-fresh 'x A B) — Phase L.5 dual dimension-creating Pi.
 * Same term shape; typing rule produces a couniverse-set. */
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

/* (Box A) — Phase M.5.1 necessity modality type constructor. */
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

/* (Diamond A) — Phase M.5.1 possibility modality type constructor. */
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

/* (box e) — Phase M.5.2 Box introduction. */
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

/* (unbox x b body) — Phase M.5.2 Box elimination. */
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

/* (diamond e) — Phase M.5.5 Diamond introduction. */
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

/* (let-diamond x b body) — Phase M.5.5 Diamond elimination. */
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

/* (box-app f a) — Phase M.5.6 K-axiom application. */
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

/* (diamond-bind f d) — Phase M.5.8 Diamond Kleisli composition. */
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

/* (dia e) — Phase M.5.9 symmetric Diamond introduction. */
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

/* (poss-coerce e) — Phase M.5.9 shift from true to poss. */
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

/* Helper for the *-set variadic constructors: parse the args as a
 * list of naturals, sort, dedup. Returns a pair (raw, count) by
 * out-parameter. Returns NULL on type error (negative or non-numeric). */
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

/* (U-set d1 d2 ...) — multi-dimensional universe. Phase L.2. */
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

/* (Co-set d1 d2 ...) — multi-dimensional COUNIVERSE. Phase L.4.
 *
 * Same shape as U-set but a separate AST type. Lives in its own
 * lattice — no auto-conversion to/from U-set. The dim space is
 * shared with U-set (both index by natural numbers) but the *kind*
 * is distinguished by the AST tag.
 */
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

/* (Co-max c1 c2) — join in the couniverse lattice. Set union, dual
 * to U-max. Phase L.4. */
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

/* (Co-min c1 c2) — meet in the couniverse lattice. Set intersection. */
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
/* (Inductive 'Name 'ctor1 'ctor2 ...) — first arg is the type name,
 * the rest are constructor specs (left opaque; user can put whatever
 * they want — symbols, lists describing fields, whatever). */
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

/* Predicates. */
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

/* Accessors — pull fields out of TT nodes. Return error on mismatch. */
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

/* Universe level — returns the integer. */
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

/* Constructors / destructors for inductive/coinductive — return a list */
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

/* ---------------------------------------------------------------------
 * Context layer — couniverse-stratified.
 *
 * Per the thesis proposal:
 *   Uco -2 : variables / binding sites
 *   Uco -1 : contexts (lists of variables)
 *   Uco  0 : substitutions / context morphisms
 *
 * The `uco-level` primitive returns the couniverse level for any of
 * these. NONE of this checks well-formedness. A context can contain
 * "bindings" that aren't really variables, a substitution can list
 * mappings that don't match its source's domain, a judgment is just a
 * three-tuple that prints like an inference-rule conclusion. The
 * forms are here so you can sketch the surface and pattern-match on
 * the structure, not because anything is verified.
 * ------------------------------------------------------------------- */

/* (variable 'name type) — at Uco -2 */
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

/* (context binding1 binding2 ...) — at Uco -1.
 * Each binding is whatever you want; the convention is that they're
 * variable values, but we don't enforce that. */
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

/* (context-extend ctx variable) — return a new context with the
 * variable appended at the end. The original is not mutated. */
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

/* (context-lookup ctx 'name) — return the matching variable, or #f. */
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

/* (context-bindings ctx) — return the list of variables in the
 * context, in declaration order. */
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

/* (context-length ctx) */
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

/* (substitution from-ctx to-ctx mappings-list) — at Uco 0.
 * mappings-list is a list of (name . term) pairs (or anything else;
 * we don't enforce). */
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

/* (judgment ctx term type) — opaque triple. */
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

/* (empty-context) — convenient nullary constructor. */
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

/* uco-level — the central operation. Returns the couniverse level
 * for any of the binding/context/substitution forms, or #f for other
 * values. This is the *only* place lizard hard-codes the
 * stratification: -2 for variables, -1 for contexts, 0 for
 * substitutions / judgments. */
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

/* Predicates and accessors. */
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

/* ---------------------------------------------------------------------
 * Identity manipulation + equivalence (NOTATION ONLY).
 *
 * These primitives let you sketch the basic HOTT-style identity
 * lemmas — symmetry, transitivity, transport, equivalence — as
 * structured values. Nothing about the validity of the constructed
 * forms is checked. (equivalence A B fwd bwd) does not verify that
 * fwd and bwd are actually inverse; (transport p x) does not verify
 * that p is an Id-proof relating two types; (Id-trans p q) does not
 * verify that p's endpoint matches q's startpoint.
 *
 * Why bother having these? They give a vocabulary for writing the
 * lemmas one would prove in a real proof assistant. With them you
 * can pattern-match on identity manipulations, define functions that
 * walk an identity term and dispatch on its head, and sketch the
 * normalisation rules a future checker might use.
 * ------------------------------------------------------------------- */

/* (equivalence A B fwd bwd) — claims A ≃ B via fwd : A -> B and
 * bwd : B -> A. */
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

/* (transport path value) — claims to transport `value` along `path`. */
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

/* (Id-sym p) — claimed symmetry of an Id-proof. */
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

/* (Id-trans p q) — claimed transitivity / composition. */
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

/* TT-level Lambda. */
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

/* ap — congruence of identity along a function. */
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

/* pair / fst / snd */
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

/* inl / inr / case */
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

/* Unit type and its inhabitant; Bot. These are nullary — no args. */
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

/* J — path induction. (J motive refl-case path). */
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

/* xport — transport with explicit motive. The motive is a Lambda
 * (Lambda 'x T) whose body T tells the engine which per-type-former
 * rule to apply. (xport (Lambda 'x T) path value). */
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

/* Universe-expression constructors. (U-var 'i), (U-suc u), (U-max u v). */
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

/* (U-min u v) — meet (greatest lower bound). Dual of U-max.
 * Phase L.1: with both join and meet, universes form a lattice. */
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

/* ===== Cubical layer ===== */

/* The interval pre-type itself and its endpoints. Nullary. */
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

/* (I-var 'i) — an interval variable */
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

/* (I-and i j) */
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

/* (I-or i j) */
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

/* (I-neg i) */
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

/* (Path A a b) — path type */
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

/* (path-abs 'i body) — path abstraction, like Lambda but for intervals */
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

/* (path-app p i) — path application, written conceptually as (p @ i) */
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

/* ===== Faces and partial elements (Turn 7) ===== */

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

/* (F-eq i j) — face: interval term i equals interval term j. */
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

/* (Partial φ A) */
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

/* (Sub A φ u) */
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

/* ===== Kan composition (Turn 8) =====
 *
 * comp, hcomp, fill all take the same 4 arguments:
 *   (op type_family face partial base)
 * and differ only in semantics. */

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

/* ===== Equivalences, Glue, and ua (Turns 9 & 10) ===== */

/* (Equiv A B) — type former. Note: distinct from `equivalence` which
 * is a notation-level 4-arg packaging. */
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

/* (id-equiv A) — the identity equivalence on A */
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

/* (equiv-fun e), (equiv-inv e) */
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

/* (Glue A φ T e) */
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

/* (glue-intro φ t a) — written conceptually `glue φ t a` */
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

/* (unglue e g) */
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

/* (ua e) */
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

/* ===== System: multi-clause partial element (Turn 11) ===== */

/* (system-nil) — the empty system, defined nowhere. */
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

/* (system-cons face value next-system) */
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

/* ===== Phase M.1 — Lisp primitives for logic-rule configuration ===== */

/* (logic-rule-register 'name)
 * Register a new rule (default disabled) if not present. Returns #t. */
lizard_ast_node_t *lizard_primitive_logic_rule_register(lz_list_t *args,
                                                        lizard_env_t *env,
                                                        lizard_heap_t *heap) {
  lizard_ast_node_t *name_arg;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (name_arg == NULL || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  lizard_logic_rule_register(name_arg->data.variable, 0);
  return lizard_make_bool(heap, 1);
}

/* (logic-rule-enable 'name)
 * Enable a rule, auto-registering if needed. */
lizard_ast_node_t *lizard_primitive_logic_rule_enable(lz_list_t *args,
                                                      lizard_env_t *env,
                                                      lizard_heap_t *heap) {
  lizard_ast_node_t *name_arg;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (name_arg == NULL || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  lizard_logic_rule_enable(name_arg->data.variable);
  return lizard_make_bool(heap, 1);
}

/* (logic-rule-disable 'name) */
lizard_ast_node_t *lizard_primitive_logic_rule_disable(lz_list_t *args,
                                                       lizard_env_t *env,
                                                       lizard_heap_t *heap) {
  lizard_ast_node_t *name_arg;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (name_arg == NULL || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  lizard_logic_rule_disable(name_arg->data.variable);
  return lizard_make_bool(heap, 1);
}

/* (logic-rule-enabled? 'name)
 * Returns #t / #f / 'unknown (a symbol). */
lizard_ast_node_t *lizard_primitive_logic_rule_enabledp(lz_list_t *args,
                                                        lizard_env_t *env,
                                                        lizard_heap_t *heap) {
  lizard_ast_node_t *name_arg;
  int r;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (name_arg == NULL || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  r = lizard_logic_rule_enabled(name_arg->data.variable);
  if (r == 1) return lizard_make_bool(heap, 1);
  if (r == 0) return lizard_make_bool(heap, 0);
  /* Unknown — return the symbol 'unknown. */
  {
    char *buf = lizard_heap_alloc(8);
    lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    strcpy(buf, "unknown");
    n->type = AST_SYMBOL;
    n->data.variable = buf;
    return n;
  }
}

/* (logic-config)
 * Returns a list of (name . enabled?) pairs — one cons per registered
 * rule. The order is registration order, reverse of internal prepend. */
typedef struct {
  lizard_heap_t *heap;
  lizard_ast_node_t *head;  /* growing list, prepended */
} logic_config_collect_t;

static int logic_config_collect_cb(const char *name, int enabled, void *ud) {
  logic_config_collect_t *c = (logic_config_collect_t *)ud;
  lizard_ast_node_t *pair, *sym, *val, *cons;
  char *namedup;
  size_t namelen;
  /* Build (name . enabled?) as a pair. */
  pair = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  pair->type = AST_PAIR;
  namelen = strlen(name) + 1;
  namedup = lizard_heap_alloc(namelen);
  memcpy(namedup, name, namelen);
  sym = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  sym->type = AST_SYMBOL;
  sym->data.variable = namedup;
  val = lizard_make_bool(c->heap, enabled);
  pair->data.pair.car = sym;
  pair->data.pair.cdr = val;
  /* Prepend to head. */
  cons = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  cons->type = AST_PAIR;
  cons->data.pair.car = pair;
  cons->data.pair.cdr = c->head;
  c->head = cons;
  return 0;
}

lizard_ast_node_t *lizard_primitive_logic_config(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  logic_config_collect_t c;
  lizard_ast_node_t *nil;
  (void)env; (void)args;
  /* Build the empty list (nil) as the initial tail. */
  nil = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  nil->type = AST_NIL;
  c.heap = heap;
  c.head = nil;
  lizard_logic_config_walk(logic_config_collect_cb, &c);
  return c.head;
}

/* (logic-config-size) — number of registered rules. */
lizard_ast_node_t *lizard_primitive_logic_config_size(lz_list_t *args,
                                                     lizard_env_t *env,
                                                     lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env; (void)args;
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_NUMBER;
  mpz_init(n->data.number);
  mpz_set_si(n->data.number, lizard_logic_config_size());
  return n;
}

/* (logic-config-reset) — clear the registry. */
lizard_ast_node_t *lizard_primitive_logic_config_reset(lz_list_t *args,
                                                      lizard_env_t *env,
                                                      lizard_heap_t *heap) {
  (void)env; (void)args;
  lizard_logic_config_reset();
  return lizard_make_bool(heap, 1);
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

/* ===== Phase M.3 — Lisp primitives for logic bundles ===== */

/* (set-logic 'NAME)
 * Apply a named logic bundle. Returns #t on success, #f if unknown.
 * Unknown name does NOT raise an error — returns #f so user code can
 * test before assuming the logic is loaded. */
lizard_ast_node_t *lizard_primitive_set_logic(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *name_arg;
  int ok;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (name_arg == NULL || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  ok = lizard_logic_set_bundle(name_arg->data.variable);
  return lizard_make_bool(heap, ok);
}

/* (current-logic)
 * Returns a symbol naming the current logic, or 'custom. */
lizard_ast_node_t *lizard_primitive_current_logic(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  const char *name;
  lizard_ast_node_t *n;
  char *buf;
  size_t len;
  (void)env; (void)args;
  name = lizard_logic_current_bundle();
  len = strlen(name) + 1;
  buf = lizard_heap_alloc(len);
  memcpy(buf, name, len);
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_SYMBOL;
  n->data.variable = buf;
  return n;
}

/* (list-logics)
 * Returns a list of symbols naming the predefined logic bundles. */
typedef struct {
  lizard_heap_t *heap;
  lizard_ast_node_t *head;
} list_logics_collect_t;

static int list_logics_collect_cb(const char *name, void *ud) {
  list_logics_collect_t *c = (list_logics_collect_t *)ud;
  lizard_ast_node_t *sym, *cons;
  char *buf;
  size_t len;
  len = strlen(name) + 1;
  buf = lizard_heap_alloc(len);
  memcpy(buf, name, len);
  sym = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  sym->type = AST_SYMBOL;
  sym->data.variable = buf;
  cons = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  cons->type = AST_PAIR;
  cons->data.pair.car = sym;
  cons->data.pair.cdr = c->head;
  c->head = cons;
  return 0;
}

lizard_ast_node_t *lizard_primitive_list_logics(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  list_logics_collect_t c;
  lizard_ast_node_t *nil;
  (void)env; (void)args;
  nil = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  nil->type = AST_NIL;
  c.heap = heap;
  c.head = nil;
  lizard_logic_bundles_walk(list_logics_collect_cb, &c);
  return c.head;
}

