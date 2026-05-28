/* tests/runtime_registry_isolation_test.c
 *
 * Regression coverage for Phase 1F's tt_registry.c split.  Logic-rule
 * state is runtime-owned via heap->runtime, so two independent runtime
 * contexts must not see each other's toggles.
 */

#include "lizard_api.h"
#include "lizard_internal.h"
#include "test_harness.h"

#include <string.h>

static int is_bool_value(lizard_value_t *v, int expected) {
  lizard_ast_node_t *node;
  node = (lizard_ast_node_t *)v;
  return node != NULL && node->type == AST_BOOL &&
         node->data.boolean == (expected ? 1 : 0);
}

static int is_symbol_value(lizard_value_t *v, const char *name) {
  lizard_ast_node_t *node;
  node = (lizard_ast_node_t *)v;
  return node != NULL && node->type == AST_SYMBOL &&
         node->data.variable != NULL && strcmp(node->data.variable, name) == 0;
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
  status = lizard_context_eval_string(ctx1, "(logic-rule-enable 'phase1f-rule)", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);

  value = NULL;
  status = lizard_context_eval_string(ctx1, "(logic-rule-enabled? 'phase1f-rule)", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT(is_bool_value(value, 1));

  value = NULL;
  status = lizard_context_eval_string(ctx2, "(logic-rule-enabled? 'phase1f-rule)", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT(is_symbol_value(value, "unknown"));

  value = NULL;
  status = lizard_context_eval_string(ctx2, "(logic-rule-enable 'phase1f-rule)", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  value = NULL;
  status = lizard_context_eval_string(ctx2, "(logic-rule-enabled? 'phase1f-rule)", &value);
  TEST_ASSERT_EQ(status, LIZARD_STATUS_OK);
  TEST_ASSERT(is_bool_value(value, 1));

  lizard_context_destroy(ctx2);
  lizard_context_destroy(ctx1);
  lizard_runtime_destroy(rt2);
  lizard_runtime_destroy(rt1);
  TEST_RETURN();
}
