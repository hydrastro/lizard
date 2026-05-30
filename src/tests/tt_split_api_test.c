/* tests/tt_split_api_test.c
 *
 * Regression guard for the Phase 1I file split. These tests exercise the
 * public/internal APIs now implemented outside tt_equality.c so the split
 * remains behavior-preserving.
 */

#include "test_harness.h"
#include "test_helpers.h"
#include "tt_lattice.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *u1;
  lizard_ast_node_t *u2;
  lizard_ast_node_t *uset_a;
  lizard_ast_node_t *uset_b;
  lizard_ast_node_t *coset_a;
  lizard_ast_node_t *coset_b;
  lizard_ast_node_t *phi;
  lizard_ast_node_t *psi;
  lizard_ast_node_t *system;
  lizard_ast_node_t *picked;
  long *dims_a;
  long *dims_b;
  long *codims_a;
  long *codims_b;

  lizard_test_env_init(&e);

  u1 = lizard_tt_make_universe(e.heap, 1);
  u2 = lizard_tt_make_universe(e.heap, 2);
  TEST_ASSERT(lizard_tt_universe_leq(u1, u2) == 1);
  TEST_ASSERT(lizard_tt_universe_leq(u2, u1) == 0);

  dims_a = lizard_heap_alloc(sizeof(long));
  dims_b = lizard_heap_alloc(sizeof(long) * 2U);
  dims_a[0] = 1;
  dims_b[0] = 1;
  dims_b[1] = 3;
  uset_a = lizard_tt_make_universe_set(e.heap, dims_a, 1);
  uset_b = lizard_tt_make_universe_set(e.heap, dims_b, 2);
  TEST_ASSERT(lizard_tt_universe_set_subset(uset_a, uset_b) == 1);
  TEST_ASSERT(lizard_tt_universe_leq(uset_a, uset_b) == 1);

  codims_a = lizard_heap_alloc(sizeof(long));
  codims_b = lizard_heap_alloc(sizeof(long) * 2U);
  codims_a[0] = 4;
  codims_b[0] = 4;
  codims_b[1] = 5;
  coset_a = lizard_tt_make_couniverse_set(e.heap, codims_a, 1);
  coset_b = lizard_tt_make_couniverse_set(e.heap, codims_b, 2);
  TEST_ASSERT(lizard_tt_couniverse_set_subset(coset_a, coset_b) == 1);
  TEST_ASSERT(lizard_tt_couniverse_leq(coset_a, coset_b) == 1);
  TEST_ASSERT(lizard_tt_couniverse_leq(coset_a, uset_b) == -1);

  phi = lizard_test_eval(&e,
      "(F-and (F-eq (I-var 'i) (i0)) (F1))");
  psi = lizard_test_eval(&e, "(F-eq (I-var 'i) (i0))");
  TEST_ASSERT(lizard_tt_face_entails(phi, psi) == 1);

  system = lizard_test_eval(&e,
      "(system-cons (F-eq (I-var 'i) (i0)) 'v0"
      " (system-cons (F-eq (I-var 'i) (i1)) 'v1 (system-nil)))");
  picked = lizard_tt_system_lookup(system, psi);
  TEST_ASSERT(lizard_test_is_symbol(picked, "v0"));

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
