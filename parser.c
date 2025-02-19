#include "parser.h"
#include "lizard.h"
#include "mem.h"
#include "tokenizer.h"

lizard_ast_node_t *lizard_parse_expression(list_t *token_list,
                                           list_node_t **current_node_pointer,
                                           int *depth, lizard_heap_t *heap) {
  lizard_ast_node_t *ast_node, *val_node, *var_node, *body_node, *lambda_node,
      *params_app, *def_node, *fn_symbol, *app_node, *var, *value, *macro_name,
      *transformer;
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
  ast_node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  switch (current_token->type) {
  case TOKEN_LEFT_PAREN: {
    *current_node_pointer = current_node->next;
    current_node = current_node->next;
    current_token = &CAST(current_node, lizard_token_list_node_t)->token;
    *depth = *depth + 1;

    if (current_token->type == TOKEN_SYMBOL) {
      if (strcmp(current_token->data.symbol, "quote") == 0) {
        ast_node->type = AST_QUOTE;
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

          lambda_node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
          lambda_node->type = AST_LAMBDA;
          lambda_node->data.lambda.closure_env = NULL;
          lambda_node->data.lambda.parameters =
              list_create_alloc(lizard_heap_alloc, lizard_heap_free);

          params_app = lizard_heap_alloc(sizeof(lizard_ast_node_t));
          params_app->type = AST_APPLICATION;
          params_list = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
          for (p = app_args->head->next; p != app_args->nil; p = p->next) {
            lizard_ast_list_node_t *param_old = (lizard_ast_list_node_t *)p;
            lizard_ast_list_node_t *param_copy =
                lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
            param_copy->ast = param_old->ast; /* shallow copy */
            list_append(params_list, &param_copy->node);
          }
          params_app->data.application_arguments = params_list;
          params_wrapper = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
          params_wrapper->ast = *params_app;
          list_append(lambda_node->data.lambda.parameters,
                      &params_wrapper->node);

          body_wrapper = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
          body_wrapper->ast = *body_node;
          list_append(lambda_node->data.lambda.parameters, &body_wrapper->node);

          def_node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
          def_node->type = AST_DEFINITION;
          fn_symbol = lizard_heap_alloc(sizeof(lizard_ast_node_t));
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
              lizard_heap_alloc(sizeof(lizard_ast_node_t));
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
        ast_node->data.lambda.parameters =
            list_create_alloc(lizard_heap_alloc, lizard_heap_free);
        ast_node->data.lambda.closure_env = NULL;

        *current_node_pointer = current_node->next;
        current_node = current_node->next;
        if (current_node == token_list->nil) {
          fprintf(stderr, "Error: missing lambda parameter(s).\n");
          exit(1);
        }
        ast_node->data.lambda.parameters =
            list_create_alloc(lizard_heap_alloc, lizard_heap_free);
        current_depth = *depth;
        while (current_node != token_list->nil &&
               CAST(current_node, lizard_token_list_node_t)->token.type !=
                   TOKEN_RIGHT_PAREN &&
               current_depth <= *depth) {
          ast_list_node = (lizard_ast_list_node_t *)lizard_heap_alloc(
              sizeof(lizard_ast_list_node_t));
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
        ast_node->data.begin_expressions =
            list_create_alloc(lizard_heap_alloc, lizard_heap_free);
        current_depth = *depth;
        while (current_node != token_list->nil &&
               CAST(current_node, lizard_token_list_node_t)->token.type !=
                   TOKEN_RIGHT_PAREN &&
               current_depth <= *depth) {
          ast_list_node = (lizard_ast_list_node_t *)lizard_heap_alloc(
              sizeof(lizard_ast_list_node_t));
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
        ast_node->data.cond_clauses =
            list_create_alloc(lizard_heap_alloc, lizard_heap_free);
        current_depth = *depth;
        while (current_node != token_list->nil &&
               CAST(current_node, lizard_token_list_node_t)->token.type !=
                   TOKEN_RIGHT_PAREN &&
               current_depth <= *depth) {
          ast_list_node = (lizard_ast_list_node_t *)lizard_heap_alloc(
              sizeof(lizard_ast_list_node_t));
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

        app_node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
        app_node->type = AST_APPLICATION;
        app_node->data.application_arguments =
            list_create_alloc(lizard_heap_alloc, lizard_heap_free);

        lambda_node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
        lambda_node->type = AST_LAMBDA;
        lambda_node->data.lambda.parameters =
            list_create_alloc(lizard_heap_alloc, lizard_heap_free);
        lambda_node->data.lambda.closure_env = NULL;

        bindings = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
        values = list_create_alloc(lizard_heap_alloc, lizard_heap_free);

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

          var_list_node = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
          var_list_node->ast = *var;
          list_append(bindings, &var_list_node->node);

          val_list_node = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
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

        params_app = lizard_heap_alloc(sizeof(lizard_ast_node_t));
        params_app->type = AST_APPLICATION;
        params_app->data.application_arguments = bindings;

        params_wrapper = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
        params_wrapper->ast = *params_app;
        list_append(lambda_node->data.lambda.parameters, &params_wrapper->node);

        while (current_node != token_list->nil &&
               CAST(current_node, lizard_token_list_node_t)->token.type !=
                   TOKEN_RIGHT_PAREN) {
          lizard_ast_list_node_t *body_node =
              lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
          body_node->ast = *lizard_parse_expression(
              token_list, current_node_pointer, depth, heap);
          list_append(lambda_node->data.lambda.parameters, &body_node->node);
          current_node = *current_node_pointer;
        }

        lambda_wrapper = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
        lambda_wrapper->ast = *lambda_node;
        list_append(app_node->data.application_arguments,
                    &lambda_wrapper->node);

        val_iter = values->head;
        while (val_iter != values->nil) {
          lizard_ast_list_node_t *val_copy =
              lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
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
        ast_node->data.application_arguments =
            list_create_alloc(lizard_heap_alloc, lizard_heap_free);
        current_depth = *depth;
        while (current_node != token_list->nil &&
               CAST(current_node, lizard_token_list_node_t)->token.type !=
                   TOKEN_RIGHT_PAREN &&
               current_depth <= *depth) {
          ast_list_node = (lizard_ast_list_node_t *)lizard_heap_alloc(
              sizeof(lizard_ast_list_node_t));
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
      ast_node->data.application_arguments =
          list_create_alloc(lizard_heap_alloc, lizard_heap_free);
      current_depth = *depth;
      while (current_node != token_list->nil &&
             CAST(current_node, lizard_token_list_node_t)->token.type !=
                 TOKEN_RIGHT_PAREN &&
             current_depth <= *depth) {
        ast_list_node = (lizard_ast_list_node_t *)lizard_heap_alloc(
            sizeof(lizard_ast_list_node_t));
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
      ast_node->data.application_arguments =
          list_create_alloc(lizard_heap_alloc, lizard_heap_free);
      current_depth = *depth;
      while (current_node != token_list->nil &&
             CAST(current_node, lizard_token_list_node_t)->token.type !=
                 TOKEN_RIGHT_PAREN &&
             current_depth <= *depth) {
        ast_list_node = (lizard_ast_list_node_t *)lizard_heap_alloc(
            sizeof(lizard_ast_list_node_t));
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
    if (strcmp(current_token->data.symbol, "'") == 0) {
      *current_node_pointer = current_node->next;
      current_node = current_node->next;
      ast_node->type = AST_QUOTE;
      ast_node->data.quoted = lizard_parse_expression(
          token_list, current_node_pointer, depth, heap);
    } else if (strcmp(current_token->data.symbol, "`") == 0) {
      *current_node_pointer = current_node->next;
      current_node = current_node->next;
      ast_node->type = AST_QUASIQUOTE;
      ast_node->data.quoted = lizard_parse_expression(
          token_list, current_node_pointer, depth, heap);
    } else if (strcmp(current_token->data.symbol, ",") == 0) {
      *current_node_pointer = current_node->next;
      current_node = current_node->next;
      ast_node->type = AST_UNQUOTE;
      ast_node->data.quoted = lizard_parse_expression(
          token_list, current_node_pointer, depth, heap);
    } else if (strcmp(current_token->data.symbol, ",@") == 0) {
      *current_node_pointer = current_node->next;
      current_node = current_node->next;
      ast_node->type = AST_UNQUOTE_SPLICING;
      ast_node->data.quoted = lizard_parse_expression(
          token_list, current_node_pointer, depth, heap);

    } else if (strcmp(current_token->data.symbol, "#") == 0) {
      *current_node_pointer = current_node->next;
      current_node = current_node->next;
      current_token = &CAST(current_node, lizard_token_list_node_t)->token;
      if (strcmp(current_token->data.symbol, "t") == 0) {
        ast_node->type = AST_BOOL;
        ast_node->data.boolean = true;
      } else if (strcmp(current_token->data.symbol, "f") == 0) {
        ast_node->type = AST_BOOL;
        ast_node->data.boolean = false;
      } else {
        fprintf(stderr, "Error: unexpected token after #.\n");
        exit(1);
      }
      *current_node_pointer = current_node->next;
      current_node = *current_node_pointer;
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

  ast_list = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  current = token_list->head;
  depth = 0;
  while (current != token_list->nil) {
    lizard_ast_node_t *ast =
        lizard_parse_expression(token_list, &current, &depth, heap);
    lizard_ast_list_node_t *ast_node =
        (lizard_ast_list_node_t *)lizard_heap_alloc(
            sizeof(lizard_ast_list_node_t));
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
