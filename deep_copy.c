#include "deep_copy.h"
#include "mem.h"
#include <gmp.h>
#include <string.h>

lizard_ast_node_t *lizard_ast_deep_copy(lizard_ast_node_t *node,
                                        lizard_heap_t *heap) {
  if (!node)
    return NULL;
  lizard_ast_node_t *copy = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  copy->type = node->type;
  switch (node->type) {
  case AST_NUMBER:
    mpz_init(copy->data.number);
    mpz_set(copy->data.number, node->data.number);
    break;
  case AST_STRING:
    copy->data.string = strdup(node->data.string);
    break;
  case AST_SYMBOL:
    copy->data.variable = strdup(node->data.variable);
    break;
  case AST_BOOL:
    copy->data.boolean = node->data.boolean;
    break;
  case AST_NIL:
    return node;
  case AST_QUOTE:
  case AST_QUASIQUOTE:
  case AST_UNQUOTE:
  case AST_UNQUOTE_SPLICING:
    copy->data.quoted = lizard_ast_deep_copy(node->data.quoted, heap);
    break;
  case AST_APPLICATION:
    copy->data.application_arguments =
        list_create_alloc(lizard_heap_alloc, lizard_heap_free);
    {
      list_node_t *iter;
      for (iter = node->data.application_arguments->head;
           iter != node->data.application_arguments->nil; iter = iter->next) {
        lizard_ast_list_node_t *orig = (lizard_ast_list_node_t *)iter;
        lizard_ast_list_node_t *copy_node =
            lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
        copy_node->ast = lizard_ast_deep_copy(orig->ast, heap);
        list_append(copy->data.application_arguments, &copy_node->node);
      }
    }
    break;
  default:
    memcpy(&(copy->data), &(node->data), sizeof(node->data));
  }
  return copy;
}
