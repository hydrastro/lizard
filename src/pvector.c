/* pvector.c — see pvector.h.  A faithful port of Clojure's PersistentVector:
 * a 32-way trie with a tail buffer, path-copying for O(log32 n) updates, and
 * O(1) amortized append.  All nodes are immutable once published; updates copy
 * the O(log32 n) nodes along a single path and share everything else. */
#include "pvector.h"
#include "mem.h"
#include <string.h>

/* ---- node helpers ---- */

static lizard_pvec_node_t *pvec_node_new(void) {
  /* lizard_heap_alloc zero-fills, so all slots start NULL. */
  return (lizard_pvec_node_t *)lizard_heap_alloc(sizeof(lizard_pvec_node_t));
}

static lizard_pvec_node_t *pvec_node_copy(const lizard_pvec_node_t *n) {
  lizard_pvec_node_t *c = pvec_node_new();
  memcpy(c, n, sizeof(lizard_pvec_node_t));
  return c;
}

/* Index of the first element held in the tail (always a multiple of 32). */
static int pvec_tailoff(int count) {
  if (count < LIZARD_PVEC_WIDTH) {
    return 0;
  }
  return ((count - 1) >> LIZARD_PVEC_BITS) << LIZARD_PVEC_BITS;
}

static lizard_ast_node_t *pvec_header(void *root, void *tail, int count,
                                      int shift, int tail_count) {
  lizard_ast_node_t *v =
      (lizard_ast_node_t *)lizard_heap_alloc(sizeof(lizard_ast_node_t));
  v->type = AST_PVEC;
  v->data.pvec.root = root;
  v->data.pvec.tail = tail;
  v->data.pvec.count = count;
  v->data.pvec.shift = shift;
  v->data.pvec.tail_count = tail_count;
  return v;
}

void lizard_pvec_init_empty(lizard_ast_node_t *v) {
  v->type = AST_PVEC;
  v->data.pvec.root = pvec_node_new();
  v->data.pvec.tail = pvec_node_new();
  v->data.pvec.count = 0;
  v->data.pvec.shift = LIZARD_PVEC_BITS;
  v->data.pvec.tail_count = 0;
}

int lizard_pvec_count(const lizard_ast_node_t *v) {
  return v->data.pvec.count;
}

/* The leaf node holding element i (requires i < tailoff(count)). */
static lizard_pvec_node_t *pvec_leaf_for(const lizard_ast_node_t *v, int i) {
  lizard_pvec_node_t *node = (lizard_pvec_node_t *)v->data.pvec.root;
  int level;
  for (level = v->data.pvec.shift; level > 0; level -= LIZARD_PVEC_BITS) {
    node = (lizard_pvec_node_t *)node->slots[(i >> level) & LIZARD_PVEC_MASK];
  }
  return node;
}

lizard_ast_node_t *lizard_pvec_ref(const lizard_ast_node_t *v, int i) {
  if (i < 0 || i >= v->data.pvec.count) {
    return NULL;
  }
  if (i >= pvec_tailoff(v->data.pvec.count)) {
    return (lizard_ast_node_t *)
        ((lizard_pvec_node_t *)v->data.pvec.tail)->slots[i & LIZARD_PVEC_MASK];
  }
  return (lizard_ast_node_t *)pvec_leaf_for(v, i)->slots[i & LIZARD_PVEC_MASK];
}

/* ---- update (assoc) ---- */

static lizard_pvec_node_t *pvec_do_assoc(int level,
                                         const lizard_pvec_node_t *node, int i,
                                         lizard_ast_node_t *val) {
  lizard_pvec_node_t *ret = pvec_node_copy(node);
  if (level == 0) {
    ret->slots[i & LIZARD_PVEC_MASK] = val;
  } else {
    int subidx = (i >> level) & LIZARD_PVEC_MASK;
    ret->slots[subidx] = pvec_do_assoc(level - LIZARD_PVEC_BITS,
                                       (lizard_pvec_node_t *)node->slots[subidx],
                                       i, val);
  }
  return ret;
}

lizard_ast_node_t *lizard_pvec_update(const lizard_ast_node_t *v, int i,
                                      lizard_ast_node_t *val) {
  int count = v->data.pvec.count;
  if (i < 0 || i >= count) {
    return NULL;
  }
  if (i >= pvec_tailoff(count)) {
    lizard_pvec_node_t *nt =
        pvec_node_copy((lizard_pvec_node_t *)v->data.pvec.tail);
    nt->slots[i & LIZARD_PVEC_MASK] = val;
    return pvec_header(v->data.pvec.root, nt, count, v->data.pvec.shift,
                       v->data.pvec.tail_count);
  } else {
    lizard_pvec_node_t *nr =
        pvec_do_assoc(v->data.pvec.shift,
                      (lizard_pvec_node_t *)v->data.pvec.root, i, val);
    return pvec_header(nr, v->data.pvec.tail, count, v->data.pvec.shift,
                       v->data.pvec.tail_count);
  }
}

/* ---- push (cons) ---- */

static lizard_pvec_node_t *pvec_new_path(int level, lizard_pvec_node_t *node) {
  lizard_pvec_node_t *ret;
  if (level == 0) {
    return node;
  }
  ret = pvec_node_new();
  ret->slots[0] = pvec_new_path(level - LIZARD_PVEC_BITS, node);
  return ret;
}

static lizard_pvec_node_t *pvec_push_tail(int count, int level,
                                          const lizard_pvec_node_t *parent,
                                          lizard_pvec_node_t *tailnode) {
  int subidx = ((count - 1) >> level) & LIZARD_PVEC_MASK;
  lizard_pvec_node_t *ret = pvec_node_copy(parent);
  lizard_pvec_node_t *to_insert;
  if (level == LIZARD_PVEC_BITS) {
    to_insert = tailnode;
  } else {
    lizard_pvec_node_t *child = (lizard_pvec_node_t *)parent->slots[subidx];
    to_insert = (child != NULL)
                    ? pvec_push_tail(count, level - LIZARD_PVEC_BITS, child,
                                     tailnode)
                    : pvec_new_path(level - LIZARD_PVEC_BITS, tailnode);
  }
  ret->slots[subidx] = to_insert;
  return ret;
}

lizard_ast_node_t *lizard_pvec_push(const lizard_ast_node_t *v,
                                    lizard_ast_node_t *val) {
  int count = v->data.pvec.count;
  int tc = v->data.pvec.tail_count;

  if (count - pvec_tailoff(count) < LIZARD_PVEC_WIDTH) {
    /* Room in the tail: copy it and append. */
    lizard_pvec_node_t *nt =
        pvec_node_copy((lizard_pvec_node_t *)v->data.pvec.tail);
    nt->slots[tc] = val;
    return pvec_header(v->data.pvec.root, nt, count + 1, v->data.pvec.shift,
                       tc + 1);
  } else {
    /* Tail is full: it becomes a leaf pushed into the trie (shared as-is). */
    lizard_pvec_node_t *tailnode = (lizard_pvec_node_t *)v->data.pvec.tail;
    lizard_pvec_node_t *newroot;
    lizard_pvec_node_t *newtail;
    int newshift = v->data.pvec.shift;

    if ((count >> LIZARD_PVEC_BITS) > (1 << v->data.pvec.shift)) {
      /* Root is full — add a level. */
      newroot = pvec_node_new();
      newroot->slots[0] = v->data.pvec.root;
      newroot->slots[1] = pvec_new_path(v->data.pvec.shift, tailnode);
      newshift += LIZARD_PVEC_BITS;
    } else {
      newroot = pvec_push_tail(count, v->data.pvec.shift,
                               (lizard_pvec_node_t *)v->data.pvec.root,
                               tailnode);
    }
    newtail = pvec_node_new();
    newtail->slots[0] = val;
    return pvec_header(newroot, newtail, count + 1, newshift, 1);
  }
}

/* ---- pop ---- */

static lizard_pvec_node_t *pvec_pop_tail(int count, int level,
                                         const lizard_pvec_node_t *node) {
  int subidx = ((count - 2) >> level) & LIZARD_PVEC_MASK;
  if (level > LIZARD_PVEC_BITS) {
    lizard_pvec_node_t *newchild = pvec_pop_tail(
        count, level - LIZARD_PVEC_BITS, (lizard_pvec_node_t *)node->slots[subidx]);
    if (newchild == NULL && subidx == 0) {
      return NULL;
    }
    {
      lizard_pvec_node_t *ret = pvec_node_copy(node);
      ret->slots[subidx] = newchild;
      return ret;
    }
  } else if (subidx == 0) {
    return NULL;
  } else {
    lizard_pvec_node_t *ret = pvec_node_copy(node);
    ret->slots[subidx] = NULL;
    return ret;
  }
}

lizard_ast_node_t *lizard_pvec_pop(const lizard_ast_node_t *v) {
  int count = v->data.pvec.count;
  if (count == 0) {
    return NULL;
  }
  if (count == 1) {
    return pvec_header(pvec_node_new(), pvec_node_new(), 0, LIZARD_PVEC_BITS, 0);
  }
  if (count - pvec_tailoff(count) > 1) {
    /* Tail has more than one element: just shrink it. */
    lizard_pvec_node_t *nt =
        pvec_node_copy((lizard_pvec_node_t *)v->data.pvec.tail);
    nt->slots[v->data.pvec.tail_count - 1] = NULL;
    return pvec_header(v->data.pvec.root, nt, count - 1, v->data.pvec.shift,
                       v->data.pvec.tail_count - 1);
  } else {
    /* Tail has one element: pull the last trie leaf up to be the new tail. */
    lizard_pvec_node_t *newtail = pvec_leaf_for(v, count - 2);
    lizard_pvec_node_t *newroot =
        pvec_pop_tail(count, v->data.pvec.shift,
                      (lizard_pvec_node_t *)v->data.pvec.root);
    int newshift = v->data.pvec.shift;
    if (newroot == NULL) {
      newroot = pvec_node_new();
    }
    if (newshift > LIZARD_PVEC_BITS && newroot->slots[1] == NULL) {
      newroot = (lizard_pvec_node_t *)newroot->slots[0];
      newshift -= LIZARD_PVEC_BITS;
    }
    return pvec_header(newroot, newtail, count - 1, newshift,
                       LIZARD_PVEC_WIDTH);
  }
}

/* ---- iteration ---- */

void lizard_pvec_foreach(const lizard_ast_node_t *v,
                         void (*cb)(lizard_ast_node_t *, int, void *),
                         void *ctx) {
  int i, count = v->data.pvec.count;
  for (i = 0; i < count; i++) {
    cb(lizard_pvec_ref(v, i), i, ctx);
  }
}

const void *lizard_pvec_leaf_ptr(const lizard_ast_node_t *v, int i) {
  if (i < 0 || i >= v->data.pvec.count) {
    return NULL;
  }
  if (i >= pvec_tailoff(v->data.pvec.count)) {
    return v->data.pvec.tail;
  }
  return (const void *)pvec_leaf_for(v, i);
}
