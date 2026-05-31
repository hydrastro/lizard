/* src/hamt.c — persistent Hash Array Mapped Trie (see hamt.h). */
#include "hamt.h"
#include "mem.h"
#include <gmp.h>
#include <stdint.h>
#include <string.h>

#define HAMT_BITS 5
#define HAMT_MASK 31u

/* A node holds a compact array of `nslots` slots, occupancy given by `bitmap`.
 * A slot with key != NULL is a leaf (key -> ptr-as-value); a slot with
 * key == NULL is a sub-node (ptr-as-node).  A node with bitmap == 0 and
 * nslots > 0 is a COLLISION node: every slot is a leaf and they all share the
 * full hash `chash`. */
typedef struct {
  lizard_ast_node_t *key;
  void *ptr;
} hamt_slot_t;

struct lizard_hamt_node {
  unsigned int bitmap;
  int nslots;
  unsigned long chash;
  hamt_slot_t *slots;
};

static int popcount32(unsigned int x) {
  int c = 0;
  while (x) { x &= x - 1; c++; }
  return c;
}

static unsigned long djb2(const char *s) {
  unsigned long h = 5381UL;
  int c;
  while ((c = (unsigned char)*s++) != 0) h = ((h << 5) + h) + (unsigned long)c;
  return h;
}

unsigned long lizard_hamt_hash(const lizard_ast_node_t *key) {
  if (!key) return 0UL;
  switch (key->type) {
    case AST_SYMBOL: return djb2(key->data.variable);
    case AST_STRING: return djb2(key->data.string);
    case AST_NUMBER: {
      unsigned long h = (unsigned long)mpz_get_ui(key->data.number);
      if (mpz_sgn(key->data.number) < 0) h = ~h;
      h ^= (h >> 16);
      h *= 2654435761UL;
      return h;
    }
    case AST_RATIONAL: {
      unsigned long hn = (unsigned long)mpz_get_ui(mpq_numref(key->data.rational));
      unsigned long hd = (unsigned long)mpz_get_ui(mpq_denref(key->data.rational));
      if (mpq_sgn(key->data.rational) < 0) hn = ~hn;
      return (hn * 2654435761UL) ^ (hd * 40503UL);
    }
    case AST_BOOL: return key->data.boolean ? 1231UL : 1237UL;
    case AST_NIL: return 2166136261UL;
    default: return (unsigned long)(uintptr_t)key;
  }
}

int lizard_hamt_key_equal(const lizard_ast_node_t *a,
                          const lizard_ast_node_t *b) {
  if (a == b) return 1;
  if (!a || !b) return 0;
  if (a->type != b->type) return 0;
  switch (a->type) {
    case AST_SYMBOL: return strcmp(a->data.variable, b->data.variable) == 0;
    case AST_STRING: return strcmp(a->data.string, b->data.string) == 0;
    case AST_NUMBER: return mpz_cmp(a->data.number, b->data.number) == 0;
    case AST_RATIONAL: return mpq_cmp(a->data.rational, b->data.rational) == 0;
    case AST_BOOL:   return a->data.boolean == b->data.boolean;
    case AST_NIL:    return 1;
    default:         return 0;  /* identity already handled */
  }
}

/* ---- node construction ------------------------------------------------- */
static lizard_hamt_node_t *node_alloc(int nslots, lizard_heap_t *heap) {
  lizard_hamt_node_t *n =
      (lizard_hamt_node_t *)lizard_heap_alloc(sizeof(lizard_hamt_node_t));
  n->bitmap = 0;
  n->nslots = nslots;
  n->chash = 0;
  n->slots = (nslots > 0)
                 ? (hamt_slot_t *)lizard_heap_alloc((size_t)nslots *
                                                    sizeof(hamt_slot_t))
                 : NULL;
  return n;
}

static lizard_hamt_node_t *leaf_node(lizard_ast_node_t *k, lizard_ast_node_t *v,
                                     unsigned int bit, lizard_heap_t *heap) {
  lizard_hamt_node_t *n = node_alloc(1, heap);
  n->bitmap = bit;
  n->slots[0].key = k;
  n->slots[0].ptr = v;
  return n;
}

static lizard_hamt_node_t *make_collision(lizard_ast_node_t *k1,
                                          lizard_ast_node_t *v1,
                                          lizard_ast_node_t *k2,
                                          lizard_ast_node_t *v2,
                                          unsigned long hash,
                                          lizard_heap_t *heap) {
  lizard_hamt_node_t *n = node_alloc(2, heap);
  n->bitmap = 0;
  n->chash = hash;
  n->slots[0].key = k1; n->slots[0].ptr = v1;
  n->slots[1].key = k2; n->slots[1].ptr = v2;
  return n;
}

static lizard_hamt_node_t *node_copy_replace(lizard_hamt_node_t *n, int idx,
                                             lizard_ast_node_t *key, void *ptr,
                                             lizard_heap_t *heap) {
  lizard_hamt_node_t *m = node_alloc(n->nslots, heap);
  m->bitmap = n->bitmap;
  m->chash = n->chash;
  memcpy(m->slots, n->slots, (size_t)n->nslots * sizeof(hamt_slot_t));
  m->slots[idx].key = key;
  m->slots[idx].ptr = ptr;
  return m;
}

static lizard_hamt_node_t *node_copy_insert(lizard_hamt_node_t *n, int idx,
                                            unsigned int bit,
                                            lizard_ast_node_t *key, void *ptr,
                                            lizard_heap_t *heap) {
  lizard_hamt_node_t *m = node_alloc(n->nslots + 1, heap);
  m->bitmap = n->bitmap | bit;
  m->chash = n->chash;
  if (idx > 0) memcpy(m->slots, n->slots, (size_t)idx * sizeof(hamt_slot_t));
  m->slots[idx].key = key;
  m->slots[idx].ptr = ptr;
  if (n->nslots - idx > 0)
    memcpy(m->slots + idx + 1, n->slots + idx,
           (size_t)(n->nslots - idx) * sizeof(hamt_slot_t));
  return m;
}

static lizard_hamt_node_t *node_copy_remove(lizard_hamt_node_t *n, int idx,
                                            unsigned int bit,
                                            lizard_heap_t *heap) {
  lizard_hamt_node_t *m;
  if (n->nslots - 1 <= 0) return NULL;
  m = node_alloc(n->nslots - 1, heap);
  m->bitmap = n->bitmap & ~bit;
  m->chash = n->chash;
  if (idx > 0) memcpy(m->slots, n->slots, (size_t)idx * sizeof(hamt_slot_t));
  if (n->nslots - idx - 1 > 0)
    memcpy(m->slots + idx, n->slots + idx + 1,
           (size_t)(n->nslots - idx - 1) * sizeof(hamt_slot_t));
  return m;
}

/* ---- lookup ------------------------------------------------------------ */
static lizard_ast_node_t *get_rec(lizard_hamt_node_t *n,
                                  const lizard_ast_node_t *key,
                                  unsigned long hash, int shift, int *found) {
  unsigned int bit;
  int idx;
  if (!n) { *found = 0; return NULL; }
  if (n->bitmap == 0) { /* collision node */
    int i;
    for (i = 0; i < n->nslots; i++)
      if (lizard_hamt_key_equal(n->slots[i].key, key)) {
        *found = 1;
        return (lizard_ast_node_t *)n->slots[i].ptr;
      }
    *found = 0;
    return NULL;
  }
  bit = 1u << ((unsigned int)(hash >> shift) & HAMT_MASK);
  if (!(n->bitmap & bit)) { *found = 0; return NULL; }
  idx = popcount32(n->bitmap & (bit - 1));
  if (n->slots[idx].key != NULL) { /* leaf */
    if (lizard_hamt_key_equal(n->slots[idx].key, key)) {
      *found = 1;
      return (lizard_ast_node_t *)n->slots[idx].ptr;
    }
    *found = 0;
    return NULL;
  }
  return get_rec((lizard_hamt_node_t *)n->slots[idx].ptr, key, hash,
                 shift + HAMT_BITS, found);
}

lizard_ast_node_t *lizard_hamt_get(lizard_hamt_node_t *root,
                                   const lizard_ast_node_t *key, int *found) {
  return get_rec(root, key, lizard_hamt_hash(key), 0, found);
}

/* ---- insert / update --------------------------------------------------- */
static lizard_hamt_node_t *assoc_rec(lizard_hamt_node_t *n,
                                     lizard_ast_node_t *key,
                                     lizard_ast_node_t *val, unsigned long hash,
                                     int shift, lizard_heap_t *heap,
                                     int *added) {
  unsigned int bit;
  int idx;
  if (!n) {
    *added = 1;
    bit = 1u << ((unsigned int)(hash >> shift) & HAMT_MASK);
    return leaf_node(key, val, bit, heap);
  }
  if (n->bitmap == 0) { /* collision node */
    int i;
    for (i = 0; i < n->nslots; i++)
      if (lizard_hamt_key_equal(n->slots[i].key, key)) {
        *added = 0;
        return node_copy_replace(n, i, key, val, heap);
      }
    *added = 1;
    { lizard_hamt_node_t *m = node_alloc(n->nslots + 1, heap);
      m->bitmap = 0; m->chash = n->chash;
      memcpy(m->slots, n->slots, (size_t)n->nslots * sizeof(hamt_slot_t));
      m->slots[n->nslots].key = key;
      m->slots[n->nslots].ptr = val;
      return m; }
  }
  bit = 1u << ((unsigned int)(hash >> shift) & HAMT_MASK);
  idx = popcount32(n->bitmap & (bit - 1));
  if (!(n->bitmap & bit)) {
    *added = 1;
    return node_copy_insert(n, idx, bit, key, val, heap);
  }
  if (n->slots[idx].key != NULL) { /* existing leaf */
    lizard_ast_node_t *ek = n->slots[idx].key;
    lizard_ast_node_t *ev = (lizard_ast_node_t *)n->slots[idx].ptr;
    if (lizard_hamt_key_equal(ek, key)) {
      *added = 0;
      return node_copy_replace(n, idx, key, val, heap);
    }
    { /* two distinct keys collide at this chunk — push both one level down */
      unsigned long ehash = lizard_hamt_hash(ek);
      lizard_hamt_node_t *child;
      int a2;
      if (ehash == hash) {
        child = make_collision(ek, ev, key, val, hash, heap);
      } else {
        child = leaf_node(
            ek, ev,
            1u << ((unsigned int)(ehash >> (shift + HAMT_BITS)) & HAMT_MASK),
            heap);
        child = assoc_rec(child, key, val, hash, shift + HAMT_BITS, heap, &a2);
      }
      *added = 1;
      return node_copy_replace(n, idx, NULL, child, heap);
    }
  } else { /* existing sub-node */
    int a2;
    lizard_hamt_node_t *child = (lizard_hamt_node_t *)n->slots[idx].ptr;
    lizard_hamt_node_t *nc =
        assoc_rec(child, key, val, hash, shift + HAMT_BITS, heap, &a2);
    *added = a2;
    return node_copy_replace(n, idx, NULL, nc, heap);
  }
}

lizard_hamt_node_t *lizard_hamt_assoc(lizard_hamt_node_t *root,
                                      lizard_ast_node_t *key,
                                      lizard_ast_node_t *val,
                                      lizard_heap_t *heap, int *added) {
  return assoc_rec(root, key, val, lizard_hamt_hash(key), 0, heap, added);
}

/* ---- delete ------------------------------------------------------------ */
static lizard_hamt_node_t *dissoc_rec(lizard_hamt_node_t *n,
                                      const lizard_ast_node_t *key,
                                      unsigned long hash, int shift,
                                      lizard_heap_t *heap, int *removed) {
  unsigned int bit;
  int idx;
  if (!n) { *removed = 0; return NULL; }
  if (n->bitmap == 0) { /* collision node */
    int i;
    for (i = 0; i < n->nslots; i++)
      if (lizard_hamt_key_equal(n->slots[i].key, key)) {
        *removed = 1;
        if (n->nslots - 1 <= 0) return NULL;
        { lizard_hamt_node_t *m = node_alloc(n->nslots - 1, heap);
          int j, t = 0;
          m->bitmap = 0; m->chash = n->chash;
          for (j = 0; j < n->nslots; j++)
            if (j != i) m->slots[t++] = n->slots[j];
          return m; }
      }
    *removed = 0;
    return n;
  }
  bit = 1u << ((unsigned int)(hash >> shift) & HAMT_MASK);
  if (!(n->bitmap & bit)) { *removed = 0; return n; }
  idx = popcount32(n->bitmap & (bit - 1));
  if (n->slots[idx].key != NULL) { /* leaf */
    if (lizard_hamt_key_equal(n->slots[idx].key, key)) {
      *removed = 1;
      return node_copy_remove(n, idx, bit, heap);
    }
    *removed = 0;
    return n;
  } else { /* sub-node */
    lizard_hamt_node_t *child = (lizard_hamt_node_t *)n->slots[idx].ptr;
    lizard_hamt_node_t *nc =
        dissoc_rec(child, key, hash, shift + HAMT_BITS, heap, removed);
    if (!*removed) return n;
    if (nc == NULL) return node_copy_remove(n, idx, bit, heap);
    return node_copy_replace(n, idx, NULL, nc, heap);
  }
}

lizard_hamt_node_t *lizard_hamt_dissoc(lizard_hamt_node_t *root,
                                       const lizard_ast_node_t *key,
                                       lizard_heap_t *heap, int *removed) {
  return dissoc_rec(root, key, lizard_hamt_hash(key), 0, heap, removed);
}

/* ---- iteration --------------------------------------------------------- */
void lizard_hamt_foreach(lizard_hamt_node_t *n, lizard_hamt_visit_fn fn,
                         void *ctx) {
  int i;
  if (!n) return;
  for (i = 0; i < n->nslots; i++) {
    if (n->slots[i].key != NULL)
      fn(n->slots[i].key, (lizard_ast_node_t *)n->slots[i].ptr, ctx);
    else
      lizard_hamt_foreach((lizard_hamt_node_t *)n->slots[i].ptr, fn, ctx);
  }
}
