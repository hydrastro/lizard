/* tests/hamt_test.c — direct unit tests for the persistent HAMT (src/hamt.c).
 * Exercises insert/lookup/update, deep sub-node splitting (keys that share
 * low-order hash chunks), structural-sharing persistence (an old root keeps its
 * values after updates to a derived map), and deletion down to empty. */
#include "hamt.h"
#include "mem.h"
#include "test_harness.h"

#include <gmp.h>

static lizard_ast_node_t *num(long v) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_NUMBER;
  mpz_init_set_si(n->data.number, v);
  return n;
}

static long getnum(lizard_hamt_node_t *root, long key) {
  int found;
  lizard_ast_node_t *k = num(key);
  lizard_ast_node_t *v = lizard_hamt_get(root, k, &found);
  if (!found) return -987654321L; /* sentinel "absent" */
  return mpz_get_si(v->data.number);
}

int main(void) {
  lizard_hamt_node_t *root, *r2, *snapshot;
  int added, removed, found, i;

  heap = lizard_heap_create(1u << 16, 1u << 16);
  TEST_ASSERT(heap != NULL);

  /* empty map: nothing is found */
  lizard_hamt_get(NULL, num(1), &found);
  TEST_ASSERT_EQ(found, 0);

  /* single insert + lookup */
  root = lizard_hamt_assoc(NULL, num(42), num(100), heap, &added);
  TEST_ASSERT_EQ(added, 1);
  TEST_ASSERT_EQ(getnum(root, 42), 100L);
  TEST_ASSERT_EQ(getnum(root, 43), -987654321L);

  /* update: same key, added flag is 0, value changes */
  root = lizard_hamt_assoc(root, num(42), num(200), heap, &added);
  TEST_ASSERT_EQ(added, 0);
  TEST_ASSERT_EQ(getnum(root, 42), 200L);

  /* bulk insert 0..499 */
  for (i = 0; i < 500; i++) {
    root = lizard_hamt_assoc(root, num(i), num(i * 3), heap, &added);
  }
  for (i = 0; i < 500; i++) {
    TEST_ASSERT_EQ(getnum(root, i), (long)i * 3);
  }

  /* deep sub-node splitting: multiples of 32 share the bottom 5 hash bits, so
   * they must be pushed into sub-nodes rather than colliding in one node. */
  for (i = 0; i < 64; i++) {
    root = lizard_hamt_assoc(root, num(1000 + i * 32), num(i), heap, &added);
    TEST_ASSERT_EQ(added, 1);
  }
  for (i = 0; i < 64; i++) {
    TEST_ASSERT_EQ(getnum(root, 1000 + i * 32), (long)i);
  }

  /* structural-sharing persistence: snapshot the map, then derive a new map
   * with overwritten values; the snapshot must keep its original values. */
  snapshot = root;
  r2 = root;
  for (i = 0; i < 500; i++) {
    r2 = lizard_hamt_assoc(r2, num(i), num(-1), heap, &added);
    TEST_ASSERT_EQ(added, 0);
  }
  for (i = 0; i < 500; i++) {
    TEST_ASSERT_EQ(getnum(r2, i), -1L);            /* new map updated */
    TEST_ASSERT_EQ(getnum(snapshot, i), (long)i * 3); /* old map intact */
  }

  /* deletion: remove the 500 base keys from r2; the multiples-of-32 remain */
  for (i = 0; i < 500; i++) {
    r2 = lizard_hamt_dissoc(r2, num(i), heap, &removed);
    TEST_ASSERT_EQ(removed, 1);
  }
  for (i = 0; i < 500; i++) {
    TEST_ASSERT_EQ(getnum(r2, i), -987654321L);    /* gone */
  }
  for (i = 0; i < 64; i++) {
    TEST_ASSERT_EQ(getnum(r2, 1000 + i * 32), (long)i); /* survivors */
  }
  /* deleting an absent key reports removed=0 */
  lizard_hamt_dissoc(r2, num(7777), heap, &removed);
  TEST_ASSERT_EQ(removed, 0);

  /* the snapshot is STILL intact after all deletions on the derived map */
  for (i = 0; i < 500; i++) {
    TEST_ASSERT_EQ(getnum(snapshot, i), (long)i * 3);
  }

  /* delete everything from the snapshot down to empty */
  for (i = 0; i < 500; i++) {
    snapshot = lizard_hamt_dissoc(snapshot, num(i), heap, &removed);
  }
  snapshot = lizard_hamt_dissoc(snapshot, num(42), heap, &removed);
  for (i = 0; i < 64; i++) {
    snapshot = lizard_hamt_dissoc(snapshot, num(1000 + i * 32), heap, &removed);
  }
  TEST_ASSERT(snapshot == NULL); /* empties back to the null root */

  TEST_RETURN();
}
