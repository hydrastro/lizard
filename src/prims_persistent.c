/* prims_persistent.c — extracted from primitives.c (#7 monolith split).
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

#include "hamt.h"
#include "pvector.h"

lizard_ast_node_t *lizard_primitive_pvec(lz_list_t *args,
                                          lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *v;
  lz_list_node_t *iter;
  (void)env;
  (void)heap;
  v = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  v->type = AST_PVEC;
  lizard_pvec_init_empty(v);
  for (iter = args->head; iter != args->nil; iter = iter->next) {
    v = lizard_pvec_push(v, ((lizard_ast_list_node_t *)iter)->ast);
  }
  return v;
}
lizard_ast_node_t *lizard_primitive_pvec_ref(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *v, *idx, *r;
  long i;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  v = ((lizard_ast_list_node_t *)args->head)->ast;
  idx = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (v->type != AST_PVEC || idx->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  i = mpz_get_si(idx->data.number);
  r = lizard_pvec_ref(v, (int)i);
  if (r == NULL) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  return r;
}
lizard_ast_node_t *lizard_primitive_pvec_set(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *v, *idx, *val, *nv;
  long i;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  v   = ((lizard_ast_list_node_t *)args->head)->ast;
  idx = ((lizard_ast_list_node_t *)args->head->next)->ast;
  val = ((lizard_ast_list_node_t *)args->head->next->next)->ast;
  if (v->type != AST_PVEC || idx->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  i = mpz_get_si(idx->data.number);
  /* O(log32 n) functional update, sharing all nodes off the updated path. */
  nv = lizard_pvec_update(v, (int)i, val);
  if (nv == NULL) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  return nv;
}
lizard_ast_node_t *lizard_primitive_pvec_push(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *v, *val;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  v   = ((lizard_ast_list_node_t *)args->head)->ast;
  val = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (v->type != AST_PVEC) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  return lizard_pvec_push(v, val); /* O(1) amortized */
}
lizard_ast_node_t *lizard_primitive_pvec_pop(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *v, *nv;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  v = ((lizard_ast_list_node_t *)args->head)->ast;
  if (v->type != AST_PVEC) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  nv = lizard_pvec_pop(v);
  if (nv == NULL) {
    return lizard_make_error(heap, LIZARD_ERROR_USER); /* pop of empty vector */
  }
  return nv;
}
lizard_ast_node_t *lizard_primitive_pvec_count(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *v, *r;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  v = ((lizard_ast_list_node_t *)args->head)->ast;
  if (v->type != AST_PVEC) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_NUMBER;
  mpz_init_set_si(r->data.number, lizard_pvec_count(v));
  return r;
}
lizard_ast_node_t *lizard_primitive_pvec_to_list(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *v, *result, *node;
  int i;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  v = ((lizard_ast_list_node_t *)args->head)->ast;
  if (v->type != AST_PVEC) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  result = lizard_make_nil(heap);
  for (i = lizard_pvec_count(v) - 1; i >= 0; i--) {
    node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    node->type = AST_PAIR;
    node->data.pair.car = lizard_pvec_ref(v, i);
    node->data.pair.cdr = result;
    result = node;
  }
  return result;
}
lizard_ast_node_t *lizard_primitive_pvecp(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  return lizard_make_bool(heap,
      ((lizard_ast_list_node_t *)args->head)->ast->type == AST_PVEC);
}
lizard_ast_node_t *lizard_primitive_list_to_pvec(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *lst, *v;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  lst = ((lizard_ast_list_node_t *)args->head)->ast;
  v = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  v->type = AST_PVEC;
  lizard_pvec_init_empty(v);
  while (lst != NULL && lst->type == AST_PAIR) {
    v = lizard_pvec_push(v, lst->data.pair.car);
    lst = lst->data.pair.cdr;
  }
  return v;
}
/* ===================================================================
 * Persistent maps + sets, backed by the real HAMT (src/hamt.c).
 * The AST node's data.hamt.root points at a lizard_hamt_node_t (NULL = empty);
 * is_set distinguishes a persistent set from a persistent map.
 * ================================================================== */

static lizard_ast_node_t *hamt_make_node(lizard_hamt_node_t *root, int count,
                                         int is_set, lizard_heap_t *heap) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_HAMT;
  n->data.hamt.root = root;
  n->data.hamt.count = count;
  n->data.hamt.is_set = is_set;
  return n;
}

typedef struct { lizard_ast_node_t *list; lizard_heap_t *heap; } hamt_acc_t;
static void hamt_collect_keys(lizard_ast_node_t *k, lizard_ast_node_t *v,
                              void *ctx) {
  hamt_acc_t *a = (hamt_acc_t *)ctx;
  lizard_ast_node_t *cell = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  (void)v;
  cell->type = AST_PAIR; cell->data.pair.car = k; cell->data.pair.cdr = a->list;
  a->list = cell;
}
static void hamt_collect_vals(lizard_ast_node_t *k, lizard_ast_node_t *v,
                              void *ctx) {
  hamt_acc_t *a = (hamt_acc_t *)ctx;
  lizard_ast_node_t *cell = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  (void)k;
  cell->type = AST_PAIR; cell->data.pair.car = v; cell->data.pair.cdr = a->list;
  a->list = cell;
}
static void hamt_collect_entries(lizard_ast_node_t *k, lizard_ast_node_t *v,
                                 void *ctx) {
  hamt_acc_t *a = (hamt_acc_t *)ctx;
  lizard_ast_node_t *pair = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  lizard_ast_node_t *cell = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  pair->type = AST_PAIR; pair->data.pair.car = k; pair->data.pair.cdr = v;
  cell->type = AST_PAIR; cell->data.pair.car = pair; cell->data.pair.cdr = a->list;
  a->list = cell;
}

lizard_ast_node_t *lizard_primitive_phash_map(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_hamt_node_t *root = NULL;
  lz_list_node_t *iter;
  int count = 0, added;
  (void)env;
  iter = args->head;
  while (iter != args->nil && iter->next != args->nil) {
    lizard_ast_node_t *k = ((lizard_ast_list_node_t *)iter)->ast;
    lizard_ast_node_t *v = ((lizard_ast_list_node_t *)iter->next)->ast;
    root = lizard_hamt_assoc(root, k, v, heap, &added);
    if (added) count++;
    iter = iter->next->next;
  }
  return hamt_make_node(root, count, 0, heap);
}

lizard_ast_node_t *lizard_primitive_phash_get(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *map_node, *key, *dflt, *val;
  int found;
  (void)env;
  map_node = ((lizard_ast_list_node_t *)args->head)->ast;
  key = ((lizard_ast_list_node_t *)args->head->next)->ast;
  dflt = (args->head->next->next != args->nil)
           ? ((lizard_ast_list_node_t *)args->head->next->next)->ast
           : lizard_make_bool(heap, 0);
  if (map_node->type != AST_HAMT) return dflt;
  val = lizard_hamt_get((lizard_hamt_node_t *)map_node->data.hamt.root, key,
                        &found);
  return found ? val : dflt;
}

lizard_ast_node_t *lizard_primitive_phash_set(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *map_node, *key, *val;
  lizard_hamt_node_t *root;
  int added;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  map_node = ((lizard_ast_list_node_t *)args->head)->ast;
  key = ((lizard_ast_list_node_t *)args->head->next)->ast;
  val = ((lizard_ast_list_node_t *)args->head->next->next)->ast;
  if (map_node->type != AST_HAMT) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  root = lizard_hamt_assoc((lizard_hamt_node_t *)map_node->data.hamt.root, key,
                           val, heap, &added);
  return hamt_make_node(root, map_node->data.hamt.count + (added ? 1 : 0),
                        map_node->data.hamt.is_set, heap);
}

/* (phash-remove map key) — a new map without `key` (no-op if absent). */
lizard_ast_node_t *lizard_primitive_phash_remove(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *map_node, *key;
  lizard_hamt_node_t *root;
  int removed;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  map_node = ((lizard_ast_list_node_t *)args->head)->ast;
  key = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (map_node->type != AST_HAMT) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  root = lizard_hamt_dissoc((lizard_hamt_node_t *)map_node->data.hamt.root, key,
                            heap, &removed);
  return hamt_make_node(root, map_node->data.hamt.count - (removed ? 1 : 0),
                        map_node->data.hamt.is_set, heap);
}

lizard_ast_node_t *lizard_primitive_phash_keys(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *map_node;
  hamt_acc_t acc;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  map_node = ((lizard_ast_list_node_t *)args->head)->ast;
  if (map_node->type != AST_HAMT) return lizard_make_nil(heap);
  acc.list = lizard_make_nil(heap); acc.heap = heap;
  lizard_hamt_foreach((lizard_hamt_node_t *)map_node->data.hamt.root,
                      hamt_collect_keys, &acc);
  return acc.list;
}

lizard_ast_node_t *lizard_primitive_phash_count(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *map_node, *r;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  map_node = ((lizard_ast_list_node_t *)args->head)->ast;
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_NUMBER;
  mpz_init_set_si(r->data.number,
      (map_node->type == AST_HAMT) ? map_node->data.hamt.count : 0);
  return r;
}

lizard_ast_node_t *lizard_primitive_phash_mapp(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  x = ((lizard_ast_list_node_t *)args->head)->ast;
  return lizard_make_bool(heap, x->type == AST_HAMT && !x->data.hamt.is_set);
}

lizard_ast_node_t *lizard_primitive_phash_values(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *map_node;
  hamt_acc_t acc;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  map_node = ((lizard_ast_list_node_t *)args->head)->ast;
  if (map_node->type != AST_HAMT) return lizard_make_nil(heap);
  acc.list = lizard_make_nil(heap); acc.heap = heap;
  lizard_hamt_foreach((lizard_hamt_node_t *)map_node->data.hamt.root,
                      hamt_collect_vals, &acc);
  return acc.list;
}

lizard_ast_node_t *lizard_primitive_phash_entries(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *map_node;
  hamt_acc_t acc;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  map_node = ((lizard_ast_list_node_t *)args->head)->ast;
  if (map_node->type != AST_HAMT) return lizard_make_nil(heap);
  acc.list = lizard_make_nil(heap); acc.heap = heap;
  lizard_hamt_foreach((lizard_hamt_node_t *)map_node->data.hamt.root,
                      hamt_collect_entries, &acc);
  return acc.list;
}

lizard_ast_node_t *lizard_primitive_transient(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *coll, *result;
  (void)env;
  if (!single_arg(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  coll = ((lizard_ast_list_node_t *)args->head)->ast;
  if (coll->type == AST_PVEC) {
    /* Convert persistent vector to mutable vector. */
    int count = lizard_pvec_count(coll);
    int i;
    result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    result->type = AST_VECTOR;
    result->data.vector.elements = (lizard_ast_node_t **)
        lizard_heap_alloc((size_t)(count + 16) * sizeof(lizard_ast_node_t *));
    result->data.vector.size = (size_t)count;
    for (i = 0; i < count; i++) {
      result->data.vector.elements[i] = lizard_pvec_ref(coll, i);
    }
    return result;
  }
  if (coll->type == AST_HAMT) {
    /* Convert persistent hash map to a mutable list of (key . val) pairs. */
    hamt_acc_t acc;
    acc.list = lizard_make_nil(heap); acc.heap = heap;
    lizard_hamt_foreach((lizard_hamt_node_t *)coll->data.hamt.root,
                        hamt_collect_entries, &acc);
    return acc.list;
  }
  return coll;
}

/* ===================================================================
 * Persistent sets — the same HAMT with is_set=1 and value == key.
 * ================================================================== */

lizard_ast_node_t *lizard_primitive_pset(lz_list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap) {
  lizard_hamt_node_t *root = NULL;
  lz_list_node_t *iter;
  int count = 0, added;
  (void)env;
  for (iter = args->head; iter != args->nil; iter = iter->next) {
    lizard_ast_node_t *k = ((lizard_ast_list_node_t *)iter)->ast;
    root = lizard_hamt_assoc(root, k, k, heap, &added);
    if (added) count++;
  }
  return hamt_make_node(root, count, 1, heap);
}

lizard_ast_node_t *lizard_primitive_pset_add(lz_list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *set_node, *key;
  lizard_hamt_node_t *root;
  int added;
  (void)env;
  if (!two_args(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  set_node = ((lizard_ast_list_node_t *)args->head)->ast;
  key = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (set_node->type != AST_HAMT) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  root = lizard_hamt_assoc((lizard_hamt_node_t *)set_node->data.hamt.root, key,
                           key, heap, &added);
  return hamt_make_node(root, set_node->data.hamt.count + (added ? 1 : 0), 1, heap);
}

lizard_ast_node_t *lizard_primitive_pset_contains(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *set_node, *key;
  int found;
  (void)env;
  if (!two_args(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  set_node = ((lizard_ast_list_node_t *)args->head)->ast;
  key = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (set_node->type != AST_HAMT) return lizard_make_bool(heap, 0);
  lizard_hamt_get((lizard_hamt_node_t *)set_node->data.hamt.root, key, &found);
  return lizard_make_bool(heap, found);
}

lizard_ast_node_t *lizard_primitive_pset_remove(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *set_node, *key;
  lizard_hamt_node_t *root;
  int removed;
  (void)env;
  if (!two_args(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  set_node = ((lizard_ast_list_node_t *)args->head)->ast;
  key = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (set_node->type != AST_HAMT) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  root = lizard_hamt_dissoc((lizard_hamt_node_t *)set_node->data.hamt.root, key,
                            heap, &removed);
  return hamt_make_node(root, set_node->data.hamt.count - (removed ? 1 : 0), 1, heap);
}

lizard_ast_node_t *lizard_primitive_pset_count(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *set_node, *r;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  set_node = ((lizard_ast_list_node_t *)args->head)->ast;
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_NUMBER;
  mpz_init_set_si(r->data.number,
      (set_node->type == AST_HAMT) ? set_node->data.hamt.count : 0);
  return r;
}

lizard_ast_node_t *lizard_primitive_pset_to_list(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *set_node;
  hamt_acc_t acc;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  set_node = ((lizard_ast_list_node_t *)args->head)->ast;
  if (set_node->type != AST_HAMT) return lizard_make_nil(heap);
  acc.list = lizard_make_nil(heap); acc.heap = heap;
  lizard_hamt_foreach((lizard_hamt_node_t *)set_node->data.hamt.root,
                      hamt_collect_keys, &acc);
  return acc.list;
}

lizard_ast_node_t *lizard_primitive_psetp(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = ((lizard_ast_list_node_t *)args->head)->ast;
  return lizard_make_bool(heap, x->type == AST_HAMT && x->data.hamt.is_set);
}
