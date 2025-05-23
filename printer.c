#include "printer.h"
#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void lizard_print_indent(int depth) {
  for (int i = 0; i < depth; i++) {
    printf("  ");
  }
}

void lizard_fprint_ast(FILE *fp, lizard_ast_node_t *node, int depth) {
  if (!node) {
    fprintf(fp, "NULL\n");
    return;
  }
  lizard_print_indent(depth);
  switch (node->type) {
  case AST_STRING:
    fprintf(fp, "String: \"%s\"\n", node->data.string);
    break;
  case AST_NUMBER:
    fprintf(fp, "Number: ");
    gmp_printf("%Zd", node->data.number);
    fprintf(fp, "\n");
    break;
  case AST_SYMBOL:
    fprintf(fp, "Symbol: %s\n", node->data.variable);
    break;
  case AST_BOOL:
    fprintf(fp, "Boolean: %s\n", node->data.boolean ? "#t" : "#f");
    break;
  case AST_PAIR:
    fprintf(fp, "Pair:\n");
    lizard_print_indent(depth + 1);
    fprintf(fp, "Car:\n");
    lizard_fprint_ast(fp, node->data.pair.car, depth + 2);
    lizard_print_indent(depth + 1);
    fprintf(fp, "Cdr:\n");
    lizard_fprint_ast(fp, node->data.pair.cdr, depth + 2);
    break;
  case AST_NIL:
    fprintf(fp, "Nil\n");
    break;
  case AST_QUOTE:
    fprintf(fp, "Quote:\n");
    lizard_fprint_ast(fp, node->data.quoted, depth + 1);
    break;
  case AST_QUASIQUOTE:
    fprintf(fp, "Quasiquote:\n");
    lizard_fprint_ast(fp, node->data.quoted, depth + 1);
    break;
  case AST_UNQUOTE:
    fprintf(fp, "Unquote:\n");
    lizard_fprint_ast(fp, node->data.quoted, depth + 1);
    break;
  case AST_UNQUOTE_SPLICING:
    fprintf(fp, "Unquote-splicing:\n");
    lizard_fprint_ast(fp, node->data.quoted, depth + 1);
    break;
  case AST_ASSIGNMENT:
    fprintf(fp, "Assignment:\n");
    lizard_fprint_ast(fp, node->data.assignment.variable, depth + 1);
    lizard_fprint_ast(fp, node->data.assignment.value, depth + 1);
    break;
  case AST_DEFINITION:
    fprintf(fp, "Definition:\n");
    lizard_fprint_ast(fp, node->data.definition.variable, depth + 1);
    lizard_fprint_ast(fp, node->data.definition.value, depth + 1);
    break;
  case AST_IF:
    fprintf(fp, "If:\n");
    lizard_fprint_ast(fp, node->data.if_clause.pred, depth + 1);
    lizard_fprint_ast(fp, node->data.if_clause.cons, depth + 1);
    if (node->data.if_clause.alt)
      lizard_fprint_ast(fp, node->data.if_clause.alt, depth + 1);
    break;
  case AST_LAMBDA:
    fprintf(fp, "Lambda:\n");
    if (node->data.lambda.parameters) {
      list_node_t *iter = node->data.lambda.parameters->head;
      while (iter != node->data.lambda.parameters->nil) {
        lizard_fprint_ast(fp, ((lizard_ast_list_node_t *)iter)->ast, depth + 1);
        iter = iter->next;
      }
    }
    break;
  case AST_BEGIN:
    fprintf(fp, "Begin:\n");
    {
      list_node_t *iter = node->data.begin_expressions->head;
      while (iter != node->data.begin_expressions->nil) {
        lizard_fprint_ast(fp, ((lizard_ast_list_node_t *)iter)->ast, depth + 1);
        iter = iter->next;
      }
    }
    break;
  case AST_APPLICATION:
    fprintf(fp, "Application:\n");
    {
      list_node_t *iter = node->data.application_arguments->head;
      if (iter == node->data.application_arguments->nil) {
        lizard_print_indent(depth + 1);
        fprintf(fp, "Empty application\n");
      }
      while (iter != node->data.application_arguments->nil) {
        lizard_fprint_ast(fp, ((lizard_ast_list_node_t *)iter)->ast, depth + 1);
        iter = iter->next;
      }
    }
    break;
  case AST_MACRO:
    fprintf(fp, "Macro:\n");
    lizard_fprint_ast(fp, node->data.macro_def.variable, depth + 1);
    lizard_fprint_ast(fp, node->data.macro_def.transformer, depth + 1);
    break;
  case AST_PROMISE:
    fprintf(fp, "Promise (forced=%s)\n",
            node->data.promise.forced ? "true" : "false");
    if (node->data.promise.forced)
      lizard_fprint_ast(fp, node->data.promise.value, depth + 1);
    else
      lizard_fprint_ast(fp, node->data.promise.expr, depth + 1);
    break;
  case AST_CONTINUATION:
    fprintf(fp, "Continuation: %p\n", node->data.continuation.captured_cont);
    break;
  case AST_CALLCC:
    fprintf(fp, "Call/cc:\n");
    {
      list_node_t *iter = node->data.application_arguments->head;
      while (iter != node->data.application_arguments->nil) {
        lizard_fprint_ast(fp, ((lizard_ast_list_node_t *)iter)->ast, depth + 1);
        iter = iter->next;
      }
    }
    break;
  case AST_ERROR:
    fprintf(fp, "Error (code %d):\n", node->data.error.code);
    {
      list_node_t *iter = node->data.error.data->head;
      while (iter != node->data.error.data->nil) {
        lizard_fprint_ast(fp, ((lizard_ast_list_node_t *)iter)->ast, depth + 1);
        iter = iter->next;
      }
    }
    break;
  default:
    fprintf(fp, "Unknown AST node type.\n");
  }
}

void lizard_print_ast(lizard_ast_node_t *node, int depth) {
  lizard_fprint_ast(stdout, node, depth);
}
