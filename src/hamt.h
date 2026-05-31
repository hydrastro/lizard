/* src/hamt.h — persistent Hash Array Mapped Trie over lizard values.
 *
 * 32-way branching (5 hash bits per level), bitmap-compressed nodes, and
 * path-copying for persistence: assoc/dissoc are O(log32 n) and SHARE structure
 * with the input map (only the nodes along the updated path are copied).  The
 * empty map is the NULL root.  Hash collisions (equal 32-bit hashes for
 * non-equal keys) are held in collision nodes.
 *
 * This is the structural-sharing substrate behind persistent maps and sets
 * (Phase 6).  It does not allocate via malloc; everything goes through the
 * lizard heap so existing GC/segment accounting applies.
 */
#ifndef LIZARD_HAMT_H
#define LIZARD_HAMT_H

#include "lizard_internal.h"

typedef struct lizard_hamt_node lizard_hamt_node_t;

/* Hash / equality used to key the trie.  Symbols and strings hash by contents,
 * numbers by value, booleans and nil by a fixed code, everything else by
 * identity. */
unsigned long lizard_hamt_hash(const lizard_ast_node_t *key);
int lizard_hamt_key_equal(const lizard_ast_node_t *a,
                          const lizard_ast_node_t *b);

/* Lookup.  *found is set to 1/0; returns the value (or NULL when absent). */
lizard_ast_node_t *lizard_hamt_get(lizard_hamt_node_t *root,
                                   const lizard_ast_node_t *key, int *found);

/* Functional insert/update.  Returns a new root that shares structure with the
 * old one; *added is 1 if the key was new, 0 if it replaced an existing value. */
lizard_hamt_node_t *lizard_hamt_assoc(lizard_hamt_node_t *root,
                                      lizard_ast_node_t *key,
                                      lizard_ast_node_t *val,
                                      lizard_heap_t *heap, int *added);

/* Functional delete.  Returns a new root; *removed is 1 if the key was present. */
lizard_hamt_node_t *lizard_hamt_dissoc(lizard_hamt_node_t *root,
                                       const lizard_ast_node_t *key,
                                       lizard_heap_t *heap, int *removed);

/* Iterate every (key,value) in unspecified (hash) order. */
typedef void (*lizard_hamt_visit_fn)(lizard_ast_node_t *key,
                                     lizard_ast_node_t *val, void *ctx);
void lizard_hamt_foreach(lizard_hamt_node_t *root,
                         lizard_hamt_visit_fn fn, void *ctx);

#endif /* LIZARD_HAMT_H */
