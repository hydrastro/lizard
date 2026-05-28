#include "primitives.h"
#include "prims_common.h"
#include "mem.h"
#include "errors.h"

/* Split from primitives.c as part of Recoverable Core file hygiene. */

typedef struct {
  lizard_ast_node_t *key;
  lizard_ast_node_t *val;
} hamt_entry_t;

typedef struct {
  hamt_entry_t *entries;
  int count;
  int capacity;
} hamt_flat_t;


static int hamt_key_equal(lizard_ast_node_t *a, lizard_ast_node_t *b) {
  if (a->type != b->type) return 0;
  switch (a->type) {
  case AST_NUMBER:  return mpz_cmp(a->data.number, b->data.number) == 0;
  case AST_SYMBOL:  return strcmp(a->data.variable, b->data.variable) == 0;
  case AST_STRING:  return strcmp(a->data.string, b->data.string) == 0;
  case AST_BOOL:    return a->data.boolean == b->data.boolean;
  case AST_NIL:     return 1;
  default:          return a == b;
  }
}

lizard_ast_node_t *lizard_primitive_pvec(lz_list_t *args,
                                          lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *v;
  lz_list_node_t *iter;
  int count, i;
  (void)env;
  count = 0;
  for (iter = args->head; iter != args->nil; iter = iter->next) count++;
  v = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  v->type = AST_PVEC;
  v->data.pvec.count = count;
  v->data.pvec.capacity = count > 0 ? count : 4;
  v->data.pvec.entries = (lizard_ast_node_t **)lizard_heap_alloc(
      (size_t)v->data.pvec.capacity * sizeof(lizard_ast_node_t *));
  i = 0;
  for (iter = args->head; iter != args->nil; iter = iter->next) {
    v->data.pvec.entries[i++] = ((lizard_ast_list_node_t *)iter)->ast;
  }
  return v;
}

lizard_ast_node_t *lizard_primitive_pvec_ref(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *v, *idx;
  long i;
  (void)env;
  if (!lizard_prim_two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  v = ((lizard_ast_list_node_t *)args->head)->ast;
  idx = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (v->type != AST_PVEC || idx->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  i = mpz_get_si(idx->data.number);
  if (i < 0 || i >= v->data.pvec.count) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  return v->data.pvec.entries[i];
}

lizard_ast_node_t *lizard_primitive_pvec_set(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *v, *idx, *val, *nv;
  long i;
  (void)env;
  if (!lizard_prim_three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  v   = ((lizard_ast_list_node_t *)args->head)->ast;
  idx = ((lizard_ast_list_node_t *)args->head->next)->ast;
  val = ((lizard_ast_list_node_t *)args->head->next->next)->ast;
  if (v->type != AST_PVEC || idx->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  i = mpz_get_si(idx->data.number);
  if (i < 0 || i >= v->data.pvec.count) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  /* Copy-on-write: allocate a new vector with the updated element. */
  nv = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  nv->type = AST_PVEC;
  nv->data.pvec.count = v->data.pvec.count;
  nv->data.pvec.capacity = v->data.pvec.count;
  nv->data.pvec.entries = (lizard_ast_node_t **)lizard_heap_alloc(
      (size_t)nv->data.pvec.count * sizeof(lizard_ast_node_t *));
  memcpy(nv->data.pvec.entries, v->data.pvec.entries,
         (size_t)nv->data.pvec.count * sizeof(lizard_ast_node_t *));
  nv->data.pvec.entries[i] = val;
  return nv;
}

lizard_ast_node_t *lizard_primitive_pvec_push(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *v, *val, *nv;
  (void)env;
  if (!lizard_prim_two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  v   = ((lizard_ast_list_node_t *)args->head)->ast;
  val = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (v->type != AST_PVEC) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  nv = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  nv->type = AST_PVEC;
  nv->data.pvec.count = v->data.pvec.count + 1;
  nv->data.pvec.capacity = nv->data.pvec.count;
  nv->data.pvec.entries = (lizard_ast_node_t **)lizard_heap_alloc(
      (size_t)nv->data.pvec.count * sizeof(lizard_ast_node_t *));
  if (v->data.pvec.count > 0) {
    memcpy(nv->data.pvec.entries, v->data.pvec.entries,
           (size_t)v->data.pvec.count * sizeof(lizard_ast_node_t *));
  }
  nv->data.pvec.entries[v->data.pvec.count] = val;
  return nv;
}

lizard_ast_node_t *lizard_primitive_pvec_count(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *v, *r;
  (void)env;
  if (!lizard_prim_single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  v = ((lizard_ast_list_node_t *)args->head)->ast;
  if (v->type != AST_PVEC) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_NUMBER;
  mpz_init_set_si(r->data.number, v->data.pvec.count);
  return r;
}

lizard_ast_node_t *lizard_primitive_pvec_to_list(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *v, *result, *node;
  int i;
  (void)env;
  if (!lizard_prim_single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  v = ((lizard_ast_list_node_t *)args->head)->ast;
  if (v->type != AST_PVEC) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  result = lizard_make_nil(heap);
  for (i = v->data.pvec.count - 1; i >= 0; i--) {
    node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    node->type = AST_PAIR;
    node->data.pair.car = v->data.pvec.entries[i];
    node->data.pair.cdr = result;
    result = node;
  }
  return result;
}

lizard_ast_node_t *lizard_primitive_pvecp(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  (void)env;
  if (!lizard_prim_single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  return lizard_make_bool(heap,
      ((lizard_ast_list_node_t *)args->head)->ast->type == AST_PVEC);
}

lizard_ast_node_t *lizard_primitive_phash_map(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *node;
  hamt_flat_t *h;
  lz_list_node_t *iter;
  int count;
  (void)env;
  count = 0;
  for (iter = args->head; iter != args->nil; iter = iter->next) count++;
  h = (hamt_flat_t *)lizard_heap_alloc(sizeof(hamt_flat_t));
  h->count = 0;
  h->capacity = (count / 2) + 4;
  h->entries = (hamt_entry_t *)lizard_heap_alloc(
      (size_t)h->capacity * sizeof(hamt_entry_t));
  /* Parse alternating key-value pairs. */
  iter = args->head;
  while (iter != args->nil && iter->next != args->nil) {
    h->entries[h->count].key = ((lizard_ast_list_node_t *)iter)->ast;
    iter = iter->next;
    h->entries[h->count].val = ((lizard_ast_list_node_t *)iter)->ast;
    iter = iter->next;
    h->count++;
  }
  node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  node->type = AST_HAMT;
  node->data.hamt.root = h;
  node->data.hamt.count = h->count;
  return node;
}

lizard_ast_node_t *lizard_primitive_phash_get(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *map_node, *key, *dflt;
  hamt_flat_t *h;
  int i;
  (void)env;
  map_node = ((lizard_ast_list_node_t *)args->head)->ast;
  key = ((lizard_ast_list_node_t *)args->head->next)->ast;
  dflt = (args->head->next->next != args->nil)
           ? ((lizard_ast_list_node_t *)args->head->next->next)->ast
           : lizard_make_bool(heap, 0);
  if (map_node->type != AST_HAMT) return dflt;
  h = (hamt_flat_t *)map_node->data.hamt.root;
  for (i = 0; i < h->count; i++) {
    if (hamt_key_equal(h->entries[i].key, key)) {
      return h->entries[i].val;
    }
  }
  return dflt;
}

lizard_ast_node_t *lizard_primitive_phash_set(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *map_node, *key, *val, *result;
  hamt_flat_t *h, *nh;
  int i, found;
  (void)env;
  if (!lizard_prim_three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  map_node = ((lizard_ast_list_node_t *)args->head)->ast;
  key = ((lizard_ast_list_node_t *)args->head->next)->ast;
  val = ((lizard_ast_list_node_t *)args->head->next->next)->ast;
  if (map_node->type != AST_HAMT) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  h = (hamt_flat_t *)map_node->data.hamt.root;
  /* Copy-on-write. */
  nh = (hamt_flat_t *)lizard_heap_alloc(sizeof(hamt_flat_t));
  nh->capacity = h->count + 2;
  nh->entries = (hamt_entry_t *)lizard_heap_alloc(
      (size_t)nh->capacity * sizeof(hamt_entry_t));
  found = 0;
  for (i = 0; i < h->count; i++) {
    if (hamt_key_equal(h->entries[i].key, key)) {
      nh->entries[i].key = key;
      nh->entries[i].val = val;
      found = 1;
    } else {
      nh->entries[i] = h->entries[i];
    }
  }
  if (!found) {
    nh->entries[h->count].key = key;
    nh->entries[h->count].val = val;
    nh->count = h->count + 1;
  } else {
    nh->count = h->count;
  }
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_HAMT;
  result->data.hamt.root = nh;
  result->data.hamt.count = nh->count;
  return result;
}

lizard_ast_node_t *lizard_primitive_phash_keys(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *map_node, *result, *node;
  hamt_flat_t *h;
  int i;
  (void)env;
  if (!lizard_prim_single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  map_node = ((lizard_ast_list_node_t *)args->head)->ast;
  if (map_node->type != AST_HAMT) return lizard_make_nil(heap);
  h = (hamt_flat_t *)map_node->data.hamt.root;
  result = lizard_make_nil(heap);
  for (i = h->count - 1; i >= 0; i--) {
    node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    node->type = AST_PAIR;
    node->data.pair.car = h->entries[i].key;
    node->data.pair.cdr = result;
    result = node;
  }
  return result;
}

lizard_ast_node_t *lizard_primitive_phash_count(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *map_node, *r;
  (void)env;
  if (!lizard_prim_single_arg(args)) {
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
  (void)env;
  if (!lizard_prim_single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  return lizard_make_bool(heap,
      ((lizard_ast_list_node_t *)args->head)->ast->type == AST_HAMT);
}

lizard_ast_node_t *lizard_primitive_phash_values(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *map_node, *result, *node;
  hamt_flat_t *h;
  int i;
  (void)env;
  if (!lizard_prim_single_arg(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  map_node = ((lizard_ast_list_node_t *)args->head)->ast;
  if (map_node->type != AST_HAMT) return lizard_make_nil(heap);
  h = (hamt_flat_t *)map_node->data.hamt.root;
  result = lizard_make_nil(heap);
  for (i = h->count - 1; i >= 0; i--) {
    node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    node->type = AST_PAIR;
    node->data.pair.car = h->entries[i].val;
    node->data.pair.cdr = result;
    result = node;
  }
  return result;
}

lizard_ast_node_t *lizard_primitive_phash_entries(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *map_node, *result, *node, *pair;
  hamt_flat_t *h;
  int i;
  (void)env;
  if (!lizard_prim_single_arg(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  map_node = ((lizard_ast_list_node_t *)args->head)->ast;
  if (map_node->type != AST_HAMT) return lizard_make_nil(heap);
  h = (hamt_flat_t *)map_node->data.hamt.root;
  result = lizard_make_nil(heap);
  for (i = h->count - 1; i >= 0; i--) {
    pair = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    pair->type = AST_PAIR;
    pair->data.pair.car = h->entries[i].key;
    pair->data.pair.cdr = h->entries[i].val;
    node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    node->type = AST_PAIR;
    node->data.pair.car = pair;
    node->data.pair.cdr = result;
    result = node;
  }
  return result;
}
