#include "lizard.h"
#include <ctype.h>
#include <ds.h>
#include <gmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_parser(char *input) {
  printf("Testing input: %s\n", input);

  list_t *tokens = lizard_tokenize(input);
  if (!tokens) {
    printf("Error: Tokenization failed.\n");
    return;
  }

  list_t *ast_list = lizard_parse(tokens);
  if (!ast_list) {
    printf("Error: Parsing failed.\n");
    return;
  }
  lizard_ast_node_t *ast;
  list_node_t *node = ast_list->head;
  while (node != ast_list->nil) {
    ast = &((lizard_ast_list_node_t *)node)->ast;
    print_ast(ast, 0);
    node = node->next;
  }
  printf("\n");
}

int main(void) {
  test_parser("(define x 10)");
  test_parser("(if (> x 5) (display \"x is greater than 5\") (display \"x is 5 or less\"))");
  test_parser("(lambda (x y) (+ x y))");
  test_parser("(xet ((a 5) (b 10)) (+ a b))");
  test_parser("(begin (set! x 5) (display x))");

  return 0;
}
