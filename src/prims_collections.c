/* src/prims_collections.c -- vector and hash primitives.
 *
 * Split out from primitives.c as part of Recoverable Core Phase 1E.
 */
#include "errors.h"
#include "lizard_internal.h"
#include "mem.h"
#include "primitives.h"

#include <stdint.h>

/* ---------------------------------------------------------------------
 * Vectors — fixed-size, mutable, O(1) indexed access.
 * ------------------------------------------------------------------- */

static lizard_ast_node_t *make_vector(lizard_heap_t *heap, size_t n,
                                      lizard_ast_node_t *fill) {
  lizard_ast_node_t *v;
  size_t i;
  v = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  v->type = AST_VECTOR;
  v->data.vector.size = n;
  v->data.vector.elements =
      lizard_heap_alloc(n * sizeof(lizard_ast_node_t *));
  for (i = 0; i < n; i++) v->data.vector.elements[i] = fill;
  return v;
}

/* (make-vector n [fill]) */
lizard_ast_node_t *lizard_primitive_make_vector(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n_ast, *fill;
  long n;
  (void)env;
  if (args->head == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n_ast = ((lizard_ast_list_node_t *)args->head)->ast;
  if (n_ast->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  n = mpz_get_si(n_ast->data.number);
  if (n < 0) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  fill = (args->head->next != args->nil)
             ? ((lizard_ast_list_node_t *)args->head->next)->ast
             : lizard_make_nil(heap);
  return make_vector(heap, (size_t)n, fill);
}

/* (vector v1 v2 v3 ...) — list-literal style. */
lizard_ast_node_t *lizard_primitive_vector(lz_list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap) {
  size_t n;
  lz_list_node_t *iter;
  lizard_ast_node_t *v;
  size_t i;
  (void)env;
  n = 0;
  for (iter = args->head; iter != args->nil; iter = iter->next) n++;
  v = make_vector(heap, n, lizard_make_nil(heap));
  i = 0;
  for (iter = args->head; iter != args->nil; iter = iter->next) {
    v->data.vector.elements[i++] = ((lizard_ast_list_node_t *)iter)->ast;
  }
  return v;
}

/* (vector? x) */
lizard_ast_node_t *lizard_primitive_vectorp(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  x = ((lizard_ast_list_node_t *)args->head)->ast;
  return lizard_make_bool(heap, x && x->type == AST_VECTOR);
}

/* (vector-length v) */
lizard_ast_node_t *lizard_primitive_vec_length(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *v, *r;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  v = ((lizard_ast_list_node_t *)args->head)->ast;
  if (v->type != AST_VECTOR) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_NUMBER;
  mpz_init_set_ui(r->data.number, (unsigned long)v->data.vector.size);
  return r;
}

/* (vector-ref v i) */
lizard_ast_node_t *lizard_primitive_vec_ref(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *v, *idx;
  long i;
  (void)env;
  if (args->head == args->nil || args->head->next == args->nil ||
      args->head->next->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  v = ((lizard_ast_list_node_t *)args->head)->ast;
  idx = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (v->type != AST_VECTOR || idx->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  i = mpz_get_si(idx->data.number);
  if (i < 0 || (size_t)i >= v->data.vector.size) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  return v->data.vector.elements[i];
}

/* (vector-set! v i x) — mutates and returns nil. */
lizard_ast_node_t *lizard_primitive_vec_set(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *v, *idx, *val;
  long i;
  (void)env;
  if (args->head == args->nil || args->head->next == args->nil ||
      args->head->next->next == args->nil ||
      args->head->next->next->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  v = ((lizard_ast_list_node_t *)args->head)->ast;
  idx = ((lizard_ast_list_node_t *)args->head->next)->ast;
  val = ((lizard_ast_list_node_t *)args->head->next->next)->ast;
  if (v->type != AST_VECTOR || idx->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  i = mpz_get_si(idx->data.number);
  if (i < 0 || (size_t)i >= v->data.vector.size) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  v->data.vector.elements[i] = val;
  return lizard_make_nil(heap);
}

/* (vector->list v) */
lizard_ast_node_t *lizard_primitive_vec_to_list(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *v, *result, *pair;
  long i;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  v = ((lizard_ast_list_node_t *)args->head)->ast;
  if (v->type != AST_VECTOR) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  result = lizard_make_nil(heap);
  for (i = (long)v->data.vector.size - 1; i >= 0; i--) {
    pair = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    pair->type = AST_PAIR;
    pair->data.pair.car = v->data.vector.elements[i];
    pair->data.pair.cdr = result;
    result = pair;
  }
  return result;
}

/* (list->vector xs) */
lizard_ast_node_t *lizard_primitive_list_to_vec(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *xs, *cur, *v;
  size_t n, i;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  xs = ((lizard_ast_list_node_t *)args->head)->ast;
  /* count length */
  n = 0;
  cur = xs;
  while (cur && cur->type == AST_PAIR) {
    n++;
    cur = cur->data.pair.cdr;
  }
  if (cur && cur->type != AST_NIL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  v = make_vector(heap, n, lizard_make_nil(heap));
  cur = xs;
  for (i = 0; i < n; i++) {
    v->data.vector.elements[i] = cur->data.pair.car;
    cur = cur->data.pair.cdr;
  }
  return v;
}

/* ---------------------------------------------------------------------
 * Hash tables — open-addressed with linear probing.
 * Equality + hashing supported for numbers, strings, symbols, booleans.
 * ------------------------------------------------------------------- */

#define LIZARD_HASH_INIT_CAP 8

static unsigned long lizard_hash_value(lizard_ast_node_t *k) {
  unsigned long h;
  const char *s;
  switch (k->type) {
  case AST_NUMBER:
    /* mpz hashed by mod-2^64 of its first limb. */
    return mpz_get_ui(k->data.number) * 2654435761UL;
  case AST_STRING:
    s = k->data.string;
    h = 2166136261UL;
    while (*s) {
      h ^= (unsigned char)*s++;
      h *= 16777619UL;
    }
    return h;
  case AST_SYMBOL:
    s = k->data.variable;
    h = 14695981039346656037UL;
    while (*s) {
      h ^= (unsigned char)*s++;
      h *= 1099511628211UL;
    }
    return h;
  case AST_BOOL:
    return k->data.boolean ? 1u : 0u;
  case AST_NIL:
    return 2u;
  default:
    return (unsigned long)(uintptr_t)k;
  }
}

static int lizard_keys_eq(lizard_ast_node_t *a, lizard_ast_node_t *b) {
  if (!a || !b || a->type != b->type) return 0;
  switch (a->type) {
  case AST_NUMBER: return mpz_cmp(a->data.number, b->data.number) == 0;
  case AST_STRING: return strcmp(a->data.string, b->data.string) == 0;
  case AST_SYMBOL: return strcmp(a->data.variable, b->data.variable) == 0;
  case AST_BOOL:   return a->data.boolean == b->data.boolean;
  case AST_NIL:    return 1;
  default:         return a == b;
  }
}

static void hash_grow(lizard_heap_t *heap, lizard_ast_node_t *h) {
  size_t old_cap = h->data.hash.cap;
  lizard_ast_node_t **old_k = h->data.hash.keys;
  lizard_ast_node_t **old_v = h->data.hash.values;
  size_t new_cap = old_cap * 2;
  size_t i, idx;
  h->data.hash.cap = new_cap;
  h->data.hash.keys = lizard_heap_alloc(new_cap * sizeof(lizard_ast_node_t *));
  h->data.hash.values = lizard_heap_alloc(new_cap * sizeof(lizard_ast_node_t *));
  for (i = 0; i < new_cap; i++) {
    h->data.hash.keys[i] = NULL;
    h->data.hash.values[i] = NULL;
  }
  for (i = 0; i < old_cap; i++) {
    if (old_k[i] != NULL) {
      idx = lizard_hash_value(old_k[i]) & (new_cap - 1);
      while (h->data.hash.keys[idx] != NULL) {
        idx = (idx + 1) & (new_cap - 1);
      }
      h->data.hash.keys[idx] = old_k[i];
      h->data.hash.values[idx] = old_v[i];
    }
  }
}

/* (make-hash-table) */
lizard_ast_node_t *lizard_primitive_make_hash(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *h;
  size_t i;
  (void)env;
  (void)args;
  h = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  h->type = AST_HASH;
  h->data.hash.size = 0;
  h->data.hash.cap = LIZARD_HASH_INIT_CAP;
  h->data.hash.keys =
      lizard_heap_alloc(LIZARD_HASH_INIT_CAP * sizeof(lizard_ast_node_t *));
  h->data.hash.values =
      lizard_heap_alloc(LIZARD_HASH_INIT_CAP * sizeof(lizard_ast_node_t *));
  for (i = 0; i < LIZARD_HASH_INIT_CAP; i++) {
    h->data.hash.keys[i] = NULL;
    h->data.hash.values[i] = NULL;
  }
  return h;
}

/* (hash? x) */
lizard_ast_node_t *lizard_primitive_hashp(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  x = ((lizard_ast_list_node_t *)args->head)->ast;
  return lizard_make_bool(heap, x && x->type == AST_HASH);
}

static size_t hash_find_slot(lizard_ast_node_t *h, lizard_ast_node_t *k,
                             int *found) {
  size_t idx = lizard_hash_value(k) & (h->data.hash.cap - 1);
  while (h->data.hash.keys[idx] != NULL) {
    if (lizard_keys_eq(h->data.hash.keys[idx], k)) {
      *found = 1;
      return idx;
    }
    idx = (idx + 1) & (h->data.hash.cap - 1);
  }
  *found = 0;
  return idx;
}

/* (hash-set! h k v) */
lizard_ast_node_t *lizard_primitive_hash_set(lz_list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *h, *k, *v;
  size_t idx;
  int found;
  (void)env;
  if (args->head == args->nil || args->head->next == args->nil ||
      args->head->next->next == args->nil ||
      args->head->next->next->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  h = ((lizard_ast_list_node_t *)args->head)->ast;
  k = ((lizard_ast_list_node_t *)args->head->next)->ast;
  v = ((lizard_ast_list_node_t *)args->head->next->next)->ast;
  if (h->type != AST_HASH) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  /* Grow if load factor would exceed 0.75. */
  if (h->data.hash.size * 4 >= h->data.hash.cap * 3) {
    hash_grow(heap, h);
  }
  idx = hash_find_slot(h, k, &found);
  if (!found) {
    h->data.hash.size++;
    h->data.hash.keys[idx] = k;
  }
  h->data.hash.values[idx] = v;
  return lizard_make_nil(heap);
}

/* (hash-ref h k [default]) */
lizard_ast_node_t *lizard_primitive_hash_ref(lz_list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *h, *k, *deflt;
  size_t idx;
  int found;
  (void)env;
  if (args->head == args->nil || args->head->next == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  h = ((lizard_ast_list_node_t *)args->head)->ast;
  k = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (h->type != AST_HASH) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  deflt = (args->head->next->next != args->nil)
              ? ((lizard_ast_list_node_t *)args->head->next->next)->ast
              : lizard_make_bool(heap, false);
  idx = hash_find_slot(h, k, &found);
  if (found) return h->data.hash.values[idx];
  return deflt;
}

/* (hash-has-key? h k) */
lizard_ast_node_t *lizard_primitive_hash_has(lz_list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *h, *k;
  int found;
  (void)env;
  if (args->head == args->nil || args->head->next == args->nil ||
      args->head->next->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  h = ((lizard_ast_list_node_t *)args->head)->ast;
  k = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (h->type != AST_HASH) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  hash_find_slot(h, k, &found);
  return lizard_make_bool(heap, found ? true : false);
}

/* (hash-size h) */
lizard_ast_node_t *lizard_primitive_hash_size(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *h, *r;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  h = ((lizard_ast_list_node_t *)args->head)->ast;
  if (h->type != AST_HASH) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_NUMBER;
  mpz_init_set_ui(r->data.number, (unsigned long)h->data.hash.size);
  return r;
}

/* (hash-keys h) -> list */
lizard_ast_node_t *lizard_primitive_hash_keys(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *h, *result, *pair;
  size_t i;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  h = ((lizard_ast_list_node_t *)args->head)->ast;
  if (h->type != AST_HASH) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  result = lizard_make_nil(heap);
  for (i = 0; i < h->data.hash.cap; i++) {
    if (h->data.hash.keys[i] != NULL) {
      pair = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      pair->type = AST_PAIR;
      pair->data.pair.car = h->data.hash.keys[i];
      pair->data.pair.cdr = result;
      result = pair;
    }
  }
  return result;
}

/* (hash-remove! h k) */
lizard_ast_node_t *lizard_primitive_hash_remove(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *h, *k;
  size_t idx, next;
  int found;
  (void)env;
  if (args->head == args->nil || args->head->next == args->nil ||
      args->head->next->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  h = ((lizard_ast_list_node_t *)args->head)->ast;
  k = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (h->type != AST_HASH) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  idx = hash_find_slot(h, k, &found);
  if (!found) return lizard_make_bool(heap, false);
  /* Linear-probing deletion: clear slot then re-insert any cluster
   * elements that may now have been displaced. */
  h->data.hash.keys[idx] = NULL;
  h->data.hash.values[idx] = NULL;
  h->data.hash.size--;
  next = (idx + 1) & (h->data.hash.cap - 1);
  while (h->data.hash.keys[next] != NULL) {
    lizard_ast_node_t *rk = h->data.hash.keys[next];
    lizard_ast_node_t *rv = h->data.hash.values[next];
    h->data.hash.keys[next] = NULL;
    h->data.hash.values[next] = NULL;
    h->data.hash.size--;
    /* re-insert */
    {
      lz_list_t *fake_args =
          list_create_alloc(lizard_heap_alloc, lizard_heap_free);
      lizard_ast_list_node_t *a, *b, *c;
      a = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
      b = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
      c = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
      a->ast = h; b->ast = rk; c->ast = rv;
      list_append(fake_args, &a->node);
      list_append(fake_args, &b->node);
      list_append(fake_args, &c->node);
      lizard_primitive_hash_set(fake_args, env, heap);
    }
    next = (next + 1) & (h->data.hash.cap - 1);
  }
  return lizard_make_bool(heap, true);
}

