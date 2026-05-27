#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lz_list_t *tokens;
  lizard_token_list_node_t *node;

  lizard_test_env_init(&e);
  tokens = lizard_tokenize("\n  (define x 10)\n  x");
  node = (lizard_token_list_node_t *)tokens->head;
  TEST_ASSERT_EQ(node->token.type, TOKEN_LEFT_PAREN);
  TEST_ASSERT_EQ(node->token.line, 2);
  TEST_ASSERT_EQ(node->token.column, 3);

  node = (lizard_token_list_node_t *)node->node.next;
  TEST_ASSERT_EQ(node->token.type, TOKEN_SYMBOL);
  TEST_ASSERT_STR(node->token.data.symbol, "define");
  TEST_ASSERT_EQ(node->token.line, 2);
  TEST_ASSERT_EQ(node->token.column, 4);

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
