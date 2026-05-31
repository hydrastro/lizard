/* pvector.h — persistent vector as a 32-way bit-partitioned trie with a tail
 * buffer (the Clojure PersistentVector design).  Gives O(1) amortized append,
 * O(log32 n) indexed lookup/update, and structural sharing: an update copies
 * only the O(log32 n) nodes on one root-to-leaf path and shares the rest.
 *
 * A vector VALUE is an AST node of type AST_PVEC whose data.pvec holds:
 *   root        the trie holding the first `tailoff(count)` elements
 *   tail        a leaf node holding the trailing 0..32 elements
 *   count       total number of elements
 *   shift       bit-shift for the root level (a multiple of 5)
 *   tail_count  number of elements currently in the tail
 * Trie nodes (lizard_pvec_node_t) are separate heap objects whose `slots` hold
 * child nodes (internal levels) or elements (leaf level).  Nodes are never
 * mutated after publication, which is what makes the sharing safe.
 */
#ifndef LIZARD_PVECTOR_H
#define LIZARD_PVECTOR_H

#include "lizard_internal.h"

#define LIZARD_PVEC_BITS 5
#define LIZARD_PVEC_WIDTH 32
#define LIZARD_PVEC_MASK 31

typedef struct lizard_pvec_node {
  void *slots[LIZARD_PVEC_WIDTH];
} lizard_pvec_node_t;

/* Initialise `v` (type set to AST_PVEC) as the empty vector. */
void lizard_pvec_init_empty(lizard_ast_node_t *v);

/* Number of elements. */
int lizard_pvec_count(const lizard_ast_node_t *v);

/* Element at index i, or NULL if out of range. */
lizard_ast_node_t *lizard_pvec_ref(const lizard_ast_node_t *v, int i);

/* Functional update: new vector equal to v but with element i replaced.
 * Returns NULL if i is out of range. O(log32 n), sharing all off-path nodes. */
lizard_ast_node_t *lizard_pvec_update(const lizard_ast_node_t *v, int i,
                                      lizard_ast_node_t *val);

/* Append val to the end. O(1) amortized. */
lizard_ast_node_t *lizard_pvec_push(const lizard_ast_node_t *v,
                                    lizard_ast_node_t *val);

/* Remove the last element. Returns NULL if v is empty. */
lizard_ast_node_t *lizard_pvec_pop(const lizard_ast_node_t *v);

/* Visit every element in index order: cb(element, index, ctx). */
void lizard_pvec_foreach(const lizard_ast_node_t *v,
                         void (*cb)(lizard_ast_node_t *, int, void *),
                         void *ctx);

/* Test/introspection hook: the address of the leaf node holding element i
 * (the tail node when i is in the tail).  Used to verify structural sharing —
 * after an update, leaves off the updated path are pointer-identical. */
const void *lizard_pvec_leaf_ptr(const lizard_ast_node_t *v, int i);

#endif /* LIZARD_PVECTOR_H */
