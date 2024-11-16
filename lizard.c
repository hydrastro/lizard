#include <ctype.h>
#include <ds.h>
#include <gmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lizard.h"

bool lizard_is_digit(char *input, int i) {
  if (input[i] >= '0' && input[i] <= '9') {
    return true;
  }
  if ((input[i] == '-' || input[i] == '+') && input[i + 1] >= '0' &&
      input[i + 1] <= '9') {
    return true;
  }
  return false;
}

void lizard_add_token(list_t *list, lizard_token_type_t token_type,
                      char *data) {
  lizard_token_list_node_t *node;
  node = malloc(sizeof(lizard_token_list_node_t));
  node->token.type = token_type;
  switch (token_type) {
  case TOKEN_LEFT_PAREN:
  case TOKEN_RIGHT_PAREN:
    node->token.string = NULL;
    break;
  case TOKEN_SYMBOL:
    node->token.symbol = data;
    break;
  case TOKEN_NUMBER:
    mpz_init(node->token.number);
    mpz_set_str(node->token.number, data, 10);
    break;
  case TOKEN_STRING:
    node->token.string = data;
    break;
  }
  list_append(list, &node->node);
}

list_t *lizard_tokenize(char *input) {
  list_t *list = list_create();
  int i, j, k;
  char *buffer;
  i = 0;
  while (input[i] != '\0') {
    if (input[i] == ' ') {
      i++;
      continue;
    }
    if (input[i] == '(') {
      lizard_add_token(list, TOKEN_LEFT_PAREN, NULL);
      i++;
    } else if (input[i] == ')') {
      lizard_add_token(list, TOKEN_RIGHT_PAREN, NULL);
      i++;
    } else if (input[i] == '"') {
      j = i;
      i++;
      while (input[i] != '"' && input[i] != '\0') {
        i++;
      }
      if (input[i] == '\0') {
        fprintf(stderr, "Error: unterminated string.\n");
        exit(1);
      }
      buffer = malloc(sizeof(char) * (i - j));
      for (k = 0, j++; j < i; j++, k++) {
        buffer[k] = input[j];
      }
      buffer[k] = '\0';
      lizard_add_token(list, TOKEN_STRING, buffer);
      i++;
    } else if (lizard_is_digit(input, i)) {
      j = i;
      while (lizard_is_digit(input, i)) {
        i++;
      }
      buffer = malloc(sizeof(char) * (i - j + 1));
      for (k = 0; j < i; j++, k++) {
        buffer[k] = input[j];
      }
      buffer[k] = '\0';
      lizard_add_token(list, TOKEN_NUMBER, buffer);
    } else {
      j = i;
      while (input[i] != ' ' && input[i] != '\0' && input[i] != '(' &&
             input[i] != ')') {
        i++;
      }
      buffer = malloc(sizeof(char) * (i - j + 1));
      for (k = 0; j < i; j++, k++) {
        buffer[k] = input[j];
      }
      buffer[k] = '\0';
      lizard_add_token(list, TOKEN_SYMBOL, buffer);
    }
  }

  return list;
}

lizard_ast_node_t *lizard_parse_expression(list_t *token_list,
                                    list_node_t **current_node_pointer,
                                    int *depth) {
  list_node_t *current_node = *current_node_pointer;
  int current_depth;
  lizard_token_t *current_token =
      &CAST(current_node, lizard_token_list_node_t)->token;

  if (current_node == token_list->nil) {
    return NULL;
  }
  lizard_ast_node_t *ast_node = malloc(sizeof(lizard_ast_node_t));
  lizard_ast_list_node_t *ast_list_node;
  switch (current_token->type) {
  case TOKEN_LEFT_PAREN: {
    *current_node_pointer = current_node->next;
    current_node = current_node->next;
    current_token = &CAST(current_node, lizard_token_list_node_t)->token;
    *depth = *depth + 1;

    if (current_token->type == TOKEN_SYMBOL) {
      if (strcmp(current_token->symbol, "quote") == 0) {
        ast_node->type = AST_QUOTED;
        *current_node_pointer = current_node->next;
        current_node = current_node->next;
        ast_node->quoted =
            lizard_parse_expression(token_list, current_node_pointer, depth);
        current_node = *current_node_pointer;
        if (current_node == token_list->nil ||
            CAST(current_node, lizard_token_list_node_t)->token.type !=
                TOKEN_RIGHT_PAREN) {
          fprintf(stderr, "Error: missing closing paren.\n");
          exit(1);
        }
        *current_node_pointer = current_node->next;
        current_node = *current_node_pointer;
        *depth -= 1;
      } else if (strcmp(current_token->symbol, "set!") == 0) {
        ast_node->type = AST_ASSIGNMENT;
        *current_node_pointer = current_node->next;
        current_node = current_node->next;
        if (current_node == token_list->nil) {
          fprintf(stderr, "Error: missing assignment variable.\n");
          exit(1);
        }
        lizard_ast_node_t *var_node =
            lizard_parse_expression(token_list, current_node_pointer, depth);
        if (current_node == token_list->nil) {
          fprintf(stderr, "Error: missing assignment value.\n");
          exit(1);
        }
        lizard_ast_node_t *val_node =
            lizard_parse_expression(token_list, current_node_pointer, depth);
        ast_node->assignment.variable = var_node;
        ast_node->assignment.value = val_node;
        current_node = *current_node_pointer;
        if (current_node == token_list->nil ||
            CAST(current_node, lizard_token_list_node_t)->token.type !=
                TOKEN_RIGHT_PAREN) {
          fprintf(stderr, "Error: missing closing paren.\n");
          exit(1);
        }
        *current_node_pointer = current_node->next;
        current_node = *current_node_pointer;
        *depth -= 1;
      } else if (strcmp(current_token->symbol, "define") == 0) {
        ast_node->type = AST_DEFINITION;
        *current_node_pointer = current_node->next;
        current_node = current_node->next;
        if (current_node == token_list->nil) {
          fprintf(stderr, "Error: missing definition variable.\n");
          exit(1);
        }
        lizard_ast_node_t *var_node =
            lizard_parse_expression(token_list, current_node_pointer, depth);
        if (current_node == token_list->nil) {
          fprintf(stderr, "Error: missing definition value.\n");
          exit(1);
        }
        lizard_ast_node_t *val_node =
            lizard_parse_expression(token_list, current_node_pointer, depth);
        ast_node->definition.variable = var_node;
        ast_node->definition.value = val_node;
        current_node = *current_node_pointer;
        if (current_node == token_list->nil ||
            CAST(current_node, lizard_token_list_node_t)->token.type !=
                TOKEN_RIGHT_PAREN) {
          fprintf(stderr, "Error: missing closing paren.\n");
          exit(1);
        }
        *current_node_pointer = current_node->next;
        current_node = *current_node_pointer;
        *depth -= 1;
      } else if (strcmp(current_token->symbol, "lambda") == 0) {
        ast_node->type = AST_LAMBDA;
        ast_node->lambda_parameters = list_create();
        *current_node_pointer = current_node->next;
        current_node = current_node->next;
        if (current_node == token_list->nil) {
          fprintf(stderr, "Error: missing lambda parameter(s).\n");
          exit(1);
        }
        ast_node->lambda_parameters = list_create();
        current_depth = *depth;
        while (current_node != token_list->nil &&
               CAST(current_node, lizard_token_list_node_t)->token.type !=
                   TOKEN_RIGHT_PAREN &&
               current_depth <= *depth) {
          ast_list_node =
              (lizard_ast_list_node_t *)malloc(sizeof(lizard_ast_list_node_t));
          ast_list_node->ast =
              *lizard_parse_expression(token_list, current_node_pointer, depth);
          list_append(ast_node->lambda_parameters, &ast_list_node->node);
          current_node = *current_node_pointer;
        }
        current_node = *current_node_pointer;
        if (current_node == token_list->nil ||
            CAST(current_node, lizard_token_list_node_t)->token.type !=
                TOKEN_RIGHT_PAREN) {
          fprintf(stderr, "Error: missing closing paren.\n");
          exit(1);
        }
        *current_node_pointer = current_node->next;
        current_node = *current_node_pointer;
        *depth -= 1;
      } else if (strcmp(current_token->symbol, "if") == 0) {
        ast_node->type = AST_IF;
        *current_node_pointer = current_node->next;
        current_node = current_node->next;
        if (current_node == token_list->nil) {
          fprintf(stderr, "Error: missing if predicate.\n");
          exit(1);
        }
        ast_node->if_clause.pred =
            lizard_parse_expression(token_list, current_node_pointer, depth);
        if (current_node == token_list->nil) {
          fprintf(stderr, "Error: missing if cons.\n");
          exit(1);
        }
        ast_node->if_clause.cons =
            lizard_parse_expression(token_list, current_node_pointer, depth);
        if (current_node->next != token_list->nil &&
            CAST(current_node->next, lizard_token_list_node_t)->token.type !=
                TOKEN_RIGHT_PAREN) {
          ast_node->if_clause.alt =
              lizard_parse_expression(token_list, current_node_pointer, depth);
        }
        current_node = *current_node_pointer;
        if (current_node == token_list->nil ||
            CAST(current_node, lizard_token_list_node_t)->token.type !=
                TOKEN_RIGHT_PAREN) {
          fprintf(stderr, "Error: missing closing paren.\n");
          exit(1);
        }
        *current_node_pointer = current_node->next;
        current_node = *current_node_pointer;
        *depth -= 1;
      } else if (strcmp(current_token->symbol, "begin") == 0) {
        ast_node->type = AST_BEGIN;
        *current_node_pointer = current_node->next;
        current_node = current_node->next;
        if (current_node == token_list->nil) {
          fprintf(stderr, "Error: missing begin predicate(s).\n");
          exit(1);
        }
        ast_node->begin_expressions = list_create();
        current_depth = *depth;
        while (current_node != token_list->nil &&
               CAST(current_node, lizard_token_list_node_t)->token.type !=
                   TOKEN_RIGHT_PAREN &&
               current_depth <= *depth) {
          ast_list_node =
              (lizard_ast_list_node_t *)malloc(sizeof(lizard_ast_list_node_t));
          ast_list_node->ast =
              *lizard_parse_expression(token_list, current_node_pointer, depth);
          list_append(ast_node->begin_expressions, &ast_list_node->node);
          current_node = *current_node_pointer;
        }
        current_node = *current_node_pointer;
        if (current_node == token_list->nil ||
            CAST(current_node, lizard_token_list_node_t)->token.type !=
                TOKEN_RIGHT_PAREN) {
          fprintf(stderr, "Error: missing closing paren.\n");
          exit(1);
        }
        *current_node_pointer = current_node->next;
        current_node = *current_node_pointer;
        *depth -= 1;
      } else if (strcmp(current_token->symbol, "cond") == 0) {
        ast_node->type = AST_COND;
        *current_node_pointer = current_node->next;
        current_node = current_node->next;
        if (current_node == token_list->nil) {
          fprintf(stderr, "Error: missing cond predicate(s).\n");
          exit(1);
        }
        ast_node->cond_clauses = list_create();
        current_depth = *depth;
        while (current_node != token_list->nil &&
               CAST(current_node, lizard_token_list_node_t)->token.type !=
                   TOKEN_RIGHT_PAREN &&
               current_depth <= *depth) {
          ast_list_node =
              (lizard_ast_list_node_t *)malloc(sizeof(lizard_ast_list_node_t));
          ast_list_node->ast =
              *lizard_parse_expression(token_list, current_node_pointer, depth);
          list_append(ast_node->cond_clauses, &ast_list_node->node);
          current_node = *current_node_pointer;
        }
        current_node = *current_node_pointer;
        if (current_node == token_list->nil ||
            CAST(current_node, lizard_token_list_node_t)->token.type !=
                TOKEN_RIGHT_PAREN) {
          fprintf(stderr, "Error: missing closing paren.\n");
          exit(1);
        }
        *current_node_pointer = current_node->next;
        current_node = *current_node_pointer;
        *depth -= 1;
      } else if (strcmp(current_token->symbol, "let") == 0) {
        ast_node->type = AST_IF; // TODO
        current_node = *current_node_pointer;
        if (current_node == token_list->nil ||
            CAST(current_node, lizard_token_list_node_t)->token.type !=
                TOKEN_RIGHT_PAREN) {
          fprintf(stderr, "Error: missing closing paren.\n");
          exit(1);
        }
        *current_node_pointer = current_node->next;
        current_node = *current_node_pointer;
        *depth -= 1;
      } else {
        ast_node->type = AST_APPLICATION;
        if (current_node == token_list->nil) {
          fprintf(stderr, "Error: missing application argument(s).\n");
          exit(1);
        }
        ast_node->application_arguments = list_create();
        current_depth = *depth;
        while (current_node != token_list->nil &&
               CAST(current_node, lizard_token_list_node_t)->token.type !=
                   TOKEN_RIGHT_PAREN &&
               current_depth <= *depth) {
          ast_list_node =
              (lizard_ast_list_node_t *)malloc(sizeof(lizard_ast_list_node_t));
          ast_list_node->ast =
              *lizard_parse_expression(token_list, current_node_pointer, depth);
          list_append(ast_node->application_arguments, &ast_list_node->node);
          current_node = *current_node_pointer;
        }
        current_node = *current_node_pointer;
        if (current_node == token_list->nil ||
            CAST(current_node, lizard_token_list_node_t)->token.type !=
                TOKEN_RIGHT_PAREN) {
          fprintf(stderr, "Error: missing closing paren.\n");
          exit(1);
        }
        current_node = *current_node_pointer;
        *current_node_pointer = current_node->next;
        current_node = *current_node_pointer;
        *depth -= 1;
      }
    } else if (current_token->symbol == TOKEN_LEFT_PAREN) {
      ast_node->type = AST_APPLICATION;
      if (current_node == token_list->nil) {
        fprintf(stderr, "Error: missing application argument(s).\n");
        exit(1);
      }
      ast_node->application_arguments = list_create();
      current_depth = *depth;
      while (current_node != token_list->nil &&
             CAST(current_node, lizard_token_list_node_t)->token.type !=
                 TOKEN_RIGHT_PAREN &&
             current_depth <= *depth) {
        ast_list_node =
            (lizard_ast_list_node_t *)malloc(sizeof(lizard_ast_list_node_t));
        ast_list_node->ast =
            *lizard_parse_expression(token_list, current_node_pointer, depth);
        list_append(ast_node->application_arguments, &ast_list_node->node);
        current_node = *current_node_pointer;
      }
      current_node = *current_node_pointer;
      if (current_node == token_list->nil ||
          CAST(current_node, lizard_token_list_node_t)->token.type !=
              TOKEN_RIGHT_PAREN) {
        fprintf(stderr, "Error: missing closing paren.\n");
        exit(1);
      }
      current_node = *current_node_pointer;
      *current_node_pointer = current_node->next;
      current_node = *current_node_pointer;
      *depth -= 1;
    }

    break;
  }
  case TOKEN_RIGHT_PAREN:
    if (*depth == 0) {
      fprintf(stderr, "Error: unmatched closing parenthesis.\n");
      exit(1);
    }
    *depth = *depth - 1;
    break;
  case TOKEN_SYMBOL:
    ast_node->type = AST_SYMBOL;
    ast_node->variable = current_token->symbol;
    *current_node_pointer = current_node->next;
    current_node = current_node->next;
    break;
  case TOKEN_NUMBER:
    ast_node->type = AST_NUMBER;
    mpz_init(ast_node->number);
    mpz_set(ast_node->number, current_token->number);
    *current_node_pointer = current_node->next;
    current_node = current_node->next;
    break;
  case TOKEN_STRING:
    ast_node->type = AST_STRING;
    ast_node->string = current_token->string;
    *current_node_pointer = current_node->next;
    current_node = current_node->next;
    break;
  default:
    fprintf(stderr, "Error: unexpected token type.\n");
    exit(1);
  }

  return ast_node;
}

list_t *lizard_parse(list_t *token_list) {
  list_t *ast_list = list_create();
  list_node_t *current = token_list->head;
  current = token_list->head;
  int depth = 0;
  while (current != token_list->nil) {
    lizard_ast_node_t *ast = lizard_parse_expression(token_list, &current, &depth);
    lizard_ast_list_node_t *ast_node =
        (lizard_ast_list_node_t *)malloc(sizeof(lizard_ast_list_node_t));
    ast_node->ast = *ast;

    if (ast != NULL) {
      list_append(ast_list, &ast_node->node);
    } else {
      fprintf(stderr, "Error: Failed to parse expression.\n");
      exit(1);
    }
  }

  return ast_list;
}

void print_ast(lizard_ast_node_t *node, int depth) {
  if (!node) {
    return;
  }

  for (int i = 0; i < depth; i++) {
    printf("  ");
  }
  switch (node->type) {
  case AST_STRING:
    printf("String: \"%s\"\n", node->string);
    break;
  case AST_NUMBER:
    gmp_printf("Number: %Zd\n", node->number);
    break;
  case AST_SYMBOL:
    printf("Symbol: %s\n", node->variable);
    break;
  case AST_QUOTED:
    printf("Quote:\n");
    print_ast(node->quoted, depth + 1);
    break;
  case AST_ASSIGNMENT:
    printf("Assignment:\n");
    print_ast(node->assignment.variable, depth + 1);
    print_ast(node->assignment.value, depth + 1);
    break;
  case AST_DEFINITION:
    printf("Definition:\n");
    print_ast(node->definition.variable, depth + 1);
    print_ast(node->definition.value, depth + 1);
    break;
  case AST_IF:
    printf("If clause:\n");
    print_ast(node->if_clause.pred, depth + 1);
    print_ast(node->if_clause.cons, depth + 1);
    if (node->if_clause.alt)
      print_ast(node->if_clause.alt, depth + 1);
    break;
  case AST_LAMBDA:
    printf("Lambda parameters:\n");
    list_node_t *param = node->lambda_parameters->head;
    while (param != node->lambda_parameters->nil) {
      print_ast(&((lizard_ast_list_node_t *)param)->ast, depth + 1);
      param = param->next;
    }
    break;
  case AST_BEGIN:
    printf("Begin:\n");
    list_node_t *expr = node->begin_expressions->head;
    while (expr != node->begin_expressions->nil) {
      print_ast(&((lizard_ast_list_node_t *)expr)->ast, depth + 1);
      expr = expr->next;
    }
    break;
  case AST_APPLICATION:
    printf("Application:\n");
    list_node_t *arg = node->application_arguments->head;
    while (arg != node->application_arguments->nil) {
      print_ast(&((lizard_ast_list_node_t *)arg)->ast, depth + 1);
      arg = arg->next;
    }
    break;
  default:
    printf("Unknown AST node type.\n");
  }
}
