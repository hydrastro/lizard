#include "lizard.h"
#include <ctype.h>
#include <ds.h>
#include <gmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

static const char *lizard_error_messages_lang_en[LIZARD_ERROR_COUNT] = {
#define X(fst, snd) snd,
    ERROR_MESSAGES_EN
#undef X
};

static const char **lizard_error_messages[LIZARD_LANG_COUNT] = {
#define X(fst, snd) snd,
    ERROR_MESSAGES
#undef X
};

bool lizard_is_digit(const char *input, int i) {
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
    node->token.data.string = NULL;
    break;
  case TOKEN_SYMBOL:
    node->token.data.symbol = data;
    break;
  case TOKEN_NUMBER:
    mpz_init(node->token.data.number);
    mpz_set_str(node->token.data.number, data, 10);
    break;
  case TOKEN_STRING:
    node->token.data.string = data;
    break;
  }
  list_append(list, &node->node);
}

list_t *lizard_tokenize(const char *input) {
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
      buffer = malloc(sizeof(char) * (long unsigned int)(i - j));
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
      buffer = malloc(sizeof(char) * (long unsigned int)(i - j + 1));
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
      buffer = malloc(sizeof(char) * (long unsigned int)(i - j + 1));
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
                                           int *depth, lizard_heap_t *heap) {
  lizard_ast_node_t *ast_node, *val_node, *var_node, *body_node, *lambda_node,
      *params_app, *def_node, *fn_symbol, *app_node, *var, *value, *macro_name,
      *transformer, *quoted_node;
  list_node_t *current_node, *val_iter, *p;
  lizard_ast_list_node_t *ast_list_node, *name_node, *params_wrapper,
      *body_wrapper, *val_list_node, *var_list_node, *lambda_wrapper;
  list_t *params_list, *bindings, *values;
  const char *fn_name;
  int current_depth;
  lizard_token_t *current_token;
  current_node = *current_node_pointer;
  current_token = &CAST(current_node, lizard_token_list_node_t)->token;

  if (current_node == token_list->nil) {
    return NULL;
  }
  ast_node = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
  switch (current_token->type) {
  case TOKEN_LEFT_PAREN: {
    *current_node_pointer = current_node->next;
    current_node = current_node->next;
    current_token = &CAST(current_node, lizard_token_list_node_t)->token;
    *depth = *depth + 1;

    if (current_token->type == TOKEN_SYMBOL) {
      if (strcmp(current_token->data.symbol, "quote") == 0) {
        ast_node->type = AST_QUOTED;
        *current_node_pointer = current_node->next;
        current_node = current_node->next;
        ast_node->data.quoted = lizard_parse_expression(
            token_list, current_node_pointer, depth, heap);
        current_node = *current_node_pointer;

        *current_node_pointer = current_node->next;
        current_node = *current_node_pointer;
        *depth -= 1;
      } else if (strcmp(current_token->data.symbol, "set!") == 0) {
        ast_node->type = AST_ASSIGNMENT;
        *current_node_pointer = current_node->next;
        current_node = current_node->next;
        if (current_node == token_list->nil) {
          fprintf(stderr, "Error: missing assignment variable.\n");
          exit(1);
        }
        var_node = lizard_parse_expression(token_list, current_node_pointer,
                                           depth, heap);
        if (current_node == token_list->nil) {
          fprintf(stderr, "Error: missing assignment value.\n");
          exit(1);
        }
        val_node = lizard_parse_expression(token_list, current_node_pointer,
                                           depth, heap);
        ast_node->data.assignment.variable = var_node;
        ast_node->data.assignment.value = val_node;
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
      } else if (strcmp(current_token->data.symbol, "define") == 0) {
        *current_node_pointer = current_node->next;
        current_node = *current_node_pointer;

        var_node = lizard_parse_expression(token_list, current_node_pointer,
                                           depth, heap);
        current_node = *current_node_pointer;

        if (var_node->type == AST_APPLICATION) {
          list_t *app_args = var_node->data.application_arguments;
          if (app_args->head == app_args->nil) {
            fprintf(stderr, "Error: invalid function definition syntax.\n");
            exit(1);
          }
          name_node = (lizard_ast_list_node_t *)app_args->head;
          if (name_node->ast.type != AST_SYMBOL) {
            fprintf(stderr, "Error: function name must be a symbol.\n");
            exit(1);
          }
          fn_name = name_node->ast.data.variable;

          body_node = lizard_parse_expression(token_list, current_node_pointer,
                                              depth, heap);
          current_node = *current_node_pointer;

          lambda_node = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
          lambda_node->type = AST_LAMBDA;
          lambda_node->data.lambda.closure_env = NULL;
          lambda_node->data.lambda.parameters = list_create();

          params_app = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
          params_app->type = AST_APPLICATION;
          params_list = list_create();
          for (p = app_args->head->next; p != app_args->nil; p = p->next) {
            lizard_ast_list_node_t *param_old = (lizard_ast_list_node_t *)p;
            lizard_ast_list_node_t *param_copy =
                lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
            param_copy->ast = param_old->ast; /* shallow copy */
            list_append(params_list, &param_copy->node);
          }
          params_app->data.application_arguments = params_list;
          params_wrapper =
              lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
          params_wrapper->ast = *params_app;
          list_append(lambda_node->data.lambda.parameters,
                      &params_wrapper->node);

          body_wrapper =
              lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
          body_wrapper->ast = *body_node;
          list_append(lambda_node->data.lambda.parameters, &body_wrapper->node);

          def_node = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
          def_node->type = AST_DEFINITION;
          fn_symbol = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
          fn_symbol->type = AST_SYMBOL;
          fn_symbol->data.variable = fn_name;
          def_node->data.definition.variable = fn_symbol;
          def_node->data.definition.value = lambda_node;

          if (current_node == token_list->nil ||
              CAST(current_node, lizard_token_list_node_t)->token.type !=
                  TOKEN_RIGHT_PAREN) {
            fprintf(stderr, "Error: missing closing paren in define.\n");
            exit(1);
          }
          *current_node_pointer = current_node->next;
          current_node = current_node->next;
          *depth -= 1;
          return def_node;
        } else if (var_node->type == AST_SYMBOL) {
          lizard_ast_node_t *val_node = lizard_parse_expression(
              token_list, current_node_pointer, depth, heap);
          lizard_ast_node_t *def_node =
              lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
          def_node->type = AST_DEFINITION;
          def_node->data.definition.variable = var_node;
          def_node->data.definition.value = val_node;
          current_node = *current_node_pointer;
          if (current_node == token_list->nil ||
              CAST(current_node, lizard_token_list_node_t)->token.type !=
                  TOKEN_RIGHT_PAREN) {
            fprintf(stderr, "Error: missing closing paren.\n");
            exit(1);
          }
          *current_node_pointer = current_node->next;
          current_node = current_node->next;
          *depth -= 1;
          return def_node;
        } else {
          fprintf(stderr, "Error: invalid define syntax.\n");
          exit(1);
        }

      } else if (strcmp(current_token->data.symbol, "lambda") == 0) {
        ast_node->type = AST_LAMBDA;
        ast_node->data.lambda.parameters = list_create();
        ast_node->data.lambda.closure_env = NULL;

        *current_node_pointer = current_node->next;
        current_node = current_node->next;
        if (current_node == token_list->nil) {
          fprintf(stderr, "Error: missing lambda parameter(s).\n");
          exit(1);
        }
        ast_node->data.lambda.parameters = list_create();
        current_depth = *depth;
        while (current_node != token_list->nil &&
               CAST(current_node, lizard_token_list_node_t)->token.type !=
                   TOKEN_RIGHT_PAREN &&
               current_depth <= *depth) {
          ast_list_node = (lizard_ast_list_node_t *)lizard_heap_alloc(
              heap, sizeof(lizard_ast_list_node_t));
          ast_list_node->ast = *lizard_parse_expression(
              token_list, current_node_pointer, depth, heap);
          list_append(ast_node->data.lambda.parameters, &ast_list_node->node);
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
      } else if (strcmp(current_token->data.symbol, "if") == 0) {
        ast_node->type = AST_IF;
        *current_node_pointer = current_node->next;
        current_node = current_node->next;
        if (current_node == token_list->nil) {
          fprintf(stderr, "Error: missing if predicate.\n");
          exit(1);
        }
        ast_node->data.if_clause.pred = lizard_parse_expression(
            token_list, current_node_pointer, depth, heap);
        if (current_node == token_list->nil) {
          fprintf(stderr, "Error: missing if cons.\n");
          exit(1);
        }
        ast_node->data.if_clause.cons = lizard_parse_expression(
            token_list, current_node_pointer, depth, heap);
        if (current_node->next != token_list->nil &&
            CAST(current_node->next, lizard_token_list_node_t)->token.type !=
                TOKEN_RIGHT_PAREN) {
          ast_node->data.if_clause.alt = lizard_parse_expression(
              token_list, current_node_pointer, depth, heap);
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
      } else if (strcmp(current_token->data.symbol, "begin") == 0) {
        ast_node->type = AST_BEGIN;
        *current_node_pointer = current_node->next;
        current_node = current_node->next;
        if (current_node == token_list->nil) {
          fprintf(stderr, "Error: missing begin predicate(s).\n");
          exit(1);
        }
        ast_node->data.begin_expressions = list_create();
        current_depth = *depth;
        while (current_node != token_list->nil &&
               CAST(current_node, lizard_token_list_node_t)->token.type !=
                   TOKEN_RIGHT_PAREN &&
               current_depth <= *depth) {
          ast_list_node = (lizard_ast_list_node_t *)lizard_heap_alloc(
              heap, sizeof(lizard_ast_list_node_t));
          ast_list_node->ast = *lizard_parse_expression(
              token_list, current_node_pointer, depth, heap);
          list_append(ast_node->data.begin_expressions, &ast_list_node->node);
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
      } else if (strcmp(current_token->data.symbol, "cond") == 0) {
        ast_node->type = AST_COND;
        *current_node_pointer = current_node->next;
        current_node = current_node->next;
        if (current_node == token_list->nil) {
          fprintf(stderr, "Error: missing cond predicate(s).\n");
          exit(1);
        }
        ast_node->data.cond_clauses = list_create();
        current_depth = *depth;
        while (current_node != token_list->nil &&
               CAST(current_node, lizard_token_list_node_t)->token.type !=
                   TOKEN_RIGHT_PAREN &&
               current_depth <= *depth) {
          ast_list_node = (lizard_ast_list_node_t *)lizard_heap_alloc(
              heap, sizeof(lizard_ast_list_node_t));
          ast_list_node->ast = *lizard_parse_expression(
              token_list, current_node_pointer, depth, heap);
          list_append(ast_node->data.cond_clauses, &ast_list_node->node);
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
      } else if (strcmp(current_token->data.symbol, "let") == 0) {

        *current_node_pointer = current_node->next;
        current_node = *current_node_pointer;

        app_node = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
        app_node->type = AST_APPLICATION;
        app_node->data.application_arguments = list_create();

        lambda_node = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
        lambda_node->type = AST_LAMBDA;
        lambda_node->data.lambda.parameters = list_create();
        lambda_node->data.lambda.closure_env = NULL;

        bindings = list_create();
        values = list_create();

        if (current_node == token_list->nil ||
            CAST(current_node, lizard_token_list_node_t)->token.type !=
                TOKEN_LEFT_PAREN) {
          fprintf(stderr, "Error: let requires a binding list\n");
          exit(1);
        }
        *current_node_pointer = current_node->next;
        current_node = *current_node_pointer;

        while (1) {
          if (current_node == token_list->nil) {
            fprintf(stderr, "Error: unexpected EOF in let bindings\n");
            exit(1);
          }
          if (CAST(current_node, lizard_token_list_node_t)->token.type ==
              TOKEN_RIGHT_PAREN) {
            *current_node_pointer = current_node->next;
            break;
          }

          if (CAST(current_node, lizard_token_list_node_t)->token.type !=
              TOKEN_LEFT_PAREN) {
            fprintf(stderr, "Error: let binding must be a pair (got %d)\n",
                    CAST(current_node, lizard_token_list_node_t)->token.type);
            exit(1);
          }
          *current_node_pointer = current_node->next;
          current_node = *current_node_pointer;

          var = lizard_parse_expression(token_list, current_node_pointer, depth,
                                        heap);
          current_node = *current_node_pointer;
          if (var->type != AST_SYMBOL) {
            fprintf(stderr, "Error: let binding name must be a symbol\n");
            exit(1);
          }

          value = lizard_parse_expression(token_list, current_node_pointer,
                                          depth, heap);
          current_node = *current_node_pointer;

          if (current_node == token_list->nil ||
              CAST(current_node, lizard_token_list_node_t)->token.type !=
                  TOKEN_RIGHT_PAREN) {
            fprintf(stderr,
                    "Error: missing closing paren in let binding pair\n");
            exit(1);
          }
          *current_node_pointer = current_node->next;
          current_node = *current_node_pointer;

          var_list_node =
              lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
          var_list_node->ast = *var;
          list_append(bindings, &var_list_node->node);

          val_list_node =
              lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
          val_list_node->ast = *value;
          list_append(values, &val_list_node->node);
        }

        if (current_node == token_list->nil ||
            CAST(current_node, lizard_token_list_node_t)->token.type !=
                TOKEN_RIGHT_PAREN) {
          fprintf(stderr, "Error: missing closing paren for let bindings\n");
          exit(1);
        }
        *current_node_pointer = current_node->next;
        current_node = *current_node_pointer;

        params_app = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
        params_app->type = AST_APPLICATION;
        params_app->data.application_arguments = bindings;

        params_wrapper =
            lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
        params_wrapper->ast = *params_app;
        list_append(lambda_node->data.lambda.parameters, &params_wrapper->node);

        while (current_node != token_list->nil &&
               CAST(current_node, lizard_token_list_node_t)->token.type !=
                   TOKEN_RIGHT_PAREN) {
          lizard_ast_list_node_t *body_node =
              lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
          body_node->ast = *lizard_parse_expression(
              token_list, current_node_pointer, depth, heap);
          list_append(lambda_node->data.lambda.parameters, &body_node->node);
          current_node = *current_node_pointer;
        }

        lambda_wrapper =
            lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
        lambda_wrapper->ast = *lambda_node;
        list_append(app_node->data.application_arguments,
                    &lambda_wrapper->node);

        val_iter = values->head;
        while (val_iter != values->nil) {
          lizard_ast_list_node_t *val_copy =
              lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
          val_copy->ast = *(&((lizard_ast_list_node_t *)val_iter)->ast);
          list_append(app_node->data.application_arguments, &val_copy->node);
          val_iter = val_iter->next;
        }

        if (current_node == token_list->nil ||
            CAST(current_node, lizard_token_list_node_t)->token.type !=
                TOKEN_RIGHT_PAREN) {
          fprintf(stderr, "Error: missing closing paren for let expression\n");
          exit(1);
        }
        *current_node_pointer = current_node->next;
        *depth -= 1;

        return app_node;

      } else if (strcmp(current_token->data.symbol, "define-syntax") == 0) {
        *current_node_pointer = current_node->next;
        current_node = *current_node_pointer;

        macro_name = lizard_parse_expression(token_list, current_node_pointer,
                                             depth, heap);
        if (macro_name->type != AST_SYMBOL) {
          fprintf(stderr, "Error: macro name must be a symbol.\n");
          exit(1);
        }

        transformer = lizard_parse_expression(token_list, current_node_pointer,
                                              depth, heap);

        ast_node = lizard_make_macro_def(heap, macro_name, transformer);

        current_node = *current_node_pointer;
        if (current_node == token_list->nil ||
            CAST(current_node, lizard_token_list_node_t)->token.type !=
                TOKEN_RIGHT_PAREN) {
          fprintf(stderr, "Error: missing closing paren in define-syntax.\n");
          exit(1);
        }
        *current_node_pointer = current_node->next;
        *depth -= 1;
        return ast_node;

      } else {
        ast_node->type = AST_APPLICATION;
        if (current_node == token_list->nil) {
          fprintf(stderr, "Error: missing application argument(s).\n");
          exit(1);
        }
        ast_node->data.application_arguments = list_create();
        current_depth = *depth;
        while (current_node != token_list->nil &&
               CAST(current_node, lizard_token_list_node_t)->token.type !=
                   TOKEN_RIGHT_PAREN &&
               current_depth <= *depth) {
          ast_list_node = (lizard_ast_list_node_t *)lizard_heap_alloc(
              heap, sizeof(lizard_ast_list_node_t));
          ast_list_node->ast = *lizard_parse_expression(
              token_list, current_node_pointer, depth, heap);
          list_append(ast_node->data.application_arguments,
                      &ast_list_node->node);
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
    } else if (current_token->data.symbol == TOKEN_LEFT_PAREN) {
      ast_node->type = AST_APPLICATION;
      if (current_node == token_list->nil) {
        fprintf(stderr, "Error: missing application argument(s).\n");
        exit(1);
      }
      ast_node->data.application_arguments = list_create();
      current_depth = *depth;
      while (current_node != token_list->nil &&
             CAST(current_node, lizard_token_list_node_t)->token.type !=
                 TOKEN_RIGHT_PAREN &&
             current_depth <= *depth) {
        ast_list_node = (lizard_ast_list_node_t *)lizard_heap_alloc(
            heap, sizeof(lizard_ast_list_node_t));
        ast_list_node->ast = *lizard_parse_expression(
            token_list, current_node_pointer, depth, heap);
        list_append(ast_node->data.application_arguments, &ast_list_node->node);
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
    } else {
      ast_node->type = AST_APPLICATION;
      ast_node->data.application_arguments = list_create();
      current_depth = *depth;
      while (current_node != token_list->nil &&
             CAST(current_node, lizard_token_list_node_t)->token.type !=
                 TOKEN_RIGHT_PAREN &&
             current_depth <= *depth) {
        ast_list_node = (lizard_ast_list_node_t *)lizard_heap_alloc(
            heap, sizeof(lizard_ast_list_node_t));
        ast_list_node->ast = *lizard_parse_expression(
            token_list, current_node_pointer, depth, heap);
        list_append(ast_node->data.application_arguments, &ast_list_node->node);
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
    if (current_token->data.symbol[0] == '\'' &&
        current_token->data.symbol[1] == '\0') {
      *current_node_pointer = current_node->next;
      current_node = current_node->next;
      ast_node->type = AST_QUOTED;
      ast_node->data.quoted = lizard_parse_expression(
          token_list, current_node_pointer, depth, heap);
    } else if (current_token->data.symbol[0] == '\'' &&
               current_token->data.symbol[1] != '\0') {
      ast_node->type = AST_QUOTED;
      quoted_node = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
      quoted_node->type = AST_SYMBOL;
      quoted_node->data.variable = current_token->data.symbol + 1;
      ast_node->data.quoted = quoted_node;
      *current_node_pointer = current_node->next;
      current_node = current_node->next;
    } else if (strcmp(current_token->data.symbol, "#t") == 0) {
      ast_node->type = AST_BOOL;
      ast_node->data.boolean = true;
      *current_node_pointer = current_node->next;
      current_node = current_node->next;
    } else if (strcmp(current_token->data.symbol, "#f") == 0) {
      ast_node->type = AST_BOOL;
      ast_node->data.boolean = false;
      *current_node_pointer = current_node->next;
      current_node = current_node->next;
    } else if (strcmp(current_token->data.symbol, "nil") == 0) {
      ast_node->type = AST_NIL;
      *current_node_pointer = current_node->next;
      current_node = current_node->next;
    } else {
      ast_node->type = AST_SYMBOL;
      ast_node->data.variable = current_token->data.symbol;
      *current_node_pointer = current_node->next;
      current_node = current_node->next;
    }
    break;
  case TOKEN_NUMBER:
    ast_node->type = AST_NUMBER;
    mpz_init(ast_node->data.number);
    mpz_set(ast_node->data.number, current_token->data.number);
    *current_node_pointer = current_node->next;
    current_node = current_node->next;
    break;
  case TOKEN_STRING:
    ast_node->type = AST_STRING;
    ast_node->data.string = current_token->data.string;
    *current_node_pointer = current_node->next;
    current_node = current_node->next;
    break;
  default:
    fprintf(stderr, "Error: unexpected token type.\n");
    exit(1);
  }

  return ast_node;
}

list_t *lizard_parse(list_t *token_list, lizard_heap_t *heap) {
  list_t *ast_list;
  list_node_t *current;
  int depth;
  ast_list = list_create();
  current = token_list->head;
  depth = 0;
  while (current != token_list->nil) {
    lizard_ast_node_t *ast =
        lizard_parse_expression(token_list, &current, &depth, heap);
    lizard_ast_list_node_t *ast_node =
        (lizard_ast_list_node_t *)lizard_heap_alloc(
            heap, sizeof(lizard_ast_list_node_t));
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
  int i;
  list_node_t *expr, *arg, *param_node, *iter;
  if (!node) {
    return;
  }

  for (i = 0; i < depth; i++) {
    printf("  ");
  }
  switch (node->type) {
  case AST_STRING:
    printf("String: \"%s\"\n", node->data.string);
    break;
  case AST_NUMBER:
    gmp_printf("Number: %Zd\n", node->data.number);
    break;
  case AST_SYMBOL:
    printf("Symbol: %s\n", node->data.variable);
    break;
  case AST_BOOL:
    printf("Boolean: %s\n", node->data.boolean ? "#t" : "#f");
    break;
  case AST_NIL:
    printf("Nil\n");
    break;
  case AST_QUOTED:
    printf("Quote:\n");
    print_ast(node->data.quoted, depth + 1);
    break;
  case AST_ASSIGNMENT:
    printf("Assignment:\n");
    print_ast(node->data.assignment.variable, depth + 1);
    print_ast(node->data.assignment.value, depth + 1);
    break;
  case AST_DEFINITION:
    printf("Definition:\n");
    print_ast(node->data.definition.variable, depth + 1);
    print_ast(node->data.definition.value, depth + 1);
    break;
  case AST_IF:
    printf("If clause:\n");
    print_ast(node->data.if_clause.pred, depth + 1);
    print_ast(node->data.if_clause.cons, depth + 1);
    if (node->data.if_clause.alt)
      print_ast(node->data.if_clause.alt, depth + 1);
    break;

  case AST_LAMBDA:
    printf("Lambda parameters:\n");
    param_node = node->data.lambda.parameters->head;
    while (param_node != node->data.lambda.parameters->nil) {
      print_ast(&((lizard_ast_list_node_t *)param_node)->ast, depth + 1);
      param_node = param_node->next;
    }

    break;

  case AST_BEGIN:
    printf("Begin:\n");
    expr = node->data.begin_expressions->head;
    while (expr != node->data.begin_expressions->nil) {
      print_ast(&((lizard_ast_list_node_t *)expr)->ast, depth + 1);
      expr = expr->next;
    }
    break;
  case AST_APPLICATION:
    printf("Application:\n");
    arg = node->data.application_arguments->head;
    while (arg != node->data.application_arguments->nil) {
      print_ast(&((lizard_ast_list_node_t *)arg)->ast, depth + 1);
      arg = arg->next;
    }
    if (node->data.application_arguments->head ==
        node->data.application_arguments->nil) {
      for (i = 0; i < depth + 1; i++) {
        printf("  ");
      }
      printf("Nil\n");
    }
    break;
  case AST_MACRO:
    printf("Macro definition:\n");
    print_ast(node->data.macro_def.variable, depth + 1);
    print_ast(node->data.macro_def.transformer, depth + 1);
    break;
  case AST_ERROR:
    printf("Error (code %d):\n", node->data.error.code);
    for (iter = node->data.error.data->head; iter != node->data.error.data->nil;
         iter = iter->next) {
      print_ast(&((lizard_ast_list_node_t *)iter)->ast, depth + 1);
    }
    break;
  default:
    printf("Unknown AST node type.\n");
  }
}

lizard_heap_t *lizard_heap_create(size_t initial_size, size_t reserved_size) {
  lizard_heap_t *heap = malloc(sizeof(lizard_heap_t));
  if (!heap) {
    fprintf(stderr, "Error: Unable to allocate lizard_heap structure.\n");
    exit(1);
  }
  heap->reserved = reserved_size;
  heap->start = mmap(NULL, reserved_size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (heap->start == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }
  heap->top = heap->start;
  heap->end = heap->start + initial_size;
  return heap;
}

void *lizard_heap_alloc(lizard_heap_t *heap, size_t size) {
  void *ptr;
  size_t alignment = sizeof(void *);
  size = (size + alignment - 1) & ~(alignment - 1);
  if (heap->top + size > heap->end) {
    size_t used = (size_t)(heap->top - heap->start);
    size_t current_committed = (size_t)(heap->end - heap->start);
    size_t new_committed = current_committed * 2;
    if (new_committed > heap->reserved)
      new_committed = heap->reserved;
    if (used + size > new_committed) {
      fprintf(stderr, "Error: Out of reserved memory in lizard_heap.\n");
      exit(1);
    }
    /* TODO: use mprotect */
    heap->end = heap->start + new_committed;
  }
  ptr = heap->top;
  heap->top += size;
  return ptr;
}

lizard_env_t *lizard_env_create(lizard_heap_t *heap, lizard_env_t *parent) {
  lizard_env_t *env = lizard_heap_alloc(heap, sizeof(lizard_env_t));
  env->entries = NULL;
  env->parent = parent;
  return env;
}

void lizard_env_define(lizard_heap_t *heap, lizard_env_t *env,
                       const char *symbol, lizard_ast_node_t *value) {
  lizard_env_entry_t *entry =
      lizard_heap_alloc(heap, sizeof(lizard_env_entry_t));
  entry->symbol = symbol;
  entry->value = value;
  entry->next = env->entries;
  env->entries = entry;
}

lizard_ast_node_t *lizard_env_lookup(lizard_env_t *env, const char *symbol) {
  lizard_env_t *e;
  lizard_env_entry_t *entry;
  for (e = env; e != NULL; e = e->parent) {
    for (entry = e->entries; entry != NULL; entry = entry->next) {
      if (strcmp(entry->symbol, symbol) == 0)
        return entry->value;
    }
  }
  return NULL;
}

int lizard_env_set(lizard_env_t *env, const char *symbol,
                   lizard_ast_node_t *value) {
  lizard_env_t *e;
  lizard_env_entry_t *entry;
  for (e = env; e != NULL; e = e->parent) {
    for (entry = e->entries; entry != NULL; entry = entry->next) {
      if (strcmp(entry->symbol, symbol) == 0) {
        entry->value = value;
        return 1;
      }
    }
  }
  return 0;
}

lizard_ast_node_t *lizard_eval(lizard_ast_node_t *node, lizard_env_t *env,
                               lizard_heap_t *heap) {
  lizard_ast_node_t *closure, *result, *func, *lambda_node, *params_app, *var;
  lizard_ast_list_node_t *name_node, *params_wrapper, *body_wrapper;
  const char *fn_name, *macro_name;
  list_t *param_list, *evaled_args;
  list_node_t *p, *iter, *arg_node;
  if (!node) {
    return NULL;
  }
  node = lizard_expand_macros(node, env, heap);

  switch (node->type) {
  case AST_BOOL:
  case AST_NIL:
  case AST_NUMBER:
  case AST_STRING:
  case AST_PRIMITIVE:
    return node;
  case AST_SYMBOL: {
    lizard_ast_node_t *val = lizard_env_lookup(env, node->data.variable);
    if (!val) {
      return lizard_make_error(heap, LIZARD_ERROR_UNBOUND_SYMBOL);
    }
    return val;
  }
  case AST_QUOTED:
    return node->data.quoted;

  case AST_DEFINITION:
    var = node->data.definition.variable;
    if (var->type == AST_SYMBOL) {
      {
        lizard_ast_node_t *value =
            lizard_eval(node->data.definition.value, env, heap);
        lizard_env_define(heap, env, var->data.variable, value);
        return value;
      }
    } else if (var->type == AST_APPLICATION) {
      list_t *app_args = var->data.application_arguments;
      if (app_args->head == app_args->nil) {
        return lizard_make_error(heap, LIZARD_ERROR_INVALID_FUNCTION_DEF);
      }
      name_node = (lizard_ast_list_node_t *)app_args->head;
      if (name_node->ast.type != AST_SYMBOL) {
        return lizard_make_error(heap, LIZARD_ERROR_INVALID_FUNCTION_NAME);
      }
      fn_name = name_node->ast.data.variable;
      param_list = list_create();
      for (p = app_args->head->next; p != app_args->nil; p = p->next) {
        lizard_ast_list_node_t *param_node = (lizard_ast_list_node_t *)p;
        if (param_node->ast.type != AST_SYMBOL) {
          return lizard_make_error(heap, LIZARD_ERROR_INVALID_PARAMETER);
        }
        list_append(param_list, p);
      }

      lambda_node = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
      lambda_node->type = AST_LAMBDA;
      lambda_node->data.lambda.closure_env = NULL;

      lambda_node->data.lambda.parameters = list_create();

      params_app = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
      params_app->type = AST_APPLICATION;
      params_app->data.application_arguments = param_list;
      params_wrapper = lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
      params_wrapper->ast = *params_app;
      list_append(lambda_node->data.lambda.parameters, &params_wrapper->node);

      body_wrapper = lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
      body_wrapper->ast = *node->data.definition.value;
      list_append(lambda_node->data.lambda.parameters, &body_wrapper->node);

      lizard_env_define(heap, env, fn_name, lambda_node);
      return lambda_node;
    } else {
      return lizard_make_error(heap, LIZARD_ERROR_INVALID_DEF);
    }

  case AST_ASSIGNMENT: {
    lizard_ast_node_t *var = node->data.assignment.variable;
    lizard_ast_node_t *value =
        lizard_eval(node->data.assignment.value, env, heap);
    if (var->type != AST_SYMBOL) {
      return lizard_make_error(heap, LIZARD_ERROR_ASSIGNMENT);
    }
    if (!lizard_env_set(env, var->data.variable, value)) {
      return lizard_make_error(heap, LIZARD_ERROR_ASSIGNMENT_UNBOUND);
    }
    return value;
  }

  case AST_IF: {
    lizard_ast_node_t *pred = lizard_eval(node->data.if_clause.pred, env, heap);
    bool is_true = false;

    if (pred->type == AST_BOOL) {
      is_true = pred->data.boolean;
    } else if (pred->type == AST_NIL) {
      is_true = false;
    } else {
      is_true = true;
    }

    if (is_true) {
      return lizard_eval(node->data.if_clause.cons, env, heap);
    } else {
      if (node->data.if_clause.alt)
        return lizard_eval(node->data.if_clause.alt, env, heap);
      else
        return lizard_make_nil(heap);
    }
  }

  case AST_BEGIN: {
    result = NULL;
    for (iter = node->data.begin_expressions->head;
         iter != node->data.begin_expressions->nil; iter = iter->next) {
      lizard_ast_list_node_t *expr_node = CAST(iter, lizard_ast_list_node_t);
      result = lizard_eval(&expr_node->ast, env, heap);
    }
    return result;
  }

  case AST_LAMBDA:
    closure = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
    closure->type = AST_LAMBDA;
    closure->data.lambda.parameters = node->data.lambda.parameters;
    closure->data.lambda.closure_env = env;
    return closure;

  case AST_APPLICATION: {
    list_node_t *func_node = node->data.application_arguments->head;
    func = lizard_eval(&((lizard_ast_list_node_t *)func_node)->ast, env, heap);

    evaled_args = list_create();
    for (arg_node = func_node->next;
         arg_node != node->data.application_arguments->nil;
         arg_node = arg_node->next) {
      lizard_ast_node_t *arg_val =
          lizard_eval(&((lizard_ast_list_node_t *)arg_node)->ast, env, heap);
      lizard_ast_list_node_t *new_arg_node =
          lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
      new_arg_node->ast = *arg_val;
      list_append(evaled_args, &new_arg_node->node);
    }
    return lizard_apply(func, evaled_args, env, heap);
  }
  case AST_MACRO: {
    lizard_ast_node_t *transformer =
        lizard_eval(node->data.macro_def.transformer, env, heap);
    if (node->data.macro_def.variable->type != AST_SYMBOL) {
      return lizard_make_error(heap, LIZARD_ERROR_INVALID_MACRO_NAME);
    }
    macro_name = node->data.macro_def.variable->data.variable;
    lizard_env_define(heap, env, macro_name, transformer);
    return lizard_make_nil(heap);
  }

  default:
    return lizard_make_error(heap, LIZARD_ERROR_NODE_TYPE);
  }
}

lizard_ast_node_t *lizard_apply(lizard_ast_node_t *func, list_t *args,
                                lizard_env_t *env, lizard_heap_t *heap) {

  list_t *formal_params;
  list_node_t *param_node, *arg_node, *body_node;
  lizard_ast_list_node_t *var_param;
  lizard_ast_node_t *rest_list, *result;
  bool variadic;
  const char *variadic_name;

  if (func->type == AST_PRIMITIVE) {
    return func->data.primitive(args, env, heap);
  } else if (func->type == AST_LAMBDA) {
    lizard_env_t *new_env =
        lizard_env_create(heap, func->data.lambda.closure_env);

    lizard_ast_node_t *param_list =
        &((lizard_ast_list_node_t *)func->data.lambda.parameters->head)->ast;
    if (param_list->type != AST_APPLICATION) {
      return lizard_make_error(heap, LIZARD_ERROR_LAMBDA_PARAMS);
    }

    formal_params = param_list->data.application_arguments;
    param_node = formal_params->head;
    arg_node = args->head;
    variadic = false;
    variadic_name = NULL;

    while (param_node != formal_params->nil) {
      lizard_ast_list_node_t *param = (lizard_ast_list_node_t *)param_node;
      if (param->ast.type == AST_SYMBOL &&
          (strcmp(param->ast.data.variable, ".") == 0 ||
           strcmp(param->ast.data.variable, "...") == 0)) {
        param_node = param_node->next;
        if (param_node == formal_params->nil) {
          return lizard_make_error(heap, LIZARD_ERROR_VARIADIC_UNFOLLOWED);
        }
        var_param = (lizard_ast_list_node_t *)param_node;
        if (var_param->ast.type != AST_SYMBOL) {
          return lizard_make_error(heap, LIZARD_ERROR_VARIADIC_SYMBOL);
        }
        variadic = true;
        variadic_name = var_param->ast.data.variable;
        param_node = param_node->next;
        break;
      }

      if (arg_node == args->nil) {
        return lizard_make_error(heap, LIZARD_ERROR_LAMBDA_ARITY_LESS);
      }

      if (param->ast.type != AST_SYMBOL) {
        return lizard_make_error(heap, LIZARD_ERROR_LAMBDA_PARAMETER);
      }
      lizard_env_define(heap, new_env, param->ast.data.variable,
                        &((lizard_ast_list_node_t *)arg_node)->ast);
      param_node = param_node->next;
      arg_node = arg_node->next;
    }

    if (!variadic && arg_node != args->nil) {
      return lizard_make_error(heap, LIZARD_ERROR_LAMBDA_ARITY_MORE);
    }

    if (variadic) {
      list_t temp_args;
      temp_args.head = arg_node;
      temp_args.nil = args->nil;
      rest_list = lizard_primitive_list(&temp_args, env, heap);
      lizard_env_define(heap, new_env, variadic_name, rest_list);
    }

    result = NULL;
    for (body_node = func->data.lambda.parameters->head->next;
         body_node != func->data.lambda.parameters->nil;
         body_node = body_node->next) {
      lizard_ast_list_node_t *body_expr = (lizard_ast_list_node_t *)body_node;
      result = lizard_eval(&body_expr->ast, new_env, heap);
    }

    return result;

  } else {
    return lizard_make_error(heap, LIZARD_ERROR_INVALID_APPLY);
  }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
lizard_ast_node_t *lizard_primitive_plus(list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap) {
  list_node_t *node;
  lizard_ast_list_node_t *arg_node;
  lizard_ast_node_t *arg;
  lizard_ast_node_t *result;
  mpz_t sum;
  mpz_init(sum);

  for (node = args->head; node != args->nil; node = node->next) {
    arg_node = CAST(node, lizard_ast_list_node_t);
    arg = &arg_node->ast;
    if (arg->type != AST_NUMBER) {
      return lizard_make_error(heap, LIZARD_ERROR_PLUS);
    }
    mpz_add(sum, sum, arg->data.number);
  }

  result = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
  result->type = AST_NUMBER;
  mpz_init(result->data.number);
  mpz_set(result->data.number, sum);
  mpz_clear(sum);
  return result;
}

lizard_ast_node_t *lizard_primitive_minus(list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  list_node_t *node;
  lizard_ast_list_node_t *first_node, *arg_node;
  lizard_ast_node_t *arg, *ret;
  mpz_t result;
  if (args->head == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_MINUS_ARGC);
  }
  node = args->head;
  first_node = CAST(node, lizard_ast_list_node_t);
  if (first_node->ast.type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_MINUS_ARGT);
  }
  mpz_init(result);
  mpz_set(result, first_node->ast.data.number);
  node = node->next;
  if (node == args->nil) {
    mpz_neg(result, result);
  } else {
    for (; node != args->nil; node = node->next) {
      arg_node = CAST(node, lizard_ast_list_node_t);
      arg = &arg_node->ast;
      if (arg->type != AST_NUMBER) {
        return lizard_make_error(heap, LIZARD_ERROR_MINUS_ARGT_2);
      }
      mpz_sub(result, result, arg->data.number);
    }
  }
  ret = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
  ret->type = AST_NUMBER;
  mpz_init(ret->data.number);
  mpz_set(ret->data.number, result);
  mpz_clear(result);
  return ret;
}

lizard_ast_node_t *lizard_primitive_multiply(list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap) {
  list_node_t *node;
  lizard_ast_list_node_t *arg_node;
  lizard_ast_node_t *arg, *ret;

  mpz_t product;
  mpz_init_set_ui(product, 1);
  for (node = args->head; node != args->nil; node = node->next) {
    arg_node = CAST(node, lizard_ast_list_node_t);
    arg = &arg_node->ast;
    if (arg->type != AST_NUMBER) {
      return lizard_make_error(heap, LIZARD_ERROR_MUL);
    }
    mpz_mul(product, product, arg->data.number);
  }
  ret = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
  ret->type = AST_NUMBER;
  mpz_init(ret->data.number);
  mpz_set(ret->data.number, product);
  mpz_clear(product);
  return ret;
}

lizard_ast_node_t *lizard_primitive_divide(list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap) {
  mpz_t quotient;
  list_node_t *node;
  lizard_ast_list_node_t *first_node, *arg_node;
  lizard_ast_node_t *arg, *ret;

  if (args->head == args->nil || args->head->next == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_DIV_ARGC);
  }
  node = args->head;
  first_node = CAST(node, lizard_ast_list_node_t);
  if (first_node->ast.type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_DIV_ARGT);
  }
  mpz_init(quotient);
  mpz_set(quotient, first_node->ast.data.number);
  for (node = node->next; node != args->nil; node = node->next) {
    arg_node = CAST(node, lizard_ast_list_node_t);
    arg = &arg_node->ast;
    if (arg->type != AST_NUMBER) {
      return lizard_make_error(heap, LIZARD_ERROR_DIV_ARGT_2);
    }
    if (mpz_cmp_ui(arg->data.number, 0) == 0) {
      return lizard_make_error(heap, LIZARD_ERROR_DIV_ZERO);
    }
    mpz_tdiv_q(quotient, quotient, arg->data.number);
  }
  ret = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
  ret->type = AST_NUMBER;
  mpz_init(ret->data.number);
  mpz_set(ret->data.number, quotient);
  mpz_clear(quotient);
  return ret;
}
lizard_ast_node_t *lizard_primitive_equal(list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  list_node_t *node;
  lizard_ast_list_node_t *first_node, *other_node;
  lizard_ast_node_t *first, *other, *result;
  int eq;
  if (args->head == args->nil || args->head->next == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_EQ_ARGC);
  }
  first_node = (lizard_ast_list_node_t *)args->head;
  first = &first_node->ast;
  eq = 1;
  for (node = args->head->next; node != args->nil; node = node->next) {
    other_node = (lizard_ast_list_node_t *)node;
    other = &other_node->ast;
    if (first->type != other->type) {
      eq = 0;
      break;
    }
    switch (first->type) {
    case AST_NUMBER:
      if (mpz_cmp(first->data.number, other->data.number) != 0)
        eq = 0;
      break;
    case AST_SYMBOL:
      if (strcmp(first->data.variable, other->data.variable) != 0)
        eq = 0;
      break;
    case AST_BOOL:
      if (first->data.boolean != other->data.boolean)
        eq = 0;
      break;
    case AST_NIL:
      break;
    default:
      /* comparing pointers */
      if (first != other)
        eq = 0;
    }
    if (!eq)
      break;
  }
  result = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
  result->type = AST_BOOL;
  result->data.boolean = (eq != 0);
  return result;
}

lizard_ast_node_t *lizard_primitive_cons(list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap) {
  lizard_ast_node_t *node;
  lizard_ast_list_node_t *car_node, *new_car_node, *cdr_node, *new_cdr_node;
  if (args->head == args->nil || args->head->next == args->nil ||
      args->head->next->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_CONS_ARGC);
  }
  node = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
  node->type = AST_APPLICATION;
  node->data.application_arguments = list_create();

  car_node = CAST(args->head, lizard_ast_list_node_t);
  new_car_node = lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
  new_car_node->ast = car_node->ast; /* shallow copy */
  list_append(node->data.application_arguments, &new_car_node->node);

  cdr_node = CAST(args->head->next, lizard_ast_list_node_t);
  new_cdr_node = lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
  new_cdr_node->ast = cdr_node->ast; /* shallow copy */
  list_append(node->data.application_arguments, &new_cdr_node->node);

  return node;
}

lizard_ast_node_t *lizard_primitive_car(list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap) {
  lizard_ast_list_node_t *node;
  list_t *app_args;
  lizard_ast_list_node_t *first_arg;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_CAR_ARGC);
  }
  node = CAST(args->head, lizard_ast_list_node_t);
  if (node->ast.type != AST_APPLICATION) {
    return lizard_make_error(heap, LIZARD_ERROR_CAR_ARGT);
  }
  app_args = node->ast.data.application_arguments;
  if (app_args->head == app_args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_CAR_NIL);
  }
  first_arg = CAST(app_args->head, lizard_ast_list_node_t);
  return &first_arg->ast;
}

lizard_ast_node_t *lizard_primitive_cdr(list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap) {
  lizard_ast_list_node_t *node, *copy;
  list_t *app_args;
  lizard_ast_node_t *cdr_result;
  list_node_t *iter;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_CDR_ARGC);
  }
  node = CAST(args->head, lizard_ast_list_node_t);
  if (node->ast.type != AST_APPLICATION) {
    return lizard_make_error(heap, LIZARD_ERROR_CDR_ARGT);
  }
  app_args = node->ast.data.application_arguments;
  if (app_args->head == app_args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_CDR_NIL);
  }
  cdr_result = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
  cdr_result->type = AST_APPLICATION;
  cdr_result->data.application_arguments = list_create();

  for (iter = app_args->head->next; iter != app_args->nil; iter = iter->next) {
    copy = lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
    copy->ast = ((lizard_ast_list_node_t *)iter)->ast; /* shallow copy */
    list_append(cdr_result->data.application_arguments, &copy->node);
  }
  return cdr_result;
}

lizard_ast_node_t *lizard_make_primitive(lizard_heap_t *heap,
                                         lizard_primitive_func_t func) {
  lizard_ast_node_t *node = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
  node->type = AST_PRIMITIVE;
  node->data.primitive = func;
  return node;
}
lizard_ast_node_t *lizard_primitive_list(list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap) {
  lizard_ast_node_t *first, *rest;
  list_t *rest_args, *cons_args;
  lizard_ast_list_node_t *first_node, *rest_node;
  if (args->head == args->nil) {
    return lizard_make_nil(heap);
  }

  first = &CAST(args->head, lizard_ast_list_node_t)->ast;
  rest_args = list_create();
  rest_args->head = args->head->next;
  rest_args->nil = args->nil;
  rest = lizard_primitive_list(rest_args, env, heap);

  cons_args = list_create();
  first_node = lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
  first_node->ast = *first;
  list_append(cons_args, &first_node->node);
  rest_node = lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
  rest_node->ast = *rest;
  list_append(cons_args, &rest_node->node);

  return lizard_primitive_cons(cons_args, env, heap);
}

lizard_ast_node_t *lizard_primitive_tokens(list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_list_node_t *node;
  const char *input;
  list_t *tokens;
  list_node_t *token_node;
  lizard_token_t *token;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_TOKENS_ARGC);
  }

  node = CAST(args->head, lizard_ast_list_node_t);
  if (node->ast.type != AST_STRING) {
    return lizard_make_error(heap, LIZARD_ERROR_TOKENS_ARGT);
  }

  input = node->ast.data.string;
  tokens = lizard_tokenize(input);

  for (token_node = tokens->head; token_node != tokens->nil;
       token_node = token_node->next) {
    token = &CAST(token_node, lizard_token_list_node_t)->token;
    switch (token->type) {
    case TOKEN_LEFT_PAREN:
      printf("TOKEN_LEFT_PAREN\n");
      break;
    case TOKEN_RIGHT_PAREN:
      printf("TOKEN_RIGHT_PAREN\n");
      break;
    case TOKEN_SYMBOL:
      printf("TOKEN_SYMBOL: %s\n", token->data.symbol);
      break;
    case TOKEN_NUMBER:
      gmp_printf("TOKEN_NUMBER: %Zd\n", token->data.number);
      break;
    case TOKEN_STRING:
      printf("TOKEN_STRING: \"%s\"\n", token->data.string);
      break;
    default:
      printf("Unknown token type.\n");
    }
  }

  return lizard_make_nil(heap);
}

lizard_ast_node_t *lizard_primitive_eval(list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap) {
  lizard_ast_list_node_t *node;
  lizard_ast_node_t *expr;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_EVAL_ARGC);
  }

  node = CAST(args->head, lizard_ast_list_node_t);
  expr = &node->ast;
  return lizard_eval(expr, env, heap);
}
#pragma GCC diagnostic pop

lizard_ast_node_t *lizard_make_bool(lizard_heap_t *heap, bool value) {
  lizard_ast_node_t *node;
  node = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
  node->type = AST_BOOL;
  node->data.boolean = value;
  return node;
}

lizard_ast_node_t *lizard_make_nil(lizard_heap_t *heap) {
  lizard_ast_node_t *node;
  node = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
  node->type = AST_NIL;
  return node;
}

lizard_ast_node_t *lizard_make_macro_def(lizard_heap_t *heap,
                                         lizard_ast_node_t *name,
                                         lizard_ast_node_t *transformer) {
  lizard_ast_node_t *node;
  node = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
  node->type = AST_MACRO;
  node->data.macro_def.variable = name;
  node->data.macro_def.transformer = transformer;
  return node;
}

lizard_ast_node_t *lizard_expand_macros(lizard_ast_node_t *node,
                                        lizard_env_t *env,
                                        lizard_heap_t *heap) {
  list_node_t *first, *arg, *body, *expr;
  lizard_ast_node_t *operator, * binding, *expanded;
  lizard_ast_list_node_t *copy;
  list_t *macro_args;
  if (!node) {
    return NULL;
  }

  if (node->type == AST_APPLICATION) {
    first = node->data.application_arguments->head;
    if (first != node->data.application_arguments->nil) {
      operator= &((lizard_ast_list_node_t *)first)->ast;
      if (operator->type == AST_SYMBOL) {
        binding = lizard_env_lookup(env, operator->data.variable);
        if (binding && binding->type == AST_MACRO) {
          macro_args = list_create();
          for (arg = first->next; arg != node->data.application_arguments->nil;
               arg = arg->next) {
            copy = lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
            copy->ast = ((lizard_ast_list_node_t *)arg)->ast; /* shallow copy */
            list_append(macro_args, &copy->node);
          }
          expanded = lizard_apply(binding, macro_args, env, heap);
          return lizard_expand_macros(expanded, env, heap);
        }
      }
    }
    for (arg = node->data.application_arguments->head;
         arg != node->data.application_arguments->nil; arg = arg->next) {
      lizard_ast_list_node_t *arg_node = (lizard_ast_list_node_t *)arg;
      arg_node->ast = *lizard_expand_macros(&arg_node->ast, env, heap);
    }
  }

  switch (node->type) {
  case AST_QUOTED:
    node->data.quoted = lizard_expand_macros(node->data.quoted, env, heap);
    break;
  case AST_ASSIGNMENT:
    node->data.assignment.variable =
        lizard_expand_macros(node->data.assignment.variable, env, heap);
    node->data.assignment.value =
        lizard_expand_macros(node->data.assignment.value, env, heap);
    break;
  case AST_DEFINITION:
    node->data.definition.variable =
        lizard_expand_macros(node->data.definition.variable, env, heap);
    node->data.definition.value =
        lizard_expand_macros(node->data.definition.value, env, heap);
    break;
  case AST_IF:
    node->data.if_clause.pred =
        lizard_expand_macros(node->data.if_clause.pred, env, heap);
    node->data.if_clause.cons =
        lizard_expand_macros(node->data.if_clause.cons, env, heap);
    if (node->data.if_clause.alt)
      node->data.if_clause.alt =
          lizard_expand_macros(node->data.if_clause.alt, env, heap);
    break;
  case AST_LAMBDA:
    for (body = node->data.lambda.parameters->head->next;
         body != node->data.lambda.parameters->nil; body = body->next) {
      lizard_ast_list_node_t *body_expr = (lizard_ast_list_node_t *)body;
      body_expr->ast = *lizard_expand_macros(&body_expr->ast, env, heap);
    }
    break;
  case AST_BEGIN:
    for (expr = node->data.begin_expressions->head;
         expr != node->data.begin_expressions->nil; expr = expr->next) {
      lizard_ast_list_node_t *expr_node = (lizard_ast_list_node_t *)expr;
      expr_node->ast = *lizard_expand_macros(&expr_node->ast, env, heap);
    }
    break;
  default:
    break;
  }
  return node;
}

lizard_ast_node_t *lizard_make_error(lizard_heap_t *heap, int error_code) {
  lizard_ast_list_node_t *msg_node;
  lizard_ast_node_t *node = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
  node->type = AST_ERROR;
  node->data.error.code = error_code;
  node->data.error.data = list_create();

  msg_node = lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
  msg_node->ast.type = AST_STRING;
  msg_node->ast.data.string = lizard_error_messages[LIZARD_LANG_EN][error_code];
  list_append(node->data.error.data, &msg_node->node);

  return node;
}
