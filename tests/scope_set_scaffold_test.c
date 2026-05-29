/* tests/scope_set_scaffold_test.c
 *
 * Phase 2D: explicit scope-set scaffold for future hygienic expansion.
 * The old lizard_surface_scope() summary remains compatible, but real
 * callers should use set containment/equality.
 */

#include "surface_term.h"
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *value;
  lizard_surface_term_t *a;
  lizard_surface_term_t *b;
  lizard_surface_scope_set_t *set0;
  lizard_surface_scope_set_t *set1;
  lizard_surface_scope_set_t *set2;
  lizard_surface_scope_set_t *set3;

  lizard_test_env_init(&e);
  value = lizard_test_eval(&e, "(+ 1 2)");
  TEST_ASSERT(lizard_test_is_int(value, 3));

  a = lizard_surface_from_ast(e.heap, value);
  b = lizard_surface_from_ast(e.heap, value);
  TEST_ASSERT(a != NULL);
  TEST_ASSERT(b != NULL);
  TEST_ASSERT(lizard_surface_same_scopes(a, b) == 1);
  TEST_ASSERT(lizard_surface_scope(a) == 0UL);

  lizard_surface_add_scope(a, 0x01UL);
  lizard_surface_add_scope(a, 0x08UL);
  TEST_ASSERT(lizard_surface_has_scope(a, 0x01UL) == 1);
  TEST_ASSERT(lizard_surface_has_scope(a, 0x08UL) == 1);
  TEST_ASSERT(lizard_surface_has_scope(a, 0x04UL) == 0);
  TEST_ASSERT_EQ(lizard_surface_scope(a), 0x09UL);
  TEST_ASSERT_EQ(lizard_surface_scope_set_count(lizard_surface_scopes(a)), 2UL);
  TEST_ASSERT(lizard_surface_same_scopes(a, b) == 0);

  lizard_surface_add_scope(b, 0x08UL);
  lizard_surface_add_scope(b, 0x01UL);
  TEST_ASSERT(lizard_surface_same_scopes(a, b) == 1);

  lizard_surface_remove_scope(b, 0x08UL);
  TEST_ASSERT(lizard_surface_has_scope(b, 0x08UL) == 0);
  TEST_ASSERT(lizard_surface_has_scope(b, 0x01UL) == 1);
  TEST_ASSERT(lizard_surface_same_scopes(a, b) == 0);

  lizard_surface_clear_scopes(a);
  TEST_ASSERT(lizard_surface_scope(a) == 0UL);
  TEST_ASSERT_EQ(lizard_surface_scope_set_count(lizard_surface_scopes(a)), 0UL);

  set0 = lizard_surface_scope_set_empty(e.heap);
  set1 = lizard_surface_scope_set_add(e.heap, set0, 7UL);
  set2 = lizard_surface_scope_set_add(e.heap, set1, 11UL);
  set3 = lizard_surface_scope_set_remove(e.heap, set2, 7UL);
  TEST_ASSERT(lizard_surface_scope_set_contains(set0, 7UL) == 0);
  TEST_ASSERT(lizard_surface_scope_set_contains(set1, 7UL) == 1);
  TEST_ASSERT(lizard_surface_scope_set_contains(set2, 11UL) == 1);
  TEST_ASSERT(lizard_surface_scope_set_contains(set3, 7UL) == 0);
  TEST_ASSERT(lizard_surface_scope_set_contains(set3, 11UL) == 1);

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
