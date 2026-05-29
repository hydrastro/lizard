/* tests/kernel_runtime_isolation_test.c
 *
 * Regression coverage for Recoverable Core Phase 1M: kernel/proof state,
 * metavariables, and kernel definitions are runtime-owned rather than
 * process-global.
 */

#include "lizard_api.h"
#include "lizard_internal.h"
#include "test_harness.h"

#include <gmp.h>
#include <string.h>

static int is_bool_value(lizard_value_t *v, int expected) {
  lizard_ast_node_t *node;
  node = (lizard_ast_node_t *)v;
  return node != NULL && node->type == AST_BOOL &&
         node->data.boolean == (expected ? 1 : 0);
}

static int is_number_value(lizard_value_t *v, long expected) {
  lizard_ast_node_t *node;
  node = (lizard_ast_node_t *)v;
  return node != NULL && node->type == AST_NUMBER &&
         mpz_cmp_si(node->data.number, expected) == 0;
}

int main(void) {
  lizard_runtime_t *rt1;
  lizard_runtime_t *rt2;
  lizard_context_t *ctx1;
  lizard_context_t *ctx2;
  lizard_value_t *value;
  lizard_status_t status;

  rt1 = lizard_runtime_create(NULL);
  rt2 = lizard_runtime_create(NULL);
  TEST_ASSERT(rt1 != NULL);
  TEST_ASSERT(rt2 != NULL);
  ctx1 = lizard_context_create(rt1);
  ctx2 = lizard_context_create(rt2);
  TEST_ASSERT(ctx1 != NULL);
  TEST_ASSERT(ctx2 != NULL);

  value = NULL;
  status = lizard_context_eval_string(ctx1, "(begin-proof '(Id Nat 0 0))", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT(value != NULL);
  TEST_ASSERT(!lizard_value_is_error(value));

  value = NULL;
  status = lizard_context_eval_string(ctx2, "(qed)", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_EVAL_ERROR);
  TEST_ASSERT(value != NULL);
  TEST_ASSERT(lizard_value_is_error(value));

  value = NULL;
  status = lizard_context_eval_string(ctx1, "(tactic-refl) (qed)", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT(value != NULL);
  TEST_ASSERT(!lizard_value_is_error(value));

  value = NULL;
  status = lizard_context_eval_string(ctx1, "(kernel-hole 'Nat)", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT(value != NULL);
  TEST_ASSERT(is_number_value(value, 0));

  value = NULL;
  status = lizard_context_eval_string(ctx1, "(kernel-meta-state)", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT(is_number_value(value, 1));

  value = NULL;
  status = lizard_context_eval_string(ctx2, "(kernel-meta-state)", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT(is_number_value(value, 0));

  value = NULL;
  status = lizard_context_eval_string(ctx1, "(kernel-define 'zero0 0 'Nat)", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT(is_bool_value(value, 1));

  value = NULL;
  status = lizard_context_eval_string(ctx2, "(kernel-lookup 'zero0)", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT(is_bool_value(value, 0));

  lizard_context_destroy(ctx2);
  lizard_context_destroy(ctx1);
  lizard_runtime_destroy(rt2);
  lizard_runtime_destroy(rt1);
  TEST_RETURN();
}
